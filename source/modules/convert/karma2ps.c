/*  karma2ps.c

    Source file for  karma2ps  (Karma to PostScript image conversion module).

    Copyright (C) 1994-1996  Richard Gooch

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
    a PostScript file.


    Written by      Richard Gooch   10-MAY-1994

    Updated by      Richard Gooch   20-MAY-1994: Took account of changes to
  psw_  package.

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   3-NOV-1994: Fixed declation of  karma2ps  .

    Last updated by Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_iarray.h>
#include <karma_dsxfr.h>
#include <karma_panel.h>
#include <karma_psw.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>


STATIC_FUNCTION (flag karma2ps, (char *command, FILE *fp) );
STATIC_FUNCTION (flag write_array, (PostScriptPage pspage,
				    multi_array *multi_desc) );
STATIC_FUNCTION (unsigned short *get_cmap, (multi_array *multi_desc,
					    unsigned int *size) );

static flag portrait = TRUE;
static flag encapsulated_postscript = FALSE;
static flag iscale_for_ubyte = TRUE;
static double hoffset = 1.0;
static double voffset = 1.0;
static double hsize = 18.0;
static double vsize = 18.0;

#define VERSION "1.1"

int main (int argc, char **argv)
{
    KControlPanel panel;
    extern flag portrait;
    extern flag encapsulated_postscript;
    extern flag iscale_for_ubyte;
    extern double hoffset;
    extern double voffset;
    extern double hsize;
    extern double vsize;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "vsize", "centimeters", K_DOUBLE, &vsize,
		    PIA_END);
    panel_add_item (panel, "hsize", "centimeters", K_DOUBLE, &hsize,
		    PIA_END);
    panel_add_item (panel, "voffset", "centimeters", K_DOUBLE, &voffset,
		    PIA_END);
    panel_add_item (panel, "hoffset", "centimeters", K_DOUBLE, &hoffset,
		    PIA_END);
    panel_add_item (panel, "iscale_for_ubyte", "flag", PIT_FLAG,
		    &iscale_for_ubyte,
		    PIA_END);
    panel_add_item (panel, "encapsulated_ps", "flag", PIT_FLAG,
		    &encapsulated_postscript,
		    PIA_END);
    panel_add_item (panel, "portrait", "flag", PIT_FLAG, &portrait,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "karma2ps", VERSION, karma2ps, -1, -1, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag karma2ps (p, fp)
char *p;
FILE *fp;
{
    Channel out;
    PostScriptPage pspage;
    multi_array *multi_desc;
    char *inp_filename, *out_filename;
    extern flag portrait;
    extern flag encapsulated_postscript;
    extern double hoffset;
    extern double voffset;
    extern double hsize;
    extern double vsize;
    extern char *sys_errlist[];

    if ( ( inp_filename = ex_word (p, &p) ) == NULL )
    {
	(void) fprintf (stderr, "Must supply input and output filenames\n");
	return (TRUE);
    }
    if ( ( out_filename = ex_word (p, &p) ) == NULL )
    {
	(void) fprintf (stderr, "Must supply input and output filenames\n");
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
    if ( ( pspage = psw_create (out, hoffset, voffset, hsize, vsize,
				portrait) ) != NULL)
    {
	if ( write_array (pspage, multi_desc) )
	{
	    if ( psw_finish (pspage, encapsulated_postscript, TRUE, TRUE) )
	    {
		/*  Success  */
		m_free (inp_filename);
		m_free (out_filename);
		ds_dealloc_multi (multi_desc);
		return (TRUE);
	    }
	}
	else
	{
	    (void) psw_finish (pspage, encapsulated_postscript, FALSE, FALSE);
	    (void) fprintf (stderr, "Error converting to PostScript\n");
	}
    }
    (void) ch_close (out);
    m_free (inp_filename);
    m_free (out_filename);
    ds_dealloc_multi (multi_desc);
    (void) unlink (out_filename);
    return (TRUE);
}   /*  End Function karma2ps  */

