/* $XConsortium: Ktoggle.c,v 1.24 91/07/25 14:07:48 converse Exp $ */

/*
 * Copyright 1989 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * Ktoggle.c - Ktoggle button widget
 *
 * Author: Chris D. Peterson
 *         MIT X Consortium 
 *         kit@expo.lcs.mit.edu
 *  
 * Date:   January 12, 1989
 *
 */

#include <stdio.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xmu/Converters.h>
#include <X11/Xmu/Misc.h>
#include <karma.h>
#include <Xkw/KtoggleP.h>

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

/* 
 * The order of ktoggle and notify are important, as the state has
 * to be set when we call the notify proc.
 */

static char defaultTranslations[] =
    "<EnterWindow>:	    highlight(Always)	\n\
     <LeaveWindow>:	    unhighlight()	\n\
     <Btn1Down>,<Btn1Up>:   ktoggle() notify() updateCross()";

#define offset(field) XtOffsetOf(KtoggleRec, field)

static XtResource resources[] = { 
   {XtNstate, XtCState, XtRBoolean, sizeof(Boolean), 
      offset(ktoggle.yes), XtRString, "off"},
   {XtNradioGroup, XtCWidget, XtRWidget, sizeof(Widget), 
      offset(ktoggle.widget), XtRWidget, (XtPointer) NULL },
   {XtNradioData, XtCRadioData, XtRPointer, sizeof(XtPointer), 
      offset(ktoggle.radio_data), XtRPointer, (XtPointer) NULL },
   {XkwNcrosses, XkwCCrosses,XtRBoolean, sizeof(Boolean),
      offset(ktoggle.CrossesOn),XtRImmediate, (XtPointer) True},
};

#undef offset


static void Ktoggle(), Initialize(), Notify(), KtoggleSet(), UpdateCross();
static void KtoggleDestroy(), ClassInit();
static Boolean SetValues();
static void Snakes(), Ladders();
/* Functions for handling the Radio Group. */

static RadioGroup * GetRadioGroup();
static void CreateRadioGroup(), AddToRadioGroup(), TurnOffRadioSiblings();
static void RemoveFromRadioGroup();

static XtActionsRec actionsList[] =
{
  {"ktoggle",	        Ktoggle},
  {"notify",	        Notify},
  {"set",	        KtoggleSet},
  {"updateCross",       UpdateCross},
};

#define SuperClass ((CommandWidgetClass)&commandClassRec)

KtoggleClassRec ktoggleClassRec = {
  {
    (WidgetClass) SuperClass,		/* superclass		  */	
    "Ktoggle",				/* class_name		  */
    sizeof(KtoggleRec),			/* size			  */
    ClassInit,				/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    Initialize,				/* initialize		  */
    NULL,				/* initialize_hook	  */
    XtInheritRealize,			/* realize		  */
    actionsList,			/* actions		  */
    XtNumber(actionsList),		/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* resource_count	  */
    NULLQUARK,				/* xrm_class		  */
    FALSE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    NULL,         			/* destroy		  */
    XtInheritResize,			/* resize		  */
    XtInheritExpose,			/* expose		  */
    SetValues,				/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    NULL,				/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    defaultTranslations,		/* tm_table		  */
    XtInheritQueryGeometry,		/* query_geometry	  */
    XtInheritDisplayAccelerator,	/* display_accelerator	  */
    NULL				/* extension		  */
  },  /* CoreClass fields initialization */
  {
    XtInheritChangeSensitive		/* change_sensitive	  */ 
  },  /* SimpleClass fields initialization */
  {
    0                                     /* field not used    */
  },  /* LabelClass fields initialization */
  {
    0                                     /* field not used    */
  },  /* CommmandClass fields initialization */
  {
      NULL,			        /* Set Procedure. */
      NULL,			        /* Unset Procedure. */
      NULL			        /* extension. */
  }  /* KtoggleClass fields initialization */
};

  /* for public consumption */
WidgetClass ktoggleWidgetClass = (WidgetClass) &ktoggleClassRec;


/***************************************************************
 *
 *
 * Private constants
 *
 ***************************************************************/

