/*LINTLIBRARY*/
/*  init.c

    This code provides miscellaneous routines for the X Intrinsics toolkit

    Copyright (C) 1996  Richard Gooch

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

/*  This file contains miscellaneous routines needed for the using the Xt
  toolkit.


    Written by      Richard Gooch   1-DEC-1996

    Last updated by Richard Gooch   7-DEC-1996: Fixed bug in passing argc.
  Caused problems on alpha_OSF1.


*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <errno.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
/*  Do not include  karma.h  first, since CONST may be screwed otherwise. This
    applies to broken versions of  X11/Intrinsic.h  */
#ifndef X11
#  define X11
#endif
#include <karma_xtmisc.h>
#include <karma_xv.h>
#include <karma_m.h>


/*  Private functions  */


/*  Private data  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
Widget xtmisc_init_app_initialise (XtAppContext *app_context_return,
				   CONST char* application_class,
				   XrmOptionDescList options,
				   Cardinal num_options,
				   int *argc_in_out, String *argv_in_out,
				   String *fallback_resources,
				   unsigned int min_ccells, ...)
/*  [SUMMARY] Initialise an application for Xt.
    [PURPOSE] This routine will initialise an application to use Xt. This
    routine is similar to the <<XtVaAppInitialize>> routine, except that a new
    colourmap for the application may be created.
    <app_context_return> As for <<XtVaAppInitialize>>.
    <application_class> As for <<XtVaAppInitialize>>.
    <options> As for <<XtVaAppInitialize>>.
    <num_options> As for <<XtVaAppInitialize>>.
    <argc_in_out> As for <<XtVaAppInitialize>>.
    <argv_in_out> As for <<XtVaAppInitialize>>.
    <fallback_resources> As for <<XtVaAppInitialize>>.
    <min_ccells> The minimum number of colourcells that the application will
    want to allocate. If this many colourcells could not be allocated from the
    default colourmap, the application is given a freshly created colourmap.
    <...> As for <<XtVaAppInitialize>>.
    [RETURNS] The application shell.
*/
{
    va_list argp;
    unsigned int count;
    unsigned long dummy;
    Colormap xcmap;
    XtAppContext ctx;
    Widget w;
    String attr;
#if XT_REVISION > 5
    XtTypedArgList typed_args;
#else
    ArgList args;
#endif
    unsigned long *pixel_values;
    Display *dpy;
    Screen *screen;
    Visual *visual;
    static char function_name[] = "xtmisc_init_app_initialise";

    va_start (argp, min_ccells);
    XtToolkitInitialize ();
    ctx = XtCreateApplicationContext ();
    if (app_context_return != NULL) *app_context_return = ctx;
    XtAppSetFallbackResources (ctx, fallback_resources);
    dpy = XtOpenDisplay (ctx, NULL, NULL, (String) application_class,
			 options, num_options, argc_in_out, argv_in_out);
    if (dpy == NULL) exit (1);
    screen = DefaultScreenOfDisplay (dpy);
    xcmap = DefaultColormapOfScreen (screen);
    if (min_ccells > 0)
    {
	xv_get_visuals (screen, &visual, NULL, NULL);
	if (visual != NULL)
	{
	    /*  Try to allocate colours  */
	    if ( ( pixel_values = (unsigned long *)
		   m_alloc (min_ccells * sizeof *pixel_values) ) == NULL )
	    {
		m_abort (function_name, "pixel values");
	    }
	    if (XAllocColorCells (dpy, xcmap, False, &dummy, 0, pixel_values,
				  min_ccells) == 0)
	    {
		/*  Not enough colourcells left: create new colourmap  */
		if ( ( xcmap = XCreateColormap (dpy,
						XRootWindowOfScreen (screen),
						visual, AllocNone) )
		     == (Colormap) NULL )
		{
		    fprintf (stderr, "Could not create colourmap\n");
		    exit (RV_UNDEF_ERROR);
		}
		XSync (dpy, False);
	    }
	    else
	    {
		/*  Was able to allocate colourcells: free them now  */
		XFreeColors (dpy, xcmap, pixel_values, min_ccells, 0);
	    }
	    m_free ( (char *) pixel_values );
	}
    }
#if XT_REVISION > 5
    if ( ( typed_args = (XtTypedArgList) malloc (sizeof *typed_args) )
	 == NULL )
    {
	m_abort (function_name, "initial ArgList");
    }
    for ( attr = va_arg (argp, String), count = 0; attr != NULL;
	  attr = va_arg (argp, String) )
    {
        if (strcmp (attr, XtVaTypedArg) == 0)
	{
            typed_args[count].name = va_arg (argp, String);
            typed_args[count].type = va_arg (argp, String);
            typed_args[count].value = va_arg (argp, XtArgVal);
            typed_args[count].size = va_arg (argp, int);
        }
	else
	{
	    typed_args[count].name = attr;
	    typed_args[count].type = NULL;
	    typed_args[count].value = va_arg (argp, XtArgVal);
	    typed_args[count].size = 0;
        }
	count++;
	if ( ( typed_args = (XtTypedArgList) 
	       realloc ( (char *) typed_args, 
			 (count + 1) * sizeof *typed_args ) ) == NULL )
	{
	    m_abort (function_name, "ArgList");
	}
    }
    typed_args[count].name = NULL;
    w = XtVaAppCreateShell (NULL, application_class, 
			    applicationShellWidgetClass, dpy,
			    XtNscreen, (XtArgVal) screen,
			    XtNcolormap, xcmap,
			    XtNargc, (XtArgVal) *argc_in_out,
			    XtNargv, (XtArgVal) argv_in_out,
			    XtVaNestedList, (XtVarArgsList) typed_args,
			    NULL);
    free ( (XtPointer) typed_args );
#else
    if ( ( args = (ArgList) malloc (4 * sizeof *args) ) == NULL )
    {
	m_abort (function_name, "initial ArgList");
    }
    XtSetArg (args[0], XtNscreen, (XtArgVal) screen);
    XtSetArg (args[1], XtNcolormap, xcmap);
    XtSetArg (args[2], XtNargc, (XtArgVal) *argc_in_out);
    XtSetArg (args[3], XtNargv, (XtArgVal) argv_in_out);
    count = 4;
    for ( attr = va_arg (argp, String); attr != NULL;
	  attr = va_arg (argp, String) )
    {
	XtSetArg ( args[count], attr, va_arg (argp, XtArgVal) );
	count++;
	if ( ( args = (ArgList) realloc ( (char *) args, 
					  (count + 1) * sizeof *args ) )
	     == NULL )
	{
	    m_abort (function_name, "ArgList");
	}
    }
    w = XtAppCreateShell (NULL, application_class, 
			  applicationShellWidgetClass, dpy, args, count);
    free ( (XtPointer) args );
#endif
    return (w);
}   /*  End Function xtmisc_init_app_initialise  */
