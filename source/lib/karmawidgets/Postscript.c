/*LINTLIBRARY*/
/*  Postscript.c

    This code provides a postscript control widget for Xt.

    PostScript code Copyright (C) 1994-1996  Richard Gooch

    Widget code Copyright (C) 1994-1996  Patrick Jordan
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

/*  This file contains all routines needed for a postscript control
    widget for Xt.


    Written by      Patrick Jordan  26-JUN-1994: Copied PostScript code from
  print_xview.c  and hard_copy.c  in  source/modules/kview_2d (XView version of
  kview_2d  ).

    Updated by      Patrick Jordan  27-JUL-1994: Removed print_mono button
   as code not implemented yet.

    Updated by      Richard Gooch   23-OCT-1994: Changed some resource names.

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Updated by      Richard Gooch   9-APR-1995: Initialised  arrayfile_name
  field to NULL to avoid freeing a non-existant block.

    Updated by      Richard Gooch   13-JUL-1995: Added resources to change
  orientation, page offset and size.

    Updated by      Richard Gooch   30-JUL-1995: Made use of XkwNvaluePtr
  resource for ValueWidgetClass.

    Updated by      Richard Gooch   23-SEP-1995: Coped with NULL name pointer
  to <XkwPostscriptRegisterImageAndName>

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   7-JUN-1996: Added dialog widget for output
  filename.

    Updated by      Richard Gooch   28-JUN-1996: Created XkwNautoIncrement
  resource.

    Updated by      Richard Gooch   1-JUL-1996: Switched to <psw_va_create> and
  <psw_close> routines.

    Last updated by Richard Gooch   30-OCT-1996: Added #include <sys/types.h>


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <Xkw/PostscriptP.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Dialog.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <varargs.h>
#include <karma.h>
#include <karma_m.h>
#include <karma_xtmisc.h>
#include <karma_kwin.h>
#include <karma_psw.h>
#include <karma_st.h>
#include <karma_ch.h>
#include <karma_r.h>
#include <Xkw/Value.h>
#include <Xkw/Ktoggle.h>
#include <Xkw/ExclusiveMenu.h>

/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Methods*/

STATIC_FUNCTION (void orientation_cbk, (Widget w, XtPointer client_data,
					XtPointer call_data) );
STATIC_FUNCTION (void Postscript__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Destroy, (Widget w) );
STATIC_FUNCTION (Boolean SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void auto_increment_cbk, (Widget w, XtPointer client_data,
					   XtPointer call_data) );

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(PostscriptWidget, postscript.field)

static XtResource PostscriptResources[] = 
{
    {XkwNportraitOrientation, XkwCPortraitOrientation, XtRBool, sizeof (Bool),
     TheOffset (portrait), XtRImmediate, (XtPointer) True},
    {XkwNpageHorizontalOffset, XkwCPageHorizontalOffset, XtRInt, sizeof (int),
     TheOffset (hoffset), XtRImmediate, (XtPointer) 10},
    {XkwNpageVerticalOffset, XkwCPageVerticalOffset, XtRInt, sizeof (int),
     TheOffset (voffset), XtRImmediate, (XtPointer) 10},
    {XkwNpageHorizontalSize, XkwCPageHorizontalSize, XtRInt, sizeof (int),
     TheOffset (hsize), XtRImmediate, (XtPointer) 180},
    {XkwNpageVerticalSize, XkwCPageVerticalSize, XtRInt, sizeof (int),
     TheOffset (vsize), XtRImmediate, (XtPointer) 180},
    {XkwNautoIncrement, XkwCAutoIncrement, XtRBool, sizeof (Bool),
     TheOffset (autoIncrement), XtRImmediate, (XtPointer) FALSE},
    {XtNcallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     TheOffset (callback), XtRCallback, (caddr_t) NULL},
};

#undef TheOffset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

