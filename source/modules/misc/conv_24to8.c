/*  conv_24to8.c

    Source file for  conv_24to8  (colourmap compression module).

    Copyright (C) 1993  Richard Gooch

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

/*  This Karma module will convert a 24bit TrueColour movie to an 8bit
    PseudoColour movie.


    Written by      Richard Gooch   15-SEP-1993

    Updated by      Richard Gooch   19-SEP-1993

    Updated by      Richard Gooch   9-OCT-1993: Changed over to  panel_
  package for command line user interface and moved  main  into this file.

    Updated by      Richard Gooch   23-NOV-1993: Changed to use of
  ex_word_skip  .

    Last updated by Richard Gooch   9-JUN-1995: Added #include <karma_ds.h>.


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_panel.h>
#include <karma_imc.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_a.h>

#define VERSION "1.1"

#define MAX_COLOURS 256

#define COMPRESSION_7BIT 0
#define COMPRESSION_EACH_FRAME 1
#define COMPRESSION_WHOLE_CUBE 2
#define NUM_COMPRESSIONS 3

EXTERN_FUNCTION (flag conv_24to8, (char *command, FILE *fp) );


static void process_arrayfile ();
static void add_cmap (/* multi_desc, pack_desc, packet */);


static int max_colours = MAX_COLOURS;

static int compression_type = COMPRESSION_7BIT;
char *compression_alternatives[NUM_COMPRESSIONS] =
{
    "7bit",
    "per_frame",
    "whole_cube"
};


