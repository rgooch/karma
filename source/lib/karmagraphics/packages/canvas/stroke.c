/*LINTLIBRARY*/
/*  stroke.c

    This code provides a routine to create an edit stroke instruction.

    Copyright (C) 1993,1994  Richard Gooch

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

/*  This file contains a routine which can create a 2-dimensional editing
    instruction for a stroke.


    Written by      Richard Gooch   1-MAY-1993

    Updated by      Richard Gooch   2-JUN-1993: Added trapping for zero brush
  with in  canvas_create_stroke_instruction  .

    Last updated by Richard Gooch   27-AUG-1993: Took account of changes to
  iedit_  package.

    Last updated by Richard Gooch   26-NOV-1994: Moved to
  packages/canvas/stroke.c


*/
#include <stdio.h>
#include <math.h>

#define KWIN_GENERIC_ONLY
#include <karma_canvas.h>

#include <karma_iedit.h>
#include <karma_a.h>


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag canvas_create_stroke_instruction (canvas, x0, y0, x1, y1,
				       brush_width, value, ilist)
/*  This routine will create a 2-dimensional edit stroke instruction, according
    to the specification in the  iedit_  package and will append it to a
    managed edit list.
    The world canvas which contains the world co-ordinate system must be given
    by  canvas  .
    The end-points (in world co-ordinates) of the stroke must be given by
    x0  ,  y0  ,  x1  and  y1  .
    The width of the stroke (in canvas pixels) must be given by  brush_width  .
    This must be at least 1.
    The data value in the stroke instruction must be pointed to by  value  .
    This must be of type K_DCOMPLEX.
    The managed edit list must be given by  ilist  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KWorldCanvas canvas;
double x0;
double y0;
double x1;
double y1;
unsigned int brush_width;
double value[2];
KImageEditList ilist;
{
    edit_coord brushradius;
    int ix, iy;
    double vector_x;
    double vector_y;
    double factor;
    double o_x, o_y;
    edit_coord *coords;
    list_entry *entry;
    static char function_name[] = "canvas_create_stroke_instruction";

    if (brush_width < 1)
    {
	(void) fprintf (stderr, "brush_width  must be at least 1\n");
	return (FALSE);
    }
    /*  Get pixel co-ordinates  */
    (void) canvas_convert_from_canvas_coord (canvas, x1, y1, &ix, &iy);
    /*  Add brush width and convert back to world co-ordinates  */
    (void) canvas_convert_to_canvas_coord (canvas,
					   ix + brush_width, iy - brush_width,
					   &brushradius.abscissa,
					   &brushradius.ordinate);
    brushradius.abscissa = (brushradius.abscissa - x1) / 2.0;
    brushradius.ordinate = (brushradius.ordinate - y1) / 2.0;
    if ( ( coords = iedit_alloc_edit_coords (4) ) == NULL )
    {
	m_error_notify (function_name, "array of edit co-ordinates");
	return (FALSE);
    }
    vector_x = (x1 - x0) / brushradius.abscissa;
    vector_y = (y1 - y0) / brushradius.ordinate;
    factor = 1.0 / sqrt (vector_x * vector_x + vector_y * vector_y);
    vector_x *= factor;
    vector_y *= factor;
    o_x = vector_y * brushradius.abscissa;
    o_y = -vector_x * brushradius.ordinate;
    coords[0].abscissa = x1 + o_x;
    coords[0].ordinate = y1 + o_y;
    coords[1].abscissa = x0 + o_x;
    coords[1].ordinate = y0 + o_y;
    coords[2].abscissa = x0 - o_x;
    coords[2].ordinate = y0 - o_y;
    coords[3].abscissa = x1 - o_x;
    coords[3].ordinate = y1 - o_y;
    /*  Create instruction entry  */
    return ( iedit_add_instruction (ilist, EDIT_INSTRUCTION_STROKE, coords,
				    4, value) );
}   /*  End Function canvas_create_stroke_instruction  */
