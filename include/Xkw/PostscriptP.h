
#ifndef PostscriptP__H
#define PostscriptP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <karma_kcmap.h>
#include <karma_kwin.h>

#include <Xkw/Postscript.h>

typedef struct _PostscriptPart
{
  flag portrait;
  double hoffset;
  double voffset;
  double hsize;
  double vsize;
  char *arrayfile_name;
  KPixCanvas pixcanvas;
  XtCallbackList callback;
} PostscriptPart, *PostscriptPartPtr;

typedef struct _PostscriptRec
{
  CorePart core;
  CompositePart composite;
  ShellPart shell;
  WMShellPart wm;
  VendorShellPart vendor;
  TopLevelShellPart topLevel;
  PostscriptPart postscript;
} PostscriptRec, *PostscriptPtr;

typedef struct _PostscriptClassPart
{
  int empty;
} PostscriptClassPart;

typedef struct _PostscriptClassRec
{
  CoreClassPart core_class;	
  CompositeClassPart composite_class;
  ShellClassPart shell_class;
  WMShellClassPart wm_shell_class;
  VendorShellClassPart vendor_shell_class;
  TopLevelShellClassPart top_level_shell_class;
  PostscriptClassPart postscript_class;
} PostscriptClassRec, *PostscriptClassPtr;

extern PostscriptClassRec postscriptClassRec;

#endif