main (argc, argv)
int argc;       /*  Count of parameters on command line */
char **argv;    /*  List of command line parameters     */
{
    KControlPanel panel;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "compression", "type", PIT_CHOICE_INDEX,
		    &compression_type,
		    PIA_NUM_CHOICE_STRINGS, NUM_COMPRESSIONS,
		    PIA_CHOICE_STRINGS, compression_alternatives,
		    PIA_END);
    panel_add_item (panel, "max_colours", "number", K_INT, &max_colours,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "conv_24to8", VERSION, conv_24to8, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag conv_24to8 (p, fp)
char *p;
FILE *fp;
{
    char filename[STRING_LENGTH+1];
    extern int max_colours;

    if (max_colours > MAX_COLOURS)
    {
	(void) fprintf (stderr, "max_colours: %d  too big: setting to: %d\n",
			max_colours, MAX_COLOURS);
	max_colours = MAX_COLOURS;
    }
    for ( ; p != NULL; p = ex_word_skip (p) )
    {
	(void) sscanf (p, "%s", filename);
	process_arrayfile (filename);
    }
    return (TRUE);
}   /*  End Function conv_24to8  */

static void process_arrayfile (arrayfile)
/*  This routine will process an arrayfile.
    The name of the arrayfile must be pointed to by  arrayfile  .
    The routine returns TRUE on success, else it returns FALSE.
*/
char *arrayfile;
{
    iarray cube24, cube8;
    unsigned int cmap_size;
    char *top_packet;
    array_desc *arr_desc;
    packet_desc *top_pack_desc;
    char txt[STRING_LENGTH];
    unsigned long dim_lengths[3];
    char *dim_names[3];
    unsigned char *palette_reds[MAX_COLOURS];
    unsigned char *palette_greens[MAX_COLOURS];
    unsigned char *palette_blues[MAX_COLOURS];
    extern int compression_type;
    extern char module_name[STRING_LENGTH + 1];
    static char function_name[] = "process_arrayfile";

    if (arrayfile == NULL)
    {
	(void) fprintf (stderr, "NULL array pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Load input array  */
    if ( ( cube24 = iarray_read_nD (arrayfile, TRUE, NULL, 3, (char **) NULL,
				    "Red Intensity", K_CH_MAP_IF_AVAILABLE) )
	== NULL )
    {
	return;
    }
    dim_lengths[0] = iarray_dim_length (cube24, 0);
    dim_lengths[1] = iarray_dim_length (cube24, 1);
    dim_lengths[2] = iarray_dim_length (cube24, 2);
    dim_names[0] = iarray_dim_name (cube24, 0);
    dim_names[1] = iarray_dim_name (cube24, 1);
    dim_names[2] = iarray_dim_name (cube24, 2);
    if ( ( cube8 = iarray_create (K_UBYTE, 3, dim_names, dim_lengths,
				  "PseudoColour", /* cube24 */ NULL) )
	== NULL )
    {
	iarray_dealloc (cube24);
	return;
    }
    arr_desc = (*cube24).arr_desc;
    /*  Create output object name. Don't use  sprintf(3)  because it's broken
	in the C library for the VX/MVX (although I don't recall having
	problems with this particular invocation), and besides, it results in
	a Host Procedure Call (ie. it's slow).
	*/
    (void) strcpy (txt, module_name);
    (void) strcat (txt, "_");
    (void) strcat (txt, arrayfile);
    switch (compression_type)
    {
      case COMPRESSION_7BIT:
	/*  Do the whole cube in in fell swoop  */
	if (imc_24to8 (ds_get_array_size (arr_desc),
		       (unsigned char *) (*cube24).data,
		       (unsigned char *) (*cube24).data + 1,
		       (unsigned char *) (*cube24).data + 2,
		       ds_get_packet_size ( (*arr_desc).packet),
		       (unsigned char *) (*cube8).data, 1, max_colours, 9,
		       &top_pack_desc, &top_packet) != TRUE)
	{
	    iarray_dealloc (cube24);
	    iarray_dealloc (cube8);
	    return;
	}
	add_cmap ( &(*cube8).multi_desc, top_pack_desc, top_packet );
	break;
      case COMPRESSION_EACH_FRAME:
	(void) fprintf (stderr, "Not finished per frame compression\n");
	break;
      case COMPRESSION_WHOLE_CUBE:
	/*  Do the whole cube in in fell swoop  */
	if (imc_24to8 (ds_get_array_size (arr_desc),
		       (unsigned char *) (*cube24).data,
		       (unsigned char *) (*cube24).data + 1,
		       (unsigned char *) (*cube24).data + 2,
		       ds_get_packet_size ( (*arr_desc).packet ),
		       (unsigned char *) (*cube8).data, 1, max_colours, 0,
		       &top_pack_desc, &top_packet) != TRUE)
	{
	    iarray_dealloc (cube24);
	    iarray_dealloc (cube8);
	    return;
	}
	add_cmap ( &(*cube8).multi_desc, top_pack_desc, top_packet );
	break;
    }
    iarray_write (cube8, txt);
    iarray_dealloc (cube24);
    iarray_dealloc (cube8);
}   /*  End Function process_arrayfile  */


static void add_cmap (multi_desc, pack_desc, packet)
/*  This routine will add a colourmap to a multi_aray data structure.
    The multi_array header pointer must be pointed to by  multi_desc  .This
    pointer will be updated with a new pointer.
    The pointer to the top level packet descriptor of the general data
    structure which contains the colourmap must be pointed to by  pack_desc  .
    The pointer to the top level packet of the general data structure which
    contains the colourmap must be pointed to by  packet  .
    The routine returns nothing.
*/
multi_array **multi_desc;
packet_desc *pack_desc;
char *packet;
{
    multi_array *new_multi_desc;
    static char function_name[] = "add_cmap";

    if ( ( new_multi_desc = ds_alloc_multi (2) ) == NULL )
    {
	m_abort (function_name, "multi_array");
    }
    if ( ( (*new_multi_desc).array_names[0] = st_dup ("Movie") ) == NULL )
    {
	m_abort (function_name, "movie name");
    }
    if ( ( (*new_multi_desc).array_names[1] = st_dup ("RGBcolourmap") )
	== NULL )
    {
	m_abort (function_name, "colourmap name");
    }
    (*new_multi_desc).headers[0] = (**multi_desc).headers[0];
    (*new_multi_desc).data[0] = (**multi_desc).data[0];
    (*new_multi_desc).headers[1] = pack_desc;
    (*new_multi_desc).data[1] = packet;
    (**multi_desc).headers[0] = NULL;
    (**multi_desc).data[0] = NULL;
    ds_dealloc_multi (*multi_desc);
    *multi_desc = new_multi_desc;
}   /*  End Function add_cmap  */
