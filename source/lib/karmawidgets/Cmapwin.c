/*LINTLIBRARY*/
/*  Cmapwin.c

    This code provides a PseudoColour colourmap control widget for Xt.

    Copyright (C) 1994-1996  Patrick Jordan
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

/*  This file contains all routines needed for manipulating a colourmap
    control widget for Xt.


    Written by      Patrick Jordan  28-JAN-1994

    Updated by      Patrick Jordan  3-FEB-1994

    Updated by      Richard Gooch   2-AUG-1994: Removed unneccessary parameter
  from call to  kcmap_list_funcs  .

    Updated by      Richard Gooch   3-JAN-1995: Added  regenerateColourmap
  resource.

    Updated by      Richard Gooch   17-APR-1995: Fixed mix of XtRBool and
  sizeof Boolean in resource fields. Use Bool because it is of int size.

    Updated by      Richard Gooch   28-OCT-1995: Pass Kcolourmap at creation
  rather than setting afterwards.

    Updated by      Richard Gooch   21-APR-1996: Added XtNvisual and
  XkwNsimpleColourbar resources.

    Updated by      Richard Gooch   28-APR-1996: Changed to
  <kcmap_get_funcs_for_cmap>.

    Updated by      Richard Gooch   3-MAY-1996: Increased width by one pixel.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   5-JUN-1996: Added colour scale sliders.

    Last updated by Richard Gooch   1-JUL-1996: Added XkwNdisableScaleSliders
  resource.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>
#include <X11/Xos.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Label.h>
#include <karma.h>
#include <Xkw/CmapwinP.h>
#include <Xkw/SimpleColourbar.h>
#include <Xkw/Palette.h>
#include <Xkw/Twodpos.h>
#include <Xkw/SimpleSlider.h>


#define MAX_INTENSITY 65535

/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

STATIC_FUNCTION (void Cmapwin__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Destroy, (Widget w) );
STATIC_FUNCTION (Boolean Cmapwin__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void ConstraintInitialise, (Widget request, Widget new) );
STATIC_FUNCTION (void red_scale_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void green_scale_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void blue_scale_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );


/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define offset(field) XtOffset(CmapwinWidget, cmapwin.field)

static XtResource CmapwinResources[] = 
{
    {XkwNcolourbarVisual, XtCVisual, XtRVisual, sizeof (Visual *),
     offset (colourbarVisual), XtRImmediate, CopyFromParent},
    {XkwNcolourCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
     offset (colourCallback), XtRCallback, (caddr_t)NULL},
    {XkwNkarmaColourmap, XkwCKarmaColourmap, XtRPointer, sizeof(XtPointer),
     offset (dcm), XtRPointer, (XtPointer)NULL},
    {XkwNregenerateColourmap, XkwCRegenerateColourmap, XtRBool, sizeof (Bool),
     offset (regenerateColourmap), XtRImmediate, (XtPointer) False},
    {XkwNsimpleColourbar, XkwCSimpleColourbar, XtRBool, sizeof (Bool),
     offset (simpleColourbar), XtRImmediate,(XtPointer) False},
    {XkwNdisableScaleSliders, XkwCDisableScaleSliders, XtRBool, sizeof (Bool),
     offset (disableScaleSliders), XtRImmediate, (XtPointer) FALSE},
};

#undef offset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

CmapwinClassRec cmapwinClassRec = 
{
  {        /* CoreClassPart */
    (WidgetClass)&formClassRec,  /* superclass */
    "Cmapwin",                     /* class_name */
    sizeof(CmapwinRec),            /* widget_size */
    NULL,                          /* class_initialise */
    NULL,                          /* class_part_initialise */
    FALSE,                         /* class_init */
    (XtInitProc)Cmapwin__Initialise, /* initialise */
    NULL,                          /* initialise_hook */
    XtInheritRealize,              /* realize */
    NULL,                          /* actions */
    0,                             /* num_actions */
    CmapwinResources,              /* resources */
    XtNumber(CmapwinResources),    /* num_resources */
    NULLQUARK,                     /* xrm_class */
    TRUE,                          /* compress_motion */
    TRUE,                          /* compress_exposure */
    TRUE,                          /* compress_enterleave */
    TRUE,                          /* visible_interest */
    Destroy,                       /* destroy */
    NULL,                          /* resize */
    NULL,                          /* expose */
    (XtSetValuesFunc)Cmapwin__SetValues, /* set_values */
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
  },{ /* Constraint */
    /* subresourses       */   NULL,
    /* subresource_count  */   0,
    /* constraint_size    */   sizeof(CmapwinConstraintsRec),
    /* initialise         */   (XtInitProc)ConstraintInitialise,
    /* destroy            */   NULL,
    /* set_values         */   NULL,
    /* extension          */   NULL
  },{ /* Form */
    /* layout */                XtInheritLayout
  },
  {  /* CmapwinClassPart */
    0 /* empty */
  }
};

