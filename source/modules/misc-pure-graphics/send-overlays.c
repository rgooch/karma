/*  send.c

    Source file for  send  (overlay list test transmitter module).

    Copyright (C) 1993-1996  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*


    Written by      Richard Gooch   6-DEC-1993

    Updated by      Richard Gooch   11-DEC-1993

    Updated by      Richard Gooch   29-FEB-1996: Added spiral.

    Last updated by Richard Gooch   27-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <math.h>
#include <sys/file.h>
#include <karma.h>
#include <karma_overlay.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_conn.h>
#include <karma_ex.h>
#include <karma_ch.h>
#include <karma_a.h>
#include <karma_m.h>

EXTERN_FUNCTION (flag send, (char *command, FILE *fp) );
STATIC_FUNCTION (void server_exited, (Connection connection, void *info) );


#define VERSION "1.2"

#define NUM_ELLIPSES 100
#define NUM_SEGMENTS 100
#define NUM_VECTORS 100

static void add_to_list ();
static void empty_list ();
static void send_ellipses ();
static void send_segments ();
static void send_vectors ();
STATIC_FUNCTION (void send_spiral, (CONST char *p) );


static KOverlayList olist = NULL;
static unsigned int num_segments = 1000;
static flag one_object = FALSE;


int main (int argc, char **argv)
{
    KControlPanel panel;
    extern KOverlayList olist;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "spiral", "sends spiral", PIT_FUNCTION,
		    (void *) send_spiral,
		    PIA_END);
    panel_add_item (panel, "send", "sends objects", PIT_FUNCTION,
		    (void *) add_to_list,
		    PIA_END);
    panel_add_item (panel, "ellipses", "sends ellipses",
		    PIT_FUNCTION, (void *) send_ellipses,
		    PIA_END);
    panel_add_item (panel, "segments", "sends segments",
		    PIT_FUNCTION, (void *) send_segments,
		    PIA_END);
    panel_add_item (panel, "vectors", "sends vectors",
		    PIT_FUNCTION, (void *) send_vectors,
		    PIA_END);
    panel_add_item (panel, "empty", "number of objects", PIT_FUNCTION,
		    (void *) empty_list,
		    PIA_END);
    panel_add_item (panel, "one_object", "flag", PIT_FLAG, &one_object,
		    PIA_END);
    panel_add_item (panel, "num_segments", "number of segments", K_UINT,
		    &num_segments,
		    PIA_MAX_VALUE, (double) NUM_SEGMENTS,
		    PIA_END);
    panel_push_onto_stack (panel);
    if ( ( olist = overlay_create_list (NULL) ) == NULL )
    {
	exit (1);
    }
    conn_register_client_protocol ("ping_server", 0, 0,
				   ( flag (*) () ) NULL,
				   ( flag (*) () ) NULL,
				   ( flag (*) () ) NULL,
				   ( void (*) () ) NULL);
    conn_register_client_protocol ("server_exit", 0, 0,
				   ( flag (*) () ) NULL,
				   ( flag (*) () ) NULL,
				   ( flag (*) () ) NULL,
				   server_exited);
    module_run (argc, argv, "send", VERSION, send, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag send (p, fp)
char *p;
FILE *fp;
{
    return (TRUE);
}   /*  End Function send  */

static void add_to_list (p)
/*  This routine will flush the accumulated arrays.
*/
char *p;
{
    unsigned int count;
    double x;
    double y;
    unsigned int types[3];
    double x_arr[3];
    double y_arr[3];
    /*static char function_name[] = "add_to_list";*/

    (void) overlay_line (olist, OVERLAY_COORD_RELATIVE, 0.0, 1.0,
			 OVERLAY_COORD_RELATIVE, 1.0, 0.0, "blue");
    (void) overlay_text (olist, "Hello there",
			 OVERLAY_COORD_RELATIVE, 0.1, 0.5, "red", "10x20",
			 FALSE);
    (void) overlay_text (olist, "Gidday!!!",
			 OVERLAY_COORD_RELATIVE, 0.1, 0.6, "red", "12x24",
			 TRUE);
    (void) overlay_ellipse (olist, OVERLAY_COORD_WORLD, 265.0, 245.0,
			    OVERLAY_COORD_WORLD, 21.0, 9.0, "green", FALSE);
    (void) overlay_ellipse (olist, OVERLAY_COORD_WORLD, 265.0, 245.0,
			    OVERLAY_COORD_RELATIVE, 0.02, 0.01, "blue", FALSE);
    (void) overlay_ellipse (olist, OVERLAY_COORD_RELATIVE, 1.0, 0.0,
			    OVERLAY_COORD_PIXEL, 10, 10, "red", TRUE);
    types[0] = types[1] = types[2] = OVERLAY_COORD_WORLD;
    x_arr[0] = 300.0;
    y_arr[0] = 186.0;
    x_arr[1] = 296.0;
    y_arr[1] = 179.0;
    x_arr[2] = 304.0;
    y_arr[2] = 179.0;
    (void) overlay_filled_polygon (olist, 3, types, x_arr, y_arr, "green");
    for (count = 0; count < NUM_ELLIPSES; ++count)
    {
	x = (double) count / (double) NUM_ELLIPSES;
	y = x;
	(void) overlay_ellipse (olist, OVERLAY_COORD_RELATIVE, x, x,
				OVERLAY_COORD_PIXEL, 10, 10, "red", FALSE);
    }
}   /*  End Function add_to_list  */

