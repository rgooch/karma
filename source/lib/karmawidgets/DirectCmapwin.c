/*LINTLIBRARY*/
/*  DirectCmapwin.c

    This code provides a DirectColour colourmap control widget for Xt.

    Copyright (C) 1996  Richard Gooch.

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

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for manipulating a colourmap
    control widget for Xt.


    Written by      Richard Gooch   28-APR-1996

    Updated by      Richard Gooch   3-MAY-1996: Increased width by one pixel.

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

#include <Xkw/DirectCmapwinP.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>
#include <X11/Xaw/Label.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>

#include <Xkw/SimpleColourbar.h>
#include <Xkw/Twodpos.h>

#define TWODTHING_SIZE 145
#define FORM_SPACING 5

STATIC_FUNCTION (void DirectCmapwin__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (Boolean SetValues,
		 (Widget current, Widget request, Widget new) );

#define offset(field) XtOffset(DirectCmapwinWidget, directCmapwin.field)

static XtResource DirectCmapwinResources[] = 
{
    {XkwNcolourbarVisual, XtCVisual, XtRVisual, sizeof (Visual *),
     offset (colourbarVisual), XtRImmediate, CopyFromParent},
    {XkwNkarmaColourmap, XkwCKarmaColourmap, XtRPointer, sizeof(XtPointer),
     offset (karmaCmap), XtRPointer, (XtPointer) NULL},
    {XkwNregenerateColourmap, XkwCRegenerateColourmap, XtRBool,
     sizeof (Bool), offset (regenerateColourmap), XtRImmediate,
     (XtPointer) False},
};
#undef offset

DirectCmapwinClassRec directCmapwinClassRec = 
{
  {        /* CoreClassPart */
    (WidgetClass) &formClassRec,   /* superclass */
    "DirectCmapwin",               /* class_name */
    sizeof(DirectCmapwinRec),      /* widget_size */
    NULL,                          /* class_initialise */
    NULL,                          /* class_part_initialise */
    FALSE,                         /* class_init */
    (XtInitProc)DirectCmapwin__Initialise,    /* initialise */
    NULL,                          /* initialise_hook */
    XtInheritRealize,              /* realize */
    NULL,                          /* actions */
    0,                             /* num_actions */
    DirectCmapwinResources,              /* resources */
    XtNumber(DirectCmapwinResources),    /* num_resources */
    NULLQUARK,                     /* xrm_class */
    TRUE,                          /* compress_motion */
    TRUE,                          /* compress_exposure */
    TRUE,                          /* compress_enterleave */
    TRUE,                          /* visible_interest */
    NULL,                          /* destroy */
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
  },{ /* Constraint */
    /* subresourses       */   NULL,
    /* subresource_count  */   0,
    /* constraint_size    */   sizeof(DirectCmapwinConstraintsRec),
    /* initialise         */   NULL,
    /* destroy            */   NULL,
    /* set_values         */   NULL,
    /* extension          */   NULL
  },{ /* Form */
    /* layout */                XtInheritLayout
  },
  {  /* DirectCmapwinClassPart */
    0 /* empty */
  }
};

WidgetClass directCmapwinWidgetClass = (WidgetClass) &directCmapwinClassRec;


static void colourmap_cbk (Widget w, XtPointer client_data,XtPointer call_data)
{
    XawListReturnStruct *lr = (XawListReturnStruct *) call_data;
    DirectCmapwinWidget cw = (DirectCmapwinWidget) client_data;

  
    kcmap_change (cw->directCmapwin.karmaCmap,
		  cw->directCmapwin.list[lr->list_index], 0, FALSE);
    kcmap_modify_direct_type (cw->directCmapwin.karmaCmap,
			      cw->directCmapwin.red_x,
			      cw->directCmapwin.red_y, NULL,
			      cw->directCmapwin.green_x,
			      cw->directCmapwin.green_y, NULL,
			      cw->directCmapwin.blue_x,
			      cw->directCmapwin.blue_y, NULL);
}   /*  End Function colourmap_cbk  */