WidgetClass cmapwinWidgetClass = (WidgetClass) &cmapwinClassRec;


/*----------------------------------------------------------------------*/
/* Constraint Initialisation method*/
/*----------------------------------------------------------------------*/

static void ConstraintInitialise(Widget request,Widget new)
{
  /* Dummy function: I dunno what it's supposed to do */
}

/*----------------------------------------------------------------------*/
/* Callback for colourmap selection*/
/* A selection has been made from the list. Change to that colourmap*/
/*----------------------------------------------------------------------*/

static void colourmap_cbk (Widget w, XtPointer client_data,XtPointer call_data)
{
    XawListReturnStruct *lr=(XawListReturnStruct *)call_data;
    CmapwinWidget cw = (CmapwinWidget) client_data;

  
    kcmap_change (cw->cmapwin.dcm, cw->cmapwin.list[lr->list_index], 0, FALSE);
    kcmap_modify (cw->cmapwin.dcm,
		  (double) cw->cmapwin.cmap_x, (double) cw->cmapwin.cmap_y,
		  NULL);
}

/*----------------------------------------------------------------------*/
/* Callback for colourmap selection*/
/* A selection has been made from the list. Change to that colourmap*/
/*----------------------------------------------------------------------*/

void XkwCmapwinSetColourmap(Widget w,char *new_cmap_name)
{
  CmapwinWidget cw = (CmapwinWidget)w;
  int i;

  for(i=0;i<cw->cmapwin.listcount;i++) {
    if(!strcmp(cw->cmapwin.list[i],new_cmap_name)) {
      kcmap_change(cw->cmapwin.dcm,new_cmap_name,0,FALSE);
      kcmap_modify(cw->cmapwin.dcm,(double)cw->cmapwin.cmap_x,
		   (double)cw->cmapwin.cmap_y,NULL);
      XawListHighlight(cw->cmapwin.selector,i);
      return;
    }
  }
  fprintf(stderr,"Cmapwin: unknow colourmap name: %s\n",new_cmap_name);
}

/*----------------------------------------------------------------------*/
/* Twodthing callback*/
/*----------------------------------------------------------------------*/

static void twodthing_cbk(Widget w,XtPointer client_data,XtPointer call_data)
{
  CmapwinWidget cw = (CmapwinWidget)client_data;
  TwodposCallbackPtr data = (TwodposCallbackPtr)call_data;

  cw->cmapwin.cmap_x=data->value_x;
  cw->cmapwin.cmap_y=data->value_y;
  kcmap_modify(cw->cmapwin.dcm,cw->cmapwin.cmap_x,cw->cmapwin.cmap_y,NULL);
}

/*----------------------------------------------------------------------*/
/* palette callback*/
/*----------------------------------------------------------------------*/

