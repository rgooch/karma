
#ifndef AnimateControlP__H
#define AnimateControlP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <Xkw/AnimateControl.h>
#include <karma.h>

typedef struct _AnimateControlPart
{
    /*  Public resources  */
    int numFrames;
    int startFrame;
    int endFrame;
    int currentFrame;
    XtCallbackList newFrameCallback;
    /*  Private resources  */
    int interval_ms;
    int inc_factor;
    int spin_mode;
    int direction;
    flag running_movie;
    Widget num_frames_lbl;
    Widget start_frame_sld;
    Widget end_frame_sld;
    Widget current_frame_sld;
} AnimateControlPart, *AnimateControlPartPtr;

typedef struct _AnimateControlRec
{
    CorePart core;
    CompositePart composite;
    ShellPart shell;
    WMShellPart wm;
    VendorShellPart vendor;
    TopLevelShellPart topLevel;
    AnimateControlPart animateControl;
} AnimateControlRec, *AnimateControlPtr;

typedef struct _AnimateControlClassPart
{
    int empty;
} AnimateControlClassPart;

typedef struct _AnimateControlClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ShellClassPart shell_class;
    WMShellClassPart wm_shell_class;
    VendorShellClassPart vendor_shell_class;
    TopLevelShellClassPart top_level_shell_class;
    AnimateControlClassPart animate_control_class;
} AnimateControlClassRec, *AnimateControlClassPtr;

extern AnimateControlClassRec animateControlClassRec;

#endif