#define tick_width 12
#define tick_height 12
#define tick_x_hot 0
#define tick_y_hot 0
static unsigned char tick_bits[] = {
   0xff, 0x0f, 0x01, 0x08, 0x01, 0x08, 0x01, 0x0a, 0x01, 0x0b, 0x81, 0x09,
   0xc5, 0x08, 0x6d, 0x08, 0x39, 0x08, 0x11, 0x08, 0x01, 0x08, 0xff, 0x0f};

#define cross_width 12
#define cross_height 12
#define cross_x_hot 0
#define cross_y_hot 0
static unsigned char cross_bits[] = {
   0xff, 0x0f, 0x01, 0x08, 0x0d, 0x0b, 0x99, 0x09, 0x91, 0x08, 0x61, 0x08,
   0x61, 0x08, 0xd1, 0x08, 0x99, 0x09, 0x0d, 0x0b, 0x01, 0x08, 0xff, 0x0f};

#define box_width 12
#define box_height 12
#define box_x_hot 0
#define box_y_hot 0
static unsigned char box_bits[] = {
   0xff, 0x0f, 0x01, 0x08, 0x01, 0x08, 0x01, 0x08, 0x01, 0x08, 0x01, 0x08,
   0x01, 0x08, 0x01, 0x08, 0x01, 0x08, 0x01, 0x08, 0x01, 0x08, 0xff, 0x0f};





#define bigtick_width 20
#define bigtick_height 20
#define bigtick_x_hot 0
#define bigtick_y_hot 0
static char unsigned bigtick_bits[] = {
   0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x01, 0x00, 0x06, 0x01, 0x00, 0x04,
   0x01, 0x00, 0x07, 0x01, 0xc0, 0x04, 0x01, 0x60, 0x06, 0x01, 0x30, 0x06,
   0x01, 0x18, 0x06, 0x01, 0x0c, 0x06, 0x01, 0x0e, 0x06, 0x19, 0x07, 0x06,
   0x9d, 0x03, 0x06, 0xbf, 0x01, 0x06, 0xfd, 0x01, 0x06, 0xfd, 0x00, 0x06,
   0x79, 0x00, 0x06, 0x31, 0x00, 0x06, 0xaf, 0xff, 0x07, 0xfe, 0xff, 0x07};



#define bigbox_width 20
#define bigbox_height 20
#define bigbox_x_hot 0
#define bigbox_y_hot 0
static unsigned char bigbox_bits[] = {
   0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x01, 0x00, 0x06, 0x01, 0x00, 0x06,
   0x01, 0x00, 0x06, 0x01, 0x00, 0x06, 0x01, 0x00, 0x06, 0x01, 0x00, 0x06,
   0x01, 0x00, 0x06, 0x01, 0x00, 0x06, 0x01, 0x00, 0x06, 0x01, 0x00, 0x06,
   0x01, 0x00, 0x06, 0x01, 0x00, 0x06, 0x01, 0x00, 0x06, 0x01, 0x00, 0x06,
   0x01, 0x00, 0x06, 0x01, 0x00, 0x06, 0xff, 0xff, 0x07, 0xfe, 0xff, 0x07};

#define bigcross_width 20
#define bigcross_height 20
#define bigcross_x_hot 0
#define bigcross_y_hot 0
static unsigned char bigcross_bits[] = {
   0x00, 0x00, 0x00, 0xff, 0xff, 0x03, 0x01, 0x00, 0x06, 0x01, 0x00, 0x06,
   0x19, 0x60, 0x06, 0x39, 0x70, 0x06, 0x71, 0x38, 0x06, 0xe1, 0x1c, 0x06,
   0xc1, 0x0f, 0x06, 0x81, 0x07, 0x06, 0x81, 0x07, 0x06, 0xc1, 0x0f, 0x06,
   0xe1, 0x1c, 0x06, 0x71, 0x38, 0x06, 0x39, 0x70, 0x06, 0x19, 0x60, 0x06,
   0x01, 0x00, 0x06, 0x01, 0x00, 0x06, 0xff, 0xff, 0x07, 0xfe, 0xff, 0x07};


