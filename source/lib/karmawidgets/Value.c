/*LINTLIBRARY*/
/*  Value.c

    This code provides a simple slider widget for Xt.

    Copyright (C) 1994  Patrick Jordan  Incorporated into Karma by permission.

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

/*  This file contains all routines needed for simple slider widget for Xt.
    As it stands, it does not actually have a slider, merely a set of buttons
    for increasing or decreasing the value, by a factor of 1xModifier and
    10xModifier. Due to a bug in Xt, it seems difficult to get this to work
    for floats, so it only handles integer values. A maximum and minimum value
    must be set, and these cannot be exceeded. An initial value should be 
    given. The modifer value would normally be 1, but may be any integer value.
    Because the default values for minimum, maximum, value and modifier
    are all zero, all must explicitly be set for the widget to do anything.


    Written by      Patrick Jordan  23-JUN-1994

    Updated by      Richard Gooch   30-JUL-1994: Doubled width of label widget.

    Updated by      Richard Gooch   31-JUL-1994: Moved dispatch of
  XtNvalueChangeCallback  callbacks from  modify_value  to  btn_cbk  .This
  prevents calls to  SetValues  from calling the callbacks (particularly
  dangerous with use of  XtSetInsensitive  ).

    Updated by      Richard Gooch   13-SEP-1994: Added  XkwNwrap  resource.

    Updated by      Richard Gooch   9-DEC-1994: Activated call to  modify_value
  in  SetValues  routine.

    Updated by      Richard Gooch   10-DEC-1994: Fixed wraparound bug.

    Updated by      Richard Gooch   12-DEC-1994: Added XtNorientation resource.

    Last updated by Richard Gooch   29-DEC-1994: Changed SetValues to update
  label when needed and removed empty Destroy method.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <Xkw/ValueP.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <Xkw/Repeater.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>


#define LABEL_HEIGHT 15
#define VALUE_HEIGHT 17
#define INCREMENTOR_WIDTH 21

/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Methods*/

STATIC_FUNCTION (void Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (Boolean SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void ConstraintInitialise, (Widget request, Widget new) );

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

static int intZero = 0.0;

#define TheOffset(field) XtOffset(ValueWidget, value.field)

static XtResource ValueResources[] = 
{
    {XkwNvalueChangeCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
     TheOffset(valueChangeCallback), XtRCallback, (caddr_t)NULL},
    {XtNlabel,  XtCLabel, XtRString, sizeof(String),
     TheOffset(label), XtRString, NULL},
    {XkwNminimum, XkwCMinimum, XtRInt, sizeof(int),
     TheOffset(minimum), XtRInt, (XtPointer)&intZero},
    {XkwNmaximum, XkwCMaximum, XtRInt, sizeof(int),
     TheOffset(maximum), XtRInt, (XtPointer)&intZero},
    {XtNvalue, XtCValue, XtRInt, sizeof(int),
     TheOffset(value), XtRInt, (XtPointer)&intZero},
    {XkwNmodifier, XkwCModifier, XtRInt, sizeof(int),
     TheOffset(modifier), XtRInt, (XtPointer)&intZero},
    {XkwNwrap, XkwCWrap, XtRBool, sizeof(Boolean),
     TheOffset(wrap), XtRImmediate, False},
    {XtNorientation, XtCOrientation, XtROrientation, sizeof(XtOrientation),
     TheOffset(layout), XtRImmediate, (XtPointer) XtorientHorizontal},
};

#undef TheOffset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

ValueClassRec valueClassRec = 
{
  {        /* CoreClassPart */
    (WidgetClass) &formClassRec,   /* superclass */
    "Value",                       /* class_name */
    sizeof (ValueRec),             /* widget_size */
    NULL,                          /* class_initialise */
    NULL,                          /* class_part_initialise */
    FALSE,                         /* class_init */
    (XtInitProc) Initialise,       /* initialise */
    NULL,                          /* initialise_hook */
    XtInheritRealize,              /* realize */
    NULL,                          /* actions */
    0,                             /* num_actions */
    ValueResources,                /* resources */
    XtNumber (ValueResources),     /* num_resources */
    NULLQUARK,                     /* xrm_class */
    TRUE,                          /* compress_motion */
    TRUE,                          /* compress_exposure */
    TRUE,                          /* compress_enterleave */
    TRUE,                          /* visible_interest */
    NULL,                          /* destroy */
    NULL,                          /* resize */
    NULL,                          /* expose */
    (XtSetValuesFunc) SetValues,   /* set_values */
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
    /* constraint_size    */   sizeof (ValueConstraintsRec),
    /* initialise         */   NULL,
    /* destroy            */   NULL,
    /* set_values         */   NULL,
    /* extension          */   NULL
  },{ /* Form */
    /* layout */                XtInheritLayout
  },
  {  /* ValueClassPart */
    0 /* empty */
  }
};

WidgetClass valueWidgetClass = (WidgetClass) &valueClassRec;

/*----------------------------------------------------------------------*/
/* Modify the current value of a widget according to the multiplier*/
/* and the (stored) modifier value*/
/*----------------------------------------------------------------------*/

static void modify_value (Widget w, int multiplier)
{
    int val;
    ValueWidget vw = (ValueWidget) w;
    char label[10];

    val = vw->value.value + vw->value.modifier * multiplier;

    if (val > vw->value.maximum)
    {
	if ( (*vw).value.wrap )
	{
	    val -= vw->value.maximum + 1;
	    val += vw->value.minimum;
	}
	else
	{
	    val = vw->value.maximum;
	}
    }
    if (val < vw->value.minimum)
    {
	if ( (*vw).value.wrap )
	{
	    val -= vw->value.minimum - 1;
	    val += vw->value.maximum;
	}
	else
	{
	    val = vw->value.minimum;
	}
    }
    vw->value.value = val;
    (void) sprintf (label, "%d", vw->value.value);
    XtVaSetValues (vw->value.vallabel,
		   XtNlabel, label,
		   XtNwidth, 40,
		   XtNheight, VALUE_HEIGHT,
		   NULL);
}

/*----------------------------------------------------------------------*/
/* Button callback. When a button is pressed, this is called. Each button*/
/* has a value, the multiplier associated with it when it is created, and*/
/* this is passed in here as the client_data.*/
/*----------------------------------------------------------------------*/

static void btn_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    int val;
    int multiplier = (iaddr) client_data;
    Widget p2, value_widget;
    ValueWidget vw;

    if ( !XtIsSensitive (w) ) return;
    /*  Get the Value widget: may be 2 or 3 levels up!  */
    p2 = XtParent ( XtParent (w) );
    value_widget = XtParent (p2);
    if ( !XtIsValue (value_widget) ) value_widget = p2;
    modify_value (value_widget, multiplier);
    vw = (ValueWidget) value_widget;
    val = vw->value.value;
    XtCallCallbacks (value_widget, XkwNvalueChangeCallback, (XtPointer) &val);
}

