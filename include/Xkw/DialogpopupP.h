#ifndef DIALOGPOPUPP__H
#define DIALOGPOPUPP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <Xkw/Dialogpopup.h>

typedef struct _DialogpopupPart
{
    /*  Public resources  */
    XtCallbackList callback;
    String defaultName;
    String extension;
    char *label;
    Bool showAutoIncrement;
    /*  Private resources  */
    Widget dialog;
    Widget extension_label;
    Bool auto_increment;
} DialogpopupPart, *DialogpopupPartPtr;

typedef struct _DialogpopupRec
{
  CorePart core;
  CompositePart composite;
  ShellPart shell;
  WMShellPart wm;
  VendorShellPart vendor;
  TopLevelShellPart topLevel;
  DialogpopupPart dialogpopup;
} DialogpopupRec, *DialogpopupPtr;

typedef struct _DialogpopupClassPart
{
  int empty;
} DialogpopupClassPart;

typedef struct _DialogpopupClassRec
{
  CoreClassPart core_class;	
  CompositeClassPart composite_class;
  ShellClassPart shell_class;
  WMShellClassPart wm_shell_class;
  VendorShellClassPart vendor_shell_class;
  TopLevelShellClassPart top_level_shell_class;
  DialogpopupClassPart dialogpopup_class;
} DialogpopupClassRec, *DialogpopupClassPtr;

extern DialogpopupClassRec dialogpopupClassRec;

#endif
