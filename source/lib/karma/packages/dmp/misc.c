/*LINTLIBRARY*/
/*  misc.c

    This code provides routines to write the Karma data structure to Channels.

    Copyright (C) 1992-1996  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*

    This file contains the various utility routines for writing the general
    data structure supported in Karma to files in an ASCII format.


    Written by      Richard Gooch   3-OCT-1992

    Updated by      Richard Gooch   9-NOV-1992

    Updated by      Richard Gooch   3-JAN-1993: Added support for K_ARRAY

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   13-JAN-1993: Added support for old ARRAYP
  in  dmp_element_desc  .

    Updated by      Richard Gooch   25-MAR-1993: Took account of changes to
  list_header  structure.

    Updated by      Richard Gooch   28-MAR-1993: Added support for tiling
  specification with  K_ARRAY  and removed support for old  ARRAYP  in
  dmp_element_desc  .

    Updated by      Richard Gooch   17-MAY-1993: Added support for string data
  types.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/dmp/misc.c

    Updated by      Richard Gooch   19-APR-1995: Cleaned some code.

    Updated by      Richard Gooch   5-JUN-1995: Added code to cope with
  misaligned data accesses on sensitive platforms.

    Updated by      Richard Gooch   7-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   3-JUN-1996: Took account of new fields in
  dimension descriptor for first and last co-ordinate.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_dmp.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>
#include <os.h>


#define ARRAYP        6    /*  Backwards compatibility  */

/*PUBLIC_FUNCTION*/
void dmp_multi_desc (FILE *fp, multi_array *multi_desc, flag comments)
/*  [SUMMARY] Dump a multi_array descriptor to a file.
    [PURPOSE] This routine will dump a multi_array descriptor to a file,
    writing in ASCII format.
    <fp> The file pointer.
    <multi_desc> The descriptor.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    unsigned int array_count;

    if (fp == NULL)
    {
	return;
    }
    (void) fprintf (fp, "\n");
    if (multi_desc == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No multi_desc descriptor to dump\n");
        }
        return;
    }
    /*  Print "MULTI_ARRAY" */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n",
			"MULTI_ARRAY", "#Multi array descriptor");
    }
    else
    {
	(void) fprintf (fp, "MULTI_ARRAY\n");
    }
    /*  Print number of data structures */
    if (comments)
    {
	(void) fprintf (fp, "\t%-32u%s\n", multi_desc->num_arrays,
			"#The number of general data structures");
    }
    else
    {
	(void) fprintf (fp, "\t%u\n", multi_desc->num_arrays);
    }
    if ( multi_desc->num_arrays > 1 )
    {
	/*  Print array names   */
        array_count = 0;
        while (array_count < multi_desc->num_arrays)
        {
	    if (comments)
            {
		(void) fprintf (fp, "\t%-32s%s %u\n",
				multi_desc->array_names[array_count],
				"#Name of array", array_count);
            }
            else
            {
		(void) fprintf (fp, "\t%s\n",
				multi_desc->array_names[array_count]);
            }
            ++array_count;
        }
    }
    /*  Print "END" */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n", "END", "#End multi array descriptor");
    }
    else
    {
	(void) fprintf (fp, "END\n");
    }
    /*  Print packet descriptors for each data structure    */
    array_count = 0;
    while (array_count < multi_desc->num_arrays)
    {
	dmp_packet_desc (fp, multi_desc->headers[array_count], comments);
        ++array_count;
    }
    if (comments)
    {
	(void) fprintf (fp, "#End multi array data structure definition\n");
    }
}   /*  End Function dmp_multi_desc    */

