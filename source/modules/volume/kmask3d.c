/*  kmask3d.c

    Source file for  kmask3d  (Mask file according to another file).

    Copyright (C) 1996  Richard Gooch

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

/*  This Karma module will read two 2-dimensional arrays. One file will be used
  to compute a new grid for the other file.


    Written by      Richard Gooch   22-NOV-1996

    Last updated by Richard Gooch   22-NOV-1996


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_module.h>
#include <karma_iarray.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_ex.h>
#include <karma_im.h>
#include <karma_m.h>
#include <karma_a.h>


#define VERSION "1.0"


/*  Private data  */
static float mask_threshold = 1.0;
static float gap = 0.0;
static flag relative_gap = FALSE;

#define OPTION_BLANK 0
#define OPTION_SHIFT 1
#define NUM_OPTIONS  2
static char *option_alternatives[NUM_OPTIONS] =
{
    "blank",
    "shift",
};
static unsigned int option = OPTION_BLANK;


/*  Private functions  */
STATIC_FUNCTION (flag kmask3d, (char *p, FILE *fp) );
STATIC_FUNCTION (void mask_file, (CONST char *infile, CONST char *maskfile,
				  CONST char *outfile) );
STATIC_FUNCTION (flag blank_func, (iarray out, iarray in, iarray mask,
				   float in_min, float in_max) );
STATIC_FUNCTION (flag shift_func, (iarray out, iarray in, iarray mask,
				   float in_min, float in_max) );


/*  Public functions follow  */