/*----------------------------------------------------------------------*/
/* Create a button with the desired parameters*/
/*----------------------------------------------------------------------*/

static Widget create_btn (Widget parent, char *name,int multiplier,char *label)
{
    Widget w;

    w = XtVaCreateManagedWidget (name, repeaterWidgetClass, parent,
				 XtNlabel, label,
				 XtNheight, LABEL_HEIGHT,
				 XtNwidth, INCREMENTOR_WIDTH,
				 NULL);
    XtAddCallback (w, XtNcallback, btn_cbk, (char *) multiplier);
    return w;
}

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Initialise (Widget Request, Widget New)
{
    ValueWidget request = (ValueWidget) Request;
    ValueWidget new = (ValueWidget) New;
    Widget w, box, label, upbtn, downbtn, upfastbtn, downfastbtn, box2;

    if ( (*new).value.layout == XtorientVertical )
    {
	/*  Label must come above incrementors  */
	box2 = XtVaCreateManagedWidget ("box", boxWidgetClass, New,
					XtNorientation, XtorientVertical,
					XtNborderWidth, 0,
					NULL);
	w = XtVaCreateManagedWidget ("label", labelWidgetClass, box2,
				     XtNlabel, new->value.label,
				     XtNheight, LABEL_HEIGHT,
				     XtNwidth, 140,
				     XtNborderWidth, 0,
				     NULL);
	new->value.labelwidget = w;
    }
    else
    {
	box2 = New;
    }
    box = XtVaCreateManagedWidget ("box", boxWidgetClass, box2,
				   XtNorientation, XtorientHorizontal,
				   XtNborderWidth, 0,
				   XtNvSpace, 0,
				   NULL);
    if (box2 == New)
    {
	w = XtVaCreateManagedWidget ("label", labelWidgetClass, box,
				     XtNlabel, new->value.label,
				     XtNheight, LABEL_HEIGHT,
				     XtNwidth, 140,
				     XtNborderWidth, 0,
				     NULL);
	new->value.labelwidget = w;
    }
    downfastbtn = create_btn (box, "downfast", -10, "<<");
    downbtn = create_btn (box, "down", -1, "<");
    new->value.vallabel = XtVaCreateManagedWidget ("vallabel",
						   labelWidgetClass, box,
						   XtNheight, VALUE_HEIGHT,
						   XtNwidth, 40,
						   NULL);
    upbtn = create_btn (box, "up", 1, ">");
    upfastbtn = create_btn (box, "upfast", 10, ">>");
    modify_value (New, 0);
}   /*  End Function Initialise  */

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean SetValues (Widget Current, Widget Request, Widget New)
{
    ValueWidget current = (ValueWidget) Current;
    ValueWidget request = (ValueWidget) Request;
    ValueWidget new = (ValueWidget) New;

    modify_value (New, 0);
    if (new->value.label != current->value.label)
    {
	XtVaSetValues (new->value.labelwidget,
		       XtNlabel, new->value.label,
		       XtNwidth, 140,
		       XtNheight, LABEL_HEIGHT,
		       NULL);
    }
    return True;
}
