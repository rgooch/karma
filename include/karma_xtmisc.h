/*  karma_xtmisc.h

    Header for  xtmisc_  package.

    Copyright (C) 1995  Richard Gooch

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

    Last updated by Richard Gooch   30-DEC-1995

*/

#include <X11/Intrinsic.h>

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_XTMISC_H
#define KARMA_XTMISC_H


/*  File:   work.c   */
EXTERN_FUNCTION (void xtmisc_support_work_funcs, (XtAppContext app_context) );

/*  File:   pop.c   */
EXTERN_FUNCTION (void xtmisc_popup_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
EXTERN_FUNCTION (void xtmisc_exclusive_popup_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
EXTERN_FUNCTION (void xtmisc_popdown_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );


#endif /*  KARMA_XTMISC_H  */