/*PUBLIC_FUNCTION*/
void dmp_packet_desc (FILE *fp, packet_desc *pack_desc, flag comments)
/*  [SUMMARY] Dump a packet descriptor to a file.
    [PURPOSE] This routine will dump a packet descriptor to a file,
    writing in ASCII format.
    <fp> The file pointer.
    <pack_desc> The descriptor.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    unsigned int element_count = 0;

    if (fp == NULL)
    {
	return;
    }
    (void) fprintf (fp, "\n");
    if (pack_desc == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No packet descriptor to dump\n");
        }
        return;
    }
    /*  Print "PACKET"  */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n", "PACKET", "#Packet descriptor");
    }
    else
    {
	(void) fprintf (fp, "PACKET\n");
    }
    /*  Print number of elements    */
    if (comments)
    {
	(void) fprintf (fp, "\t%-32u%s\n", pack_desc->num_elements,
			"#Number of elements in this packet");
    }
    else
    {
	(void) fprintf (fp, "\t%u\n", pack_desc->num_elements);
    }
    /*  Print "END" */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n", "END", "#End packet descriptor");
    }
    else
    {
	(void) fprintf (fp, "END\n");
    }
    /*  Print element descriptors   */
    while (element_count < pack_desc->num_elements)
    {
	/*  Print an element descriptor */
        dmp_element_desc (fp, pack_desc->element_types[element_count],
			  pack_desc->element_desc[element_count],
			  comments);
        ++element_count;
    }
}   /*  End Function dmp_packet_desc   */

/*PUBLIC_FUNCTION*/
void dmp_element_desc (FILE *fp, unsigned int type, char *desc, flag comments)
/*  [SUMMARY] Dump an element descriptor to a file.
    [PURPOSE] This routine will dump an element descriptor to a file, writing
    in ASCII.
    <fp> The file.
    <type> The type of the element.
    <desc> The element descriptor. If the element is an array pointer or a
    linked list pointer, that descriptor will also be dumped.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "dmp_element_desc";

    if (fp == NULL)
    {
	return;
    }
    (void) fprintf (fp, "\n");
    if (desc == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No element descriptor to dump\n");
        }
        return;
    }
    /*  Print "ELEMENT" */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n", "ELEMENT", "#Element descriptor");
    }
    else
    {
	(void) fprintf (fp, "ELEMENT\n");
    }
    /*  Print element type  */
    switch (type)
    {
      case ARRAYP:
	(void) fprintf (stderr, "Should not be writing type ARRAYP\n");
	a_prog_bug (function_name);
#ifdef dummy
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "ARRAYP",
			    "#Pointer to an unpadded array");
	    (void) fprintf (fp, "%-40s%s\n", "END",
			    "#End element descriptor");
	}
	else
	{
	    (void) fprintf (fp, "\tARRAYP\n");
	    (void) fprintf (fp, "END\n");
	}
	dmp_array_desc (fp, (array_desc *) desc, comments);
	return;
#endif
	break;
      case K_ARRAY:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "K_ARRAY",
			    "#Pointer to a (padded) array");
	    (void) fprintf (fp, "%-40s%s\n", "END",
			    "#End element descriptor");
	}
	else
	{
	    (void) fprintf (fp, "\tK_ARRAY\n");
	    (void) fprintf (fp, "END\n");
	}
	dmp_array_desc (fp, (array_desc *) desc, comments);
	return;
/*
	break;
*/
      case LISTP:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "LISTP",
			    "#Pointer to a linked list header");
	    (void) fprintf (fp, "%-40s%s\n", "END",
			    "#End element descriptor");
	}
	else
	{
	    (void) fprintf (fp, "\tLISTP\n");
	    (void) fprintf (fp, "END\n");
	}
	dmp_packet_desc (fp, (packet_desc *) desc, comments);
	return;