/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static void
ClassInit()
{
    XtActionList actions;
    Cardinal num_actions;
    KtoggleWidgetClass class = (KtoggleWidgetClass) ktoggleWidgetClass;
    static XtConvertArgRec parentCvtArgs[] = {
	{XtBaseOffset, (XtPointer)XtOffsetOf(WidgetRec, core.parent),
	 sizeof(Widget)}
    };

    XawInitializeWidgetSet();
    XtSetTypeConverter(XtRString, XtRWidget, XmuNewCvtStringToWidget,
		       parentCvtArgs, XtNumber(parentCvtArgs), XtCacheNone,
		       NULL);
/* 
 * Find the set and unset actions in the command widget's action table. 
 */

  XtGetActionList(commandWidgetClass, &actions, &num_actions);

/*  for (i = 0 ; i < num_actions ; i++) {
    if (streq(actions[i].string, "set"))
	class->ktoggle_class.Set = actions[i].proc;
    if (streq(actions[i].string, "unset")) 
	class->ktoggle_class.Unset = actions[i].proc;

    if ( (class->ktoggle_class.Set != NULL) &&
	 (class->ktoggle_class.Unset != NULL) ) {
	XtFree((char *) actions);
	return;
    }
  }  
  */
  class->ktoggle_class.Set = Ladders;
  class->ktoggle_class.Unset = Snakes;
}

static void Initialize(request, new)
 Widget request, new;
{
    KtoggleWidget tw = (KtoggleWidget) new;
    KtoggleWidget tw_req = (KtoggleWidget) request;


    tw->ktoggle.radio_group = NULL;

    if (tw->ktoggle.radio_data == NULL) 
      tw->ktoggle.radio_data = (XtPointer) new->core.name;

    if (tw->ktoggle.widget != NULL) {
      if ( GetRadioGroup(tw->ktoggle.widget) == NULL) 
	CreateRadioGroup(new, tw->ktoggle.widget);
      else
	AddToRadioGroup( GetRadioGroup(tw->ktoggle.widget), new);
    }      
    XtAddCallback(new, XtNdestroyCallback, KtoggleDestroy, NULL);

/*
 * Command widget assumes that the widget is unset, so we only 
 * have to handle the case where it needs to be set.
 *
 * If this widget is in a radio group then it may cause another
 * widget to be unset, thus calling the notify proceedure.
 *
 * I want to set the ktoggle if the user set the state to "On" in 
 * the resource group, reguardless of what my ancestors did.
 */

    if (tw_req->ktoggle.yes)
      KtoggleSet(new, NULL, NULL, 0);



    /* This is not efficient.  We are creating a bitmap for every instance of
       the object.  I should compress this so that it share two maps between
       widgets appearing on the same screen.

       Have I got "width" and "height" around the right way?
       */
    

#define MinimumForBigBitmap 23

    
    tw->ktoggle.Tick = XCreateBitmapFromData
      (
       XtDisplay(tw),
       RootWindowOfScreen(XtScreen(tw)),
       (char *) (tw->core.height > MinimumForBigBitmap ? bigtick_bits : tick_bits),
       tw->core.height > MinimumForBigBitmap ? bigtick_width : tick_width,
       tw->core.height > MinimumForBigBitmap ? bigtick_height : tick_height
       );
    tw->ktoggle.Cross = XCreateBitmapFromData
	(
	 XtDisplay(tw),
	 RootWindowOfScreen(XtScreen(tw)),
	 (char *) (tw->core.height > MinimumForBigBitmap ? bigcross_bits : cross_bits),
	 tw->core.height > MinimumForBigBitmap ? bigcross_width : cross_width,
	 tw->core.height > MinimumForBigBitmap ? bigcross_height : cross_height
	 );
  tw->ktoggle.Neither = XCreateBitmapFromData
      (
       XtDisplay(tw),
       RootWindowOfScreen(XtScreen(tw)),
       (char *) (tw->core.height > MinimumForBigBitmap ? bigbox_bits : box_bits),
       tw->core.height > MinimumForBigBitmap ? bigbox_width : box_width,
       tw->core.height > MinimumForBigBitmap ? bigbox_height : box_height
       );
    UpdateCross(new);
  }

