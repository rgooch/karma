/*  karma_kwin_hooks.h

    Header for  kwin_  package. This file ONLY contains the hook functions

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

/*

    This include file contains all the definitions for the psw_ package in the
    Karma library.


    Written by      Richard Gooch   19-APR-1996: Moved from karma_kwin.h

    Last updated by Richard Gooch   25-AUG-1996

*/

#ifndef KARMA_KWIN_HOOKS_H
#define KARMA_KWIN_HOOKS_H
#include <stdarg.h>


#ifndef KPixHookCanvas
#  define KPixHookCanvas void *
#endif

/*  Codes for optional functions  */
#define KWIN_FUNC_DRAW_PC_IMAGE     10000
#define KWIN_FUNC_DRAW_RGB_IMAGE    10001
#define KWIN_FUNC_DRAW_CACHED_IMAGE 10002
#define KWIN_FUNC_FREE_CACHE_DATA   10003
#define KWIN_FUNC_DRAW_LINE         10004
#define KWIN_FUNC_DRAW_ARC          10005
#define KWIN_FUNC_DRAW_POLYGON      10006
#define KWIN_FUNC_DRAW_STRING       10007
#define KWIN_FUNC_DRAW_RECTANGLE    10008
#define KWIN_FUNC_DRAW_LINES        10009
#define KWIN_FUNC_DRAW_ARCS         10010
#define KWIN_FUNC_DRAW_SEGMENTS     10011
#define KWIN_FUNC_GET_COLOUR        10012
#define KWIN_FUNC_LOAD_FONT         10013
#define KWIN_FUNC_GET_STRING_SIZE   10014
#define KWIN_FUNC_SET_FONT          10015
#define KWIN_FUNC_QUERY_COLOURMAP   10016
#define KWIN_FUNC_RESIZE            10017
#define KWIN_FUNC_DRAW_POINTS       10018
#define KWIN_FUNC_SET_LINEWIDTH     10019

/*  Mandatory function types  */
typedef flag (*KPixFuncDrawPoint) (KPixHookCanvas info, double x, double y,
				   unsigned long pixel_value);
typedef KPixHookCanvas (*KPixFuncCreateChild) (KPixHookCanvas parent,
					       KPixCanvas child);
typedef flag (*KPixFuncClearArea) (KPixHookCanvas info, int x, int y,
				   int width, int height);

/*  Optional function types  */
typedef flag (*KPixFuncDrawPCImage)
     (KPixHookCanvas info, int x_off, int y_off, int x_pixels, int y_pixels,
      CONST char *slice, CONST uaddr *hoffsets, CONST uaddr *voffsets,
      unsigned int width, unsigned int height,
      unsigned int type, unsigned int conv_type,
      unsigned int num_pixels, unsigned long *pixel_values,
      unsigned long blank_pixel,
      unsigned long min_sat_pixel, unsigned long max_sat_pixel,
      double i_min, double i_max,
      flag (*iscale_func) (), void *iscale_info,
      KPixCanvasImageCache *cache_ptr);
typedef flag (*KPixFuncDrawRGBImage)
     (KPixHookCanvas info, int x_off, int y_off, int x_pixels, int y_pixels,
      CONST unsigned char *red_slice, CONST unsigned char *green_slice,
      CONST unsigned char *blue_slice,
      CONST uaddr *hoffsets, CONST uaddr *voffsets,
      unsigned int width, unsigned int height,
      KPixCanvasImageCache *cache_ptr);
typedef flag (*KPixFuncDrawCachedImage)
     (KPixCanvasImageCache cache, flag wait,
      int parent_x_off, int parent_y_off, int image_width, int image_height,
      int image_x_off, int image_y_off, int canvas_x_off, int canvas_y_off,
      int canvas_width, int canvas_height);
typedef void (*KPixFuncFreeCacheData) (KPixCanvasImageCache cache);
typedef flag (*KPixFuncDrawLine) (KPixHookCanvas info, double x0, double y0,
				  double x1, double y1,
				  unsigned long pixel_value);