/*
	break;
*/
      case K_FLOAT:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "FLOAT",
			    "#Floating type");
	}
	else
	{
	    (void) fprintf (fp, "\tFLOAT\n");
	}
	break;
      case K_DOUBLE:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "DOUBLE",
			    "#Double type");
	}
	else
	{
	    (void) fprintf (fp, "\tDOUBLE\n");
	}
	break;
      case K_BYTE:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "BYTE",
			    "#Byte type");
	}
	else
	{
	    (void) fprintf (fp, "\tBYTE\n");
	}
	break;
      case K_INT:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "INT",
			    "#Integer type");
	}
	else
	{
	    (void) fprintf (fp, "\tINT\n");
	}
	break;
      case K_SHORT:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "SHORT",
			    "#Short type");
	}
	else
	{
	    (void) fprintf (fp, "\tSHORT\n");
	}
	break;
      case K_COMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "COMPLEX",
			    "#Complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tCOMPLEX\n");
	}
	break;
      case K_DCOMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "DCOMPLEX",
			    "#Double complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tDCOMPLEX\n");
	}
	break;
      case K_BCOMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "BCOMPLEX",
			    "#Byte complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tBCOMPLEX\n");
	}
	break;
      case K_ICOMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "ICOMPLEX",
			    "#Integer complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tICOMPLEX\n");
	}
	break;
      case K_SCOMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "SCOMPLEX",
			    "#Short complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tSCOMPLEX\n");
	}
	break;
      case K_LONG:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "LONG",
			    "#Long type");
	}
	else
	{
	    (void) fprintf (fp, "\tLONG\n");
	}
	break;
      case K_LCOMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "LCOMPLEX",
			    "#Long complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tLCOMPLEX\n");
	}
	break;
      case K_UBYTE:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "UBYTE",
			    "#Unsigned byte type");
	}
	else
	{
	    (void) fprintf (fp, "\tUBYTE\n");
	}
	break;
      case K_UINT:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "UINT",
			    "#Unsigned int type");
	}
	else
	{
	    (void) fprintf (fp, "\tUINT\n");
	}
	break;
      case K_USHORT:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "USHORT",
			    "#Unsigned short type");
	}
	else
	{
	    (void) fprintf (fp, "\tUSHORT\n");
	}
	break;
      case K_ULONG:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "ULONG",
			    "#Unsigned long type");
	}
	else
	{
	    (void) fprintf (fp, "\tULONG\n");
	}
	break;
      case K_UBCOMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "UBCOMPLEX",
			    "#Unsigned byte complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tUBCOMPLEX\n");
	}
	break;
      case K_UICOMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "UICOMPLEX",
			    "#Unsigned int complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tUICOMPLEX\n");
	}
	break;
      case K_USCOMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "USCOMPLEX",
			    "#Unsigned short complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tUSCOMPLEX\n");
	}
	break;
      case K_ULCOMPLEX:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "ULCOMPLEX",
			    "#Unsigned long complex type");
	}
	else
	{
	    (void) fprintf (fp, "\tULCOMPLEX\n");
	}
	break;
      case K_VSTRING:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "VSTRING",
			    "#Pointer to variable string");
	}
	else
	{
	    (void) fprintf (fp, "\tVSTRING\n");
	}
	break;
      case K_FSTRING:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "FSTRING",
			    "#Pointer to fixed string");
	}
	else
	{
	    (void) fprintf (fp, "\tFSTRING\n");
	}
	break;
      default:
	if (comments)
	{
	    (void) fprintf (fp, "\t%-32s%s\n", "NONE",
			    "#Invalid data type");
	}
	else
	{
	    (void) fprintf (fp, "\tNONE\n");
	}
	break;
    }
    /*  Print element name  */
    if (comments)
    {
	(void) fprintf (fp, "\t%-32s%s\n", desc, "#Element name");
    }
    else
    {
	(void) fprintf (fp, "\t%s\n", desc);
    }
    /*  Print "END" */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n", "END", "#End element descriptor");
    }
    else
    {
	(void) fprintf (fp, "END\n");
    }           
}   /*  End Function dmp_element_desc  */

