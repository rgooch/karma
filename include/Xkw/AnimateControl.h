/*  AnimateControl.h

    Public header for  AnimateControl  widget class.

    Copyright (C) 1994-1996  Richard Gooch

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

    This include file contains the public class declarations for the
    AnimateControl widget.


    Written by      Richard Gooch   8-DEC-1994

    Last updated by Richard Gooch   4-DEC-1996

*/

/*----------------------------------------------------------------------
   This widget implements an animation control

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 XkwNnumFrames           Int              int             0
 XkwNstartFrame          Int              int             0
 XkwNendFrame            Int              int             numFrames-1
 XkwNnewFrameCallback    Callback         Callback        NULL

------------------------------------------------------------------------*/

#ifndef ANIMATECONTROL__H
#define ANIMATECONTROL__H

extern WidgetClass animateControlWidgetClass;
typedef struct _AnimateControlClassRec *AnimateControlWidgetClass;
typedef struct _AnimateControlRec *AnimateControlWidget;

#define XtIsAnimateControl(w) XtIsSubclass((w), animateControlWidgetClass)

#define XkwNnumFrames "numFrames"
#define XkwNstartFrame "startFrame"
#define XkwNendFrame "endFrame"
#define XkwNnewFrameCallback "newFrameCallback"

#define XkwCNumFrames "NumFrames"
#define XkwCStartFrame "StartFrame"
#define XkwCEndFrame "EndFrame"

#endif
