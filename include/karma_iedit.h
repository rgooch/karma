/*  karma_iedit.h

    Header for  iedit_  package.

    Copyright (C) 1993,1994  Richard Gooch

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

    This include file contains all the definitions and function declarations
  needed to interface to the iedit_ routines in the Karma library.


    Written by      Richard Gooch   24-MAR-1993

    Last updated by Richard Gooch   26-AUG-1994

*/

#ifndef KARMA_IEDIT_H
#define KARMA_IEDIT_H


#include <karma_ds_def.h>

#define EDIT_INSTRUCTION_DAB (unsigned int) 0
#define EDIT_INSTRUCTION_STROKE (unsigned int) 1
#define EDIT_INSTRUCTION_FPOLY (unsigned int) 2
#define EDIT_APPLY_INSTRUCTIONS (unsigned int) 3
#define EDIT_UNDO_INSTRUCTIONS (unsigned int) 4
#define EDIT_SET_POINT (unsigned int) 5


typedef struct instruction_list_type * KImageEditList;


/*  File:   image_edit.c   */
EXTERN_FUNCTION (KImageEditList iedit_create_list,
		 (void (*add_func) (), void (*loss_func) (),
		  void (*apply_func) (), void *info) );
EXTERN_FUNCTION (packet_desc *iedit_get_instruction_desc, () );
EXTERN_FUNCTION (edit_coord *iedit_alloc_edit_coords,
		 (unsigned int num_coords) );
EXTERN_FUNCTION (flag iedit_get_edit_coords, (list_header *list_head,
					      edit_coord **coords) );
EXTERN_FUNCTION (flag iedit_add_instruction,
		 (KImageEditList ilist, unsigned int instruction_code,
		  edit_coord *coords, unsigned int num_coords,
		  double intensity[2]) );
EXTERN_FUNCTION (flag iedit_remove_instructions,
		 (KImageEditList ilist, unsigned int num_to_remove) );
EXTERN_FUNCTION (flag iedit_apply_instructions, (KImageEditList ilist) );
EXTERN_FUNCTION (list_header *iedit_get_list, (KImageEditList ilist) );
EXTERN_FUNCTION (void iedit_make_list_default_master, (KImageEditList ilist) );
EXTERN_FUNCTION (void iedit_make_list_default_slave, (KImageEditList ilist) );


#endif /*  KARMA_IEDIT_H  */