/*PUBLIC_FUNCTION*/
void dmp_array_desc (FILE *fp, array_desc *arr_desc, flag comments)
/*  [SUMMARY] Dump an array descriptor to a file.
    [PURPOSE] This routine will dump an array descriptor to a file,
    writing in ASCII format. The packet descriptor for the array will also be
    dumped (this is a recursive process).
    <fp> The file pointer.
    <array_desc> The descriptor.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    unsigned int dim_count;
    unsigned int level_count;

    if (fp == NULL)
    {
	return;
    }
    (void) fprintf (fp, "\n");
    if (arr_desc == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No array descriptor to dump\n");
        }
        return;
    }
    /*  Print "ARRAY"   */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n",
			"ARRAY", "#Array descriptor (padded array)");
    }
    else
    {
	(void) fprintf (fp, "ARRAY\n");
    }
    /*  Print number of dimensions  */
    if (comments)
    {
	(void) fprintf (fp, "\t%-32u%s\n", arr_desc->num_dimensions,
			"#Number of dimensions");
    }
    else
    {
	(void) fprintf (fp, "\t%u\n", arr_desc->num_dimensions);
    }
    /*  Print number of levels of tiling  */
    if (comments)
    {
	(void) fprintf (fp, "\t%-32u%s\n", arr_desc->num_levels,
			"#Number of levels of tiling");
    }
    else
    {
	(void) fprintf (fp, "\t%u\n", arr_desc->num_levels);
    }
    /*  Print tile lengths  */
    for (dim_count = 0; dim_count < arr_desc->num_dimensions; ++dim_count)
    {
	/*  Print tile lengths for this dimension  */
	for (level_count = 0; level_count < arr_desc->num_levels;
	     ++level_count)
	{
	    if (comments)
	    {
		(void) fprintf (fp, "\t%-32u%s %u  %s %u\n",
				arr_desc->tile_lengths[dim_count][level_count],
				"#Tile length for dimension:", dim_count,
				"level:", level_count);
	    }
	    else
	    {
		(void) fprintf (fp, "\t%u\n",
				arr_desc->tile_lengths[dim_count][level_count]);
	    }
	}
    }
    /*  Print "END" */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n", "END", "#End array descriptor");
    }
    else
    {
	(void) fprintf (fp, "END\n");
    }
    /*  Print dimension descriptors */
    for (dim_count = 0; dim_count < arr_desc->num_dimensions; ++dim_count)
    {
	dmp_dim_desc (fp, arr_desc->dimensions[dim_count], comments);
    }
    /*  Print array packet descriptor   */
    dmp_packet_desc (fp, arr_desc->packet, comments);
}   /*  End Function dmp_array_desc    */