static void empty_list (p)
/*  This routine will flush the accumulated arrays.
*/
char *p;
{
    int num_objects;
    /*static char function_name[] = "empty_list";*/

    num_objects = ex_int (p, &p);
    (void) overlay_remove_objects (olist, (unsigned int) num_objects);
}   /*  End Function empty_list  */

static void send_ellipses (p)
char *p;
{
    unsigned int count;
    unsigned int ctypes[NUM_ELLIPSES];
    unsigned int rtypes[NUM_ELLIPSES];
    double cx[NUM_ELLIPSES];
    double cy[NUM_ELLIPSES];
    double rx[NUM_ELLIPSES];
    double ry[NUM_ELLIPSES];
    /*static char function_name[] = "send_ellipses";*/

    for (count = 0; count < NUM_ELLIPSES; ++count)
    {
	ctypes[count] = OVERLAY_COORD_RELATIVE;
	cx[count] = (double) count / (double) NUM_ELLIPSES;
	cy[count] = cx[count];
	rtypes[count] = OVERLAY_COORD_PIXEL;
	rx[count] = 10.0;
	ry[count] = 10.0;
    }
    (void) overlay_ellipses (olist, NUM_ELLIPSES,
			     ctypes, cx, cy, rtypes, rx, ry, "red", FALSE);
}   /*  End Function send_ellipses  */

static void send_segments (p)
char *p;
{
    unsigned int count;
    unsigned int types0[NUM_SEGMENTS];
    unsigned int types1[NUM_SEGMENTS];
    double x0[NUM_SEGMENTS];
    double y0[NUM_SEGMENTS];
    double x1[NUM_SEGMENTS];
    double y1[NUM_SEGMENTS];
    /*static char function_name[] = "send_segments";*/

    for (count = 0; count < NUM_SEGMENTS; ++count)
    {
	types0[count] = OVERLAY_COORD_RELATIVE;
	x0[count] = (double) count / (double) NUM_SEGMENTS;
	y0[count] = x0[count];
	types1[count] = OVERLAY_COORD_RELATIVE;
	x1[count] = 0.0;
	y1[count] = 1.0;
    }
    (void) overlay_segments (olist, NUM_SEGMENTS,
			     types0, x0, y0, types1, x1, y1, "red");
}   /*  End Function send_segments  */

static void send_vectors (p)
char *p;
{
    unsigned int count;
    unsigned int stypes[NUM_VECTORS];
    unsigned int dtypes[NUM_VECTORS];
    double sx[NUM_VECTORS];
    double sy[NUM_VECTORS];
    double dx[NUM_VECTORS];
    double dy[NUM_VECTORS];
    /*static char function_name[] = "send_vectors";*/

    for (count = 0; count < NUM_VECTORS; ++count)
    {
	stypes[count] = OVERLAY_COORD_RELATIVE;
	sx[count] = (double) count / (double) NUM_VECTORS;
	sy[count] = sx[count];
	dtypes[count] = OVERLAY_COORD_PIXEL;
	dx[count] = 10.0;
	dy[count] = 10.0;
    }
    (void) overlay_vectors (olist, NUM_VECTORS,
			     stypes, sx, sy, dtypes, dx, dy, "red");
}   /*  End Function send_vectors  */

static void server_exited (Connection connection, void *info)
{
    (void) fprintf (stderr, "Server exited\n");
}   /*  End Function server_exited  */

static void send_spiral (CONST char *p)
/*  [PURPOSE] This routine will send a spiral.
    <p> Optional arguments. Ignored.
    [RETURNS] Nothing.
*/
{
    int seg_count;
    int segs_per_rot;
    int seg_in_rot;
    double angle;
    double centre_x = 0.5;
    double centre_y = 0.5;
    double radius_x = 0.1;
    double radius_y = 0.1;
    double radius_increment;
    unsigned int types[NUM_SEGMENTS];
    double x_arr[NUM_SEGMENTS];
    double y_arr[NUM_SEGMENTS];

    (void) fprintf (stderr, "Drawing spiral with %u segments as %s\n",
		    num_segments, one_object ? "one object" : "many objects");
    segs_per_rot = num_segments / 10;
    radius_increment = 0.35 / (double) num_segments;
    for (seg_count = 0; seg_count < num_segments; ++seg_count)
    {
	seg_in_rot = seg_count % segs_per_rot;
	angle = TWOPI * (double) seg_in_rot / (double) segs_per_rot;
	x_arr[seg_count] = centre_x + radius_x * sin (angle);
	y_arr[seg_count] = centre_y + radius_y * cos (angle);
	types[seg_count] = OVERLAY_COORD_RELATIVE;
	radius_x += radius_increment;
	radius_y += radius_increment;
    }
    if (one_object)
    {
	(void) overlay_lines (olist, num_segments, types, x_arr, y_arr,
			      "green");
    }
    else
    {
	for (seg_count = 0; seg_count < num_segments - 1; ++seg_count)
	{
	    (void) overlay_line (olist, types[seg_count],
				 x_arr[seg_count], y_arr[seg_count],
				 types[seg_count + 1],
				 x_arr[seg_count + 1], y_arr[seg_count + 1],
				 "red");
	}
    }
}   /*  End Function send_spiral  */