PostscriptClassRec postscriptClassRec = 
{
  {        /* CoreClassPart */
    (WidgetClass)&topLevelShellClassRec,  /* superclass */
    "Postscript",                     /* class_name */
    sizeof(PostscriptRec),            /* widget_size */
    NULL,                          /* class_initialise */
    NULL,                          /* class_part_initialise */
    FALSE,                         /* class_init */
    (XtInitProc)Postscript__Initialise,        /* initialise */
    NULL,                          /* initialise_hook */
    XtInheritRealize,              /* realize */
    NULL,                          /* actions */
    0,                             /* num_actions */
    PostscriptResources,              /* resources */
    XtNumber(PostscriptResources),    /* num_resources */
    NULLQUARK,                     /* xrm_class */
    TRUE,                          /* compress_motion */
    TRUE,                          /* compress_exposure */
    TRUE,                          /* compress_enterleave */
    TRUE,                          /* visible_interest */
    Destroy,                       /* destroy */
    NULL,                          /* resize */
    NULL,                          /* expose */
    (XtSetValuesFunc)SetValues,    /* set_values */
    NULL,                          /* set_values_hook */
    XtInheritSetValuesAlmost,      /* set_values_almost */
    NULL,                          /* get_values_hook */
    NULL,                          /* accept_focus */
    XtVersion,                     /* version */
    NULL,                          /* callback_private */
    NULL,                          /* tm_translations */
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
  },{ /* Shell */
    /* extension	  */	NULL
  },{ /* WMShell */
    /* extension	  */	NULL
  },{ /* VendorShell */
    /* extension	  */	NULL
  },{ /* TopLevelShell */
    /* extension	  */	NULL
  },
  {  /* PostscriptClassPart */
    0 /* empty */
  }
};

WidgetClass postscriptWidgetClass = (WidgetClass) &postscriptClassRec;


/*  Orientation choices  */
#define ORIENT_PORTRAIT  0
#define ORIENT_LANDSCAPE 1
#define NUM_ORIENTATIONS 2

static char *orientations[NUM_ORIENTATIONS] =
{   "portrait", "landscape"};


/*----------------------------------------------------------------------*/
/* check that a widget is actually a PostscriptWidget                      */
/*----------------------------------------------------------------------*/

static int check_type (Widget w, char *function_name)
{
    int fl;
    /*static char func_name[] = "Postscript.check_type";*/

    fl = XtIsSubclass (w, postscriptWidgetClass);
    if (!fl)
    (void) fprintf (stderr,
		    "ERROR: Widget passed to %s is not a PostscriptWidget\n",
		    function_name);
    return fl;
}

/*----------------------------------------------------------------------*/
/*  This routine will print the image to a channel. It first prints the */
/*  PostScript header.                                                  */
/*  The channel to write to must be given by  channel  .                */
/*  If the value of  colour  is TRUE, colour PostScript is written.     */
/*  If the value of  eps  is TRUE, encapsulated PostScript is written.  */
/*  The routine returns TRUE on succes, else it returns FALSE.          */
/*----------------------------------------------------------------------*/

static flag write_ps (PostscriptWidget w, Channel channel,flag colour,flag eps)
{
  PostScriptPage pspage;
  static char function_name[] = "write_ps";

  check_type((Widget)w,function_name);
  FLAG_VERIFY (colour);
  pspage = psw_va_create (channel, (double) w->postscript.hoffset / 10.0,
			  (double) w->postscript.voffset / 10.0,
			  (double) w->postscript.hsize / 10.0,
			  (double) w->postscript.vsize / 10.0,
			  w->postscript.portrait, eps,
			  PSW_ATT_END);
  if(pspage == NULL ) return (FALSE);
  if(w->postscript.pixcanvas==NULL) {
    fprintf(stderr,"Postscript Widget: NULL Canvas passed - No PS written\n");
    return FALSE;
  }
  if ( kwin_write_ps (w->postscript.pixcanvas, pspage) )  {
    return ( psw_close (pspage, FALSE, FALSE) );
  }
  return (TRUE);
}

