/*  karma2sunras.c

    Source file for  karma2sunras  (Karma to Sun rasterfile image/movie
    conversion module).

    Copyright (C) 1995  Richard Gooch

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

/*  This Karma module will read in a Karma data file and convert the
    image/movie to a Sun Rasterfile.


    Written by      Richard Gooch   6-SEP-1995

    Last updated by Richard Gooch   6-SEP-1995


*/
#include <stdio.h>
#include <math.h>
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

STATIC_FUNCTION (flag karma2sunras, (char *command, FILE *fp) );
STATIC_FUNCTION (flag write_movie,
		 (CONST char *filename,
		  iarray pseudo, iarray red, iarray green, iarray blue,
		  packet_desc *cmap_pack_desc, char *cmap_packet) );

#define VERSION "1.0"

main (argc, argv)
int argc;
char **argv;
{
    KControlPanel panel;
    extern flag binary;
    static char function_name[] = "main";

    im_register_lib_version (KARMA_VERSION);
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_push_onto_stack (panel);
    module_run (argc, argv, "karma2sunras", VERSION, karma2sunras, -1, -1,
		FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag karma2sunras (p, fp)
char *p;
FILE *fp;
{
    Channel out;
    iarray pseudo, red, blue, green;
    flag ok, no_image_found;
    unsigned int cmap_index;
    multi_array *multi_desc;
    packet_desc *pack_desc;
    char *packet;
    char *inp_filename, *out_filename;
    extern flag binary;
    extern char *sys_errlist[];

    if ( ( inp_filename = ex_word (p, &p) ) == NULL )
    {
	(void) fprintf (stderr, "Must supply input and output filenames\n");
	return (TRUE);
    }
    if ( ( out_filename = ex_word (p, &p) ) == NULL )
    {
	(void) fprintf (stderr, "Must also supply output filename\n");
	m_free (inp_filename);
	return (TRUE);
    }
    /*  Read input file  */
    /*  Don't mmap as colourmap might be re-ordered  */
    if ( ( multi_desc = dsxfr_get_multi (inp_filename, FALSE,
					 K_CH_MAP_NEVER, FALSE) )
	== NULL )
    {
	(void) fprintf (stderr, "Error getting arrayfile: \"%s\"\n",
			inp_filename);
	m_free (inp_filename);
	m_free (out_filename);
	return (TRUE);
    }
    if ( ( out = ch_open_file (out_filename, "w") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			out_filename, sys_errlist[errno]);
	m_free (inp_filename);
	m_free (out_filename);
	ds_dealloc_multi (multi_desc);
	return (TRUE);
    }
    ok = foreign_sunras_write (out, multi_desc,
			       FA_SUNRAS_WRITE_NO_IMAGE, &no_image_found,
			       FA_SUNRAS_WRITE_END);
    (void) ch_close (out);
    if ( ok || (!ok && !no_image_found) )
    {
	m_free (inp_filename);
	m_free (out_filename);
	ds_dealloc_multi (multi_desc);
    }
    if (ok) return (TRUE);
    (void) unlink (out_filename);
    if (!no_image_found)
    {
	(void) fprintf (stderr, "Error writing Sun Rasterfile\n");
	return (TRUE);
    }
    if ( iarray_get_movie_from_multi (multi_desc, &pseudo, &red, &green, &blue,
				      &cmap_index) )
    {
	/*  Got it  */
	if (cmap_index >= multi_desc->num_arrays)
	{
	    pack_desc = NULL;
	    packet = NULL;
	}
	else
	{
	    pack_desc = multi_desc->headers[cmap_index];
	    packet = multi_desc->data[cmap_index];
	}
	ok = write_movie (out_filename, pseudo, red, green, blue,
			  pack_desc, packet);
	if (pseudo != NULL) iarray_dealloc (pseudo);
	if (red != NULL) iarray_dealloc (red);
	if (green != NULL) iarray_dealloc (green);
	if (blue != NULL) iarray_dealloc (blue);
    }
    else ok = FALSE;
    m_free (inp_filename);
    m_free (out_filename);
    ds_dealloc_multi (multi_desc);
    return (TRUE);
}   /*  End Function karma2sunras  */

static flag write_movie (CONST char *filename, iarray pseudo, iarray red,
			 iarray green, iarray blue,
			 packet_desc *cmap_pack_desc, char *cmap_packet)
/*  [PURPOSE] This routine will write a movie to sequence of Sun Rasterfiles.
    The movie will be either PseudoColour (single channel) or TrueColour
    (3 channel).
    <filename> The base filename to create. Each frame will have the name
    "filename.%u.sun" where %u is the frame number, counting from 0.
    <pseudo> The PseudoColour movie. If NULL, no single-channel movie available
    <red> The red component TrueColour movie. If NULL, no 3-channel movie
    available.
    <green> The green component TrueColour movie. If NULL, no 3-channel movie
    available.
    <blue> The blue component TrueColour movie. If NULL, no 3-channel movie
    available.
    <cmap_pack_desc> The packet descriptor for the PseudoColour colourmap. If
    NULL, the PseudoColour movie is assumed to be GreyScale.
    <cmap_packet> The packet for the PseudoColour colourmap. If NULL, the
    PseudoColour movie is assumed to be GreyScale.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    flag reordered;
    unsigned int frame_count, cmap_size;
    double i_min, i_max;
    unsigned short *cmap;
    char fname[STRING_LENGTH];
    extern char *sys_errlist[];
    static char function_name[] = "write_movie";

    if (red != NULL)
    {
	/*  TrueColour movie: the easiest!  */
	(void) fprintf (stderr, "%s: TrueColour movie not supported yet\n",
			function_name);
	return (FALSE);
    }
    /*  PseudoColour movie  */
    if ( !iarray_min_max (pseudo, CONV1_REAL, &i_min, &i_max) ) return (FALSE);
    if (cmap_pack_desc == NULL)
    {
	cmap_size = 0;
	cmap = NULL;
    }
    else
    {
	if ( ( cmap = ds_cmap_find_colourmap (cmap_pack_desc, cmap_packet,
					      &cmap_size, &reordered,
					      (CONST char **) NULL,
					      (double *) NULL, 0) )
	    == NULL ) return (FALSE);
    }
    (void) fprintf ( stderr, "Frames to write: %u\n",
		    iarray_dim_length (pseudo, 0) );
    for (frame_count = 0; frame_count < iarray_dim_length (pseudo, 0);
	 ++frame_count)
    {
	(void) sprintf (fname, "%s.%u.ras", filename, frame_count);
	if ( ( channel = ch_open_file (fname, "w") ) == NULL )
	{
	    (void) fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			    fname, sys_errlist[errno]);
	    return (FALSE);
	}
	(void) fprintf (stderr, "%u  ", frame_count);
	if ( !foreign_sunras_write_pseudo (channel,
					   pseudo->data +
					   pseudo->offsets[0][frame_count],
					   iarray_type (pseudo),
					   pseudo->offsets[2],
					   pseudo->offsets[1],
					   iarray_dim_length (pseudo, 2),
					   iarray_dim_length (pseudo, 1),
					   cmap, cmap + 1, cmap + 2, cmap_size,
					   3, i_min, i_max) )
	{
	    (void) ch_close (channel);
	    (void) unlink (fname);
	    return (FALSE);
	}
	(void) ch_close (channel);
    }
    (void) fprintf (stderr, "\tdone\n");
    return (TRUE);
}   /*  End Function write_movie  */