static flag write_array (pspage, multi_desc)
/*  This routine will write an image to a PostScriptPage object.
    The PostScriptPage object must be given by  pspage  .
    The input data structure must be pointed to by  multi_desc  .
    The routine returns TRUE on success, else it returns FALSE.
*/
PostScriptPage pspage;
multi_array *multi_desc;
{
    iarray image;
    iarray image_red, image_green, image_blue;
    flag retval;
    unsigned int cmap_size;
    unsigned short *cmap;
    extern flag iscale_for_ubyte;

    if ( (*multi_desc).num_arrays > 1 )
    {
	if ( ( image = iarray_get_from_multi_array (multi_desc, "Frame", 2,
						    NULL, NULL) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Error getting Intelligent Array: Frame\n");
	    return (FALSE);
	}
	(void) fprintf (stderr, "Read: %u structures\n",
			(*multi_desc).num_arrays);
	cmap = get_cmap (multi_desc, &cmap_size);
	if (cmap == NULL)
	{
	    /*  Monochrome  */
	    retval = iarray_write_mono_ps (image, pspage, 0.0, 0.0, 1.0, 1.0,
					   iscale_for_ubyte);
	    iarray_dealloc (image);
	    return (retval);
	}
	/*  Pseudocolour  */
	retval = iarray_write_pseudocolour_ps (image, pspage,
					       0.0, 0.0, 1.0, 1.0,
					       cmap, cmap_size);
	iarray_dealloc (image);
	return (retval);
    }
    /*  Only one data structure in file  */
    if ( ( image = iarray_get_from_multi_array (multi_desc, NULL, 2,
						NULL, NULL) ) != NULL )
    {
	/*  Monochrome  */
	retval = iarray_write_mono_ps (image, pspage, 0.0, 0.0, 1.0, 1.0,
				       iscale_for_ubyte);
	iarray_dealloc (image);
	return (retval);
    }
    (void) fprintf (stderr,
		    "Error getting PseudoColour Intelligent Array\n");
    (void) fprintf (stderr, "Trying TrueColour...\n");
    if ( ( image_red =
	  iarray_get_from_multi_array (multi_desc, NULL, 2, NULL,
				       "Red Intensity") )
	== NULL )
    {
	(void) fprintf (stderr, "Error getting red array\n");
	return (FALSE);
    }
    if (iarray_type (image_red) != K_UBYTE)
    {
	(void) fprintf (stderr, "Red array must be of type K_UBYTE\n");
	iarray_dealloc (image_red);
	return (FALSE);
    }
    if ( ( image_green =
	  iarray_get_from_multi_array (multi_desc, NULL, 2, NULL,
				       "Green Intensity") )
	== NULL )
    {
	(void) fprintf (stderr, "Error getting green array\n");
	iarray_dealloc (image_red);
	return (FALSE);
    }
    if (iarray_type (image_green) != K_UBYTE)
    {
	(void) fprintf (stderr,
			"Green array must be of type K_UBYTE\n");
	iarray_dealloc (image_red);
	iarray_dealloc (image_green);
	return (FALSE);
    }
    if ( (*image_red).arr_desc != (*image_green).arr_desc )
    {
	(void) fprintf (stderr,
			"Green array descriptor different than red\n");
	iarray_dealloc (image_red);
	iarray_dealloc (image_green);
	return (FALSE);
    }
    if ( ( image_blue =
	  iarray_get_from_multi_array (multi_desc, NULL, 2, NULL,
				       "Blue Intensity") )
	== NULL )
    {
	(void) fprintf (stderr, "Error getting blue array\n");
	iarray_dealloc (image_red);
	iarray_dealloc (image_green);
	return (FALSE);
    }
    if (iarray_type (image_blue) != K_UBYTE)
    {
	(void) fprintf (stderr,
			"Blue array must be of type K_UBYTE\n");
	iarray_dealloc (image_red);
	iarray_dealloc (image_green);
	iarray_dealloc (image_blue);
	return (FALSE);
    }
    if ( (*image_red).arr_desc != (*image_blue).arr_desc )
    {
	(void) fprintf (stderr,
			"Blue array descriptor different than red\n");
	iarray_dealloc (image_red);
	iarray_dealloc (image_green);
	iarray_dealloc (image_blue);
	return (FALSE);
    }
    /*  Truecolour  */
    retval = iarray_write_rgb_ps (image_red, image_green, image_blue,
				  pspage, 0.0, 0.0, 1.0, 1.0);
    iarray_dealloc (image_red);
    iarray_dealloc (image_green);
    iarray_dealloc (image_blue);
    return (retval);
}   /*  End Function write_array  */

static unsigned short *get_cmap (multi_desc, size)
/*  This routine will attempt to load the colourmap for a frame.
    The data structure to get the colourmap from must be pointed to by
    multi_desc  .
    The size of the colourmap will be written to the storage pointed to by
    size  .
    The routine returns a pointer to the colourmap on success,
    else it returns NULL.
*/
multi_array *multi_desc;
unsigned int *size;
{
    unsigned int cmap_index;
    unsigned int dummy;
    static char function_name[] = "get_cmap";

    /*  Check if colourmap update required  */
    if ( (*multi_desc).num_arrays > 1 )
    {
	/*  Check colourmap  */
	switch ( dummy = ds_f_array_name (multi_desc, "RGBcolourmap",
					  (char **) NULL, &cmap_index) )
	{
	  case IDENT_NOT_FOUND:
	    (void) fprintf (stderr, "Colourmap not found for frame\n");
	    break;
	  case IDENT_GEN_STRUCT:
	    /*  Got it!  */
	    return (ds_cmap_find_colourmap ( (*multi_desc).headers[cmap_index],
					    (*multi_desc).data[cmap_index],
					    size, (flag *) NULL,
					    (CONST char **) NULL,
					    (double *) NULL,
					    0) );
/*
	    break;
*/
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr,
			    "Multiple RGBcolourmap structures found\n");
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value: %u from: ds_f_array_name\n",
			    dummy);
	    a_prog_bug (function_name);
	    break;
	}
    }
    return (NULL);
}   /*  End Function get_cmap  */
