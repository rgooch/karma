/*LINTLIBRARY*/
/*  ChoiceMenu.c

    This code provides a simple choice menu widget for Xt.

    Copyright (C) 1994-1996  Richard Gooch

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

/*  This file contains all routines needed for a simple choice menu widget for
  Xt.


    Written by      Richard Gooch   9-DEC-1994

    Updated by      Richard Gooch   10-DEC-1994

    Updated by      Richard Gooch   12-DEC-1994: Fixed bug in creation of
  simple menu widget: use XtVaCreatePopupShell.

    Updated by      Richard Gooch   14-DEC-1994: Removed unneccessary call to
  XtParent in Initialise.

    Updated by      Richard Gooch   4-JAN-1995: Cosmetic changes.

    Updated by      Richard Gooch   4-MAY-1996: Added small leftBitmap.

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <Xkw/ChoiceMenuP.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Label.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>

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
STATIC_FUNCTION (void item_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(ChoiceMenuWidget, choiceMenu.field)

static XtResource ChoiceMenuResources[] = 
{
    {XkwNmenuTitle, XkwCMenuTitle, XtRString, sizeof (String),
     TheOffset(menuTitle), XtRString, NULL},
    {XkwNitemStrings, XkwCItemStrings, XtRPointer, sizeof (XtPointer),
     TheOffset(itemStrings), XtRImmediate, NULL},
    {XkwNnumItems, XkwCNumItems, XtRInt, sizeof (int),
     TheOffset(numItems), XtRImmediate, 0},
    {XkwNselectCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     TheOffset(selectCallback), XtRCallback, (caddr_t) NULL},
};

#undef TheOffset

#define bitmap_width 10
#define bitmap_height 10
static unsigned char bitmap_bits[] = {
   0xff, 0x03, 0x01, 0x02, 0x03, 0x03, 0x02, 0x01, 0x86, 0x01, 0x84, 0x00,
   0xcc, 0x00, 0x48, 0x00, 0x48, 0x00, 0x30, 0x00};

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

ChoiceMenuClassRec choiceMenuClassRec = 
{
  {        /* CoreClassPart */
      (WidgetClass) &menuButtonClassRec,   /* superclass */
      "ChoiceMenu",                        /* class_name */
      sizeof(ChoiceMenuRec),               /* widget_size */
      NULL,                                /* class_initialise */
      NULL,                                /* class_part_initialise */
      FALSE,                               /* class_init */
      (XtInitProc)Initialise,              /* initialise */
      NULL,                                /* initialise_hook */
      XtInheritRealize,                    /* realise */
      NULL,                                /* actions */
      0,                                   /* num_actions */
      ChoiceMenuResources,                 /* resources */
      XtNumber(ChoiceMenuResources),       /* num_resources */
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
  {  /* ChoiceMenuClassPart */
    0 /* empty */
  },
};

WidgetClass choiceMenuWidgetClass = (WidgetClass) &choiceMenuClassRec;

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Initialise (Widget Request, Widget New)
{
    int count;
    /*ChoiceMenuWidget request = (ChoiceMenuWidget) Request;*/
    ChoiceMenuWidget new = (ChoiceMenuWidget) New;
    Pixmap pixmap;
    Widget menu, menu_entry;
    char **names;
    struct item_data_type *item_data;
    Display *dpy;
    Screen *screen;
    static char function_name[] = "ChoiceMenu::Initialise";

    dpy = XtDisplay (New);
    screen = XtScreen (New);
    /*  Create bitmap  */
    if ( ( pixmap = XCreateBitmapFromData (dpy, RootWindowOfScreen (screen),
					   (char *) bitmap_bits, bitmap_width,
					   bitmap_height) ) == 0 )
    {
	m_abort (function_name, "bitmap");
    }
    XtVaSetValues (New,
		   XtNleftBitmap, pixmap,
		   NULL);
    menu = XtVaCreatePopupShell (new->menuButton.menu_name,
				 simpleMenuWidgetClass, New,
				 XtNlabel, new->choiceMenu.menuTitle,
				 NULL);
    menu_entry = XtVaCreateManagedWidget ("dividingLine", smeLineObjectClass,
					  menu,
					  NULL);
    names = (char **) new->choiceMenu.itemStrings;
    if ( ( item_data = (struct item_data_type *)
	  m_alloc (sizeof *item_data * new->choiceMenu.numItems) ) == NULL )
    {
	m_abort (function_name, "item data");
    }
    new->choiceMenu.item_data = item_data;
    for (count = 0; count < new->choiceMenu.numItems; ++count)
    {
	menu_entry = XtVaCreateManagedWidget (names[count], smeBSBObjectClass,
					      menu,
					      XtNlabel, names[count],
					      NULL);
	item_data[count].me = New;
	item_data[count].index = count;
	XtAddCallback ( menu_entry, XtNcallback, item_cbk,
		       (XtPointer) (item_data + count) );
    }
}   /*  End Function Initialise  */

/*----------------------------------------------------------------------*/
/* Destroy method*/
/*----------------------------------------------------------------------*/

static void Destroy (Widget W)
{
    ChoiceMenuWidget w = (ChoiceMenuWidget) W;

    m_free ( (char *) w->choiceMenu.item_data );
}   /*  End Function Destroy  */

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean SetValues (Widget Current, Widget Request, Widget New)
{
    /*ChoiceMenuWidget current = (ChoiceMenuWidget) Current;
      ChoiceMenuWidget request = (ChoiceMenuWidget) Request;
      ChoiceMenuWidget new = (ChoiceMenuWidget) New;*/

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
    struct item_data_type *item_data = (struct item_data_type *) client_data;

    val = item_data->index;
    XtCallCallbacks (item_data->me, XkwNselectCallback, (XtPointer) &val );
}   /*  End Function item_cbk  */
