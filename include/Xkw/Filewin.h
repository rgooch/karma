
/*----------------------------------------------------------------------*/
/* This widget implements a file selector */
/*
 *
   Resources:               Type:                Defaults:

   XkwNfileSelectCallback   Callback             NULL
   XkwNfilenameTester       Callback             NULL

 *----------------------------------------------------------------------*/

#ifndef FILEWIN__H
#define FILEWIN__H

#if !defined(KARMA_C_DEF_H) || defined(MAKEDEPEND)
#  include <karma_c_def.h>
#endif

#if !defined(KARMA_DIR_H) || defined(MAKEDEPEND)
#  include <karma_dir.h>
#endif

extern WidgetClass filewinWidgetClass;
typedef struct _FilewinClassRec *FilewinWidgetClass;
typedef struct _FilewinRec *FilewinWidget;

#define XtIsFilewin(w) XtIsSubclass((w), filewinWidgetClass)

#define XkwNfileSelectCallback "fileSelectCallback"
#define XkwNfilenameTester "filenameTester"

/*----------------------------------------------------------------------
 * Functions
 *----------------------------------------------------------------------*/

EXTERN_FUNCTION (void XkwFilewinRescan,
		 (Widget w,XtPointer client_data,XtPointer call_data) );
EXTERN_FUNCTION (char *XkwFilewinCurrentDirectory, (Widget W) );
EXTERN_FUNCTION (KCallbackFunc XkwFilewinRegisterDirCbk,
		 (Widget w,
		  flag (*callback) (Widget w, void *info, CONST char *dirname),
		  void *info) );
EXTERN_FUNCTION (flag XkwFilewinStandardFileTester_nD, (KFileInfo finfo) );
EXTERN_FUNCTION (flag XkwFilewinStandardFileTester_3D, (KFileInfo finfo) );


#endif