/*----------------------------------------------------------------------*/
/*  This routine will print the image to the queue named by the PRINTER */
/*  environmental variable.                                             */
/*  If the value of client_data is TRUE, colour PostScript is written.  */
/*  The case where client_data is FALSE is not implemented yet.         */
/*  It is setup as a callback for the "queue" buttons.                  */
/*  The routine returns nothing.                                        */
/*----------------------------------------------------------------------*/

static void send_to_queue (Widget w, XtPointer client_data,XtPointer call_data)
{
  Channel read_ch, write_ch;
  int pid;
  int statusp;
  flag colour = (uaddr) client_data;
  PostscriptWidget pw=(PostscriptWidget)XtParent(XtParent(XtParent(w)));
  char *postscript;
  char txt[STRING_LENGTH];
  extern char *sys_errlist[];
  static char function_name[] = "send_to_queue";

  XtCallCallbacks((Widget)pw,XtNcallback,NULL);
  FLAG_VERIFY (colour);
  if ( ( postscript = r_getenv ("PRINTER") ) == NULL )  {
    fprintf (stderr, "No PRINTER environment variable set\n");
    return;
  }
  if ( !ch_create_pipe (&read_ch, &write_ch) )  {
    fprintf (stderr, "Error opening pipe\t%s\n",sys_errlist[errno]);
    return;
  }
  sprintf (txt, "-P%s", postscript);
  switch ( pid = fork () )  {
  case 0:
    /*  Child  */
    ch_close (write_ch);
    if (dup2 (ch_get_descriptor (read_ch), 0) < 0) {
      fprintf (stderr, "Error duplicating descriptor\t%s\n",sys_errlist[errno]);
      exit (RV_SYS_ERROR);
    }
    execlp ("lpr", "lpr", txt, NULL);
    fprintf (stderr, "Error execing\t%s\n", sys_errlist[errno]);
    exit (RV_SYS_ERROR);
    break;
  case -1:
    /*  Failure  */
    fprintf (stderr, "Error forking\t%s\n", sys_errlist[errno]);
    ch_close (read_ch);
    ch_close (write_ch);
    return;
  default:
    /*  Parent  */
    break;
  }
  ch_close (read_ch);
  if(write_ps(pw,write_ch,colour,FALSE)) {
    if ( ch_close (write_ch) ) {
      /*  Success: wait for child  */
      if (waitpid (pid, &statusp, 0) < 0)  {
	fprintf (stderr, "Error waiting\t%s\n",sys_errlist[errno]);
	kill (pid, SIGKILL);
	return;
      }
      if (statusp == 0) {
	fprintf (stderr, "lpr successfully finished.\n");
      } else {
	fprintf (stderr, "lpr returned with status: %d\n", statusp);
      }
      return;
    }
  }
  ch_close (write_ch);
  kill (pid, SIGKILL);
}

/*----------------------------------------------------------------------*/
/*  This routine will print the image to a file.                        */
/*  If the value of client_data is TRUE, encapsulated PS is written.    */
/*  It is setup as a callback for the "write" buttons.                  */
/*----------------------------------------------------------------------*/

