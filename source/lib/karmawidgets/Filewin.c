/*LINTLIBRARY*/
/*  Filewin.c

    This code provides a file selector widget for Xt.

    Copyright (C) 1993,1994,1995  Patrick Jordan
    Incorporated into Karma by permission.

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

    Patrick Jordan may be reached by email at  pjordan@rp.csiro.au
    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for manipulating a file selector
    widget for Xt.


    Written by      Patrick Jordan  7-JUL-1993

    Updated by      Richard Gooch   29-SEP-1993

    Updated by      Richard Gooch   8-DEC-1993: Added more patches from Patrick

    Updated by      Richard Gooch   17-JAN-1994: Added more patches from
  Patrick

    Updated by      Richard Gooch   28-JUL-1994: Added  #include <karma_m.h>

    Updated by      Richard Gooch   30-JUL-1994: Removed height setting of
  viewport: this should be done externally.

    Updated by      Richard Gooch   5-OCT-1994: Worked around bug in AIX/rs6000
  native compiler.

    Updated by      Richard Gooch   28-SEP-1995: Added directory callbacks.

    Last updated by Richard Gooch   20-OCT-1995: Added minimum list height.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <Xkw/FilewinP.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>

#include <karma_dir.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_c.h>

#define MINIMUM_LIST_ROWS 20

/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Methods*/

static void Initialize(Widget request,Widget new);
static void Destroy(Widget w);
static Boolean SetValues(Widget current,Widget request,Widget new);
static void ConstraintInitialize(Widget request,Widget new);

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(FilewinWidget, filewin.field)

static XtResource FilewinResources[] = 
{
  {XkwNfileSelectCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
   TheOffset(fileSelectCallback), XtRCallback, (caddr_t)NULL},
  {XkwNfilenameTester, XtCCallback, XtRPointer, sizeof(caddr_t),
   TheOffset(accept_file), XtRPointer, (caddr_t)NULL},
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
   TheOffset(foreground), XtRString, XtDefaultForeground},
};

#undef TheOffset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

FilewinClassRec filewinClassRec = 
{
  {        /* CoreClassPart */
    /* superclass */              (WidgetClass)&formClassRec,
    /* class_name */              "Filewin",
    /* widget_size */             sizeof(FilewinRec),
    /* class_initialize */        NULL,                          
    /* class_part_initialize */   NULL,                          
    /* class_init */              FALSE,                         
    /* initialize */              (XtInitProc)Initialize,        
    /* initialize_hook */         NULL,                          
    /* realize */                 XtInheritRealize,              
    /* actions */                 NULL,                          
    /* num_actions */             0,                             
    /* resources */               FilewinResources,              
    /* num_resources */           XtNumber(FilewinResources),    
    /* xrm_class */               NULLQUARK,                     
    /* compress_motion */         TRUE,                          
    /* compress_exposure */       TRUE,                          
    /* compress_enterleave */     TRUE,                          
    /* visible_interest */        TRUE,                          
    /* destroy */                 Destroy,                       
    /* resize */                  NULL,                          
    /* expose */                  NULL,                          
    /* set_values */              (XtSetValuesFunc)SetValues,    
    /* set_values_hook */         NULL,                          
    /* set_values_almost */       XtInheritSetValuesAlmost,      
    /* get_values_hook */         NULL,                          
    /* accept_focus */            NULL,                          
    /* version */                 XtVersion,                     
    /* callback_private */        NULL,                          
    /* tm_translations */         NULL,                          
				  NULL,
				  NULL,
				  NULL,
  },
  {     /* CompositeClassPart */
    /* geometry_manager	  */	XtInheritGeometryManager,
    /* change_managed	  */	XtInheritChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */	NULL
  },{ /* Constraint */
    /* subresourses       */   NULL,
    /* subresource_count  */   0,
    /* constraint_size    */   sizeof(FilewinConstraintsRec),
    /* initialize         */   (XtInitProc)ConstraintInitialize,
    /* destroy            */   NULL,
    /* set_values         */   NULL,
    /* extension          */   NULL
  },{ /* Form */
    /* layout */                XtInheritLayout
  },
  {  /* FilewinClassPart */
    0 /* empty */
  }
};