/************************************************************
 *
 *  Action Procedures
 *
 ************************************************************/

/* ARGSUSED */
static void 
KtoggleSet(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;	/* unused */
{
    KtoggleWidgetClass class = (KtoggleWidgetClass) w->core.widget_class;

    TurnOffRadioSiblings(w);
    class->ktoggle_class.Set(w, event, NULL, 0);
}

/* ARGSUSED */
static void 
Ktoggle(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;	/* unused */
{
  KtoggleWidget tw = (KtoggleWidget)w;
  KtoggleWidgetClass class = (KtoggleWidgetClass) w->core.widget_class;

  if (tw->ktoggle.yes) 
      class->ktoggle_class.Unset(w, event, NULL, 0);
  else 
    KtoggleSet(w, event, params, num_params);
}

/* ARGSUSED */
static void Notify(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;		/* unused */
Cardinal *num_params;	/* unused */
{
  KtoggleWidget tw = (KtoggleWidget) w;
  XtCallCallbacks(w, XtNcallback, (XtPointer) (uaddr) tw->ktoggle.yes);
}



/************************************************************
 *
 * Set specified arguments into widget
 *
 ***********************************************************/

/* ARGSUSED */
static Boolean 
SetValues (current, request, new)
Widget current, request, new;
{
    KtoggleWidget oldtw = (KtoggleWidget) current;
    KtoggleWidget tw = (KtoggleWidget) new;
    KtoggleWidget rtw = (KtoggleWidget) request;

    if (oldtw->ktoggle.widget != tw->ktoggle.widget)
      XawKtoggleChangeRadioGroup(new, tw->ktoggle.widget);

    if (!tw->core.sensitive && oldtw->core.sensitive && rtw->ktoggle.yes)
	tw->ktoggle.yes = True;

    if (oldtw->ktoggle.yes != tw->ktoggle.yes)
      UpdateCross(new);
    
    if (oldtw->ktoggle.yes != tw->ktoggle.yes) {
	tw->ktoggle.yes = oldtw->ktoggle.yes;
	Ktoggle(new, NULL, NULL, 0); /* Does a redisplay. */
    }
    return(FALSE);
}

/*	Function Name: KtoggleDestroy
 *	Description: Destroy Callback for ktoggle widget.
 *	Arguments: w - the ktoggle widget that is being destroyed.
 *                 junk, grabage - not used.
 *	Returns: none.
 */

/* ARGSUSED */
static void
KtoggleDestroy(w, junk, garbage)
Widget w;
XtPointer junk, garbage;
{
  RemoveFromRadioGroup(w);
}

/************************************************************
 *
 * Below are all the private proceedures that handle 
 * radio ktoggle buttons.
 *
 ************************************************************/

/*	Function Name: GetRadioGroup
 *	Description: Gets the radio group associated with a give ktoggle
 *                   widget.
 *	Arguments: w - the ktoggle widget who's radio group we are getting.
 *	Returns: the radio group associated with this ktoggle group.
 */

static RadioGroup *
GetRadioGroup(w)
Widget w;
{
  KtoggleWidget tw = (KtoggleWidget) w;

  if (tw == NULL) return(NULL);
  return( tw->ktoggle.radio_group );
}

/*	Function Name: CreateRadioGroup
 *	Description: Creates a radio group. give two widgets.
 *	Arguments: w1, w2 - the ktoggle widgets to add to the radio group.
 *	Returns: none.
 * 
 *      NOTE:  A pointer to the group is added to each widget's radio_group
 *             field.
 */

static void
CreateRadioGroup(w1, w2)
Widget w1, w2;
{
  char error_buf[BUFSIZ];
  KtoggleWidget tw1 = (KtoggleWidget) w1;
  KtoggleWidget tw2 = (KtoggleWidget) w2;

  if ( (tw1->ktoggle.radio_group != NULL) || (tw2->ktoggle.radio_group != NULL) ) {
    sprintf(error_buf, "%s %s", "Ktoggle Widget Error - Attempting",
	    "to create a new ktoggle group, when one already exists.");
    XtWarning(error_buf);
  }

  AddToRadioGroup( NULL, w1 );
  AddToRadioGroup( GetRadioGroup(w1), w2 );
}

/*	Function Name: AddToRadioGroup
 *	Description: Adds a ktoggle to the radio group.
 *	Arguments: group - any element of the radio group the we are adding to.
 *                 w - the new ktoggle widget to add to the group.
 *	Returns: none.
 */

static void
AddToRadioGroup(group, w)
RadioGroup * group;
Widget w;
{
  KtoggleWidget tw = (KtoggleWidget) w;
  RadioGroup * local;

  local = (RadioGroup *) XtMalloc( sizeof(RadioGroup) );
  local->widget = w;
  tw->ktoggle.radio_group = local;

  if (group == NULL) {		/* Creating new group. */
    group = local;
    group->next = NULL;
    group->prev = NULL;
    return;
  }
  local->prev = group;		/* Adding to previous group. */
  if ((local->next = group->next) != NULL)
      local->next->prev = local;
  group->next = local;
}

/*	Function Name: TurnOffRadioSiblings
 *	Description: Deactivates all radio siblings.
 *	Arguments: widget - a ktoggle widget.
 *	Returns: none.
 */

static void
TurnOffRadioSiblings(w)
Widget w;
{
  RadioGroup * group;
  KtoggleWidgetClass class = (KtoggleWidgetClass) w->core.widget_class;

  if ( (group = GetRadioGroup(w)) == NULL)  /* Punt if there is no group */
    return;

  /* Go to the top of the group. */

  for ( ; group->prev != NULL ; group = group->prev );

  while ( group != NULL ) {
    KtoggleWidget local_tog = (KtoggleWidget) group->widget;
    if ( local_tog->ktoggle.yes ) {
      class->ktoggle_class.Unset(group->widget, NULL, NULL, 0);
      Notify( group->widget, NULL, NULL, 0);
    }
    group = group->next;
  }
}

/*	Function Name: RemoveFromRadioGroup
 *	Description: Removes a ktoggle from a RadioGroup.
 *	Arguments: w - the ktoggle widget to remove.
 *	Returns: none.
 */

static void
RemoveFromRadioGroup(w)
Widget w;
{
  RadioGroup * group = GetRadioGroup(w);
  if (group != NULL) {
    if (group->prev != NULL)
      (group->prev)->next = group->next;
    if (group->next != NULL)
      (group->next)->prev = group->prev;
    XtFree((char *) group);
  }
}

/************************************************************
 *
 * Public Routines
 *
 ************************************************************/
   
/*	Function Name: XawKtoggleChangeRadioGroup
 *	Description: Allows a ktoggle widget to change radio groups.
 *	Arguments: w - The ktoggle widget to change groups.
 *                 radio_group - any widget in the new group.
 *	Returns: none.
 */

void
#if NeedFunctionPrototypes
XawKtoggleChangeRadioGroup(Widget w, Widget radio_group)
#else
XawKtoggleChangeRadioGroup(w, radio_group)
Widget w, radio_group;
#endif
{
  KtoggleWidget tw = (KtoggleWidget) w;
  RadioGroup * group;

  RemoveFromRadioGroup(w);

/*
 * If the ktoggle that we are about to add is set then we will 
 * unset all ktoggles in the new radio group.
 */

  if ( tw->ktoggle.yes && radio_group != NULL )
    XawKtoggleUnsetCurrent(radio_group);

  if (radio_group != NULL)
      if ((group = GetRadioGroup(radio_group)) == NULL)
	  CreateRadioGroup(w, radio_group);
      else AddToRadioGroup(group, w);
}

/*	Function Name: XawKtoggleGetCurrent
 *	Description: Returns the RadioData associated with the ktoggle
 *                   widget that is currently active in a ktoggle group.
 *	Arguments: w - any ktoggle widget in the ktoggle group.
 *	Returns: The XtNradioData associated with the ktoggle widget.
 */

XtPointer
#if NeedFunctionPrototypes
XawKtoggleGetCurrent(Widget w)
#else
XawKtoggleGetCurrent(w)
Widget w;
#endif
{
  RadioGroup * group;

  if ( (group = GetRadioGroup(w)) == NULL) return(NULL);
  for ( ; group->prev != NULL ; group = group->prev);

  while ( group != NULL ) {
    KtoggleWidget local_tog = (KtoggleWidget) group->widget;
    if ( local_tog->ktoggle.yes )
      return( local_tog->ktoggle.radio_data );
    group = group->next;
  }
  return(NULL);
}

/*	Function Name: XawKtoggleSetCurrent
 *	Description: Sets the Ktoggle widget associated with the
 *                   radio_data specified.
 *	Arguments: radio_group - any ktoggle widget in the ktoggle group.
 *                 radio_data - radio data of the ktoggle widget to set.
 *	Returns: none.
 */

void
#if NeedFunctionPrototypes
XawKtoggleSetCurrent(Widget radio_group, XtPointer radio_data)
#else
XawKtoggleSetCurrent(radio_group, radio_data)
Widget radio_group;
XtPointer radio_data;
#endif
{
  RadioGroup * group;
  KtoggleWidget local_tog; 

/* Special case case of no radio group. */

  if ( (group = GetRadioGroup(radio_group)) == NULL) {
    local_tog = (KtoggleWidget) radio_group;
    if ( (local_tog->ktoggle.radio_data == radio_data) )     
      if (!local_tog->ktoggle.yes) {
	KtoggleSet((Widget) local_tog, NULL, NULL, 0);
	Notify((Widget) local_tog, NULL, NULL, 0);
      }
    return;
  }

/*
 * find top of radio_roup 
 */

  for ( ; group->prev != NULL ; group = group->prev);

/*
 * search for matching radio data.
 */

  while ( group != NULL ) {
    local_tog = (KtoggleWidget) group->widget;
    if ( (local_tog->ktoggle.radio_data == radio_data) ) {
      if (!local_tog->ktoggle.yes) { /* if not already set. */
	KtoggleSet((Widget) local_tog, NULL, NULL, 0);
	Notify((Widget) local_tog, NULL, NULL, 0);
      }
      return;			/* found it, done */
    }
    group = group->next;
  }
}
 
/*	Function Name: XawKtoggleUnsetCurrent
 *	Description: Unsets all Ktoggles in the radio_group specified.
 *	Arguments: radio_group - any ktoggle widget in the ktoggle group.
 *	Returns: none.
 */

void
#if NeedFunctionPrototypes
XawKtoggleUnsetCurrent(Widget radio_group)
#else
XawKtoggleUnsetCurrent(radio_group)
Widget radio_group;
#endif
{
  KtoggleWidgetClass class;
  KtoggleWidget local_tog = (KtoggleWidget) radio_group;

  /* Special Case no radio group. */

  if (local_tog->ktoggle.yes) {
    class = (KtoggleWidgetClass) local_tog->core.widget_class;
    class->ktoggle_class.Unset(radio_group, NULL, NULL, 0);
    Notify(radio_group, NULL, NULL, 0);
  }
  if ( GetRadioGroup(radio_group) == NULL) return;
  TurnOffRadioSiblings(radio_group);
}




/* Placed down here so that I don't need to give it the right number of
   parameters ever */


static void UpdateCross(Widget w,XEvent e, String *params, Cardinal *num_p)
{
  KtoggleWidget kw = (KtoggleWidget) w;

  XtVaSetValues(w,XtNleftBitmap,kw->ktoggle.yes ?
		kw->ktoggle.Tick :
			(kw->ktoggle.CrossesOn ?
		 kw->ktoggle.Cross
		   : kw ->ktoggle.Neither)
		,
    NULL);


}



static void Snakes(Widget w)
{
  ((KtoggleWidget) w)->ktoggle.yes = 0;
}

static void Ladders(Widget w)
{
  ((KtoggleWidget) w)->ktoggle.yes = 1;
}
