/*LINTLIBRARY*/
/*  pop.c

    This code provides miscellaneous routines for the X Intrinsics toolkit

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

/*  This file contains miscellaneous routines needed for the using the Xt
  toolkit.


    Written by      Richard Gooch   30-DEC-1995

    Last updated by Richard Gooch   16-APR-1996: Changed to new documentation
  format.


*/
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
/*  Do not include  karma.h  first, since CONST may be screwed otherwise. This
    applies to broken versions of  X11/Intrinsic.h  */
#include <karma_xtmisc.h>


/*  Private functions  */


/*  Private data  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void xtmisc_popup_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Pop up a popup widget.
    [PURPOSE] This routine will pop up a popup widget. This routine is a
    convenience routine to be used as a callback.
    <w> The widget on which the event occurred. This is ignored.
    <client_data> The callback client data. This must be the popup widget.
    <call_data> The callback call data. This is ignored.
    [RETURNS] Nothing.
*/
{
    XtPopup ( (Widget) client_data, XtGrabNone );
}   /*  End Function xtmisc_popup_cbk  */

/*PUBLIC_FUNCTION*/
void xtmisc_exclusive_popup_cbk (Widget w, XtPointer client_data,
				 XtPointer call_data)
/*  [SUMMARY] Pop up a popup widget with exclusive grab.
    [PURPOSE] This routine will pop up a popup widget. This routine is a
    convenience routine to be used as a callback. An exclusive grab is
    performed on the popped up widget.
    <w> The widget on which the event occurred. This is ignored.
    <client_data> The callback client data. This must be the popup widget.
    <call_data> The callback call data. This is ignored.
    [RETURNS] Nothing.
*/
{
    XtPopup ( (Widget) client_data, XtGrabExclusive );
}   /*  End Function xtmisc_exclusive_popup_cbk  */

/*PUBLIC_FUNCTION*/
void xtmisc_popdown_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Pop down a popup widget.
    [PURPOSE] This routine will pop down a popup widget. This routine is a
    convenience routine to be used as a callback.
    <w> The widget on which the event occurred. This is ignored.
    <client_data> The callback client data. This must be the popup widget.
    <call_data> The callback call data. This is ignored.
    [RETURNS] Nothing.
*/
{
    XtPopdown ( (Widget) client_data );
}   /*  End Function xtmisc_popdown_cbk  */