static void palette_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    PaletteCallbackPtr data = (PaletteCallbackPtr) call_data;
    CmapwinWidget cw = (CmapwinWidget)client_data;
    int numpix;
    unsigned long *pixels;
    char num[10];

    numpix = kcmap_get_pixels(cw->cmapwin.dcm,&pixels)-1;
    sprintf (num, "%d", (int) (data->value*numpix));
    XtVaSetValues(cw->cmapwin.thecolour,
		  XtNborderColor,pixels[(int)(data->value*numpix)],
		  XtNlabel,num,
		  XtNwidth,80,
		  XtNheight,20,
		  NULL);
    XtCallCallbacks((Widget)cw, XkwNcolourCallback, call_data);
}

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Cmapwin__Initialise (Widget Request, Widget New)
{
    /*CmapwinWidget request = (CmapwinWidget) Request;*/
    CmapwinWidget new = (CmapwinWidget) New;
    Widget view, palette, form, w;
    int i;

    new->form.default_spacing = 0;
    new->cmapwin.cmap_x = 0.5;
    new->cmapwin.cmap_y = 0.5;

    new->cmapwin.listcount = 0;
    new->cmapwin.list = kcmap_get_funcs_for_cmap (new->cmapwin.dcm);
    for (i=0; new->cmapwin.list[i] != NULL; i++)
    new->cmapwin.listcount++;	/* Count the list */

    if (new->cmapwin.simpleColourbar)
    {
	palette = XtVaCreateManagedWidget ("colourbar",
					   simpleColourbarWidgetClass, New,
					   XtNwidth, 408,
					   XtNheight, 33,
					   XtNhorizDistance, 0,
					   XtNvertDistance, 0,
					   XtNvisual,
					   new->cmapwin.colourbarVisual,
					   XkwNkarmaColourmap,new->cmapwin.dcm,
					   NULL);
    }
    else
    {
	palette = XtVaCreateManagedWidget ("palette", paletteWidgetClass, New,
					   XtNwidth, 351,
					   XtNheight, 50,
					   XtNvalue, 0,
					   XtNhorizDistance, 0,
					   XtNvertDistance, 0,
					   XkwNkarmaColourmap,new->cmapwin.dcm,
					   NULL);
	XtAddCallback (palette, XkwNvalueChangeCallback, palette_cbk, new);
	form = XtVaCreateManagedWidget ("form", formWidgetClass, New,
					XtNdefaultDistance, 0,
					XtNborderWidth, 1,
					XtNwidth, 50,
					XtNheight, 50,
					XtNhorizDistance, 5,
					XtNvertDistance, 0,
					XtNfromHoriz, palette,
					NULL);
	new->cmapwin.thecolour = XtVaCreateManagedWidget
	    ("thecolour", labelWidgetClass,form,
	     XtNlabel, "",
	     XtNwidth, 30,
	     XtNheight, 30,
	     XtNhorizDistance, 0,
	     XtNvertDistance, 0,
	     XtNborderColor, kcmap_get_pixel (new->cmapwin.dcm, 0),
	     XtNborderWidth, 10,
	     NULL);
    }

    new->cmapwin.twodthing = XtVaCreateManagedWidget
	("twodthing", twodposWidgetClass,New,
	 XtNwidth, 155,
	 XtNheight, 155,
	 XtNhorizDistance, 0,
	 XtNvertDistance, 5,
	 XtNfromVert, palette,
	 NULL);
    XtAddCallback (new->cmapwin.twodthing, XkwNvalueChangeCallback,
		   twodthing_cbk, new);

  
    view = XtVaCreateManagedWidget ("view", viewportWidgetClass, New,
				    XtNforceBars, True,
				    XtNwidth, 246,
				    XtNheight, 155,
				    XtNhorizDistance, 5,
				    XtNvertDistance, 5,
				    XtNfromVert, palette,
				    XtNfromHoriz, new->cmapwin.twodthing,
				    XtNuseRight, True,
				    XtNallowVert, True,
				    NULL);
  
    new->cmapwin.selector = XtVaCreateManagedWidget
	("selector", listWidgetClass, view,
	 XtNlist, new->cmapwin.list,
	 XtNnumberStrings, new->cmapwin.listcount,
	 XtNdefaultColumns, 1,
	 XtNforceColumns, True,
	 NULL);
    XtAddCallback (new->cmapwin.selector, XtNcallback, colourmap_cbk, new);
    if (new->cmapwin.disableScaleSliders) return;
    w = XtVaCreateManagedWidget ("redScaleSlider", simpleSliderWidgetClass,New,
				 XtNfromVert, view,
				 XtNwidth, 408,
				 XtNlabel, "Red Scale",
				 XkwNminimum, 0,
				 XkwNmaximum, MAX_INTENSITY,
				 XkwNmodifier, 100,
				 XtNvalue, MAX_INTENSITY,
				 XkwNshowValue, FALSE,
				 NULL);
    XtAddCallback (w, XkwNvalueChangeCallback, red_scale_cbk, new);
    w = XtVaCreateManagedWidget ("greenScaleSlider", simpleSliderWidgetClass,
				 New,
				 XtNfromVert, w,
				 XtNwidth, 408,
				 XtNlabel, "Green Scale",
				 XkwNminimum, 0,
				 XkwNmaximum, MAX_INTENSITY,
				 XkwNmodifier, 100,
				 XtNvalue, MAX_INTENSITY,
				 XkwNshowValue, FALSE,
				 NULL);
    XtAddCallback (w, XkwNvalueChangeCallback, green_scale_cbk, new);
    w = XtVaCreateManagedWidget ("blueScaleSlider",simpleSliderWidgetClass,New,
				 XtNfromVert, w,
				 XtNwidth, 408,
				 XtNlabel, "Blue Scale",
				 XkwNminimum, 0,
				 XkwNmaximum, MAX_INTENSITY,
				 XkwNmodifier, 100,
				 XtNvalue, MAX_INTENSITY,
				 XkwNshowValue, FALSE,
				 NULL);
    XtAddCallback (w, XkwNvalueChangeCallback, blue_scale_cbk, new);
}   /*  End Function Initialise  */

