/*  karma2ppm.c

    Source file for  karma2ppm  (Karma to PPM image conversion module).

    Copyright (C) 1995-1996  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This Karma module will read in a Karma data file and convert the image into
    a PPM file.


    Written by      Richard Gooch   15-APR-1995

    Last updated by Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_iarray.h>
#include <karma_module.h>
#include <karma_dsxfr.h>
#include <karma_panel.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_im.h>
#include <karma_m.h>


STATIC_FUNCTION (flag karma2ppm, (char *command, FILE *fp) );

static flag binary = TRUE;

#define VERSION "1.1"

int main (int argc, char **argv)
{
    KControlPanel panel;
    extern flag binary;
    static char function_name[] = "main";

    im_register_lib_version (KARMA_VERSION);
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "binary", "flag", PIT_FLAG, &binary,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "karma2ppm", VERSION, karma2ppm, -1, -1, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag karma2ppm (char *p, FILE *fp)
{
    Channel out;
    iarray image_pseudo, image_red, image_green, image_blue;
    iarray movie_pseudo, movie_red, movie_green, movie_blue;
    unsigned int cmap_index, count, num_frames;
    multi_array *multi_desc;
    char *inp_filename, *out_filename;
    char fname[STRING_LENGTH];
    extern flag binary;
    extern char *data_type_names[NUMTYPES];
    extern char *sys_errlist[];

    if ( ( inp_filename = ex_word (p, &p) ) == NULL )
    {
	fprintf (stderr, "Must supply input and output filenames\n");
	return (TRUE);
    }
    if ( ( out_filename = ex_word (p, &p) ) == NULL )
    {
	fprintf (stderr, "Must also supply output filename\n");
	m_free (inp_filename);
	return (TRUE);
    }
    /*  Read input file  */
    /*  Don't mmap as colourmap might be re-ordered  */
    if ( ( multi_desc = dsxfr_get_multi (inp_filename, FALSE, K_CH_MAP_LOCAL,
					 FALSE) ) == NULL )
    {
	fprintf (stderr, "Error getting arrayfile: \"%s\"\n", inp_filename);
	m_free (inp_filename);
	m_free (out_filename);
	return (TRUE);
    }
    m_free (inp_filename);
    /*  Try to get 2-D image  */
    if ( iarray_get_image_from_multi (multi_desc, &image_pseudo,
				      &image_red, &image_green, &image_blue,
				      &cmap_index) )
    {
	/*  Easy! Open output file and write  */
	/*  Don't need Intelligent Arrays  */
	if (image_pseudo != NULL) iarray_dealloc (image_pseudo);
	if (image_red != NULL) iarray_dealloc (image_red);
	if (image_green != NULL) iarray_dealloc (image_green);
	if (image_blue != NULL) iarray_dealloc (image_blue);
	if ( ( out = ch_open_file (out_filename, "w") ) == NULL )
	{
	    fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		     out_filename, sys_errlist[errno]);
	    m_free (out_filename);
	    ds_dealloc_multi (multi_desc);
	    return (TRUE);
	}
	if ( !foreign_ppm_write (out, multi_desc, binary,
				 FA_PPM_WRITE_END) )
	{
	    fprintf (stderr, "Error writing PPM file\n");
	    unlink (out_filename);
	}
	ch_close (out);
	m_free (out_filename);
	ds_dealloc_multi (multi_desc);
	return (TRUE);
    }
    /*  Try to get movie  */
    if ( !iarray_get_movie_from_multi (multi_desc, &movie_pseudo,
				       &movie_red, &movie_green, &movie_blue,
				       &cmap_index) )
    {
	fprintf (stderr, "Error getting movie\n");
	ds_dealloc_multi (multi_desc);
	return (TRUE);
    }
    ds_dealloc_multi (multi_desc);
    if (movie_pseudo != NULL)
    {
	fprintf (stderr, "PseudoColour movies not supported\n");
	ds_dealloc_multi (multi_desc);
	iarray_dealloc (movie_red);
	iarray_dealloc (movie_green);
	iarray_dealloc (movie_blue);
	return (TRUE);
    }
    /*  Sanity checking  */
    if (iarray_type (movie_red) != K_UBYTE)
    {
	fprintf (stderr, "Array data is of type: %s, must be ubyte\n",
		 data_type_names[iarray_type (movie_red)]);
	m_free (out_filename);
	iarray_dealloc (movie_red);
	iarray_dealloc (movie_green);
	iarray_dealloc (movie_blue);
	return (TRUE);
    }
    num_frames = iarray_dim_length (movie_red, 0);
    for (count = 0; count < num_frames; ++count)
    {
	sprintf (fname, "%s.%u.ppm", out_filename, count);
	if ( ( out = ch_open_file (fname, "w") ) == NULL )
	{
	    fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		     fname, sys_errlist[errno]);
	    m_free (out_filename);
	    iarray_dealloc (movie_red);
	    iarray_dealloc (movie_green);
	    iarray_dealloc (movie_blue);
	    return (TRUE);
	}
	fprintf (stderr, "Writing frame: \"%s\"\n", fname);
	if ( !foreign_ppm_write_rgb (out, TRUE,
				     (CONST unsigned char *) movie_red->data +
				     movie_red->offsets[0][count],
				     (CONST unsigned char *) movie_green->data+
				     movie_red->offsets[0][count],
				     (CONST unsigned char *) movie_blue->data +
				     movie_red->offsets[0][count],
				     movie_red->offsets[2],
				     movie_red->offsets[1],
				     iarray_dim_length (movie_red, 2),
				     iarray_dim_length (movie_red, 1),
				     NULL, NULL, NULL, 0) )
	{
	    fprintf (stderr, "Error writing PPM file: \"%s\"\n", fname);
	    m_free (out_filename);
	    iarray_dealloc (movie_red);
	    iarray_dealloc (movie_green);
	    iarray_dealloc (movie_blue);
	    ch_close (out);
	    return (TRUE);
	}
	ch_close (out);
    }
    m_free (out_filename);
    iarray_dealloc (movie_red);
    iarray_dealloc (movie_green);
    iarray_dealloc (movie_blue);
    return (TRUE);
}   /*  End Function karma2ppm  */
