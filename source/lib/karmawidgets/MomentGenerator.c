/*LINTLIBRARY*/
/*  MomentGenerator.c

    This code provides a moment generator control widget for Xt.

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

/*  This file contains all routines needed for a moment generator control
  widget for Xt.


    Written by      Richard Gooch   11-SEP-1996

    Updated by      Richard Gooch   14-SEP-1996

    Updated by      Richard Gooch   19-SEP-1996: Changed to special function
  call interface for setting new array.

    Updated by      Richard Gooch   26-SEP-1996: Changed to implement the new
  FITS-style co-ordinate handling, where dimension co-ordinates range from 0
  to length - 1.

    Updated by      Richard Gooch   30-SEP-1996: Fixed bug in initialisation
  where cube_arr and cube_ap were not initialised.

    Last updated by Richard Gooch   14-OCT-1996: Copy "OBSRA" and "OBSDEC" from
  cube to moment maps.


*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/wait.h>
#include <signal.h>
#include <X11/Xos.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Dialog.h>
#ifndef X11
#  define X11
#endif
#include <karma.h>
#include <karma_iarray.h>
#include <karma_c.h>
#include <karma_xtmisc.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_m.h>
#include <karma_a.h>
#include <Xkw/MomentGeneratorP.h>
#include <Xkw/Ktoggle.h>
#include <Xkw/ExclusiveMenu.h>


#define MOM1_ALGORITHM_WEIGHTED_MEAN    0
#define MOM1_ALGORITHM_MEDIAN           1
#define NUM_MOM1_ALGORITHM_ALTERNATIVES 2

static char *mom1_algorithm_alternatives[] =
{
    "weighted mean",
    "median",
};

static char *keywords[] =
{
    "OBJECT",
    "DATE-OBS",
    "OBSRA",
    "OBSDEC",
    "EPOCH",
    "BUNIT",
    "TELESCOP",
    "INSTRUME",
    "EQUINOX",
    "BTYPE",
    "BMAJ",
    "BMIN",
    "BPA",
    "RESTFREQ",
    "VELREF",
    "PBFWHM",
    NULL
};


/*  Private functions  */

STATIC_FUNCTION (void MomentGenerator__Initialise,
		 (Widget request, Widget new) );
STATIC_FUNCTION (Boolean MomentGenerator__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void iarr_destroy_callback,
		 (iarray arr, MomentGeneratorWidget top) );