/*PUBLIC_FUNCTION*/
void dmp_dim_desc (FILE *fp, dim_desc *dimension, flag comments)
/*  [SUMMARY] Dump a dimension descriptor to a file.
    [PURPOSE] This routine will dump a dimension descriptor to a file,
    writing in ASCII format. The packet descriptor for the array will also be
    dumped (this is a recursive process).
    <fp> The file pointer.
    <dimension> The descriptor.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    unsigned int coord_count = 0;

    if (fp == NULL)
    {
	return;
    }
    (void) fprintf (fp, "\n");
    if (dimension == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No dimension descriptor to dump\n");
        }
        return;
    }
    /*  Print "DIMENSION"   */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n", "DIMENSION", "#Dimension descriptor");
    }
    else
    {
	(void) fprintf (fp, "DIMENSION\n");
    }
    /*  Print dimension name    */
    if ( ( dimension->name == NULL ) || ( *( dimension->name ) == '\0' ) )
    {
	if (comments)
        {
	    (void) fprintf (fp, "\t%-32s%s\n",
			    "(null)", "#No dimension name to dump");
        }
        else
        {
	    (void) fprintf (fp, "\t(null)\n");
        }
    }
    else
    {
	if (comments)
        {
	    (void) fprintf (fp, "\t%-32s%s\n",
			    dimension->name, "#Dimension name");
        }
        else
        {
	    (void) fprintf (fp, "\t%s\n", dimension->name);
        }
    }
    /*  Print dimension length  */
    if (comments)
    {
	(void) fprintf (fp, "\t%-32lu%s\n",
			dimension->length, "#Dimension length");
    }
    else
    {
	(void) fprintf (fp, "\t%lu\n", dimension->length);
    }
    /*  Print "REGULAR" or "RANDOM" */
    if (comments)
    {
	if ( dimension->coordinates == NULL )
        {
	    (void) fprintf (fp, "\t%-32s%s\n", "REGULAR",
			    "#Co-ordinates are regularly spaced");
        }
        else
        {
	    (void) fprintf (fp, "\t%-32s%s\n", "RANDOM",
			    "#Co-ordinates are randomly spaced");
        }
    }
    else
    {
	if ( dimension->coordinates == NULL )
        {
	    (void) fprintf (fp, "\tREGULAR\n");
        }
        else
        {
	    (void) fprintf (fp, "\tRANDOM\n");
        }
    }
    if (dimension->coordinates == NULL)
    {
	/*  Print first and last co-ordinates   */
        if (comments)
        {
	    (void) fprintf (fp, "\t%-32.16e%s\n", dimension->first_coord,
			    "#First co-ordinate");
            (void) fprintf (fp, "\t%-32.16e%s\n", dimension->last_coord,
			    "#Last co-ordinate");
        }
        else
        {
	    (void) fprintf (fp, "\t%.16e\n", dimension->first_coord);
            (void) fprintf (fp, "\t%.16e\n", dimension->last_coord);
        }
    }
    else
    {
	/*  Print co-ordinates  */
        while (coord_count < dimension->length)
        {
	    if (comments)
            {
		(void) fprintf (fp, "\t%-32.16e%s %u\n",
				dimension->coordinates[coord_count],
			 "#Co-ordinate number", coord_count);
            }
            else
            {
		(void) fprintf (fp, "\t%.16e\n",
				dimension->coordinates[coord_count]);
            }
            ++coord_count;
        }
    }
    /*  Print "END" */
    if (comments)
    {
	(void) fprintf (fp, "%-40s%s\n", "END", "#End dimension descriptor");
    }
    else
    {
	(void) fprintf (fp, "END\n");
    }
}   /*  End Function dmp_dim_desc  */

/*PUBLIC_FUNCTION*/
void dmp_multi_data (FILE *fp, multi_array *multi_desc, flag comments)
/*  [SUMMARY] Dump the data in a multi_array descriptor to a file.
    [PURPOSE] This routine will dump the data in a multi_array descriptor to a
    file, writing in ASCII format.
    <fp> The file pointer.
    <multi_desc> The descriptor.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    unsigned int array_count = 0;

    if (fp == NULL)
    {
	return;
    }
    if (multi_desc == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp,
			    "#No multi array descriptor to dump data for\n");
        }
        return;
    }
    if (multi_desc->data == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No data for multi_desc descriptor\n");
        }
        return;
    }
    /*  Print "#Multi array data starts here"   */
    if (comments)
    {
	(void) fprintf (fp, "#Multi array data starts here\n");
    }
    /*  Dump top level packets  */
    while (array_count < multi_desc->num_arrays)
    {
	if (multi_desc->data[array_count] == NULL)
        {
	    if (comments)
            {
		(void) fprintf (fp, "#No data for data structure number %u\n",
				array_count);
            }
        }
        else
        {
	    if (comments)
            {
		(void) fprintf (fp,
				"#Data for data structure number %u starts here\n",
				array_count);
            }
            dmp_packet (fp, multi_desc->headers[array_count],
			multi_desc->data[array_count], comments);
        }
        ++array_count;
    }
    /*  Print "#End multi array data"   */
    if (comments)
    {
	(void) fprintf (fp, "#End multi array data\n");
    }
}   /*  End Function dmp_multi_data    */

