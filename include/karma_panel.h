/*  karma_panel.h

    Header for  panel_  package.

    Copyright (C) 1993  Richard Gooch

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
  needed to interface to the panel_ routines in the Karma library.


    Written by      Richard Gooch   1-OCT-1993

    Last updated by Richard Gooch   19-OCT-1993

*/

#ifndef KARMA_PANEL_H
#define KARMA_PANEL_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif

#ifndef KPANEL_INTERNAL
typedef void * KControlPanel;
#endif

/*  Panel item attribute definitions  */
#define PIA_END 0                /*  End of varargs list  */
#define PIA_NUM_CHOICE_STRINGS 31001
#define PIA_CHOICE_STRINGS 31002
#define PIA_ARRAY_LENGTH 31003
#define PIA_ARRAY_MIN_LENGTH 31004
#define PIA_ARRAY_MAX_LENGTH 31005

/*  Extra panel item type definitions  */
#define PIT_FUNCTION 30000
#define PIT_EXIT_FORM 30001
#define PIT_FLAG 30002
#define PIT_CHOICE_INDEX 30003


/*  File:   panel.c   */
EXTERN_FUNCTION (KControlPanel panel_create, (flag blank) );
EXTERN_FUNCTION (void panel_push_onto_stack, (KControlPanel panel) );
EXTERN_FUNCTION (void panel_pop_from_stack, () );
EXTERN_FUNCTION (flag panel_process_command_with_stack,
		 (char *cmd, flag (*unknown_func) (), FILE *fp) );

#ifndef KPANEL_INTERNAL
EXTERN_FUNCTION (void panel_add_item, (KControlPanel panel, char *name,
				       char *comment, unsigned int type,
				       void *value_ptr, ...) );
#endif


#endif /*  KARMA_PANEL_H  */
