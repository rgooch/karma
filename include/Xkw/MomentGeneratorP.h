
#ifndef MomentGeneratorP__H
#define MomentGeneratorP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <karma_canvas.h>
#include <karma_wcs.h>

#include <Xkw/MomentGenerator.h>

typedef struct _MomentGeneratorPart
{
    /*  Public resources  */
    Bool            verbose;
    XtCallbackList  momentCallback;
    iarray          mom0Array;
    iarray          mom1Array;
    /*  Private resources  */
    iarray          cube_arr;
    KwcsAstro       cube_ap;
    Widget          cube_min_label;
    Widget          cube_max_label;
    Widget          sum_min_label;
    Widget          sum_max_label;
    Widget          lower_clip_dlg;
    Widget          upper_clip_dlg;
    Widget          sum_clip_dlg;
    KCallbackFunc   iarr_destroy_callback;
    unsigned int    mom1_algorithm;
} MomentGeneratorPart, *MomentGeneratorPartPtr;

typedef struct _MomentGeneratorRec
{
    CorePart core;
    CompositePart composite;
    ShellPart shell;
    WMShellPart wm;
    VendorShellPart vendor;
    TopLevelShellPart topLevel;
    MomentGeneratorPart momentGenerator;
} MomentGeneratorRec, *MomentGeneratorPtr;

typedef struct _MomentGeneratorClassPart
{
    int empty;
} MomentGeneratorClassPart;

typedef struct _MomentGeneratorClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ShellClassPart shell_class;
    WMShellClassPart wm_shell_class;
    VendorShellClassPart vendor_shell_class;
    TopLevelShellClassPart top_level_shell_class;
    MomentGeneratorClassPart momentGenerator_class;
} MomentGeneratorClassRec, *MomentGeneratorClassPtr;

extern MomentGeneratorClassRec momentGeneratorClassRec;

#endif
