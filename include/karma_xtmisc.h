/*  karma_xtmisc.h

    Header for  xtmisc_  package.

    Copyright (C) 1995-1996  Richard Gooch

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
  needed to interface to the xtmisc_ routines in the Karma library.


    Written by      Richard Gooch   30-DEC-1995

    Last updated by Richard Gooch   1-DEC-1996

*/

#include <X11/Intrinsic.h>

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(KARMA_E_DEF_H) || defined(MAKEDEPEND)
#  include <karma_e_def.h>
#endif

#ifndef KARMA_XTMISC_H
#define KARMA_XTMISC_H


/*  File:  event.c  */
EXTERN_FUNCTION (KPeriodicEventList xtmisc_event_create_list,
		 (XtAppContext app_context,
		  unsigned long interval_us, unsigned long interval_s,
		  void *list_info) );

/*  File:  icon.c  */
EXTERN_FUNCTION (void xtmisc_set_icon,
		 ( Widget top_level,
		   void (*icon_func) (Display *display, Pixmap pixmap,
				      int width, int height) ) );

/*  File:  icon.c  */
EXTERN_FUNCTION (Widget xtmisc_init_app_initialise,
		 (XtAppContext *app_context_return,
		  CONST char* application_class,
		  XrmOptionDescList options, Cardinal num_options,
		  int *argc_in_out, String *argv_in_out,
		  String *fallback_resources, unsigned int min_ccells, ...) );

/*  File:   pop.c   */
EXTERN_FUNCTION (void xtmisc_popup_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
EXTERN_FUNCTION (void xtmisc_exclusive_popup_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
EXTERN_FUNCTION (void xtmisc_popdown_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );

/*  File:   work.c   */
EXTERN_FUNCTION (void xtmisc_support_work_funcs, (XtAppContext app_context) );


#endif /*  KARMA_XTMISC_H  */
