
#ifndef ZoomPolicyP__H
#define ZoomPolicyP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <Xkw/ZoomPolicy.h>

typedef struct _ZoomPolicyPart
{
    /*  Public resources  */
    KWorldCanvas  *canvases;
    Bool          autoIntensityScale;
    /*  Private resources  */
    flag auto_refresh;
    double log_cycles;
} ZoomPolicyPart, *ZoomPolicyPartPtr;

typedef struct _ZoomPolicyRec
{
    CorePart core;
    CompositePart composite;
    ShellPart shell;
    WMShellPart wm;
    VendorShellPart vendor;
    TopLevelShellPart topLevel;
    ZoomPolicyPart zoomPolicy;
} ZoomPolicyRec, *ZoomPolicyPtr;

typedef struct _ZoomPolicyClassPart
{
    int empty;
} ZoomPolicyClassPart;

typedef struct _ZoomPolicyClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ShellClassPart shell_class;
    WMShellClassPart wm_shell_class;
    VendorShellClassPart vendor_shell_class;
    TopLevelShellClassPart top_level_shell_class;
    ZoomPolicyClassPart zoomPolicy_class;
} ZoomPolicyClassRec, *ZoomPolicyClassPtr;

extern ZoomPolicyClassRec zoomPolicyClassRec;

#endif