void XkwDirectCmapwinSetColourmap (Widget w, CONST char *new_cmap_name)
{
    DirectCmapwinWidget cw = (DirectCmapwinWidget)w;
    int i;

    for (i = 0; i < cw->directCmapwin.listcount; i++)
    {
	if (strcmp (cw->directCmapwin.list[i], new_cmap_name) != 0) continue;
	kcmap_change (cw->directCmapwin.karmaCmap, new_cmap_name, 0, FALSE);
	kcmap_modify_direct_type (cw->directCmapwin.karmaCmap,
				  cw->directCmapwin.red_x,
				  cw->directCmapwin.red_y, NULL,
				  cw->directCmapwin.green_x,
				  cw->directCmapwin.green_y, NULL,
				  cw->directCmapwin.blue_x,
				  cw->directCmapwin.blue_y, NULL);
	XawListHighlight(cw->directCmapwin.selector,i);
	return;
    }
    (void) fprintf (stderr, "DirectCmapwin: unknow colourmap name: %s\n",
		    new_cmap_name);
}   /*  End Function XkwDirectCmapwinSetColourmap  */

static void red_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    DirectCmapwinWidget cw = (DirectCmapwinWidget) client_data;
    TwodposCallbackPtr data = (TwodposCallbackPtr) call_data;

    cw->directCmapwin.red_x = data->value_x;
    cw->directCmapwin.red_y = data->value_y;
    kcmap_modify_direct_type (cw->directCmapwin.karmaCmap,
			      cw->directCmapwin.red_x,
			      cw->directCmapwin.red_y, NULL,
			      cw->directCmapwin.green_x,
			      cw->directCmapwin.green_y, NULL,
			      cw->directCmapwin.blue_x,
			      cw->directCmapwin.blue_y, NULL);
}   /*  End Function red_cbk  */

static void green_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    DirectCmapwinWidget cw = (DirectCmapwinWidget) client_data;
    TwodposCallbackPtr data = (TwodposCallbackPtr) call_data;

    cw->directCmapwin.green_x = data->value_x;
    cw->directCmapwin.green_y = data->value_y;
    kcmap_modify_direct_type (cw->directCmapwin.karmaCmap,
			      cw->directCmapwin.red_x,
			      cw->directCmapwin.red_y, NULL,
			      cw->directCmapwin.green_x,
			      cw->directCmapwin.green_y, NULL,
			      cw->directCmapwin.blue_x,
			      cw->directCmapwin.blue_y, NULL);
}   /*  End Function green_cbk  */

static void blue_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    DirectCmapwinWidget cw = (DirectCmapwinWidget) client_data;
    TwodposCallbackPtr data = (TwodposCallbackPtr) call_data;

    cw->directCmapwin.blue_x = data->value_x;
    cw->directCmapwin.blue_y = data->value_y;
    kcmap_modify_direct_type (cw->directCmapwin.karmaCmap,
			      cw->directCmapwin.red_x,
			      cw->directCmapwin.red_y, NULL,
			      cw->directCmapwin.green_x,
			      cw->directCmapwin.green_y, NULL,
			      cw->directCmapwin.blue_x,
			      cw->directCmapwin.blue_y, NULL);
}   /*  End Function blue_cbk  */