typedef flag (*KPixFuncDrawArc)
     (KPixHookCanvas info, double x, double y,
      double width, double height, int angle1, int angle2,
      unsigned long pixel_value, flag fill);
typedef flag (*KPixFuncDrawPolygon)
     (KPixHookCanvas info, double *x_arr, double *y_arr,
      unsigned int num_vertices, unsigned long pixel_value,
      flag convex, flag fill);
typedef flag (*KPixFuncDrawString)
     (KPixHookCanvas info, double x, double y, CONST char *string,
      unsigned long pixel_value, flag clear_under);
typedef flag (*KPixFuncDrawRectangle)
     (KPixHookCanvas info, double x, double y, double width, double height,
      unsigned long pixel_value, flag fill);
typedef flag (*KPixFuncDrawLines)
     (KPixHookCanvas info, CONST double *x_arr, CONST double *y_arr,
      unsigned int num_points, unsigned long pixel_value);
typedef flag (*KPixFuncDrawArcs)
     (KPixHookCanvas info, double *x, double *y, double *width, double *height,
      int *angle1, int *angle2, unsigned int num_ellipses,
      unsigned long pixel_value, flag fill);
typedef flag (*KPixFuncDrawSegments)
     (KPixHookCanvas info, double *x0, double *y0, double *x1, double *y1,
      unsigned int num_segments, unsigned long pixel_value);
typedef flag (*KPixFuncGetColour)
     (KPixHookCanvas info, CONST char *colourname,
      unsigned long *pixel_value, unsigned short *red,
      unsigned short *green, unsigned short *blue);
typedef void *(*KPixFuncLoadFont) (KPixHookCanvas info, CONST char *fontname);
typedef flag (*KPixFuncGetStringSize) (KPixHookCanvas info, CONST char *string,
				       va_list argp);
typedef void (*KPixFuncSetFont) (KPixHookCanvas info, void *font_info);
typedef flag (*KPixFuncQueryColourmap)
     (KPixHookCanvas info, unsigned long *pixels,
      unsigned short *reds, unsigned short *greens,
      unsigned short *blues, unsigned int num_colours);
typedef flag (*KPixFuncResize) (KPixHookCanvas info, int xoff, int yoff,
				int width, int height);
typedef flag (*KPixFuncDrawPoints)
     (KPixHookCanvas info, double *x_arr, double *y_arr,
      unsigned int num_points, unsigned long pixel_value);
typedef flag (*KPixFuncSetLinewidth) (KPixHookCanvas info, double linewidth);

typedef struct  /*  All drawing functions must be containted herein  */
{
    /*  Mandatory graphics system specific hooks  */
    KPixFuncDrawPoint        point;
    /*  Optional graphics system specific hooks  */
    KPixFuncDrawPCImage      pc_image;
    KPixFuncDrawRGBImage     rgb_image;
    KPixFuncDrawCachedImage  cached_image;
    KPixFuncDrawLine         line;
    KPixFuncDrawArc          arc;
    KPixFuncDrawPolygon      polygon;
    KPixFuncDrawString       string;
    KPixFuncDrawRectangle    rectangle;
    KPixFuncDrawLines        lines;
    KPixFuncDrawArcs         arcs;
    KPixFuncDrawSegments     segments;
    KPixFuncDrawPoints       points;
    KPixFuncSetLinewidth     set_linewidth;
    /*  The info for the drawing functions  */
    void *info;
} KPixDrawFuncs;


/*  File: generic.c  */
EXTERN_FUNCTION (KPixCanvas kwin_create_generic,
		 (void *info, int xoff, int yoff, int width, int height,
		  unsigned int depth, unsigned int visual, flag visible,
		  KPixFuncDrawPoint draw_point,
		  KPixFuncCreateChild create_child,
		  KPixFuncClearArea clear_area, ...) );
EXTERN_FUNCTION (flag kwin_new_driver_refresh,
		 (KPixCanvas canvas, KPixDrawFuncs draw_funcs,
		  PostScriptPage pspage) );

#endif /*  KARMA_KWIN_HOOKS_H  */