/*PUBLIC_FUNCTION*/
void dmp_packet (FILE *fp, packet_desc *pack_desc, char *packet, flag comments)
/*  [SUMMARY] Dump a packet to a file.
    [PURPOSE] This routine will dump a packet to a file, writing in ASCII
    format.
    <fp> The file pointer.
    <pack_desc> The descriptor.
    <packet> The packet.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    unsigned int element_count = 0;
    unsigned int type;
    extern char host_type_sizes[NUMTYPES];

    if (fp == NULL)
    {
	return;
    }
    if (pack_desc == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No packet descriptor to dump data for\n");
        }
        return;
    }
    if (packet == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No data for packet descriptor\n");
        }
        return;
    }
    /*  Print "#Packet data starts here"    */
    if (comments)
    {
	(void) fprintf (fp, "#Packet data starts here\n");
    }
    /*  Print packet data   */
    while (element_count < pack_desc->num_elements)
    {
	/*  Print element   */
        type = pack_desc->element_types[element_count];
        dmp_element (fp, type, pack_desc->element_desc[element_count],
		     packet, comments);
        packet += host_type_sizes[type];
        ++element_count;
    }
    /*  Print "#End packet data"    */
    if (comments)
    {
	(void) fprintf (fp, "#End packet data\n");
    }
}   /*  End Function dmp_packet    */

/*PUBLIC_FUNCTION*/
void dmp_element (FILE *fp, unsigned int type, char *desc, char *element,
		  flag comments)
/*  [SUMMARY] Dump an element to a file.
    [PURPOSE] This routine will dump an element to a file, writing in ASCII
    format.
    <fp> The file.
    <type> The type of the element.
    <desc> The element descriptor. If the element is an array pointer or a
    linked list pointer, that data will also be dumped.
    <element> The element.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    FString *fstring;
#ifdef NEED_ALIGNED_DATA
    int datum_size;
    double buf[2];
    extern char host_type_sizes[NUMTYPES];
#endif
    static char function_name[] = "dmp_element";

    if (fp == NULL) return;
    if (element == NULL)
    {
	(void) fprintf (stderr, "No element data to dump\n");
        a_prog_bug (function_name);
    }
#ifdef NEED_ALIGNED_DATA
    datum_size = host_type_sizes[type];
    if ( (iaddr) element % datum_size != 0 )
    {
	m_copy ( (char *) buf, element, datum_size );
	element = (char *) buf;
    }
#endif
    switch (type)
    {
      case K_FLOAT:
	(void) fprintf ( fp, "%.7e\n", *( (float *) element ) );
	break;
      case K_DOUBLE:
	(void) fprintf ( fp, "%.15e\n", *( (double *) element ) );
	break;
      case K_BYTE:
	(void) fprintf (fp, "%d\n", (int) *element);
	break;
      case K_INT:
	(void) fprintf ( fp, "%d\n", *( (int *) element ) );
	break;
      case K_SHORT:
	(void) fprintf ( fp, "%hd\n", *( (short *) element ) );
	break;
      case K_COMPLEX:
	(void) fprintf ( fp, "%.7e %.7e\n", *( (float *) element ),
			*( (float *) element + 1 ) );
	break;
      case K_DCOMPLEX:
	(void) fprintf ( fp, "%.15e %.15e\n",
			*( (double *) element ),
			*( (double *) element + 1 ) );
	break;
      case K_BCOMPLEX:
	(void) fprintf ( fp, "%d %d\n", (int) *element,
			(int) *(element + 1) );
	break;
      case K_ICOMPLEX:
	(void) fprintf ( fp, "%d %d\n", *( (int *) element ),
			*( (int *) element + 1 ) );
	break;
      case K_SCOMPLEX:
	(void) fprintf ( fp, "%hd %hd\n", *( (short *) element ),
			*( (short *) element + 1 ) );
	break;
      case K_LONG:
	(void) fprintf ( fp, "%ld\n", *( (long *) element ) );
	break;
      case K_LCOMPLEX:
	(void) fprintf ( fp, "%ld %ld\n", *( (long *) element ),
			*( (long *) element + 1 ) );
	break;
      case K_UBYTE:
	(void) fprintf ( fp, "%u\n", *( (unsigned char *) element ) );
	break;
      case K_UINT:
	(void) fprintf ( fp, "%u\n", *( (unsigned int *) element ) );
	break;
      case K_USHORT:
	(void) fprintf ( fp, "%hu\n", *( (unsigned short *) element ) );
	break;
      case K_ULONG:
	(void) fprintf ( fp, "%lu\n", *( (unsigned long *) element ) );
	break;
      case K_UBCOMPLEX:
	(void) fprintf ( fp, "%u %u\n", *( (unsigned char *) element ),
			*( (unsigned char *) element + 1 ) );
	break;
      case K_UICOMPLEX:
	(void) fprintf ( fp, "%u %u\n", *( (unsigned int *) element ),
			*( (unsigned int *) element + 1 ) );
	break;
      case K_USCOMPLEX:
	(void) fprintf ( fp, "%u %u\n", *( (unsigned short *) element ),
			*( (unsigned short *) element + 1 ) );
	break;
      case K_ULCOMPLEX:
	(void) fprintf ( fp, "%lu %lu\n", *( (unsigned long *) element ),
			*( (unsigned long *) element + 1 ) );
	break;
      case K_VSTRING:
	(void) fprintf (fp, "%s\n", *(char **) element);
	break;
      case K_FSTRING:
	fstring = (FString *) element;
	(void) fprintf (fp, "%u\n%s\n", fstring->max_len, fstring->string);
	break;
      case K_ARRAY:
	dmp_array (fp, (array_desc *) desc,
		   *(char **) element, comments);
	break;
      case LISTP:
	dmp_list (fp, (packet_desc *) desc,
		  *(list_header **) element, comments);
	break;
      default:
	if (comments)
	{   (void) fprintf (fp, "#Bad element type: %u\n", type);
	}
	break;
    }
}   /*  End Function dmp_element   */

