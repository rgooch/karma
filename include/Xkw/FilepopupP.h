#ifndef FILEPOPUPP__H
#define FILEPOPUPP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <karma_dir.h>

#include <Xkw/Filepopup.h>

typedef struct _FilepopupPart
{
    /*  Public resources  */
    flag            (*accept_file) ();
    XtCallbackList  fileSelectCallback;
    Bool            autoPopdown;
    /*  Private resources  */
    Widget          selector;
    char            **list;
    char            curdir[500];
    int             listcount;
    int             listmax;
} FilepopupPart, *FilepopupPartPtr;

typedef struct _FilepopupRec
{
    CorePart core;
    CompositePart composite;
    ShellPart shell;
    WMShellPart wm;
    VendorShellPart vendor;
    TopLevelShellPart topLevel;
    FilepopupPart filepopup;
} FilepopupRec, *FilepopupPtr;

typedef struct _FilepopupClassPart
{
    int empty;
} FilepopupClassPart;

typedef struct _FilepopupClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ShellClassPart shell_class;
    WMShellClassPart wm_shell_class;
    VendorShellClassPart vendor_shell_class;
    TopLevelShellClassPart top_level_shell_class;
    FilepopupClassPart filepopup_class;
} FilepopupClassRec, *FilepopupClassPtr;

extern FilepopupClassRec filepopupClassRec;

#endif
