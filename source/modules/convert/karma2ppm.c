/*  karma2ppm.c

    Source file for  karma2ppm  (Karma to PPM image conversion module).

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

/*  This Karma module will read in a Karma data file and convert the image into
    a PPM file.


    Written by      Richard Gooch   15-APR-1995

    Last updated by Richard Gooch   15-APR-1995


*/
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_module.h>
#include <karma_dsxfr.h>
#include <karma_panel.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_im.h>

STATIC_FUNCTION (flag karma2ppm, (char *command, FILE *fp) );

static flag binary = TRUE;

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
    panel_add_item (panel, "binary", "flag", PIT_FLAG, &binary,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "karma2ppm", VERSION, karma2ppm, -1, -1, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag karma2ppm (p, fp)
char *p;
FILE *fp;
{
    Channel out;
    multi_array *multi_desc;
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
    if ( !foreign_ppm_write (out, multi_desc, binary,
			     FA_PPM_WRITE_END) )
    {
	(void) fprintf (stderr, "Error writing PPM file\n");
	(void) unlink (out_filename);
    }
    (void) ch_close (out);
    m_free (inp_filename);
    m_free (out_filename);
    ds_dealloc_multi (multi_desc);
    return (TRUE);
}   /*  End Function karma2ppm  */
