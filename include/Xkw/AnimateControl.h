
/*----------------------------------------------------------------------
   This widget implements an animation control

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 numFrames               Int              int             0
 startFrame              Int              int             0
 endFrame                Int              int             numFrames-1
 newFrameCallback        Callback         Callback        NULL

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