STATIC_FUNCTION (void apply_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (flag compute_moments,
		 (iarray mom0, iarray mom1, iarray cube, KwcsAstro cube_ap,
		  float lower_clip, float upper_clip,float sum_clip,
		  unsigned int mom1_algorithm,
		  float *mom0_min, float *mom0_max) );
STATIC_FUNCTION (flag copy_header_info, (iarray out, iarray in) );


/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define offset(field) XtOffset(MomentGeneratorWidget, momentGenerator.field)
#define XkwRDouble "Double"

static XtResource MomentGeneratorResources[] = 
{
    {XkwNverbose, XkwCVerbose, XtRBool, sizeof (Bool),
     offset (verbose), XtRImmediate, FALSE},
    {XkwNmom0Array, XkwCArray, XtRPointer, sizeof (XtPointer),
     offset (mom0Array), XtRImmediate, NULL},
    {XkwNmom1Array, XkwCArray, XtRPointer, sizeof (XtPointer),
     offset (mom1Array), XtRImmediate, NULL},
    {XkwNmomentCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     offset (momentCallback), XtRCallback, (caddr_t) NULL},
};

#undef offset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

MomentGeneratorClassRec momentGeneratorClassRec = 
{
  {
      /* CoreClassPart */
    (WidgetClass)&topLevelShellClassRec,          /* superclass */
    "MomentGenerator",                            /* class_name */
    sizeof (MomentGeneratorRec),                  /* widget_size */
    NULL,                                         /* class_initialise */
    NULL,                                         /* class_part_initialise */
    FALSE,                                        /* class_init */
    (XtInitProc) MomentGenerator__Initialise,     /* initialise */
    NULL,                                         /* initialise_hook */
    XtInheritRealize,                             /* realise */
    NULL,                                         /* actions */
    0,                                            /* num_actions */
    MomentGeneratorResources,                     /* resources */
    XtNumber (MomentGeneratorResources),          /* num_resources */
    NULLQUARK,                                    /* xrm_class */
    TRUE,                                         /* compress_motion */
    TRUE,                                         /* compress_exposure */
    TRUE,                                         /* compress_enterleave */
    TRUE,                                         /* visible_interest */
    NULL,                                         /* destroy */
    XtInheritResize,                              /* resize */
    NULL,                                         /* expose */
    (XtSetValuesFunc) MomentGenerator__SetValues, /* set_values */
    NULL,                                         /* set_values_hook */
    XtInheritSetValuesAlmost,                     /* set_values_almost */
    NULL,                                         /* get_values_hook */
    NULL,                                         /* accept_focus */
    XtVersion,                                    /* version */
    NULL,                                         /* callback_private */
    NULL,                                         /* tm_translations */
    NULL,
    NULL,
    NULL,
  },
  {     /* CompositeClassPart */
      /* geometry_manager */	XtInheritGeometryManager,
      /* change_managed	  */	XtInheritChangeManaged,
      /* insert_child	  */	XtInheritInsertChild,
      /* delete_child	  */	XtInheritDeleteChild,
      /* extension	  */	NULL
  },
  { /* Shell */
      /* extension	  */	NULL
  },
  { /* WMShell */
      /* extension	  */	NULL
  },
  { /* VendorShell */
      /* extension	  */	NULL
  },
  { /* TopLevelShell */
      /* extension	  */	NULL
  },
  {  /* MomentGeneratorClassPart */
      0                   /* empty */
  }
};

WidgetClass momentGeneratorWidgetClass = (WidgetClass)&momentGeneratorClassRec;


static void MomentGenerator__Initialise (Widget Request, Widget New)
{
    /*MomentGeneratorWidget request = (MomentGeneratorWidget) Request;*/
    MomentGeneratorWidget new = (MomentGeneratorWidget) New;
    Widget form, close_btn, w;
    /*static char function_name[] = "MomentGeneratorWidget::Initialise";*/

    new->momentGenerator.cube_arr = NULL;
    new->momentGenerator.cube_ap = NULL;
    new->momentGenerator.iarr_destroy_callback = NULL;
    new->momentGenerator.mom1_algorithm = MOM1_ALGORITHM_WEIGHTED_MEAN;
    form = XtVaCreateManagedWidget ("form", formWidgetClass, New,
				    XtNborderWidth, 0,
				    NULL);
    w = XtVaCreateManagedWidget ("closeButton", commandWidgetClass, form,
				 XtNlabel, "Close",
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 NULL);
    close_btn = w;
    XtAddCallback (close_btn, XtNcallback, xtmisc_popdown_cbk, New);
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				   XtNlabel, "Apply Parameters",
				   XtNfromHoriz, w,
				   NULL);
    XtAddCallback (w, XtNcallback, apply_cbk, New);
    w = XtVaCreateManagedWidget ("menuButton", exclusiveMenuWidgetClass, form,
				 XtNmenuName, "mom1AlgorithmMenu",
				 XtNfromVert, w,
				 XkwNchoiceName, "1st Moment Algorithm",
				 XkwNnumItems, NUM_MOM1_ALGORITHM_ALTERNATIVES,
				 XkwNitemStrings, mom1_algorithm_alternatives,
				 XkwNvaluePtr,
				 &new->momentGenerator.mom1_algorithm,
				 NULL);
    w = XtVaCreateManagedWidget ("minLabel", labelWidgetClass, form,
				 XtNfromVert, w,
				 XtNborderWidth, 0,
				 XtNlabel, "Cube min:",
				 XtNresizable, TRUE,
				 NULL);
    new->momentGenerator.cube_min_label = w;
    w = XtVaCreateManagedWidget ("maxLabel", labelWidgetClass, form,
				 XtNfromVert, w,
				 XtNborderWidth, 0,
				 XtNlabel, "Cube max:",
				 XtNresizable, TRUE,
				 NULL);
    new->momentGenerator.cube_max_label = w;
    w = XtVaCreateManagedWidget ("minLabel", labelWidgetClass, form,
				 XtNfromVert, w,
				 XtNborderWidth, 0,
				 XtNlabel, "0th moment min:",
				 XtNresizable, TRUE,
				 NULL);
    new->momentGenerator.sum_min_label = w;
    w = XtVaCreateManagedWidget ("maxLabel", labelWidgetClass, form,
				 XtNfromVert, w,
				 XtNborderWidth, 0,
				 XtNlabel, "0th moment max:",
				 XtNresizable, TRUE,
				 NULL);
    new->momentGenerator.sum_max_label = w;
    w = XtVaCreateManagedWidget ("lowerClipLevel", dialogWidgetClass, form,
				 XtNlabel, "Lower Clip Level",
				 XtNfromVert, w,
				 XtNvalue, "5e-3",
				 NULL);
    new->momentGenerator.lower_clip_dlg = w;
    w = XtVaCreateManagedWidget ("upperClipLevel", dialogWidgetClass, form,
				 XtNlabel, "Upper Clip Level",
				 XtNfromVert,
				 new->momentGenerator.sum_max_label,
				 XtNfromHoriz,
				 new->momentGenerator.lower_clip_dlg,
				 XtNvalue, "0.5",
				 NULL);
    new->momentGenerator.upper_clip_dlg = w;
    w = XtVaCreateManagedWidget ("sumClipLevel", dialogWidgetClass, form,
				 XtNlabel, "Sum Clip Level",
				 XtNfromVert, w,
				 XtNvalue, "5e-2",
				 NULL);
    new->momentGenerator.sum_clip_dlg = w;
}   /*  End Function Initialise  */

