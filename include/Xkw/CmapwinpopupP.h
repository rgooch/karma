
#ifndef CmapwinpopupP__H
#define CmapwinpopupP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <Xkw/Cmapwinpopup.h>

typedef struct _CmapwinpopupPart
{
    /*  Public resources  */
    Kcolourmap cmap;
    Visual *colourbarVisual;
    Bool simpleColourbar;
    /*  Private resources  */
    Widget cmapwin;
    Widget save_dialog;
    Widget filepopup;
} CmapwinpopupPart, *CmapwinpopupPartPtr;

typedef struct _CmapwinpopupRec
{
    CorePart core;
    CompositePart composite;
    ShellPart shell;
    WMShellPart wm;
    VendorShellPart vendor;
    TopLevelShellPart topLevel;
    CmapwinpopupPart cmapwinpopup;
} CmapwinpopupRec, *CmapwinpopupPtr;

typedef struct _CmapwinpopupClassPart
{
    int empty;
} CmapwinpopupClassPart;

typedef struct _CmapwinpopupClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ShellClassPart shell_class;
    WMShellClassPart wm_shell_class;
    VendorShellClassPart vendor_shell_class;
    TopLevelShellClassPart top_level_shell_class;
    CmapwinpopupClassPart cmapwinpopup_class;
} CmapwinpopupClassRec, *CmapwinpopupClassPtr;

extern CmapwinpopupClassRec cmapwinpopupClassRec;

#endif
