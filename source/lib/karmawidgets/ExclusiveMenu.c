/*LINTLIBRARY*/
/*  ExclusiveMenu.c

    This code provides a simple exclusive menu widget for Xt.

    Copyright (C) 1994,1995  Richard Gooch

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

/*  This file contains all routines needed for a simple exclusive menu widget
  for Xt.


    Written by      Richard Gooch   10-DEC-1994

    Updated by      Richard Gooch   10-DEC-1994

    Updated by      Richard Gooch   12-DEC-1994: Fixed bug in creation of
  simple menu widget: use XtVaCreatePopupShell.

    Updated by      Richard Gooch   14-DEC-1994: Removed unneccessary call to
  XtParent in Initialise.

    Last updated by Richard Gooch   4-JAN-1995: Cosmetic changes.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <Xkw/ExclusiveMenuP.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Label.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>

#include <karma.h>
#include <karma_m.h>

struct item_data_type
{
    Widget me;
    int index;
};


/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Methods*/

STATIC_FUNCTION (void Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Destroy, (Widget w) );
STATIC_FUNCTION (Boolean SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void ConstraintInitialise, (Widget request, Widget new) );
STATIC_FUNCTION (void item_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(ExclusiveMenuWidget, exclusiveMenu.field)

static XtResource ExclusiveMenuResources[] = 
{
  {XkwNchoiceName, XkwCChoiceName, XtRString, sizeof (String),
    TheOffset(choiceName), XtRImmediate, NULL},
  {XkwNitemStrings, XkwCItemStrings, XtRPointer, sizeof (XtPointer),
    TheOffset(itemStrings), XtRImmediate, NULL},
  {XkwNnumItems, XkwCNumItems, XtRInt, sizeof (int),
    TheOffset(numItems), XtRImmediate, 0},
  {XkwNselectCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
    TheOffset(selectCallback), XtRCallback, (caddr_t) NULL},
};

#undef TheOffset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

ExclusiveMenuClassRec exclusiveMenuClassRec = 
{
  {        /* CoreClassPart */
      (WidgetClass) &menuButtonClassRec,   /* superclass */
      "ExclusiveMenu",                     /* class_name */
      sizeof(ExclusiveMenuRec),            /* widget_size */
      NULL,                                /* class_initialise */
      NULL,                                /* class_part_initialise */
      FALSE,                               /* class_init */
      (XtInitProc)Initialise,              /* initialise */
      NULL,                                /* initialise_hook */
      XtInheritRealize,                    /* realise */
      NULL,                                /* actions */
      0,                                   /* num_actions */
      ExclusiveMenuResources,              /* resources */
      XtNumber(ExclusiveMenuResources),    /* num_resources */
      NULLQUARK,                           /* xrm_class */
      TRUE,                                /* compress_motion */
      TRUE,                                /* compress_exposure */
      TRUE,                                /* compress_enterleave */
      TRUE,                                /* visible_interest */
      Destroy,                             /* destroy */
      XtInheritResize,                     /* resize */
      XtInheritExpose,                     /* expose */
      (XtSetValuesFunc)SetValues,          /* set_values */
      NULL,                                /* set_values_hook */
      XtInheritSetValuesAlmost,            /* set_values_almost */
      NULL,                                /* get_values_hook */
      NULL,                                /* accept_focus */
      XtVersion,                           /* version */
      NULL,                                /* callback_private */
      XtInheritTranslations,               /* tm_translations */
      NULL,
      NULL,
      NULL,
  },
  {  /* SimpleClassPart */
    XtInheritChangeSensitive /* change_sensitive */
  },
  {  /* LabelClassPart */
    0 /* empty */
  },
  {  /* CommandClassPart */
    0 /* empty */
  },
  {  /* MenuButtonClassPart */
    0 /* empty */
  },
  {  /* ExclusiveMenuClassPart */
    0 /* empty */
  },
};

WidgetClass exclusiveMenuWidgetClass = (WidgetClass) &exclusiveMenuClassRec;

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Initialise (Widget Request, Widget New)
{
    int count, width, tmp;
    ExclusiveMenuWidget request = (ExclusiveMenuWidget) Request;
    ExclusiveMenuWidget new = (ExclusiveMenuWidget) New;
    Widget menu, menu_entry;
    char **names;
    struct item_data_type *item_data;
    char txt[STRING_LENGTH];
    static char function_name[] = "ExclusiveMenu::Initialise";

    menu = XtVaCreatePopupShell (new->menuButton.menu_name,
				 simpleMenuWidgetClass, New,
				 NULL);
    names = (char **) new->exclusiveMenu.itemStrings;
    if ( ( item_data = (struct item_data_type *)
	  m_alloc (sizeof *item_data * new->exclusiveMenu.numItems) )
	== NULL )
    {
	m_abort (function_name, "item data");
    }
    new->exclusiveMenu.item_data = item_data;
    width = 0;
    for (count = 0; count < new->exclusiveMenu.numItems; ++count)
    {
	menu_entry = XtVaCreateManagedWidget (names[count], smeBSBObjectClass,
					      menu,
					      XtNlabel, names[count],
					      NULL);
	item_data[count].me = New;
	item_data[count].index = count;
	XtAddCallback ( menu_entry, XtNcallback, item_cbk,
		       (XtPointer) (item_data + count) );
	tmp = strlen (names[count]);
	if (tmp > width) width = tmp;
    }
    (void) sprintf (txt, "%s: %s", new->exclusiveMenu.choiceName, names[0]);
    for (width -= strlen (names[0]); width > 0; --width)
    {
	(void) strcat (txt, " ");
    }
    XtVaSetValues (New,
		   XtNlabel, txt,
		   NULL);
}

/*----------------------------------------------------------------------*/
/* Destroy method*/
/*----------------------------------------------------------------------*/

static void Destroy (Widget W)
{
    ExclusiveMenuWidget w = (ExclusiveMenuWidget) W;

    m_free ( (char *) w->exclusiveMenu.item_data );
}   /*  End Function Destroy  */

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean SetValues (Widget Current, Widget Request, Widget New)
{
    ExclusiveMenuWidget current = (ExclusiveMenuWidget) Current;
    ExclusiveMenuWidget request = (ExclusiveMenuWidget) Request;
    ExclusiveMenuWidget new = (ExclusiveMenuWidget) New;

    return True;
}   /*  End Function SetValues  */

static void item_cbk (w, client_data, call_data)
/*  This is the item callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    int val;
    ExclusiveMenuWidget top;
    char **names;
    struct item_data_type *item_data = (struct item_data_type *) client_data;
    char txt[STRING_LENGTH];

    top = (ExclusiveMenuWidget) item_data->me;
    names = (char **) top->exclusiveMenu.itemStrings;
    (void) sprintf (txt, "%s: %s", top->exclusiveMenu.choiceName,
		    names[item_data->index]);
    XtVaSetValues ( (Widget) top,
		   XtNlabel, txt,
		   NULL );
    val = item_data->index;
    XtCallCallbacks (item_data->me, XkwNselectCallback, (XtPointer) &val);
}   /*  End Function item_cbk  */