static Boolean MomentGenerator__SetValues (Widget Current, Widget Request,
					   Widget New)
{
    /*static char function_name[] = "MomentGeneratorWidget::SetValues";*/

    return False;
}   /*  End Function SetValues  */

static void iarr_destroy_callback (iarray arr, MomentGeneratorWidget top)
/*  [PURPOSE] This routine will register the destruction of the Intelligent
    Array.
    <arr> The Intelligent Array.
    <top> The MomentGenerator widget.
    [RETURNS] Nothing.
*/
{
    top->momentGenerator.cube_arr = NULL;
    top->momentGenerator.iarr_destroy_callback = NULL;
    iarray_dealloc (top->momentGenerator.mom0Array);
    iarray_dealloc (top->momentGenerator.mom1Array);
    top->momentGenerator.mom0Array = NULL;
    top->momentGenerator.mom1Array = NULL;
}   /*  End Function iarr_destroy_callback  */

static void apply_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Moment computation callback.
    <w> The widget the event occurred on.
    <client_data> Client data. The MomentGeneratorWidget.
    <call_data> Call data.
    [RETURNS] Nothing.
*/
{
    float lower_clip, upper_clip, sum_clip;
    MomentGeneratorWidget top = (MomentGeneratorWidget) client_data;
    float mom0_min, mom0_max;
    char *clip_str, *p;
    char txt[STRING_LENGTH];
    /*static char function_name[] = "MomentGeneratorWidget::apply_cbk";*/

    if (top->momentGenerator.cube_arr == NULL) return;
    XtVaGetValues (top->momentGenerator.lower_clip_dlg,
		   XtNvalue, &clip_str,
		   NULL);
    lower_clip = ex_float (clip_str, &p);
    XtVaGetValues (top->momentGenerator.upper_clip_dlg,
		   XtNvalue, &clip_str,
		   NULL);
    upper_clip = ex_float (clip_str, &p);
    XtVaGetValues (top->momentGenerator.sum_clip_dlg,
		   XtNvalue, &clip_str,
		   NULL);
    sum_clip = ex_float (clip_str, &p);
    fprintf (stderr, "Computing moment maps...");
    compute_moments (top->momentGenerator.mom0Array,
		     top->momentGenerator.mom1Array,
		     top->momentGenerator.cube_arr,
		     top->momentGenerator.cube_ap, lower_clip, upper_clip,
		     sum_clip, top->momentGenerator.mom1_algorithm,
		     &mom0_min, &mom0_max);
    fprintf (stderr, "done\n");
    sprintf (txt, "0th moment min: %e", mom0_min);
    XtVaSetValues (top->momentGenerator.sum_min_label,
		   XtNlabel, txt,
		   NULL);
    sprintf (txt, "0th moment max: %e", mom0_max);
    XtVaSetValues (top->momentGenerator.sum_max_label,
		   XtNlabel, txt,
		   NULL);
    XtCallCallbacks ( (Widget) top, XkwNmomentCallback, NULL );
}   /*  End Function apply_cbk  */

