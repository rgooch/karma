/*LINTLIBRARY*/
/*  draw.c

    This code provides routines to draw objects into Karma data structures.

    Copyright (C) 1992,1993,1994,1995  Richard Gooch

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

    This file contains the various utility routines for drawing (writing)
    objects (such as ellipses) into a Karma general data structure.


    Written by      Richard Gooch   3-FEB-1992

    Updated by      Patrick Jordan  10-FEB-1992

    Updated by      Richard Gooch   30-DEC-1992

    Updated by      Richard Gooch   3-MAY-1993: Fixed bug in  ds_draw_ellipse

    Updated by      Patrick Jordan  27-MAY-1993: Whole new  ds_draw_polygon

    Updated by      Patrick Jordan  2-JUN-1993: Added error checking to
  ds_draw_ellipse  .

    Updated by      Richard Gooch   31-AUG-1993: Made  compare_ind  and
  compare_active  private.

    Updated by      Richard Gooch   23-NOV-1993: Applied patch submitted by
  Patrick Jordan to fatten drawn objects.

    Updated by      Patrick Jordan  12-JAN-1994: Fixed error in ds_draw
  polygon that caused the top and right hand lines not to be drawn.

    Updated by      Richard Gooch   4-OCT-1994: Made use of  m_copy  routine in
  order to avoid having to link with buggy UCB compatibility library in
  Slowaris 2.3

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ds/draw.c

    Last updated by Richard Gooch   19-APR-1995: Cleaned some code.


*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>

static void put_segment_at();
static void put_segment();

typedef struct {
  double x, y;
} Point2;

typedef struct {		/* a polygon edge */
    double x;	/* x coordinate of edge's intersection with current scanline */
    double dx;	/* change in x with respect to y */
    int i;	/* edge number: edge i goes from pt[i] to pt[i+1] */
} Edge;

/* Disgustingly, these must be globals, as they are used in compare_ind
   and compare_active, and there is no other way to get them in 
*/

static int n;		       /* number of vertices */
static Point2 *pt;	       /* vertices */

static int nact;	       /* number of active edges */
static Edge *active;	       /* active edge list:edges crossing scanline y */


STATIC_FUNCTION (void delete, (int i) );
STATIC_FUNCTION (void insert, (int i, int y) );
static int compare_ind(), compare_active();


/*PUBLIC_FUNCTION*/
flag ds_draw_ellipse (array, elem_type, abs_dim_desc, abs_stride,
		      ord_dim_desc, ord_stride,
		      centre_abs, centre_ord, radius_abs, radius_ord, value)