static void write_to_file (Widget w, XtPointer client_data,XtPointer call_data)
{
    Channel channel;
    flag check = TRUE;
    flag eps = (uaddr) client_data;
    unsigned int sequence_number = 0;
    struct stat statbuf;
    char filename[STRING_LENGTH];
    char basefile[STRING_LENGTH];
    String dialog_name;
    extern char *sys_errlist[];
    static char function_name[] = "write_to_file";
    PostscriptWidget pw = (PostscriptWidget) XtParent(XtParent(XtParent(w)));

    XtCallCallbacks((Widget)pw,XtNcallback,NULL);
    FLAG_VERIFY (eps);
    XtVaGetValues (pw->postscript.name_dialog,
		   XtNvalue, &dialog_name,
		   NULL);
    if ( (dialog_name == NULL) || (*dialog_name == '\n') )
    {
	(void) strcpy (basefile, "fred");
    }
    else (void) strcpy (basefile, dialog_name);
    while (check)
    {
	if ( pw->postscript.autoIncrement && (sequence_number > 0) )
	{
	    (void) sprintf (filename, "%s.%u.%s",
			    basefile, sequence_number, eps ? "eps" : "ps");
	}
	else
	{
	    (void) sprintf (filename, "%s.%s", basefile, eps ? "eps" : "ps");
	}
	if (pw->postscript.autoIncrement)
	{
	    ++sequence_number;
	    if (stat (filename, &statbuf) != 0) check = FALSE;
	}
	else check = FALSE;
    }
    if ( ( channel = ch_open_file (filename, "w") ) == NULL )  {
	fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
		 filename, sys_errlist[errno]);
	return;
    }
    if(write_ps(pw,channel,TRUE,eps)) {
	if ( ch_close (channel) )  {
	    fprintf (stderr, "Wrote PostScript file: \"%s\"\n",filename);
	    return;
	}
    }
    fprintf (stderr, "Error writing PostScript\n");
    ch_close (channel);
    unlink (filename);
}

/*----------------------------------------------------------------------*/
/* Register image and filename                                          */
/*----------------------------------------------------------------------*/

void XkwPostscriptRegisterImageAndName (Widget w, KPixCanvas image, char *name)
{
    PostscriptWidget pw = (PostscriptWidget) w;
    static char fn_name[] = "XkwPostscriptRegisterImageAndName";

    check_type (w, fn_name);
    pw->postscript.pixcanvas = image;
    if ( (name == NULL) || (*name == '\0') ) return;
    XtVaSetValues (pw->postscript.name_dialog,
		   XtNvalue, name,
		   NULL);
}   /*  End Function XkwPostscriptRegisterImageAndName  */

static void orientation_cbk (Widget w, XtPointer client_data,
			     XtPointer call_data)
{
    unsigned int orientation = *(int *) call_data;
    PostscriptWidget top = (PostscriptWidget) client_data;

    switch (orientation)
    {
      case ORIENT_PORTRAIT:
	top->postscript.portrait = TRUE;
	break;
      case ORIENT_LANDSCAPE:
	top->postscript.portrait = FALSE;
	break;
      default:
	(void) fprintf (stderr, "Illegal orientation: %u\n", orientation);
	break;
    }
}   /*  End Function orientation_cbk  */

/*----------------------------------------------------------------------*/
/* Create the Value widgets                                             */
/*----------------------------------------------------------------------*/

static Widget create_value (Widget parent, char *name, int *valuepointer)
{
    Widget w;

    w = XtVaCreateManagedWidget (name, valueWidgetClass, parent,
				 XkwNmodifier, 1,
				 XkwNminimum, 0,
				 XkwNmaximum, 300,
				 XtNvalue, *valuepointer,
				 XtNborderWidth, 0,
				 XtNlabel, name,
				 XkwNvaluePtr, valuepointer,
				 NULL);
    return w;
}

/*----------------------------------------------------------------------*/
/* Create a text button                                                 */
/*----------------------------------------------------------------------*/

static void create_text_btn (Widget *btn, char *name,char *cbk_val,
			     XtCallbackProc proc, Widget parent)
{
  *btn=XtVaCreateManagedWidget
    ("button",commandWidgetClass,parent,
     XtNlabel,name,
     XtNhorizDistance,5,
     XtNvertDistance,5,
     NULL);
  XtAddCallback(*btn,XtNcallback,proc,cbk_val);
}

/*----------------------------------------------------------------------*/
/* Initialisation method                                                */
/*----------------------------------------------------------------------*/