static void DirectCmapwin__Initialise (Widget Request, Widget New)
{
    /*DirectCmapwinWidget request = (DirectCmapwinWidget) Request;*/
    DirectCmapwinWidget new = (DirectCmapwinWidget) New;
    Widget view, palette;
    Widget w;
    int i;

    new->form.default_spacing = 0;
    new->directCmapwin.red_x = 0.5;
    new->directCmapwin.red_y = 0.5;
    new->directCmapwin.green_x = 0.5;
    new->directCmapwin.green_y = 0.5;
    new->directCmapwin.blue_x = 0.5;
    new->directCmapwin.blue_y = 0.5;

    /*  First create the common list of functions  */
    new->directCmapwin.listcount = 0;
    new->directCmapwin.list = kcmap_get_funcs_for_cmap (new->directCmapwin.karmaCmap);
    for (i = 0; new->directCmapwin.list[i] != NULL; i++)
    {
	new->directCmapwin.listcount++;
    }
    view = XtVaCreateManagedWidget ("view", viewportWidgetClass, New,
				    XtNforceBars, True,
				    XtNwidth, 408,
				    XtNheight, 155,
				    XtNvertDistance, FORM_SPACING,
				    XtNuseRight, True,
				    XtNallowVert, True,
				    NULL);
    w = XtVaCreateManagedWidget ("selector", listWidgetClass, view,
				 XtNhorizDistance, FORM_SPACING,
				 XtNvertDistance, FORM_SPACING,
				 XtNlist, new->directCmapwin.list,
				 XtNnumberStrings,new->directCmapwin.listcount,
				 XtNdefaultColumns, 1,
				 XtNforceColumns, True,
				 NULL);
    new->directCmapwin.selector = w;
    XtAddCallback(new->directCmapwin.selector, XtNcallback, colourmap_cbk,new);
    /*  Create the red control and colourbar  */
    w = XtVaCreateManagedWidget ("twodthing", twodposWidgetClass, New,
				 XtNwidth, TWODTHING_SIZE,
				 XtNheight, TWODTHING_SIZE,
				 XtNvertDistance, FORM_SPACING,
				 XtNfromVert, view,
				 NULL);
    new->directCmapwin.red_twodthing = w;
    XtAddCallback (new->directCmapwin.red_twodthing, XkwNvalueChangeCallback,
		   red_cbk, new);
    palette = XtVaCreateManagedWidget ("colourbar",
				       simpleColourbarWidgetClass, New,
				       XtNwidth, 256,
				       XtNheight, TWODTHING_SIZE,
				       XtNhorizDistance, FORM_SPACING,
				       XtNvertDistance, FORM_SPACING,
				       XtNvisual,
				       new->directCmapwin.colourbarVisual,
				       XkwNkarmaColourmap,
				       new->directCmapwin.karmaCmap,
				       XkwNmaskGreen, True,
				       XkwNmaskBlue, True,
				       XtNfromVert, view,
				       XtNfromHoriz, w,
				       NULL);
    /*  Create the green control and colourbar  */
    w = XtVaCreateManagedWidget ("twodthing", twodposWidgetClass, New,
				 XtNwidth, TWODTHING_SIZE,
				 XtNheight, TWODTHING_SIZE,
				 XtNvertDistance, FORM_SPACING,
				 XtNfromVert, new->directCmapwin.red_twodthing,
				 NULL);
    new->directCmapwin.green_twodthing = w;
    XtAddCallback (new->directCmapwin.green_twodthing, XkwNvalueChangeCallback,
		   green_cbk, new);
    palette = XtVaCreateManagedWidget ("colourbar",
				       simpleColourbarWidgetClass, New,
				       XtNwidth, 256,
				       XtNheight, TWODTHING_SIZE,
				       XtNhorizDistance, FORM_SPACING,
				       XtNvertDistance, FORM_SPACING,
				       XtNvisual,
				       new->directCmapwin.colourbarVisual,
				       XkwNkarmaColourmap,
				       new->directCmapwin.karmaCmap,
				       XkwNmaskRed, True,
				       XkwNmaskBlue, True,
				       XtNfromVert,
				       new->directCmapwin.red_twodthing,
				       XtNfromHoriz, w,
				       NULL);
    /*  Create the blue control and colourbar  */
    w = XtVaCreateManagedWidget("twodthing", twodposWidgetClass, New,
				XtNwidth, TWODTHING_SIZE,
				XtNheight, TWODTHING_SIZE,
				XtNvertDistance, FORM_SPACING,
/*
				XtNbackground, 1,
*/
				XtNfromVert,new->directCmapwin.green_twodthing,
				NULL);
    new->directCmapwin.blue_twodthing = w;
    XtAddCallback (new->directCmapwin.blue_twodthing, XkwNvalueChangeCallback,
		   blue_cbk, new);
    palette = XtVaCreateManagedWidget ("colourbar",
				       simpleColourbarWidgetClass, New,
				       XtNwidth, 256,
				       XtNheight, TWODTHING_SIZE,
				       XtNhorizDistance, FORM_SPACING,
				       XtNvertDistance, FORM_SPACING,
				       XtNvisual,
				       new->directCmapwin.colourbarVisual,
				       XkwNkarmaColourmap,
				       new->directCmapwin.karmaCmap,
				       XkwNmaskRed, True,
				       XkwNmaskGreen, True,
				       XtNfromVert,
				       new->directCmapwin.green_twodthing,
				       XtNfromHoriz, w,
				       NULL);


}   /*  End Function Initialise  */

static Boolean SetValues (Widget Current, Widget Request, Widget New)
{
    /*DirectCmapwinWidget current = (DirectCmapwinWidget) Current;
      DirectCmapwinWidget request = (DirectCmapwinWidget) Request;*/
    DirectCmapwinWidget new = (DirectCmapwinWidget) New;

    if (new->directCmapwin.regenerateColourmap)
    {
	kcmap_modify_direct_type (new->directCmapwin.karmaCmap,
				  new->directCmapwin.red_x,
				  new->directCmapwin.red_y, NULL,
				  new->directCmapwin.green_x,
				  new->directCmapwin.green_y, NULL,
				  new->directCmapwin.blue_x,
				  new->directCmapwin.blue_y, NULL);
	new->directCmapwin.regenerateColourmap = False;
    }
    return True;
}   /*  End Function SetValues  */