/*PUBLIC_FUNCTION*/
void dmp_array (FILE *fp, array_desc *arr_desc, char *array, flag comments)
/*  [SUMMARY] Dump an array to a file.
    [PURPOSE] This routine will dump an array to a file, writing in ASCII
    format.
    <fp> The file pointer.
    <array_desc> The descriptor.
    <array> The array.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    unsigned int array_size;
    unsigned int array_count = 0;
    unsigned int packet_size;
    char *data;

    if (fp == NULL)
    {
	return;
    }
    if (arr_desc == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No array descriptor to dump data for\n");
        }
        return;
    }
    if (array == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No data for array descriptor\n");
        }
        return;
    }
    /*  Print "#Array data starts here" */
    if (comments)
    {
	(void) fprintf (fp, "#Array data starts here\n");
    }
    array_size = ds_get_array_size (arr_desc);
    packet_size = ds_get_packet_size ( arr_desc->packet );
    data = array;
    while (array_count++ < array_size)
    {
	/*  Dump a packet in the array  */
        dmp_packet (fp, arr_desc->packet, data, comments);
        data += packet_size;
    }
    /*  Print "#End array data" */
    if (comments)
    {
	(void) fprintf (fp, "#End array data\n");
    }
}   /*  End Function dmp_array */

/*PUBLIC_FUNCTION*/
void dmp_list (FILE *fp, packet_desc *pack_desc, list_header *list_head,
	       flag comments)