int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "relative_gap", "gap is relative to input range",
		    PIT_FLAG, &relative_gap,
		    PIA_END);
    panel_add_item (panel, "option", "how to mask",
		    PIT_CHOICE_INDEX, &option,
		    PIA_NUM_CHOICE_STRINGS, NUM_OPTIONS,
		    PIA_CHOICE_STRINGS, option_alternatives,
		    PIA_END);
    panel_add_item (panel, "mask_threshold", "threshold for mask", K_FLOAT,
		    &mask_threshold,
		    PIA_END);
    panel_add_item (panel, "gap", "gap for shift", K_FLOAT, &gap,
		    PIA_END);
    panel_push_onto_stack (panel);
    im_register_lib_version (KARMA_VERSION);
    module_run (argc, argv, "kmask3d", VERSION, kmask3d, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag kmask3d (char *p, FILE *fp)
{
    char *infile, *maskfile, *outfile;

    if ( ( infile = ex_str (p, &p) ) == NULL )
    {
	fprintf (fp, "Error extracting infile name\n");
	return (TRUE);
    }
    if ( ( maskfile = ex_str (p, &p) ) == NULL )
    {
	fprintf (fp, "Error extracting maskfile name\n");
	m_free (infile);
	return (TRUE);
    }
    if ( ( outfile = ex_str (p, &p) ) == NULL )
    {
	fprintf (fp, "Error extracting outfile name\n");
	m_free (infile);
	m_free (maskfile);
	return (TRUE);
    }
    mask_file (infile, maskfile, outfile);
    m_free (infile);
    m_free (maskfile);
    m_free (outfile);
    return (TRUE);
}   /*  End Function kmask3d  */

static void mask_file (CONST char *infile, CONST char *maskfile,
		       CONST char *outfile)
/*  [SUMMARY] Mask a file.
    <infile> The input file to mask.
    <maskfile> The file to use in constructing the mask.
    <outfile> The output file.
    [RETURNS] Nothing.
*/
{
    flag ok = FALSE;
    iarray in_arr = NULL;
    iarray mask_arr = NULL;
    iarray out_arr;
    unsigned int dummy;
    unsigned long xlen, ylen, zlen;
    double in_min, in_max;
    static char function_name[] = "mask_file";

    if ( !foreign_read_and_setup (infile, K_CH_MAP_IF_AVAILABLE, FALSE, &dummy,
				  TRUE, 3, K_FLOAT, TRUE, &in_arr,
				  &in_min, &in_max, TRUE, NULL) ) return;
    xlen = iarray_dim_length (in_arr, 2);
    ylen = iarray_dim_length (in_arr, 1);
    zlen = iarray_dim_length (in_arr, 0);
    if ( !foreign_read_and_setup (maskfile, K_CH_MAP_IF_AVAILABLE, FALSE,
				  &dummy, TRUE, 3, K_FLOAT, TRUE, &mask_arr,
				  NULL, NULL, TRUE, NULL) )
    {
	iarray_dealloc (in_arr);
	return;
    }
    if ( (iarray_dim_length (mask_arr, 2) != xlen) ||
	 (iarray_dim_length (mask_arr, 1) != ylen) ||
	 (iarray_dim_length (mask_arr, 0) != zlen) )
    {
	fprintf (stderr, "Array sizes do not match\n");
	iarray_dealloc (in_arr);
	iarray_dealloc (mask_arr);
	return;
    }
    if ( ( out_arr = iarray_create_from_template (in_arr, K_FLOAT, TRUE, TRUE,
						  TRUE) ) == NULL )
    {
	fprintf (stderr, "Error creating output array\n");
	iarray_dealloc (in_arr);
	iarray_dealloc (mask_arr);
	return;
    }
    switch (option)
    {
      case OPTION_BLANK:
	ok = blank_func (out_arr, in_arr, mask_arr, in_min, in_max);
	break;
      case OPTION_SHIFT:
	ok = shift_func (out_arr, in_arr, mask_arr, in_min, in_max);
	break;
      default:
	fprintf (stderr, "Illegal option value: %u\n", option);
	a_prog_bug (function_name);
	break;
    }
    if (ok) iarray_write (out_arr, outfile);
    iarray_dealloc (in_arr);
    iarray_dealloc (mask_arr);
    iarray_dealloc (out_arr);
}   /*  End Function mask_file  */

static flag blank_func (iarray out, iarray in, iarray mask,
			float in_min, float in_max)
/*  [SUMMARY] Mask an array using plain blanking.
    <out> The output array.
    <in> The input array.
    <mask> The mask array.
    <in_min> The minimum value in the input array.
    <in_max> The maximum value in the input array.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned long x, y, z, xlen, ylen, zlen;
    float mask_val, out_val;
    float toobig = TOOBIG;

    xlen = iarray_dim_length (in, 2);
    ylen = iarray_dim_length (in, 1);
    zlen = iarray_dim_length (in, 0);
    for (z = 0; z < zlen; ++z) for (y = 0; y < ylen; ++y)
	for (x = 0; x < xlen; ++x)
	{
	    if ( ( mask_val = F3 (mask, z, y, x) ) >= toobig )
	    {
		out_val = toobig;
	    }
	    else if (mask_val < mask_threshold) out_val = toobig;
	    else out_val = F3 (in, z, y, x);
	    F3 (out, z, y, x) = out_val;
	}
    return (TRUE);
}   /*  End Function blank_func  */

static flag shift_func (iarray out, iarray in, iarray mask,
			float in_min, float in_max)
/*  [SUMMARY] Mask an array using shifting.
    <out> The output array.
    <in> The input array.
    <mask> The mask array.
    <in_min> The minimum value in the input array.
    <in_max> The maximum value in the input array.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned long x, y, z, xlen, ylen, zlen;
    float in_val, mask_val, out_val, shift;
    float toobig = TOOBIG;

    xlen = iarray_dim_length (in, 2);
    ylen = iarray_dim_length (in, 1);
    zlen = iarray_dim_length (in, 0);
    shift = in_max + gap * relative_gap ? (in_max - in_min) : 1.0 - in_min;
    for (z = 0; z < zlen; ++z) for (y = 0; y < ylen; ++y)
	for (x = 0; x < xlen; ++x)
	{
	    if ( ( mask_val = F3 (mask, z, y, x) ) >= toobig )
	    {
		out_val = toobig;
	    }
	    else if (mask_val < mask_threshold) out_val = F3 (in, z, y, x);
	    else
	    {
		if ( ( in_val = F3 (in, z, y,x) ) >= toobig ) out_val = toobig;
		else out_val = in_val + shift;
	    }
	    F3 (out, z, y, x) = out_val;
	}
    return (TRUE);
}   /*  End Function shift_func  */