static flag compute_moments (iarray mom0, iarray mom1, iarray cube,
			     KwcsAstro cube_ap,
			     float lower_clip, float upper_clip,float sum_clip,
			     unsigned int mom1_algorithm,
			     float *mom0_min, float *mom0_max)
/*  [SUMMARY] Compute the 0th and 1st moments along the Z axis of a cube.
    <mom0> The 0th moment array.
    <mom1> The 1st moment array.
    <cube> The cube.
    <cube_ap> The cube KwcsAstro object.
    <lower_clip> Values in the cube lower than this value are not used in the
    computation of the moments.
    <upper_clip> Values in the cube higher than this value are not used in the
    computation of the moment.
    <sum_clip> Values in the 0th moment map lower than this value are not used
    in the computation of the 1st moment.
    <mom1_algorithm> The 1st moment algorithm.
    <mom0_min> The minimum value in the 0th moment image is written here.
    <mom0_max> The maximum value in the 0th moment image is written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int x, y, z, xlen, ylen, zlen;
    float val, weighted_sum, sum, half_mom0_val, index;
    float mom0_val = 0;  /*  Initialised to keep compiler happy  */
    double velocity;
    /*char txt[STRING_LENGTH];*/
    /*extern char module_name[STRING_LENGTH + 1];*/
    /*static char function_name[] = "MomentGeneratorWidget::compute_moments";*/

    xlen = iarray_dim_length (cube, 2);
    ylen = iarray_dim_length (cube, 1);
    zlen = iarray_dim_length (cube, 0);
    *mom0_min = TOOBIG;
    *mom0_max = -TOOBIG;
#ifdef dummy
    /*  Append a bit of history  */
    sprintf (txt, "%s: 1st moment map  low clip %e  high clip %e",
	     module_name, lower_clip, upper_clip);
    iarray_append_history_string (mom1, txt, TRUE);
    sprintf (txt, "%s: sum_clip: %e  algorithm: %s",
	     module_name, sum_clip,
	     mom1_algorithm_alternatives[mom1_algorithm]);
    iarray_append_history_string (mom1, txt, TRUE);
