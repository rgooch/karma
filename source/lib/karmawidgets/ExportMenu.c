/*LINTLIBRARY*/
/*  ExportMenu.c

    This code provides a simple export menu widget for Xt.

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

/*  This file contains all routines needed for a simple export menu widget for
  Xt.


    Written by      Richard Gooch   1-OCT-1996

    Updated by      Richard Gooch   22-OCT-1996: Set dialog widget label and
  increased dialog widget size.

    Last updated by Richard Gooch   27-NOV-1996: Fixed bug where PseudoColour
  colourmap was being used when writing TrueColour images in PPM or Sun
  Rasterfile formats.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <Xkw/ExportMenuP.h>
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
#include <karma_viewimg.h>
#include <karma_foreign.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>
#include <Xkw/ChoiceMenu.h>
#include <Xkw/Dialogpopup.h>
#include <Xkw/Postscript.h>


/* Methods*/

STATIC_FUNCTION (void Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (Boolean SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void export_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );
STATIC_FUNCTION (void save_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );
STATIC_FUNCTION (void postscript_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (multi_array *create_sub,
		 (multi_array *old, flag *allocated,
		  array_desc *arr_desc, unsigned int hdim,
		  unsigned int vdim, KWorldCanvas canvas) );
STATIC_FUNCTION (array_desc *find_array,
		 (CONST multi_array *multi_desc, CONST char *name,
		  unsigned int *index) );
STATIC_FUNCTION (flag copy_plane,
		 (CONST char *inp_data, CONST array_desc *inp_arr_desc,
		  unsigned int hdim, unsigned int vdim,
		  uaddr hstart, uaddr vstart,
		  char *out_data, CONST array_desc *out_arr_desc) );


#define offset(field) XtOffset(ExportMenuWidget, exportMenu.field)

static XtResource ExportMenuResources[] = 
{
    {XkwNiarray, XkwCIarray, XtRPointer, sizeof (XtPointer),
     offset (array), XtRImmediate, NULL},
    {XkwNworldCanvas, XkwCWorldCanvas, XtRPointer, sizeof (XtPointer),
     offset (worldCanvas), XtRImmediate, NULL},
};

#undef offset

#define EXPORT_POSTSCRIPT    0
#define EXPORT_SUNRAS        1
#define EXPORT_PPM           2
#define EXPORT_WHOLE_KARMA   3
#define EXPORT_WHOLE_FITS    4
#define EXPORT_WHOLE_MIRIAD  5
#define EXPORT_WHOLE_GIPSY   6
#define EXPORT_SUB_KARMA     7
#define EXPORT_SUB_FITS      8
#define EXPORT_SUB_MIRIAD    9
#define EXPORT_SUB_GIPSY     10
#define NUM_EXPORT_CHOICES   11

static char *export_choices[NUM_EXPORT_CHOICES] =
{
    "PostScript", "SunRasterfile", "PortablePixelMap", "Karma (whole dataset)",
    "FITS (whole dataset)", "Miriad (whole dataset)", "GIPSY (whole dataset)",
    "Karma (subset)", "FITS (subset)", "Miriad (subset)", "GIPSY (subset)",
};

ExportMenuClassRec exportMenuClassRec = 
{
  {        /* CoreClassPart */
      (WidgetClass) &formClassRec,         /* superclass */
      "ExportMenu",                        /* class_name */
      sizeof(ExportMenuRec),               /* widget_size */
      NULL,                                /* class_initialise */
      NULL,                                /* class_part_initialise */
      FALSE,                               /* class_init */
      (XtInitProc)Initialise,              /* initialise */
      NULL,                                /* initialise_hook */
      XtInheritRealize,                    /* realise */
      NULL,                                /* actions */
      0,                                   /* num_actions */
      ExportMenuResources,                 /* resources */
      XtNumber(ExportMenuResources),       /* num_resources */
      NULLQUARK,                           /* xrm_class */
      TRUE,                                /* compress_motion */
      TRUE,                                /* compress_exposure */
      TRUE,                                /* compress_enterleave */
      TRUE,                                /* visible_interest */
      NULL,                                /* destroy */
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
  {     /* CompositeClassPart */
    /* geometry_manager	  */	XtInheritGeometryManager,
    /* change_managed	  */	XtInheritChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */	NULL
  },{ /* Constraint */
    /* subresourses       */   NULL,
    /* subresource_count  */   0,
    /* constraint_size    */   sizeof (ExportMenuConstraintsRec),
    /* initialise         */   NULL,
    /* destroy            */   NULL,
    /* set_values         */   NULL,
    /* extension          */   NULL
  },{ /* Form */
    /* layout */                XtInheritLayout
  },
  {  /* ExportMenuClassPart */
    0 /* empty */
  },
};

WidgetClass exportMenuWidgetClass = (WidgetClass) &exportMenuClassRec;

static void Initialise (Widget Request, Widget New)
{
    /*ExportMenuWidget request = (ExportMenuWidget) Request;*/
    ExportMenuWidget new = (ExportMenuWidget) New;
    Widget w;
    /*static char function_name[] = "ExportMenu::Initialise";*/

    new->form.default_spacing = 0;
    new->core.border_width = 0;
    w = XtVaCreateManagedWidget ("menuButton", choiceMenuWidgetClass, New,
				 XtNlabel, "Export",
				 XkwNmenuTitle, "Export Menu",
				 XtNmenuName, "theMenu",
				 XkwNnumItems, NUM_EXPORT_CHOICES,
				 XkwNitemStrings, export_choices,
				 NULL);
    XtAddCallback (w, XkwNselectCallback, export_cbk, New);
    w = XtVaCreatePopupShell ("savePopup", dialogpopupWidgetClass, New,
			      XtNlabel, "Output basename         ",
			      NULL);
    new->exportMenu.save_dialog_popup = w;
    XtAddCallback (w, XtNcallback, save_cbk, New);
    w = XtVaCreatePopupShell ("postscriptwinpopup", postscriptWidgetClass, New,
			      XtNtitle, "Postscript Window",
			      XkwNautoIncrement, TRUE,
			      NULL);
    new->exportMenu.pswinpopup = w;
    XtAddCallback (w, XtNcallback, postscript_cbk, New);
}   /*  End Function Initialise  */

static Boolean SetValues (Widget Current, Widget Request, Widget New)
{
    /*ExportMenuWidget current = (ExportMenuWidget) Current;
      ExportMenuWidget request = (ExportMenuWidget) Request;
      ExportMenuWidget new = (ExportMenuWidget) New;*/

    return True;
}   /*  End Function SetValues  */

static void export_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the export menu callback.
*/
{
    ViewableImage vimage;
    unsigned int exportcode = *(int *) call_data;
    ExportMenuWidget top = (ExportMenuWidget) client_data;
    static char function_name[] = "ExportMenuWidget::export_cbk";

    if (exportcode == EXPORT_POSTSCRIPT)
    {
	XtPopup (top->exportMenu.pswinpopup, XtGrabNone);
	return;
    }
    if (top->exportMenu.worldCanvas == NULL)
    {
	fprintf (stderr, "No visible canvas!\n");
	return;
    }
    if ( ( vimage = viewimg_get_active (top->exportMenu.worldCanvas) )
	== NULL )
    {
	fprintf (stderr, "No visible image!\n");
	return;
    }
    top->exportMenu.export_type = exportcode;
    switch (top->exportMenu.export_type)
    {
      case EXPORT_PPM:
	XtVaSetValues (top->exportMenu.save_dialog_popup,
		       XkwNextension, ".ppm",
		       NULL);
	break;
      case EXPORT_SUNRAS:
	XtVaSetValues (top->exportMenu.save_dialog_popup,
		       XkwNextension, ".ras",
		       NULL);
	break;
      case EXPORT_WHOLE_KARMA:
      case EXPORT_SUB_KARMA:
	XtVaSetValues (top->exportMenu.save_dialog_popup,
		       XkwNextension, ".kf",
		       NULL);
	break;
      case EXPORT_WHOLE_FITS:
      case EXPORT_SUB_FITS:
	XtVaSetValues (top->exportMenu.save_dialog_popup,
		       XkwNextension, ".fits",
		       NULL);
	break;
      case EXPORT_WHOLE_MIRIAD:
      case EXPORT_SUB_MIRIAD:
	XtVaSetValues (top->exportMenu.save_dialog_popup,
		       XkwNextension, "",
		       NULL);
	break;
      case EXPORT_WHOLE_GIPSY:
      case EXPORT_SUB_GIPSY:
	XtVaSetValues (top->exportMenu.save_dialog_popup,
		       XkwNextension, ".image",
		       NULL);
	break;
      default:
	fprintf (stderr, "Illegal export code: %u\n", exportcode);
	a_prog_bug (function_name);
	break;
    }
    XtPopup (top->exportMenu.save_dialog_popup, XtGrabNone);
}   /*  End Function export_cbk   */

static void save_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the save callback.
*/
{
    flag dealloc;
    flag ok;
    ViewableImage vimage;
    Kcolourmap cmap = NULL;  /*  Initialised to keep compiler happy  */
    Channel channel;
    flag truecolour;
    unsigned int hdim, vdim;
    unsigned int pseudo_index, red_index, green_index, blue_index;
    unsigned int cmap_size, visual, count;
    ExportMenuWidget top = (ExportMenuWidget) client_data;
    double i_min, i_max;
    array_desc *arr_desc;
    multi_array *multi_desc;
    CONST unsigned char *image;
    unsigned short *intensities = NULL;
    unsigned short *cmap_red, *cmap_green, *cmap_blue;
    unsigned short tmp_cmap[256 * 3];
    String fname = (String) call_data;
    static char function_name[] = "ExportMenuWidget::save_cbk";

    if (top->exportMenu.worldCanvas == NULL)
    {
	fprintf (stderr, "No visible canvas!\n");
	return;
    }
    if ( ( vimage = viewimg_get_active (top->exportMenu.worldCanvas) )
	 == NULL )
    {
	fprintf (stderr, "No visible image!\n");
	return;
    }
    viewimg_get_attributes (vimage,
			    VIEWIMG_VATT_TRUECOLOUR, &truecolour,
			    VIEWIMG_VATT_ARRAY_DESC, &arr_desc,
			    VIEWIMG_VATT_SLICE, &image,
			    VIEWIMG_VATT_HDIM, &hdim,
			    VIEWIMG_VATT_VDIM, &vdim,
			    VIEWIMG_VATT_MULTI_ARRAY, &multi_desc,
			    VIEWIMG_VATT_END);
    switch (top->exportMenu.export_type)
    {
      case EXPORT_SUB_KARMA:
      case EXPORT_SUB_FITS:
      case EXPORT_SUB_MIRIAD:
      case EXPORT_SUB_GIPSY:
	/*  Need to create a sub-array  */
	if ( ( multi_desc = create_sub (multi_desc, &dealloc,
					arr_desc, hdim, vdim,
					top->exportMenu.worldCanvas) )
	     == NULL )
	{
	    m_error_notify (function_name, "sub-array");
	    return;
	}
	break;
      default:
	/*  Other export types use the original data or the specific image  */
	dealloc = FALSE;
	break;
    }
    kwin_get_attributes (canvas_get_pixcanvas (top->exportMenu.worldCanvas),
			 KWIN_ATT_VISUAL, &visual,
			 KWIN_ATT_END);
    cmap = canvas_get_cmap (top->exportMenu.worldCanvas);
    if ( (cmap == NULL) ||
	 ( (visual == KWIN_VISUAL_PSEUDOCOLOUR) && truecolour ) )
    {
	cmap_red = NULL;
	cmap_green = NULL;
	cmap_blue = NULL;
    }
    else
    {
	if ( ( intensities = kcmap_get_rgb_values (cmap, &cmap_size) )
	     == NULL )
	{
	    return;
	}
	cmap_red = tmp_cmap;
	cmap_green = tmp_cmap + 1;
	cmap_blue = tmp_cmap + 2;
	if ( (visual == KWIN_VISUAL_DIRECTCOLOUR) && (cmap_size < 256) )
	{
	    /*  Set extra colours at front to first colour in colourmap  */
	    for (count = 0; count < (256 - cmap_size);
		 ++count, cmap_red += 3, cmap_green += 3, cmap_blue += 3)
	    {
		*cmap_red = intensities[0];
		*cmap_green = intensities[1];
		*cmap_blue = intensities[2];
	    }
	    /*  Copy remaining colours  */
	    for (count = 0; count < cmap_size; ++count)
	    {
		cmap_red[count * 3] = intensities[count * 3];
		cmap_green[count * 3 + 1] = intensities[count * 3 + 1];
		cmap_blue[count * 3 + 2] = intensities[count * 3 + 2];
	    }
	    cmap_red = tmp_cmap;
	    cmap_green = tmp_cmap + 1;
	    cmap_blue = tmp_cmap + 2;
	}
	else
	{
	    for (count = 0; count < cmap_size; ++count)
	    {
		cmap_red[count * 3] = intensities[count * 3];
		cmap_green[count * 3 + 1] = intensities[count * 3 + 1];
		cmap_blue[count * 3 + 2] = intensities[count * 3 + 2];
	    }
	}
	m_free ( (char *) intensities );
    }
    if (truecolour)
    {
	viewimg_get_attributes (vimage,
				VIEWIMG_VATT_RED_INDEX, &red_index,
				VIEWIMG_VATT_GREEN_INDEX, &green_index,
				VIEWIMG_VATT_BLUE_INDEX, &blue_index,
				VIEWIMG_VATT_END);
    }
    else
    {
	if (cmap == NULL)
	{
	    fprintf (stderr, "%s: no colourmap!\n", function_name);
	    return;
	}
	canvas_get_attributes (top->exportMenu.worldCanvas,
			       CANVAS_ATT_VALUE_MIN, &i_min,
			       CANVAS_ATT_VALUE_MAX, &i_max,
			       CANVAS_ATT_END);
	viewimg_get_attributes (vimage,
				VIEWIMG_VATT_PSEUDO_INDEX, &pseudo_index,
				VIEWIMG_VATT_END);
    }
    switch (top->exportMenu.export_type)
    {
      case EXPORT_PPM:
	if ( ( channel = ch_open_file (fname, "w") ) == NULL ) return;
	if (truecolour)
	{
	    ok = foreign_ppm_write_rgb
		(channel, TRUE,
		 image + ds_get_element_offset (arr_desc->packet, red_index),
		 image + ds_get_element_offset (arr_desc->packet,green_index),
		 image + ds_get_element_offset (arr_desc->packet, blue_index),
		 arr_desc->offsets[hdim], arr_desc->offsets[vdim],
		 arr_desc->dimensions[hdim]->length,
		 arr_desc->dimensions[vdim]->length,
		 cmap_red, cmap_green, cmap_blue, 3);
	}
	else ok = foreign_ppm_write_pseudo
		 (channel, TRUE,
		  (CONST char *) image +
		  ds_get_element_offset (arr_desc->packet,pseudo_index),
		  arr_desc->packet->element_types[pseudo_index],
		  arr_desc->offsets[hdim], arr_desc->offsets[vdim],
		  arr_desc->dimensions[hdim]->length,
		  arr_desc->dimensions[vdim]->length,
		  cmap_red, cmap_green, cmap_blue, cmap_size, 3,
		  i_min, i_max);
	if (!ok)
	{
	    ch_close (channel);
	    unlink (fname);
	    return;
	}
	ch_close (channel);
	fprintf (stderr, "Wrote PPM file: \"%s\"\n", fname);
	break;
      case EXPORT_SUNRAS:
	if ( ( channel = ch_open_file (fname, "w") ) == NULL ) return;
	if (truecolour)
	{
	    ok = foreign_sunras_write_rgb
		(channel,
		 image + ds_get_element_offset (arr_desc->packet, red_index),
		 image + ds_get_element_offset (arr_desc->packet, green_index),
		 image + ds_get_element_offset (arr_desc->packet, blue_index),
		 arr_desc->offsets[hdim], arr_desc->offsets[vdim],
		 arr_desc->dimensions[hdim]->length,
		 arr_desc->dimensions[vdim]->length,
		 cmap_red, cmap_green, cmap_blue, 3);
	}
	else ok = foreign_sunras_write_pseudo
		 (channel,
		  (CONST char *) image +
		  ds_get_element_offset (arr_desc->packet,pseudo_index),
		  arr_desc->packet->element_types[pseudo_index],
		  arr_desc->offsets[hdim], arr_desc->offsets[vdim],
		  arr_desc->dimensions[hdim]->length,
		  arr_desc->dimensions[vdim]->length,
		  cmap_red, cmap_green, cmap_blue, cmap_size, 3,
		  i_min, i_max);
	if (!ok)
	{
	    ch_close (channel);
	    unlink (fname);
	    return;
	}
	ch_close (channel);
	fprintf (stderr, "Wrote Sun Rasterfile: \"%s\"\n", fname);
	break;
      case EXPORT_WHOLE_KARMA:
      case EXPORT_SUB_KARMA:
	if (multi_desc == NULL)
	{
	    fprintf (stderr, "%s: NULL multi_array\n", function_name);
	    return;
	}
	if ( !dsxfr_put_multi (fname, multi_desc) )
	{
	    unlink (fname);
	    if (dealloc) ds_dealloc_multi (multi_desc);
	    return;
	}
	fprintf (stderr, "Wrote Karma file: \"%s\"\n", fname);
	break;
      case EXPORT_WHOLE_FITS:
      case EXPORT_SUB_FITS:
	if (multi_desc == NULL)
	{
	    fprintf (stderr, "%s: NULL multi_array\n", function_name);
	    return;
	}
	if ( ( channel = ch_open_file (fname, "w") ) == NULL )
	{
	    if (dealloc) ds_dealloc_multi (multi_desc);
	    return;
	}
	if ( !foreign_fits_write (channel, multi_desc,
				  FA_FITS_WRITE_END) )
	{
	    ch_close (channel);
	    unlink (fname);
	    if (dealloc) ds_dealloc_multi (multi_desc);
	    return;
	}
	ch_close (channel);
	fprintf (stderr, "Wrote FITS file: \"%s\"\n", fname);
	break;
      case EXPORT_WHOLE_MIRIAD:
      case EXPORT_SUB_MIRIAD:
	if (multi_desc == NULL)
	{
	    fprintf (stderr, "%s: NULL multi_array\n", function_name);
	    return;
	}
	if ( !foreign_miriad_write (fname, multi_desc,
				    FA_MIRIAD_WRITE_END) )
	{
	    if (dealloc) ds_dealloc_multi (multi_desc);
	    return;
	}
	fprintf (stderr, "Wrote Miriad file: \"%s\"\n", fname);
	break;
      case EXPORT_WHOLE_GIPSY:
      case EXPORT_SUB_GIPSY:
	if (multi_desc == NULL)
	{
	    fprintf (stderr, "%s: NULL multi_array\n", function_name);
	    return;
	}
	if ( !foreign_gipsy_write (fname, multi_desc,
				   FA_GIPSY_WRITE_END) )
	{
	    if (dealloc) ds_dealloc_multi (multi_desc);
	    return;
	}
	fprintf (stderr, "Wrote GIPSY file: \"%s\"\n", fname);
	break;
      default:
	fprintf (stderr, "Illegal export code: %u\n",
		 top->exportMenu.export_type);
	a_prog_bug (function_name);
	break;
    }
    if (dealloc) ds_dealloc_multi (multi_desc);
}   /*  End Function save_cbk   */

static void postscript_cbk (Widget w, XtPointer client_data,
			    XtPointer call_data)
/*  [SUMMARY] PostScript callback.
    [PURPOSE] This is the PostScript callback. It is called when the PostScript
    widget needs to know the pixel canvas and image name.
*/
{
    KPixCanvas pixcanvas;
    ExportMenuWidget top = (ExportMenuWidget) client_data;

    pixcanvas = canvas_get_pixcanvas (top->exportMenu.worldCanvas);
    XkwPostscriptRegisterImageAndName (w, pixcanvas, NULL);
}   /*  End Function postscript_cbk   */

static multi_array *create_sub (multi_array *old, flag *allocated,
				array_desc *inp_arr_desc, unsigned int hdim,
				unsigned int vdim, KWorldCanvas canvas)
/*  [SUMMARY] Create a sub-array.
    <old> The old multi_array descriptor.
    <allocated> TRUE is written here if a new data structure was allocated.
    <inp_arr_desc> The input array descriptor.
    <hdim> The horizontal dimension.
    <vdim> The vertical dimension.
    <canvas> The KWorldCanvas from which the subimage parameters are taken.
    [RETURNS] A new multi_array object on success, else NULL.
*/
{
    flag more_to_do;
    int count;
    uaddr inp_offset, out_offset;
    uaddr hstart, hstop, vstart, vstop;
    unsigned int num_dim, fits_axis, index;
    double left_x, right_x, bottom_y, top_y;
    dim_desc *inp_hd, *inp_vd, *out_hd, *out_vd;
    array_desc *out_arr_desc;
    multi_array *new;
    uaddr *coordinates;
    CONST char *inp_array;
    char *out_array;
    char txt[STRING_LENGTH];
    double crpix[2];
    static char function_name[] = "ExportMenuWidget::create_sub";

    canvas_get_attributes (canvas,
			   CANVAS_ATT_LEFT_X, &left_x,
			   CANVAS_ATT_RIGHT_X, &right_x,
			   CANVAS_ATT_BOTTOM_Y, &bottom_y,
			   CANVAS_ATT_TOP_Y, &top_y,
			   CANVAS_ATT_END);
    num_dim = inp_arr_desc->num_dimensions;
    inp_hd = inp_arr_desc->dimensions[hdim];
    inp_vd = inp_arr_desc->dimensions[vdim];
    /*  Check input for sanity  */
    if ( !ds_packet_all_data (inp_arr_desc->packet) )
    {
	fprintf (stderr, "%s: array packets are not all atomic data\n",
		 function_name);
	return (NULL);
    }
    if ( ( out_arr_desc = find_array (old, inp_hd->name, &index) )
	 == NULL )
    {
	fprintf (stderr, "Dimension: \"%s\" not found!\n", inp_hd->name);
	a_prog_bug (function_name);
    }
    if (out_arr_desc != inp_arr_desc)
    {
	fprintf (stderr, "Array missmatch!\n");
	a_prog_bug (function_name);
    }
    if (inp_arr_desc != (array_desc *) old->headers[index]->element_desc[0])
    {
	fprintf (stderr,
		 "%s: array is not the first in it's general structure!\n",
		 function_name);
	return (NULL);
    }
    inp_array = *(char **) old->data[index];
    if ( !ds_compute_array_offsets (inp_arr_desc) )
    {
	m_error_notify (function_name, "array offsets");
	return (NULL);
    }
    hstart = ds_get_coord_num (inp_hd, left_x, SEARCH_BIAS_CLOSEST);
    hstop = ds_get_coord_num (inp_hd, right_x, SEARCH_BIAS_CLOSEST);
    if (hstop <= hstart)
    {
	fprintf (stderr,
		 "Horizontal end index: %lu is not greater than start: %lu\n",
		 hstop, hstart);
	return (NULL);
    }
    ++hstop;
    vstart = ds_get_coord_num (inp_vd, bottom_y, SEARCH_BIAS_CLOSEST);
    vstop = ds_get_coord_num (inp_vd, top_y, SEARCH_BIAS_CLOSEST);
    if (vstop <= vstart)
    {
	fprintf (stderr,
		 "Vertical end index: %lu is not greater than start: %lu\n",
		 vstop, vstart);
	return (NULL);
    }
    ++vstop;
    if ( (inp_hd->length == hstop - hstart) &&
	 (inp_vd->length == vstop - vstart) )
    {
	*allocated = FALSE;
	return (old);
    }
    *allocated = TRUE;
    /*  Create new data structure descriptors  */
    /*  Allocate the multi_array  */
    if ( ( new = ds_alloc_multi (old->num_arrays) ) == NULL )
    {
	m_error_notify (function_name, "multi_array");
	return (NULL);
    }
    /*  Copy top-level packet descriptors  */
    for (count = 0; count < old->num_arrays; ++count)
    {
	if ( ( new->headers[count] = ds_copy_desc_until (old->headers[count],
							 NULL) ) == NULL )
	{
	    m_error_notify (function_name,
			    "general data structure descriptors");
	    ds_dealloc_multi (new);
	    return (NULL);
	}
    }
    /*  Find and fiddle the two dimensions that need to be changed  */
    if ( ( out_arr_desc = find_array (new, inp_hd->name, &index) )
	 == NULL )
    {
	fprintf (stderr, "Dimension: \"%s\" not found!\n", inp_hd->name);
	a_prog_bug (function_name);
    }
    out_hd = out_arr_desc->dimensions[hdim];
    out_vd = out_arr_desc->dimensions[vdim];
    /*  Modify dimension lengths and first and last co-ordinates  */
    out_hd->length = hstop - hstart;
    out_vd->length = vstop - vstart;
    out_arr_desc->lengths[hdim] = out_hd->length;
    out_arr_desc->lengths[vdim] = out_vd->length;
    out_hd->first_coord = ds_get_coordinate (inp_hd, hstart);
    out_hd->last_coord = ds_get_coordinate (inp_hd, hstop - 1);
    out_vd->first_coord = ds_get_coordinate (inp_vd, vstart);
    out_vd->last_coord = ds_get_coordinate (inp_vd, vstop - 1);
    ds_remove_tiling_info (out_arr_desc);
    if ( !ds_compute_array_offsets (out_arr_desc) )
    {
	m_error_notify (function_name, "array offsets");
	ds_dealloc_multi (new);
	return (NULL);
    }
    /*  Allocate and copy data areas. This should not copy the actual array
	data since the array size has changed  */
    for (count = 0; count < old->num_arrays; ++count)
    {
	if ( ( new->data[count] = ds_alloc_data (new->headers[count], FALSE,
						 TRUE) ) == NULL )
	{
	    m_error_notify (function_name, "data");
	    ds_dealloc_multi (new);
	    return (NULL);
	}
	if ( !ds_copy_data (old->headers[count], old->data[count],
			    new->headers[count], new->data[count]) )
	{
	    fprintf (stderr, "%s: error copying data\n", function_name);
	    ds_dealloc_multi (new);
	    return (NULL);
	}
    }
    /*  Fiddle any FITS-style co-ordinates  */
    if ( ( fits_axis = ds_get_fits_axis (new->headers[index],
					 new->data[index], out_hd->name) )
	 > 0 )
    {
	/*  Do it for the horizontal axis  */
	sprintf (txt, "CRPIX%u", fits_axis);
	if ( !ds_get_unique_named_value (new->headers[index],
					 new->data[index], txt, NULL,
					 crpix) )
	{
	    a_func_abort (function_name, "error getting FITS keyword");
	    ds_dealloc_multi (new);
	    return (NULL);
	}
	crpix[0] -= out_hd->first_coord;
	if ( !ds_put_unique_named_value (new->headers[index],
					 new->data + index, txt, K_DOUBLE,
					 crpix, TRUE) )
	{
	    ds_dealloc_multi (new);
	    return (NULL);
	}
	out_hd->first_coord = 0.0;
	out_hd->last_coord = out_hd->length - 1;
    }
    if ( ( fits_axis = ds_get_fits_axis (new->headers[index],
					 new->data[index], out_vd->name) )
	 > 0 )
    {
	/*  Do it for the vertical axis  */
	sprintf (txt, "CRPIX%u", fits_axis);
	if ( !ds_get_unique_named_value (new->headers[index],
					 new->data[index], txt, NULL,
					 crpix) )
	{
	    a_func_abort (function_name, "error getting FITS keyword");
	    ds_dealloc_multi (new);
	    return (NULL);
	}
	crpix[0] -= out_vd->first_coord;
	if ( !ds_put_unique_named_value (new->headers[index],
					 new->data + index, txt, K_DOUBLE,
					 crpix, TRUE) )
	{
	    ds_dealloc_multi (new);
	    return (NULL);
	}
	out_vd->first_coord = 0.0;
	out_vd->last_coord = out_vd->length - 1;
    }
    /*  Proceed to copy data  */
    out_array = *(char **) new->data[index];
    /*  Allocate and initialise co-ordinate array  */
    if ( ( coordinates = (uaddr *) m_alloc (sizeof *coordinates * num_dim) )
	 == NULL )
    {
	m_abort (function_name, "co-ordinate array");
    }
    m_clear ( (char *) coordinates, sizeof *coordinates * num_dim );
    /*  Iterate over all co-ordinates except that the specified dimensions are
	skipped  */
    more_to_do = TRUE;
    while (more_to_do)
    {
	/*  Process  */
	inp_offset = 0;
	out_offset = 0;
	for (count = 0; count < num_dim; ++count)
	{
	    inp_offset += inp_arr_desc->offsets[count][ coordinates[count] ];
	    out_offset += out_arr_desc->offsets[count][ coordinates[count] ];
	}
	if ( !copy_plane (inp_array + inp_offset, inp_arr_desc, hdim, vdim,
			  hstart, vstart,
			  out_array + out_offset, out_arr_desc) )
	{
	    m_free ( (char *) coordinates );
	    return (FALSE);
	}
	/*  Increment co-ordinates  */
	count = num_dim - 1;
	more_to_do = TRUE;
	while ( (count >= 0) && more_to_do )
	{
	    if ( (count == hdim) || (count == vdim) )
	    {
		--count;
		continue;
	    }
	    /*  Increment co-ordinate in this dimension  */
	    if (++coordinates[count] < inp_arr_desc->dimensions[count]->length)
	    {
		/*  Co-ordinate for this dimension still valid  */
		more_to_do = FALSE;
	    }
	    else
	    {
		/*  Co-ordinate has gone past limits: go up to the next
		    dimension  */
		coordinates[count] = 0;
		--count;
	    }
	}
	more_to_do = (count < 0) ? FALSE : TRUE;
    }
    m_free ( (char *) coordinates );
    return (new);
}   /*  End Function create_sub  */

static array_desc *find_array (CONST multi_array *multi_desc, CONST char *name,
			       unsigned int *index)
/*  [SUMMARY] Find a multi-dimensional array.
    <multi_desc> The data structure to search.
    <name> The name of a dimension in the array to search for.
    <index> The index of the general data structure.
    [RETURNS] A pointer to the array descriptor on success, else NULL.
*/
{
    unsigned int count;
    unsigned int dim_index;
    array_desc *arr_desc;
    static char function_name[] = "find_array";

    for (count = 0; count < multi_desc->num_arrays; ++count)
    {
	switch ( ds_f_name_in_packet (multi_desc->headers[count], name,
				      (char **) &arr_desc, &dim_index) )
	{
	  case IDENT_NOT_FOUND:
	    continue;
	    /*break;*/
	  case IDENT_ELEMENT:
	    continue;
	    /*break;*/
	  case IDENT_MULTIPLE:
	    fprintf (stderr, "Item \"%s\" found more than once\n", name);
	    return (NULL);
	    /*break;*/
	  case IDENT_DIMENSION:
	    *index = count;
	    return (arr_desc);
	    /*break;*/
	  case IDENT_GEN_STRUCT:
	  default:
	    fprintf (stderr,
		     "Bad return value from function: ds_f_name_in_packet\n");
	    a_prog_bug (function_name);
	    break;
	}
    }
    return (arr_desc);
}   /*  End Function find_array  */

static flag copy_plane (CONST char *inp_data, CONST array_desc *inp_arr_desc,
			unsigned int hdim, unsigned int vdim,
			uaddr hstart, uaddr vstart,
			char *out_data, CONST array_desc *out_arr_desc)
/*  [SUMMARY] Copy part of a plane of data.
    <inp_data> The input data.
    <inp_arr_desc> The input array descriptor.
    <out_data> The output data.
    <out_arr_desc> The output array descriptor.
    <dim_index> The dimension to copy.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    uaddr hcount, vcount, hlen, vlen;
    unsigned int pack_size;
    /*static char function_name[] = "copy_plane";*/

    pack_size = ds_get_packet_size (inp_arr_desc->packet);
    hlen = out_arr_desc->dimensions[hdim]->length;
    vlen = out_arr_desc->dimensions[vdim]->length;
    for (vcount = 0; vcount < vlen; ++vcount)
	for (hcount = 0; hcount < hlen; ++hcount)
	{
	    m_copy (out_data + out_arr_desc->offsets[hdim][hcount] +
		    out_arr_desc->offsets[vdim][vcount],
		    inp_data + inp_arr_desc->offsets[hdim][hcount + hstart] +
		    inp_arr_desc->offsets[vdim][vcount + vstart],
		    pack_size);
	}
    return (TRUE);
}   /*  End Function copy_dimension  */