/*  This routine will draw an ellipse into a 2 dimensional Karma array.
    The array (plane) must be pointed to by  array  .
    The type of the element to draw must be given by  elem_type  .
    The abscissa dimension descriptor must be pointed to by  abs_dim_desc  .
    The stride of abscissa co-ordinates in memory (in bytes) must be given by
    abs_stride  .
    The ordinate dimension descriptor must be pointed to by  ord_dim_desc  .
    The stride of ordinate co-ordinates in memory (in bytes) must be given by
    ord_stride  .
    The centre of the ellipse (in abscissa and ordinate real-world
    co-ordinates) must be given by  centre_abs  and  centre_ord  ,respectively.
    The abscissa and ordinate radii must be given by  radius_abd  and
    radius_ord  ,respectively. These must NOT be equal or less than 0.0
    The value to write the ellipse into the array must be given by  value  .
    The routine returns TRUE on success, else it returns FALSE.
*/
char *array;
unsigned int elem_type;
dim_desc *abs_dim_desc;
unsigned int abs_stride;
dim_desc *ord_dim_desc;
unsigned int ord_stride;
double centre_abs;
double centre_ord;
double radius_abs;
double radius_ord;
double *value;
{
    double x, y, scale;
    static char function_name[] = "ds_draw_ellipse";

    if ( (array == NULL) || (abs_dim_desc == NULL) || (ord_dim_desc == NULL) ||
	(value == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (radius_abs <= 0.0)
    {
	(void) fprintf (stderr, "Illegal abscissa radius: %e\n", radius_abs);
	a_prog_bug (function_name);
    }
    if (radius_ord <= 0.0)
    {
	(void) fprintf (stderr, "Illegal ordinate radius: %e\n", radius_ord);
	a_prog_bug (function_name);
    }
    scale = ( ( (*ord_dim_desc).maximum - (*ord_dim_desc).minimum ) /
	     ( (*ord_dim_desc).length - 1) );

    for (y = 0; y <= radius_ord; y += scale)
    {
	x = radius_abs / radius_ord * sqrt (radius_ord * radius_ord - y * y);
	put_segment_at (array, elem_type, ord_stride, abs_stride,
			ord_dim_desc, abs_dim_desc, centre_ord + y,
			scale, centre_abs - x, centre_abs + x,
			value, SEARCH_BIAS_CLOSEST);
	put_segment_at (array, elem_type, ord_stride, abs_stride,
			ord_dim_desc, abs_dim_desc, centre_ord - y,
			scale, centre_abs - x, centre_abs + x,
			value, SEARCH_BIAS_CLOSEST);
    }
    /*  All done: OK  */
    return (TRUE);
}   /*  End Function ds_draw_ellipse  */


/*PUBLIC_FUNCTION*/
flag ds_draw_polygon (array, elem_type, abs_dim_desc, abs_stride, ord_dim_desc,
		      ord_stride, coords, num_points, value)
/*  This routine will draw a concave non-simple polygon into a 2
    dimensional Karma array. The polygon can be clockwise or anti-clockwise.
    Inside-outside test done by Jordan's rule: a point is considered inside if
    an emanating ray intersects the polygon an odd number of times.
    The algorithm is modified from the  Concave Polygon Scan Conversion
    by Paul Heckbert from "Graphics Gems", Academic Press, 1990.
    The array (plane) must be pointed to by  array  .
    The type of the element to draw must be given by  elem_type  .
    The abscissa dimension descriptor must be pointed to by  abs_dim_desc  .
    The stride of abscissa co-ordinates in memory (in bytes) must be given by
    abs_stride  .
    The ordinate dimension descriptor must be pointed to by  ord_dim_desc  .
    The stride of ordinate co-ordinates in memory (in bytes) must be given by
    ord_stride  .
    The co-ordinates must be stored in clockwise rotation order (any point to
    start) in the array coords[num_points].
    The value to write the parallelogram into the array must be given by
    value  .
    The routine returns TRUE on success, else it returns FALSE.
*/
char *array;
unsigned int elem_type;
dim_desc *abs_dim_desc;
unsigned int abs_stride;
dim_desc *ord_dim_desc;
unsigned int ord_stride;
edit_coord *coords;
unsigned int num_points;
double *value;
{
    Point2 *point;
    int bias=SEARCH_BIAS_LOWER;
    int k, i, j;
    int y0,y1,xl,xr,y;
    int *ind;	/* list of vertex indices, sorted by pt[ind[j]].y */
    static char function_name[] = "ds_draw_polygon";

    if ( (array == NULL) || (abs_dim_desc == NULL) || (ord_dim_desc == NULL) ||
	(coords == NULL) || (value == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    point=(Point2 *)m_alloc(sizeof(Point2)*num_points);
    if(point==NULL)  m_error_notify(function_name,"points array");

    for(i=0;i<num_points;i++)
    {
	point[i].x = ds_get_coord_num(abs_dim_desc,coords[i].abscissa,bias);
	point[i].y = ds_get_coord_num(ord_dim_desc,coords[i].ordinate,bias);
    }

    n = num_points;
    pt = point;
    if (n<=0) return (TRUE);

    ind=(int *)m_alloc(sizeof(int)*n);
    if(ind==NULL)  m_error_notify(function_name,"ind array");
    active=(Edge *)m_alloc(sizeof(Edge)*n);
    if(active==NULL)  m_error_notify(function_name,"active array");

    /* create y-sorted array of indices ind[k] into vertex list */
    for (k=0; k<n; k++)  ind[k] = k;
    qsort(ind, n, sizeof ind[0], compare_ind); /* sort ind by pt[ind[k]].y */

    nact = 0;			/* start with empty active list */
    k = 0;			/* ind[k] is next vertex to process */
    y0 = pt[ind[0]].y;		/* ymin of polygon */
    y1 = pt[ind[n-1]].y;	/* ymax of polygon */

    for (y=y0; y<=y1; y++)
    {				/* step through scanlines */
	/* check vertices between previous scanline and current one, if any */
	for (; k<n && pt[ind[k]].y<=y; k++)
	{
	    i = ind[k];	
	    /*
	     * insert or delete edges before and after vertex i (i-1 to i,
	     * and i to i+1) from active list if they cross scanline y
	     */
	    j = i>0 ? i-1 : n-1; /* vertex previous to i */
	    if (pt[j].y <= y && y!=y1)	/* old edge, remove from active list */
	      delete(j);
	    else if (pt[j].y > y) /* new edge, add to active list */
	      insert(j, y);
	    j = i<n-1 ? i+1 : 0; /* vertex next after i */
	    if (pt[j].y <= y && y!=y1)	/* old edge, remove from active list */
	      delete(i);
	    else if (pt[j].y > y) /* new edge, add to active list */
	      insert(i, y);
	  }

	/* sort active edge list by active[j].x */
	qsort(active, nact, sizeof active[0], compare_active);

	/* draw horizontal segments for scanline y */
        for (j=0; j<nact; j+=2)
	{			/* draw horizontal segments */
	    /* span 'tween j & j+1 is inside, span tween j+1 & j+2 is outside*/
	    xl = floor(active[j].x); /* left end of span */
	    xr = ceil(active[j+1].x); /* right end of span */
	    if (xl<=xr)
	    (void) ds_put_element_many_times
	    (array + ord_stride * y + abs_stride * xl,
	     elem_type, abs_stride, value, 1+xr-xl); /* draw segment */
	    active[j].x += active[j].dx; /* increment edge coords */
	    active[j+1].x += active[j+1].dx;
	}
    }
    /*  All done: OK  */
    m_free((char *)point);
    m_free((char *)active);
    m_free((char *)ind);
    return (TRUE);
}   /*  End Function ds_draw_polygon  */


/*  Private functions follow  */

static void delete (int i)		/* remove edge i from active list */
{
    int j;
    for (j=0; j<nact && active[j].i!=i; j++);
    if (j>=nact) return;       /* edge not in active list; happens at win->y0*/
    nact--;
    m_copy ( (char *) &active[j], (char *) &active[j+1],
	    (nact-j)*sizeof active[0] );
}

static void insert (int i, int y)     /* append edge i to end of active list */
{
    int j;
    double dx;
    Point2 *p, *q;

    j = i<n-1 ? i+1 : 0;
    if (pt[i].y < pt[j].y) {p = &pt[i]; q = &pt[j];}
    else		   {p = &pt[j]; q = &pt[i];}
    /* initialize x position at intersection of edge with scanline y */
    active[nact].dx = dx = (q->x-p->x)/(q->y-p->y);
    active[nact].x = dx*(y-p->y)+p->x;
    active[nact].i = i;
    nact++;
}

/* comparison routines for qsort */
static int compare_ind(u, v) int *u, *v; {return pt[*u].y <= pt[*v].y ? -1 :1;}
static int compare_active(u, v) Edge *u, *v; {return u->x <= v->x ? -1 : 1;}

static void put_segment_at(data,data_type,linestride,segstride,line_dim_desc,
			   seg_dim_desc,line_num,line_scale,
			   seg_start,seg_end,value,abs_bias)
/*  This routine fills a segment in the given line using put_segment.
    line_num is a world coordinate for the line to fill.
*/
char *data;
unsigned int data_type;
unsigned int linestride;
unsigned int segstride;
dim_desc *line_dim_desc;
dim_desc *seg_dim_desc;
double line_num;
double line_scale;
double seg_start;
double seg_end;
double *value;
unsigned int abs_bias;
{
    int co;

    /* Check for out of bounds and return */
    line_scale = line_scale / 2.0;
    if (line_num-line_scale >= line_dim_desc->maximum) return;
    if (line_num+line_scale <= line_dim_desc->minimum) return;

    co = ds_get_coord_num (line_dim_desc, line_num, SEARCH_BIAS_CLOSEST);
    put_segment (data + linestride * co, data_type, segstride, seg_dim_desc,
		 seg_start, seg_end, value, abs_bias);
}

static void put_segment(data,data_type,stride,seg_dim_desc,seg_start,seg_end,
			value, bias)
/*  This routine fills along a dimension from seg_start to seg_end (in world
    coordinates), clipping as required.
    The array of output data must be pointed to by data and the data type
    value must be in data_type.
    The stride of data elements in memory (in bytes) must be given by stride.
    The data value will be read from the storage pointed to by value.
    The dimension descriptor for the dimension to fill along must be pointed
    to by seg_dim_desc and the start and end coordinates must be in 
    seg_start and seg_end.
*/
char *data;
unsigned int data_type;
unsigned int stride;
dim_desc *seg_dim_desc;
double seg_start;
double seg_end;
double *value;
unsigned int bias;
{
    unsigned int st_co, end_co, num_els;

    st_co = ds_get_coord_num (seg_dim_desc, seg_start, bias);
    end_co = ds_get_coord_num (seg_dim_desc, seg_end, bias);
    if (st_co == end_co) return;
    if (st_co > end_co)
    {
	unsigned int swap;

	swap = st_co;
	st_co = end_co;
	end_co = swap;
    }
    num_els = end_co - st_co + 1;

    (void) ds_put_element_many_times (data + stride * st_co, data_type, stride,
				      value, num_els);
}