#endif
    /*  Loop through image pixels  */
    for (y = 0; y < ylen; ++y) for (x = 0; x < xlen; ++x)
    {
	switch (mom1_algorithm)
	{
	  case MOM1_ALGORITHM_WEIGHTED_MEAN:
	    mom0_val = 0.0;
	    weighted_sum = 0.0;
	    for (z = 0; z < zlen; ++z)
	    {
		if ( ( val = F3 (cube, z, y, x) ) >= TOOBIG ) continue;
		if (val > upper_clip) continue;
		if (val < lower_clip) continue;
		mom0_val += val;
		weighted_sum += val * (float) z;
	    }
	    if (mom0_val < lower_clip)
	    {
		F2 (mom0, y, x) = TOOBIG;
		F2 (mom1, y, x) = TOOBIG;
	    }
	    else
	    {
		F2 (mom0, y, x) = mom0_val;
		if (mom0_val < sum_clip) F2 (mom1, y, x) = TOOBIG;
		else
		{
		    z = (weighted_sum / mom0_val);
		    if (z < 0) z = 0;
		    else if (z > zlen - 1) z = zlen - 1;
		    if (cube_ap == NULL)
		    {
			F2 (mom1, y, x) = iarray_get_coordinate (cube, 0, z);
		    }
		    else
		    {
			velocity = z;
			wcs_astro_transform (cube_ap, 1,
					     NULL, FALSE, NULL, FALSE,
					     &velocity, FALSE,
					     0, NULL, NULL);
			F2 (mom1, y, x) = velocity;
		    }
		}
	    }
	    break;
	  case MOM1_ALGORITHM_MEDIAN:
	    /*  First compute 0th moment  */
	    mom0_val = 0.0;
	    for (z = 0; z < zlen; ++z)
	    {
		if ( ( val = F3 (cube, z, y, x) ) >= TOOBIG ) continue;
		if (val > upper_clip) continue;
		if (val < lower_clip) continue;
		mom0_val += val;
	    }
	    if (mom0_val < lower_clip)
	    {
		F2 (mom0, y, x) = TOOBIG;
		F2 (mom1, y, x) = TOOBIG;
		continue;
	    }
	    else F2 (mom0, y, x) = mom0_val;
	    /*  Now compute 1st moment  */
	    if (mom0_val < sum_clip)
	    {
		F2 (mom1, y, x) = TOOBIG;
		continue;
	    }
	    half_mom0_val = mom0_val * 0.5;
	    sum = 0.0;
	    index = -1.0;
	    for (z = 0; z < zlen; ++z)
	    {
		if ( ( val = F3 (cube, z, y, x) ) >= TOOBIG ) continue;
		if (val > upper_clip) continue;
		if (val < lower_clip) continue;
		sum += val;
		if (sum >= half_mom0_val)
		{
		    index = (float) z + (half_mom0_val - sum + val) /val - 0.5;
		    if (cube_ap == NULL)
		    {
			F2 (mom1, y, x) = iarray_get_coordinate (cube, 0,
								 index);
		    }
		    else
		    {
			velocity = index;
			wcs_astro_transform (cube_ap, 1,
					     NULL, FALSE, NULL, FALSE,
					     &velocity, FALSE,
					     0, NULL, NULL);
			F2 (mom1, y, x) = velocity;
		    }
		    break;
		}
	    }
	    if (index < 0.0) F2 (mom1, y, x) = TOOBIG;
	    break;
	  default:
	    break;
	}
	if (mom0_val < *mom0_min) *mom0_min = mom0_val;
	if (mom0_val > *mom0_max) *mom0_max = mom0_val;
    }
    return (TRUE);
}   /*  End Function compute_moments  */


/*  Public functions follow  */