WidgetClass filewinWidgetClass = (WidgetClass) &filewinClassRec;

/*----------------------------------------------------------------------*/
/* Reallocate a bigger directory space */
/*----------------------------------------------------------------------*/

static void increase_list_size (FilewinWidget w)
{
  char **new;
  int i;

  w->filewin.listmax*=2;
  new=(char **)m_alloc(w->filewin.listmax*sizeof(char *));
  for(i=0;i<w->filewin.listcount;i++) {
    new[i]=w->filewin.list[i];
  }
  m_free( (char *) w->filewin.list);
  w->filewin.list=new;
}

/*----------------------------------------------------------------------*/
/* Create the list*/
/*----------------------------------------------------------------------*/

static void create_list (FilewinWidget w)
{
#define W w->filewin
    KDir thedir;
    KFileInfo *finfo;
    flag ok;
    char *ptr;
    static char function_name[] = "Filewin::create_list";

    while (W.listcount > 0)
    {
	W.listcount--;
	m_free (W.list[W.listcount]);
    }
    thedir = dir_open (W.curdir);
    while ( (finfo = dir_read (thedir, KDIR_DOTDOT) ) != NULL )
    {
	if ( W.accept_file==NULL || W.accept_file(*finfo) )
	{
	    ptr = (char *) m_alloc (strlen (finfo->filename) + 4);
	    W.list[W.listcount] = ptr;
	    /*  The following reference must be done through  ptr  because the
		AIX/rs6000 compiler is fucked.  */
	    (void) strcpy (ptr + 3, finfo->filename);
	    if (finfo->type == KFILETYPE_DIRECTORY)
	    {
		(void) strncpy (ptr, "D  ", 3);
	    }
	    else
	    {
		(void) strncpy (ptr, "F  ", 3);
	    }
	    W.listcount++;
	    if (W.listcount == W.listmax) increase_list_size (w);
	}
    }
    dir_close (thedir);
    st_qsort (W.list, 0, W.listcount - 1);
    while (W.listcount < MINIMUM_LIST_ROWS)
    {
	if ( ( W.list[W.listcount] = st_dup ("") ) == NULL )
	{
	    m_abort (function_name, "empty string");
	}
	++W.listcount;
    }
#undef W
}

/*----------------------------------------------------------------------*/
/* Callback for file selection*/
/* A selection has been made from the list. It may be a directory though*/
/*----------------------------------------------------------------------*/

static void filesel_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    XawListReturnStruct *lr=(XawListReturnStruct *)call_data;
    FilewinWidget fw = (FilewinWidget)client_data;
    char dirname[STRING_LENGTH];
    static char fname[200];

    if (fw->filewin.list[lr->list_index][0] == '\0')
    {
	XawListUnhighlight (fw->filewin.listwidget);
	return;
    }
    if ( (fw->filewin.list[lr->list_index])[0] == 'D' )
    {
	(void) sprintf (dirname, "%s/%s", fw->filewin.curdir,
			fw->filewin.list[lr->list_index] + 3);
	if ( c_call_callbacks (fw->filewin.dir_callbacks, dirname) ) return;
	(void) strcpy (fw->filewin.curdir, dirname);
	create_list(fw);
	XawListChange(w,fw->filewin.list,fw->filewin.listcount,0,True);
    }
    else
    {
	sprintf(fname,"%s/%s",fw->filewin.curdir,fw->filewin.list[lr->list_index]+3);
	XtCallCallbacks((Widget)fw,XkwNfileSelectCallback,fname);
    }
}

/*----------------------------------------------------------------------*/
/* Public function to rescan directory. */
/* Can be used as a callback for a commandwidget */
/* The widget w parameter is ignored. client_data should be the filewin */
/* widget. This seems odd, but is essential for use as a callback */
/*----------------------------------------------------------------------*/

void XkwFilewinRescan(Widget w,XtPointer client_data,XtPointer call_data)
{
  FilewinWidget fw = (FilewinWidget)client_data;

  create_list(fw);
  XawListChange(fw->filewin.listwidget,fw->filewin.list,fw->filewin.listcount,
		0,True);
}