/*----------------------------------------------------------------------*/
/* Destroy method*/
/*----------------------------------------------------------------------*/

static void Destroy(Widget W)
{
  XtRemoveAllCallbacks(W, XkwNcolourCallback);
}

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean Cmapwin__SetValues (Widget Current, Widget Request, Widget New)
{
    /*CmapwinWidget current = (CmapwinWidget) Current;
      CmapwinWidget request = (CmapwinWidget) Request;*/
    CmapwinWidget new = (CmapwinWidget) New;

    if (new->cmapwin.regenerateColourmap)
    {
	kcmap_modify (new->cmapwin.dcm,
		      new->cmapwin.cmap_x, new->cmapwin.cmap_y, NULL);
	new->cmapwin.regenerateColourmap = False;
    }
    return True;
}   /*  End Function SetValues  */

static void red_scale_cbk (Widget w, XtPointer client_data,XtPointer call_data)
/*  [SUMMARY] Red scale callback.
    [RETURNS] Nothing.
*/
{
    CmapwinWidget top = (CmapwinWidget) client_data;
    unsigned short value = *(int *) call_data;

    kcmap_set_attributes (top->cmapwin.dcm,
			  KCMAP_ATT_RED_SCALE, value,
			  KCMAP_ATT_END);
    kcmap_modify (top->cmapwin.dcm,
		  top->cmapwin.cmap_x, top->cmapwin.cmap_y, NULL);
}   /*  End Function red_scale_cbk  */

static void green_scale_cbk (Widget w, XtPointer client_data,
			     XtPointer call_data)
/*  [SUMMARY] Green scale callback.
    [RETURNS] Nothing.
*/
{
    CmapwinWidget top = (CmapwinWidget) client_data;
    unsigned short value = *(int *) call_data;

    kcmap_set_attributes (top->cmapwin.dcm,
			  KCMAP_ATT_GREEN_SCALE, value,
			  KCMAP_ATT_END);
    kcmap_modify (top->cmapwin.dcm,
		  top->cmapwin.cmap_x, top->cmapwin.cmap_y, NULL);
}   /*  End Function green_scale_cbk  */

static void blue_scale_cbk (Widget w, XtPointer client_data,
			    XtPointer call_data)
/*  [SUMMARY] Blue scale callback.
    [RETURNS] Nothing.
*/
{
    CmapwinWidget top = (CmapwinWidget) client_data;
    unsigned short value = *(int *) call_data;

    kcmap_set_attributes (top->cmapwin.dcm,
			  KCMAP_ATT_BLUE_SCALE, value,
			  KCMAP_ATT_END);
    kcmap_modify (top->cmapwin.dcm,
		  top->cmapwin.cmap_x, top->cmapwin.cmap_y, NULL);
}   /*  End Function blue_scale_cbk  */

