/*LINTLIBRARY*/
/*  Postscript.c

    This code provides a postscript control widget for Xt.

    PostScript code Copyright (C) 1994  Richard Gooch

    Widget code Copyright (C) 1994  Patrick Jordan
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

    Last updated by Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>


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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>

#include <Xkw/Value.h>

#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <karma_kwin.h>
#include <karma_psw.h>
#include <karma_st.h>
#include <karma_ch.h>
#include <karma_r.h>

/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Methods*/

static void Initialize(Widget request,Widget new);
static void Destroy(Widget w);
static Boolean SetValues(Widget current,Widget request,Widget new);

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(PostscriptWidget, postscript.field)

static XtResource PostscriptResources[] = 
{
  {XtNcallback, XtCCallback, XtRCallback, sizeof(caddr_t),
    TheOffset(callback), XtRCallback, (caddr_t)NULL},
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
    NULL,                          /* class_initialize */
    NULL,                          /* class_part_initialize */
    FALSE,                         /* class_init */
    (XtInitProc)Initialize,        /* initialize */
    NULL,                          /* initialize_hook */
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

/*----------------------------------------------------------------------*/
/* check that a widget is actually a PostscriptWidget                      */
/*----------------------------------------------------------------------*/

static int check_type(Widget w,char *function_name)
{
  static char func_name[]="Postscript.check_type";
  int fl;
  fl=XtIsSubclass(w,postscriptWidgetClass);
  if(!fl)
    fprintf(stderr,
    "ERROR: Widget passed to %s is not a PostscriptWidget\n",function_name);
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
  pspage = psw_create (channel,w->postscript.hoffset/10.0,
		       w->postscript.voffset/10.0,
		       w->postscript.hsize/10.0,w->postscript.vsize/10.0,
		       w->postscript.portrait);
  if(pspage == NULL ) return (FALSE);
  if(w->postscript.pixcanvas==NULL) {
    fprintf(stderr,"Postscript Widget: NULL Canvas passed - No PS written\n");
    return FALSE;
  }
  if ( kwin_write_ps (w->postscript.pixcanvas, pspage) )  {
    return ( psw_finish (pspage, eps, FALSE, FALSE) );
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

static void send_to_queue (Widget w,XtPointer client_data,XtPointer call_data)
{
  Channel read_ch, write_ch;
  int pid;
  int statusp;
  flag colour = (flag)client_data;
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
  if (ch_create_pipe (&read_ch, &write_ch) != TRUE)  {
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

static void write_to_file(Widget w,XtPointer client_data,XtPointer call_data)
{
  Channel channel;
  char filename[STRING_LENGTH];
  extern char *sys_errlist[];
  static char function_name[] = "write_to_file";
  flag eps = (flag) client_data;
  PostscriptWidget pw=(PostscriptWidget)XtParent(XtParent(XtParent(w)));

  XtCallCallbacks((Widget)pw,XtNcallback,NULL);
  FLAG_VERIFY (eps);
  if (pw->postscript.arrayfile_name == NULL) {
    sprintf (filename, "kview_2d.%s", eps ? "eps" : "ps");
  } else {
    sprintf (filename, "%s.%s", pw->postscript.arrayfile_name,
	     eps ? "eps" : "ps");
  }
  if ( ( channel = ch_open_file (filename, "w") ) == NULL )  {
    fprintf(stderr,
	    "Error opening file: \"%s\"\t%s\n",filename,sys_errlist[errno]);
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
/* This is the callback for the value widgets. Each ValueWidget has a   */
/* pointer associated with it when the callback is attached, and this   */
/* is returned as client_data. It points to the variable to be changed  */
/* by this widget. The new value of the widget is pointed to by         */
/* call_data .                                                          */
/*----------------------------------------------------------------------*/

static void dial_cbk(Widget w,XtPointer client_data,XtPointer call_data)
{
  int newmin;
  double *new_value=(double *)client_data;
  int *fval=(int *)call_data;

  *new_value=(double)(*fval);
}

/*----------------------------------------------------------------------*/
/* Register image and filename                                          */
/*----------------------------------------------------------------------*/

void XkwPostscriptRegisterImageAndName(Widget w,KPixCanvas image,char *name)
{
    static char fn_name[]="XkwPostscriptRegisterImageAndName";
    PostscriptWidget pw=(PostscriptWidget) w;
  
    check_type(w,fn_name);
    pw->postscript.pixcanvas=image;
    if(pw->postscript.arrayfile_name!=NULL)
    m_free(pw->postscript.arrayfile_name);
    pw->postscript.arrayfile_name=st_dup(name);
}

/*----------------------------------------------------------------------*/
/* Callback for portrait/landscape button                               */
/*----------------------------------------------------------------------*/

static void portrait_cbk(Widget w,char *client_data,char *call_data)
{
  flag *portrait = (flag *)client_data;
  char label[20];
  
  if(*portrait==TRUE) {
    sprintf(label,"Landscape");
    *portrait=FALSE;
  } else {
    sprintf(label,"Portrait");
    *portrait=TRUE;
  } 
  XtVaSetValues(w,XtNlabel,label,NULL);		
}

/*----------------------------------------------------------------------*/
/* Callback for close button                                            */
/*----------------------------------------------------------------------*/

static void close_cbk(Widget w,XtPointer client_data,XtPointer call_data)
{
  XtPopdown(XtParent(XtParent(w)));
}

/*----------------------------------------------------------------------*/
/* Create the Value widgets                                             */
/*----------------------------------------------------------------------*/

static Widget create_value(Widget parent,char *name,double *valuepointer)
{
  Widget w;
  int initvalue=(int)*valuepointer;

  w=XtVaCreateManagedWidget
    (name,valueWidgetClass,parent,
     XkwNmodifier,1,
     XkwNminimum,0,
     XkwNmaximum,200,
     XtNvalue,initvalue,
     XtNborderWidth,0,
     XtNlabel,name,
     NULL);
  XtAddCallback(w,XkwNvalueChangeCallback,dial_cbk,valuepointer);
  return w;
}

/*----------------------------------------------------------------------*/
/* Create a text button                                                 */
/*----------------------------------------------------------------------*/

static void create_text_btn(Widget *btn,char *name,char *cbk_val,
			    XtCallbackProc proc,Widget parent)
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

static void Initialize(Widget Request,Widget New)
{
  PostscriptWidget request = (PostscriptWidget) Request;
  PostscriptWidget new = (PostscriptWidget) New;
  Widget form,box2,hoffsetdial,voffsetdial,hsizedial,vsizedial;
  Widget psbtn,epsbtn,qcolourbtn,portraitbtn,closebtn;

  new->postscript.portrait = TRUE;
  new->postscript.hoffset = 10;
  new->postscript.voffset = 10;
  new->postscript.hsize = 180;
  new->postscript.vsize = 180;
  
  form=XtVaCreateManagedWidget
      ("form",boxWidgetClass,(Widget)new,
       XtNborderWidth,0,
       NULL);

  box2=XtVaCreateManagedWidget
      ("box2",boxWidgetClass,(Widget)form,
       XtNborderWidth,0,
       XtNorientation,XtorientHorizontal,
       NULL);

  create_text_btn(&psbtn,"save .ps",(char *)FALSE,write_to_file,box2);
  create_text_btn(&epsbtn,"save .eps",(char *)TRUE,write_to_file,box2);
  create_text_btn(&qcolourbtn,"print",(char *)TRUE,send_to_queue,box2);

  portraitbtn=XtVaCreateManagedWidget
    ("toggle",commandWidgetClass,box2,
     XtNlabel,"Portrait ",
     NULL);
  XtAddCallback(portraitbtn,XtNcallback,(XtCallbackProc)portrait_cbk,
		(char *)&new->postscript.portrait);

  hoffsetdial=create_value(form,"hoffset",&(new->postscript.hoffset));
  voffsetdial=create_value(form,"voffset",&(new->postscript.voffset));
  hsizedial=create_value(form,"hsize",&(new->postscript.hsize));
  vsizedial=create_value(form,"vsize",&(new->postscript.vsize));

  closebtn=XtVaCreateManagedWidget
    ("closeButton",commandWidgetClass,form,
     XtNlabel,"close",
     XtNhorizDistance,5,
     XtNvertDistance,5,
     NULL);
  XtAddCallback(closebtn,XtNcallback,close_cbk,NULL);
}

/*----------------------------------------------------------------------*/
/* Destroy method                                                       */
/*----------------------------------------------------------------------*/

static void Destroy(Widget W)
{
  /* Nothing to destroy */
}

/*----------------------------------------------------------------------*/
/* SetValues method                                                     */
/*----------------------------------------------------------------------*/

static Boolean SetValues(Widget Current,Widget Request,Widget New)
{
  PostscriptWidget current = (PostscriptWidget) Current;
  PostscriptWidget request = (PostscriptWidget) Request;
  PostscriptWidget new = (PostscriptWidget) New;

  /* Not needed at the moment */

  return True;
}