/*  [SUMMARY] Dump a linked list to a file.
    [PURPOSE] This routine will dump an array descriptor to a file,
    writing in ASCII format.
    <fp> The file pointer.
    <pack_desc> The list descriptor.
    <list_head> A pointer to the list header.
    <comments> If TRUE, the routine will append comments to the lines written,
    else it will not.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    unsigned int pack_size;
    char *data;
    list_entry *curr_entry;
    static char function_name[] = "dmp_list";

    if (fp == NULL)
    {
	return;
    }
    if (pack_desc == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No list descriptor to dump data for\n");
        }
        return;
    }
    if (list_head == NULL)
    {
	if (comments)
        {
	    (void) fprintf (fp, "#No list header for list descriptor\n");
        }
        return;
    }
    if ( list_head->magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    if ( list_head->length < 1 )
    {
	if (comments)
        {
	    (void) fprintf (fp,
			    "#No list (zero length) for list descriptor\n");
        }
        return;
    }
    /*  Print "#Linked list starts here"    */
    if (comments)
    {
	(void) fprintf (fp, "#Linked list starts here\n");
    }
    /*  Print list length   */
    if (comments)
    {
	(void) fprintf (fp, "%-40lu%s\n", list_head->length,
			"#Length of linked list");
    }
    else
    {
	(void) fprintf (fp, "%lu\n", list_head->length);
    }
    /*  Print list sort type    */
    switch ( list_head->sort_type )
    {
      case SORT_INCREASING:
	if (comments)
	{
	    (void) fprintf (fp, "%-40s%s\n", "INCREASING", "#List sort type");
	}
	else
	{
	    (void) fprintf (fp, "INCREASING\n");
	}
	break;
      case SORT_DECREASING:
	if (comments)
	{
	    (void) fprintf (fp, "%-40s%s\n", "DECREASING", "#List sort type");
	}
	else
	{
	    (void) fprintf (fp, "DECREASING\n");
	}
	break;
      case SORT_RANDOM:
	if (comments)
	{
	    (void) fprintf (fp, "%-40s%s\n", "RANDOM", "#List sort type");
	}
	else
	{
	    (void) fprintf (fp, "RANDOM\n");
	}
	break;
      default:
	if (comments)
	{
	    (void) fprintf (fp,"#Sort type not recognised\n");
	}
	break;
    }
    /*  Print list sort element number  */
    if (comments)
    {
	(void) fprintf (fp, "%-40u%s\n", list_head->sort_elem_num,
			"#Element number list sorted by");
    }
    else
    {
	(void) fprintf (fp, "%u\n", list_head->sort_elem_num);
    }
    
    /*  Print list packets  */
    /*  Dump list packets in contiguous section  */
    pack_size = ds_get_packet_size (pack_desc);
    for (count = 0, data = list_head->contiguous_data;
	 count < list_head->contiguous_length; ++count, data += pack_size)
    {
	dmp_packet (fp, pack_desc, data, comments);
    }
    /*  Dump list packets in fragmented section  */
    for (curr_entry = list_head->first_frag_entry; curr_entry != NULL;
	 curr_entry = curr_entry->next)
    {
	dmp_packet (fp, pack_desc, curr_entry->data, comments);
    }
    /*  Print "#End linked list */
    if (comments)
    {
	(void) fprintf (fp, "#End linked list\n");
    }
}   /*  End Function dmp_list  */

/*PUBLIC_FUNCTION*/
void dmp_flag (FILE *fp, flag logical, char *comment_string, flag comments)
/*  [SUMMARY] Dump a flag value to a file.
    [PURPOSE] This routine will dump the ASCII representation of a flag value
    to a file.
    <fp> The file.
    <logical> The flag value.
    <comment_string> The comment string.
    <comments> If TRUE, the comment string is written.
    [RETURNS] Nothing.
*/
{
    if (fp == NULL)
    {
	return;
    }
    if (comments)
    {
	if (logical == TRUE)
        {
	    (void) fprintf (fp, "%-40s#%s\n", "TRUE", comment_string);
        }
        else
        {
	    (void) fprintf (fp, "%-40s#%s\n", "FALSE", comment_string);
        }
    }
    else
    {
	if (logical == TRUE)
        {
	    (void) fprintf (fp, "TRUE\n");
        }
        else
        {
	    (void) fprintf (fp, "FALSE\n");
        }
    }
}   /*  End Function dmp_flag  */
