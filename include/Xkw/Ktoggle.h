/*
 * $XConsortium: Ktoggle.h,v 1.13 91/05/04 18:59:01 rws Exp $
 *
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * KtoggleP.h - Private definitions for Ktoggle widget
 *
 * Author: Chris D. Peterson
 *         MIT X Consortium
 *         kit@expo.lcs.mit.edu
 *  
 * Date:   January 12, 1989
 */

#ifndef _XawKtoggle_h
#define _XawKtoggle_h

/***********************************************************************
 *
 * Ktoggle Widget
 *
 ***********************************************************************/

#include <X11/Xaw/Command.h>
#include <X11/Xfuncproto.h>

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 radioGroup          RadioGroup         Widget          NULL              +
 radioData           RadioData          Pointer         (caddr_t) Widget  ++
 state               State              Boolean         Off

 background	     Background		Pixel		XtDefaultBackground
 bitmap		     Pixmap		Pixmap		None
 border		     BorderColor	Pixel		XtDefaultForeground
 borderWidth	     BorderWidth	Dimension	1
 callback	     Callback		Pointer		NULL
 cursor		     Cursor		Cursor		None
 destroyCallback     Callback		Pointer		NULL
 font		     Font		XFontStructx*	XtDefaultFont
 foreground	     Foreground		Pixel		XtDefaultForeground
 height		     Height		Dimension	text height
 highlightThickness  Thickness		Dimension	2
 insensitiveBorder   Insensitive	Pixmap		Gray
 internalHeight	     Height		Dimension	2
 internalWidth	     Width		Dimension	4
 justify	     Justify		XtJustify	XtJustifyCenter
 label		     Label		String		NULL
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 resize		     Resize		Boolean		True
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	text width
 x		     Position		Position	0
 y		     Position		Position	0

+ To use the ktoggle as a radio ktoggle button, set this resource to point to
  any other widget in the radio group.

++ This is the data returned from a call to XtKtoggleGetCurrent, by default
   this is set to the name of ktoggle widget.

*/

/*
 * These should be in StringDefs.h but aren't so we will define
 * them here if they are needed.
 */


#define XtCWidget "Widget"
#define XtCState "State"
#define XtCRadioGroup "RadioGroup"
#define XtCRadioData "RadioData"
#define XkwCCrosses "Crosses"

#ifndef _XtStringDefs_h_
#define XtRWidget "Widget"
#endif

#define XtNstate "state"
#define XtNradioGroup "radioGroup"
#define XtNradioData "radioData"
#define XkwNcrosses "crosses"

extern WidgetClass               ktoggleWidgetClass;

typedef struct _KtoggleClassRec   *KtoggleWidgetClass;
typedef struct _KtoggleRec        *KtoggleWidget;


/************************************************************
 * 
 * Public Functions
 *
 ************************************************************/

_XFUNCPROTOBEGIN
   
/*	Function Name: XawKtoggleChangeRadioGroup
 *	Description: Allows a ktoggle widget to change radio lists.
 *	Arguments: w - The ktoggle widget to change lists.
 *                 radio_group - any widget in the new list.
 *	Returns: none.
 */

extern void XawKtoggleChangeRadioGroup(
#if NeedFunctionPrototypes
    Widget		/* w */,
    Widget		/* radio_group */
#endif
);

/*	Function Name: XawKtoggleGetCurrent
 *	Description: Returns the RadioData associated with the ktoggle
 *                   widget that is currently active in a ktoggle list.
 *	Arguments: radio_group - any ktoggle widget in the ktoggle list.
 *	Returns: The XtNradioData associated with the ktoggle widget.
 */

extern XtPointer XawKtoggleGetCurrent(
#if NeedFunctionPrototypes
    Widget		/* radio_group */
#endif
);

/*	Function Name: XawKtoggleSetCurrent
 *	Description: Sets the Ktoggle widget associated with the
 *                   radio_data specified.
 *	Arguments: radio_group - any ktoggle widget in the ktoggle list.
 *                 radio_data - radio data of the ktoggle widget to set.
 *	Returns: none.
 */

extern void XawKtoggleSetCurrent(
#if NeedFunctionPrototypes
    Widget		/* radio_group */,
    XtPointer		/* radio_data */
#endif
);
 
/*	Function Name: XawKtoggleUnsetCurrent
 *	Description: Unsets all Ktoggles in the radio_group specified.
 *	Arguments: radio_group - any ktoggle widget in the ktoggle list.
 *	Returns: none.
 */

extern void XawKtoggleUnsetCurrent(
#if NeedFunctionPrototypes
    Widget		/* radio_group */
#endif
);

_XFUNCPROTOEND

#endif /* _XawKtoggle_h */
/* DON'T ADD STUFF AFTER THIS */