static void Postscript__Initialise (Widget Request, Widget New)
{
    /*PostscriptWidget request = (PostscriptWidget) Request;*/
    PostscriptWidget new = (PostscriptWidget) New;
    Widget form, box2, hoffsetdial, voffsetdial, hsizedial, vsizedial;
    Widget psbtn, epsbtn, qcolourbtn, inc_tgl, orient_menu, closebtn, name_dlg;
    char *def_orientation;

    def_orientation = new->postscript.portrait ? "Portrait " : "Landscape";
    form = XtVaCreateManagedWidget ("form", boxWidgetClass, (Widget) new,
				    XtNborderWidth, 0,
				    NULL);

    box2 = XtVaCreateManagedWidget ("box2", boxWidgetClass, (Widget) form,
				    XtNborderWidth, 0,
				    XtNorientation, XtorientHorizontal,
				    NULL);

    closebtn = XtVaCreateManagedWidget ("closeButton", commandWidgetClass,box2,
					XtNlabel, "close",
					XtNhorizDistance, 5,
					XtNvertDistance, 5,
					NULL);
    XtAddCallback (closebtn, XtNcallback, xtmisc_popdown_cbk, New);
    create_text_btn(&psbtn,"save .ps",(char *)FALSE,write_to_file,box2);
    create_text_btn(&epsbtn,"save .eps",(char *)TRUE,write_to_file,box2);
    create_text_btn(&qcolourbtn,"print",(char *)TRUE,send_to_queue,box2);
    orient_menu = XtVaCreateManagedWidget ("menuButton",
					   exclusiveMenuWidgetClass, form,
					   XtNmenuName, "orientationMenu",
					   XkwNchoiceName, "Orientation",
					   XkwNnumItems, NUM_ORIENTATIONS,
					   XkwNitemStrings, orientations,
					   NULL);
    XtAddCallback (orient_menu, XkwNselectCallback, orientation_cbk, New);
    inc_tgl = XtVaCreateManagedWidget ("incToggle", ktoggleWidgetClass, form,
				       XtNlabel, "Auto Increment",
				       XtNstate, new->postscript.autoIncrement,
				       XkwNcrosses, FALSE,
				       NULL);
    XtAddCallback (inc_tgl, XtNcallback, auto_increment_cbk, (XtPointer) New);
    hoffsetdial = create_value (form, "hoffset (mm)",
				&(new->postscript.hoffset));
    voffsetdial = create_value (form, "voffset (mm)",
				&(new->postscript.voffset));
    hsizedial = create_value (form, "hsize (mm)", &(new->postscript.hsize));
    vsizedial = create_value (form, "vsize (mm)", &(new->postscript.vsize));

    name_dlg = XtVaCreateManagedWidget ("nameDialog", dialogWidgetClass, form,
					XtNlabel, "Output file:",
					XtNwidth, 400,
					XtNvalue, "fred",
					NULL);
    new->postscript.name_dialog = name_dlg;
}  /*  End Function Initialise  */

/*----------------------------------------------------------------------*/
/* Destroy method                                                       */
/*----------------------------------------------------------------------*/

static void Destroy (Widget W)
{
  /* Nothing to destroy */
}

/*----------------------------------------------------------------------*/
/* SetValues method                                                     */
/*----------------------------------------------------------------------*/

static Boolean SetValues (Widget Current, Widget Request, Widget New)
{
    /*PostscriptWidget current = (PostscriptWidget) Current;
      PostscriptWidget request = (PostscriptWidget) Request;
      PostscriptWidget new = (PostscriptWidget) New;*/

    /* Not needed at the moment */

    return True;
}

static void auto_increment_cbk (Widget w, XtPointer client_data,
				XtPointer call_data)
{
    flag bool = (uaddr) call_data;
    PostscriptWidget top = (PostscriptWidget) client_data;

    top->postscript.autoIncrement = bool;
}   /*  End Function auto_increment_cbk  */