void XkwMomentGeneratorNewArray (Widget W, iarray array, double min,double max)
/*  [SUMMARY] Register new array.
    <W> The MomentGenerator widget.
    <array> The new array. This may be NULL.
    <min> The minimum data value in the array.
    <max> The maximum data value in the array. If this is less than <<min>>
    then the minimum and maximum values are computed.
    [RETURNS] Nothing.
*/
{
    iarray mom0_arr, mom1_arr;
    unsigned int count;
    unsigned int axis_num;
    MomentGeneratorWidget w = (MomentGeneratorWidget) W;
    double crval[2], crpix[2], cdelt[2];
    CONST char *elem_name;
    CONST char *z_name;
    unsigned long dim_lengths[2];
    CONST char *dim_names[2];
    char txt[STRING_LENGTH];
    extern char module_name[STRING_LENGTH + 1];
    extern char module_version_date[STRING_LENGTH + 1];
    extern char module_lib_version[STRING_LENGTH + 1];
    extern char karma_library_version[STRING_LENGTH + 1];
    static char function_name[] = "XkwMomentGeneratorNewArray";

    /*  Clean up and old arrays  */
    if (w->momentGenerator.iarr_destroy_callback != NULL)
    {
	c_unregister_callback (w->momentGenerator.iarr_destroy_callback);
	w->momentGenerator.iarr_destroy_callback = NULL;
    }
    if (w->momentGenerator.cube_ap != NULL)
    {
	wcs_astro_destroy (w->momentGenerator.cube_ap);
    }
    if (w->momentGenerator.mom0Array != NULL)
    {
	iarray_dealloc (w->momentGenerator.mom0Array);
    }
    w->momentGenerator.mom0Array = NULL;
    if (w->momentGenerator.mom1Array != NULL)
    {
	iarray_dealloc (w->momentGenerator.mom1Array);
    }
    w->momentGenerator.mom1Array = NULL;
    /*  Process new cube  */
    if (array == NULL) return;
    w->momentGenerator.cube_arr = array;
    w->momentGenerator.cube_ap = wcs_astro_setup (array->top_pack_desc,
						  *array->top_packet);
    z_name = iarray_dim_name (w->momentGenerator.cube_arr, 0);
    dim_lengths[0] = iarray_dim_length (w->momentGenerator.cube_arr, 1);
    dim_lengths[1] = iarray_dim_length (w->momentGenerator.cube_arr, 2);
    dim_names[0] = iarray_dim_name (w->momentGenerator.cube_arr, 1);
    dim_names[1] = iarray_dim_name (w->momentGenerator.cube_arr, 2);
    /*  Create the 0th moment map array  */
    /*  The value name should be the same as the cube value name  */
    elem_name = iarray_value_name (w->momentGenerator.cube_arr);
    if ( ( mom0_arr = iarray_create (K_FLOAT, 2, dim_names, dim_lengths,
				     elem_name, NULL) )
	 == NULL ) m_abort (function_name, "0th moment array");
    /*  Copy over FITS co-ordinate information for the RA and DEC axes  */
    for (count = 0; count < 2; ++count)
    {
	if ( ( axis_num =
	       iarray_get_fits_axis (w->momentGenerator.cube_arr,
				     count + 1) ) == 0 )
	    continue;
	/*  Set the "CTYPEn" keyword with the dimension name so that the <wcs>
	    package can deal with it  */
	sprintf (txt, "CTYPE%u", 2 - count);
	iarray_put_named_string ( mom0_arr, txt,
				  iarray_dim_name (w->momentGenerator.cube_arr,
						   count + 1) );
	sprintf (txt, "CRVAL%u", axis_num);
	if ( iarray_get_named_value (w->momentGenerator.cube_arr, txt,
				     NULL, crval) )
	{
	    sprintf (txt, "CRVAL%u", 2 - count);
	    iarray_put_named_value (mom0_arr, txt, K_DOUBLE, crval);
	}
	sprintf (txt, "CRPIX%u", axis_num);
	if ( iarray_get_named_value (w->momentGenerator.cube_arr, txt,
				     NULL, crpix) )
	{
	    sprintf (txt, "CRPIX%u", 2 - count);
	    iarray_put_named_value (mom0_arr, txt, K_DOUBLE, crpix);
	}
	sprintf (txt, "CDELT%u", axis_num);
	if ( iarray_get_named_value (w->momentGenerator.cube_arr, txt,
				     NULL, cdelt) )
	{
	    sprintf (txt, "CDELT%u", 2 - count);
	    iarray_put_named_value (mom0_arr, txt, K_DOUBLE, cdelt);
	}
    }
    /*  Copy over some other header information  */
    copy_header_info (mom0_arr, w->momentGenerator.cube_arr);
    /*  Append a bit of history  */
    sprintf (txt, "%s: Module version %s  Karma v%s  compiled with v%s",
	     module_name, module_version_date,
	     karma_library_version, module_lib_version);
    iarray_append_history_string (mom0_arr, txt, TRUE);
    sprintf (txt, "%s: 0th moment map", module_name);
    iarray_append_history_string (mom0_arr, txt, TRUE);
    /*  Create the 1st moment array  */
    /*  The value name should reflect the z-axis name  */
    if (strncmp (z_name, "FREQ", 4) == 0) elem_name = "HZ";
    else if ( (strncmp (z_name, "VELO", 4) == 0) ||
	      (strncmp (z_name, "FELO", 4) == 0) )
    {
	elem_name = "M/S";
    }
    else elem_name = z_name;
    if ( ( mom1_arr = iarray_create (K_FLOAT, 2, dim_names, dim_lengths,
				     elem_name, NULL) )
	 == NULL ) m_abort (function_name, "1st moment array");
    /*  Copy over FITS co-ordinate information for the RA and DEC axes  */
    for (count = 0; count < 2; ++count)
    {
	if ( ( axis_num =
	       iarray_get_fits_axis (w->momentGenerator.cube_arr,
				     count + 1) ) == 0 )
	    continue;
	/*  Set the "CTYPEn" keyword with the dimension name so that the <wcs>
	    package can deal with it  */
	sprintf (txt, "CTYPE%u", 2 - count);
	iarray_put_named_string ( mom1_arr, txt,
				  iarray_dim_name (w->momentGenerator.cube_arr,
						   count + 1) );
	sprintf (txt, "CRVAL%u", axis_num);
	if ( iarray_get_named_value (w->momentGenerator.cube_arr, txt,
				     NULL, crval) )
	{
	    sprintf (txt, "CRVAL%u", 2 - count);
	    iarray_put_named_value (mom1_arr, txt, K_DOUBLE, crval);
	}
	sprintf (txt, "CRPIX%u", axis_num);
	if ( iarray_get_named_value (w->momentGenerator.cube_arr, txt,
				     NULL, crpix) )
	{
	    sprintf (txt, "CRPIX%u", 2 - count);
	    iarray_put_named_value (mom1_arr, txt, K_DOUBLE, crpix);
	}
	sprintf (txt, "CDELT%u", axis_num);
	if ( iarray_get_named_value (w->momentGenerator.cube_arr, txt,
				     NULL, cdelt) )
	{
	    sprintf (txt, "CDELT%u", 2 - count);
	    iarray_put_named_value (mom1_arr, txt, K_DOUBLE, cdelt);
	}
    }
    /*  Copy over some other header information  */
    copy_header_info (mom1_arr, w->momentGenerator.cube_arr);
    /*  Append a bit of history  */
    sprintf (txt, "%s: Module version %s  Karma v%s  compiled with v%s",
	     module_name, module_version_date,
	     karma_library_version, module_lib_version);
    iarray_append_history_string (mom1_arr, txt, TRUE);
    if (max < min)
    {
	/*  Compute the minimum and maximum  */
	iarray_min_max (w->momentGenerator.cube_arr, CONV_CtoR_REAL,
			&min, &max);
    }
    sprintf (txt, "Cube min: %e", min);
    XtVaSetValues (w->momentGenerator.cube_min_label,
		   XtNlabel, txt,
		   NULL);
    sprintf (txt, "Cube max: %e", max);
    XtVaSetValues (w->momentGenerator.cube_max_label,
		   XtNlabel, txt,
		   NULL);
    w->momentGenerator.mom0Array = mom0_arr;
    w->momentGenerator.mom1Array = mom1_arr;
    w->momentGenerator.iarr_destroy_callback = 
	iarray_register_destroy_func (w->momentGenerator.cube_arr,
				      ( flag (*) () )iarr_destroy_callback,
				      w);
}   /*  End Function XkwMomentGeneratorNewArray  */

static flag copy_header_info (iarray out, iarray in)
/*  [SUMMARY] Copy some header information from one array to another.
    <out> The output array.
    <in> The input array.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int count;
    static char function_name[] = "copy_header_info";

    for (count = 0; keywords[count] != NULL; ++count)
    {
	if ( !iarray_copy_named_element (out, in, keywords[count],
					 FALSE, FALSE, TRUE) )
	{
	    fprintf (stderr, "%s: Failed to copy header keyword %s\n",
		     function_name, keywords[count]);
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function copy_header_info  */