/*----------------------------------------------------------------------*/
/* Constraint Initialisation method*/
/*----------------------------------------------------------------------*/

static void ConstraintInitialize(Widget request,Widget new)
{
  /* Dummy function: I dunno what it's supposed to do */
}

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Initialize(Widget Request,Widget New)
{
    int list_width;
    FilewinWidget request = (FilewinWidget) Request;
    FilewinWidget new = (FilewinWidget) New;
    Widget view,cancelbtn,rescanbtn;
  
    /* Initialise the directory list space */
    new->filewin.listmax = 64;
    new->filewin.list = (char **)m_alloc (new->filewin.listmax*sizeof(char *));
    new->filewin.listcount = 0;
    new->filewin.dir_callbacks = NULL;

    sprintf (new->filewin.curdir,".");
    create_list (new);
  
    view=XtVaCreateManagedWidget
    ("view",viewportWidgetClass,New,
     XtNforceBars,True,
     XtNwidth,190,
     /*
	XtNheight,250,
	*/
     XtNuseRight,True,
     XtNallowVert,True,
     NULL);

    new->filewin.listwidget=XtVaCreateManagedWidget
    ("listwidget",listWidgetClass,view,
     XtNlist,new->filewin.list,
     XtNnumberStrings,new->filewin.listcount,
     XtNbackground,new->core.background_pixel,
     XtNforeground,new->filewin.foreground,
     XtNdefaultColumns,1,
     XtNforceColumns,True,
     NULL);
    XtAddCallback(new->filewin.listwidget,XtNcallback,filesel_cbk,new);
}

/*----------------------------------------------------------------------*/
/* Destroy method*/
/*----------------------------------------------------------------------*/

static void Destroy(Widget W)
{
  XtRemoveAllCallbacks(W, XkwNfileSelectCallback);
}

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean SetValues(Widget Current,Widget Request,Widget New)
{
  FilewinWidget current = (FilewinWidget) Current;
  FilewinWidget request = (FilewinWidget) Request;
  FilewinWidget new = (FilewinWidget) New;

  if(new->core.background_pixel!=current->core.background_pixel ||
     new->filewin.foreground!=current->filewin.foreground) {
    XtVaSetValues(new->filewin.listwidget,
		  XtNbackground,new->core.background_pixel,
		  XtNforeground,new->filewin.foreground,
		  NULL);
  }
  return True;
}

/*----------------------------------------------------------------------*/
/* Provide the current working directory */
/* returns a copy of the current working directory name */
/* The returned string is allocated with m_alloc */
/*----------------------------------------------------------------------*/

char *XkwFilewinCurrentDirectory(Widget W)
{
  FilewinWidget w = (FilewinWidget) W;
  return st_dup(w->filewin.curdir);
}
  
KCallbackFunc XkwFilewinRegisterDirCbk (Widget w,
					flag (*callback) (Widget w,
							  void *info,
							  CONST char *dirname),
					void *info)
/*  [PURPOSE] This routine will register a callback to capture directory
    selection events prior to the Filewin widget changing directory.
    <w> The Filewin widget.
    <callback> The routine which will be called when directories are selected.
    The interface to this routine is given below:
    [<pre>]
    flag callback (Widget w, void *info, CONST char *dirname)
    *   [PURPOSE] This routine will process a directory selection event.
        <w> The Filewin widget on which the selection occurred.
	<info> A pointer to arbitrary information.
	<dirname> The name of the directory.
	[RETURNS] TRUE if the selection event is to be consumed, else FALSE
	(meaning the event should be passed on to other callbacks, or, if none
	consume the event, the widget will change it's active directory).
    *
    [</pre>]
    <info> A pointer to arbitrary information.
    [RETURNS] A KCallbackFunc object.
*/
{
    FilewinWidget fw = (FilewinWidget) w;

    return ( c_register_callback (&fw->filewin.dir_callbacks, callback, w,
				  info, FALSE, NULL, FALSE, TRUE) );
}   /*  End Function XkwFilewinRegisterDirCbk  */
