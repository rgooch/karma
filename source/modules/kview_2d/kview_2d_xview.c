/*  kview_2d_xview.c

    XView setup file for  kview_2d  (X11 image display tool for Karma).

    Copyright (C) 1993  Richard Gooch

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
    This Karma module will enable on-screen imaging of a single arrayfile of
    the general data structure format. The arrayfile must contain a 2
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   12-JUL-1993

    Last updated by Richard Gooch   21-NOV-1993


*/
#include <stdio.h>
#include <math.h>
#include <sys/param.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <k_event_codes.h>
#include "kview_2d.h"
#include <karma_viewimg.h>
#include <karma_kcmap.h>
#include <karma_dsxfr.h>
#include <karma_conn.h>
#include <karma_dir.h>
#include <karma_notify_chm.h>
#include <karma_ds.h>
#include <karma_xc.h>
#include <karma_ic.h>
#include <karma_st.h>
#include <karma_r.h>
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/svrimage.h>
#include <xview/icon.h>
#include <xview/font.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/xv_xrect.h>
#include <xview/tty.h>
#include <xview/termsw.h>
#include <xview/notice.h>
#include <xview/server.h>

#define DEFAULT_FRAME_WIDTH 560
#define DEFAULT_FRAME_HEIGHT 630
#define DEFAULT_ICON_WIDTH 64
#define DEFAULT_ICON_HEIGHT 64
#define DEFAULT_COLOURMAP_NAME "Greyscale1"
#define DEFAULT_LASER_QUEUE_COMMAND "laser"
#define LASER_COMMAND_VARIABLE "LASER_CMD"
#define DEFAULT_COMMAND_ROWS 42
#define DEFAULT_COMMAND_COLUMNS 80
#define DEFAULT_COLOURBAR_FRAME_WIDTH 256
#define DEFAULT_COLOURBAR_FRAME_HEIGHT 50
#define DEFAULT_TRACK_FRAME_WIDTH 600
#define DEFAULT_TRACK_FRAME_HEIGHT 40
#define DEFAULT_AUTO_BLINK_INTERVAL 100
#define MIN_AUTO_BLINK_INTERVAL 0
#define MAX_AUTO_BLINK_INTERVAL 10000
#define COLOUR_INTENSITY_RANGE 1000
#define MAX_COLOURS (unsigned int) 200

#define CMAP_CONNECTION_CLIENT (Notify_client) 50
#define AUTO_BLINK_TIMER_CLIENT (Notify_client) 51

#define SPECIAL_COLOUR_MIN_SAT_INDEX 0
#define SPECIAL_COLOUR_MAX_SAT_INDEX 1
#define SPECIAL_COLOUR_BLANK_INDEX 2
#define NUM_SPECIAL_COLOURS 3


/*  Local functions  */
void clear_crosshairs ();
void zoom_horizontal ();
void zoom_vertical ();
void zoom_2d ();
void unzoom ();
void show_crosshair_positions ();
int quit ();
void image_window_repaint_proc ();
void canvas_resize_proc ();
void paint_window_event ();
Panel_setting cmd_panel_event ();
Icon create_icon ();
void show_error_message ();
void cmap_menu_proc ();
void show_colourbar_frame ();
void colourbar_repaint_proc ();
void colourbar_resize_proc ();
void show_track_frame ();
void dismiss_track_frame ();
void track_frame_repaint_proc ();
void track_frame_resize_proc ();
Notify_value process_input_on_cmap_connection ();
Notify_value base_frame_destroy_func ();
void mouse_mode_menu_proc ();
void write_colourmap_proc ();
void attach_colourmap_proc ();
flag crosshair_click_consumer (/* canvas, x, y, event_code, e_info, f_info */);
void kview_2d_init ();


/*  Private functions  */
static void scan_directory (/* item, event */);
static void register_server_exit ();
static int file_select (/* item, string, client_data, op, event, row */);
static void choice_notify_proc (/* item, value, event */);
static flag load (/* arrayfile */);
static void new_data_on_connection (/* first_time_data */);
static void connection_closed (/* data_deallocated */);
static void load_cmap_for_frame (/* multi_desc */);


/*  External functions  */
extern void refresh_colourbar (/* canvas, width, height, info */);
extern void colourbar_cmap_resize_func (/* cmap, info */);
extern void draw_track_window ();
extern void shimmer_button (/* item, event */);
extern void create_edit_frame (/* base_frame */);
extern void show_edit_frame (/* item, event */);
extern void create_colour_slider_frame (/* base_frame */);
extern void write_colourmap ();
extern void attach_colourmap ();
extern char **extract_cmap_names (/* colour */);
extern flag attempt_slave_cmap_connection (/* full */);
extern flag track_canvas_event (/* canvas, x, y, event_code, e_info,f_info */);
extern flag unit_canvas_position_func (/* canvas, x, y, event_code, e_info,
					  f_info */);
extern void draw_crosshair (/* x, y */);


/*  Structures  */
struct crosshair_type
{
    double x;
    double y;
};

struct dual_crosshair_type
{
    struct crosshair_type first;
    struct crosshair_type second;
    unsigned int num_crosshairs;
};


/*  Global variables for XView  */

/*  Main window info  */
static Frame base_frame;
static int canvas_width = -1;
static int canvas_height = -1;
static Xv_Window cnv_paint_window = (Xv_Window) NULL;
KPixCanvas image_pixcanvas = NULL;
KWorldCanvas image_worldcanvas = NULL;
GC image_gc = NULL;
GC crosshair_gc = NULL;
static ViewableImage vimage = NULL;
static unsigned long special_colours[NUM_SPECIAL_COLOURS];

/*  Command window info  */
static Frame cmd_frame;

/*  Colourbar window info  */
static Frame colourbar_frame;
static int colourbar_win_width = -1;
static int colourbar_win_height = -1;
static Xv_Window colourbar_paint_window = (Xv_Window) NULL;

/*  Track window info  */
static Frame track_frame;
static Xv_Window track_paint_window = (Xv_Window) NULL;
static int track_win_width = -1;
static int track_win_height = -1;

/*  Global variables for zoom functions  */
static struct dual_crosshair_type crosshairs =
{
    { 0, 0 },  { 0, 0 },  { 0 }
};

/*  Globals variables for mouse mode  */
unsigned int mouse_mode = MOUSE_MODE_CROSSHAIR;

/*  Other global variables  */
Panel_item directory_list = (Panel_item) NULL;
Display *my_display = NULL;

char **colourmap_names = NULL;
char *choosen_colourmap = NULL;

Kcolourmap image_cmap = NULL;

flag image_auto_x = TRUE;
flag image_auto_y = TRUE;
flag image_auto_v = TRUE;
flag maintain_aspect_ratio = FALSE;
static struct win_scale_type plotted_scale;

char *cmap_display = NULL;
char *cmap_file = DEFAULT_CMAP_FILENAME;
int cmap_port = -1;
char *track_font_name = "9x15";


/*  Public routines follow  */

void kview_2d_xview (argc, argv)
int argc;
char *argv[];
{
    int scrn;
    Panel panel;
    Canvas canvas;
    Menu colourmap_menu;
    Menu savecolourmap_menu;
    Menu compute_menu;
    Menu zoom_menu;
    Menu draw_menu;
    Menu mouse_mode_menu;
    Menu_item menu_item;
    Xv_Server server;
    char **cmap_name;
    char hostname[MAXHOSTNAMELEN + 1];
    char header_name[STRING_LENGTH];
    char curr_dir_name[STRING_LENGTH];
    extern Display *my_display;
    extern Frame base_frame;
    extern Frame cmd_frame;
    extern Frame colourbar_frame;
    extern Frame track_frame;
    extern Frame auto_blink_frame;
    extern Xv_Window cnv_paint_window;
    extern Xv_Window colourbar_paint_window;
    extern Xv_Window track_paint_window;
    extern Panel_item directory_list;
    extern char module_name[STRING_LENGTH + 1];
    extern unsigned long special_colours[NUM_SPECIAL_COLOURS];
    extern char *choosen_colourmap;
    extern char **colourmap_names;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "kview_2d_xview";

    if (gethostname (hostname, MAXHOSTNAMELEN + 1) != 0)
    {
	(void) fprintf (stderr, "Error getting hostname\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    if (getcwd (curr_dir_name, STRING_LENGTH) == NULL)
    {
	(void) fprintf (stderr,
			"Error getting current working directory\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    (void) sprintf (header_name, "%s @ %s        cwd: %s",
		    module_name, hostname, curr_dir_name);
    /*  Start up X server connection, initialise XView  */
    server = xv_init (XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);

    my_display = (Display *) xv_get (server, XV_DISPLAY);
    /*  Get special colours  */
    scrn = DefaultScreen (my_display);
    special_colours[SPECIAL_COLOUR_MIN_SAT_INDEX] =BlackPixel(my_display,scrn);
    special_colours[SPECIAL_COLOUR_MAX_SAT_INDEX] =WhitePixel(my_display,scrn);
    special_colours[SPECIAL_COLOUR_BLANK_INDEX] = WhitePixel (my_display,scrn);
    /*  Initialise rest of module  */
    kview_2d_init (my_display);

    base_frame = (Frame) xv_create ( (Xv_object) NULL, FRAME,
				    FRAME_LABEL, header_name,
				    XV_WIDTH, DEFAULT_FRAME_WIDTH,
				    XV_HEIGHT, DEFAULT_FRAME_HEIGHT,
				    NULL );
    /*  Create the colourmap menu  */
    if ( ( colourmap_names = kcmap_list_funcs () ) == NULL )
    {
	(void) fprintf (stderr, "Error getting any supported colourmaps\n");
	exit (RV_UNDEF_ERROR);
    }
    colourmap_menu = (Menu) xv_create ( (Xv_object) NULL, MENU_CHOICE_MENU,
				       MENU_GEN_PIN_WINDOW, base_frame,
				       "Colourmap Menu",
				       NULL );
    cmap_name = colourmap_names;
    while (*cmap_name != NULL)
    {
	menu_item = (Menu_item) xv_create ( (Xv_object) NULL, MENUITEM,
					   MENU_STRING, *cmap_name,
					   MENU_NOTIFY_PROC, cmap_menu_proc,
					   MENU_RELEASE,
					   NULL );
	xv_set (colourmap_menu, MENU_APPEND_ITEM, menu_item, NULL);
	if (strcmp (*cmap_name, DEFAULT_COLOURMAP_NAME) == 0)
	{
	    /*  Default  */
	    xv_set (colourmap_menu, MENU_DEFAULT_ITEM, menu_item, NULL);
	    choosen_colourmap = *cmap_name;
	}
	++cmap_name;
    }
    if (choosen_colourmap == NULL)
    {
	(void) fprintf (stderr, "No default colourmap found\n");
	a_prog_bug (function_name);
    }
    menu_item = (Menu_item) xv_create ( (Xv_object) NULL, MENUITEM,
				       MENU_STRING, "Slave",
				       MENU_NOTIFY_PROC, cmap_menu_proc,
				       MENU_RELEASE,
				       NULL );
    xv_set (colourmap_menu, MENU_APPEND_ITEM, menu_item, NULL);
    menu_item = (Menu_item) xv_create ( (Xv_object) NULL, MENUITEM,
				       MENU_STRING, "Read",
				       MENU_NOTIFY_PROC, cmap_menu_proc,
				       MENU_RELEASE,
				       NULL );
    xv_set (colourmap_menu, MENU_APPEND_ITEM, menu_item, NULL);
    /*  Create the save colourmap menu  */
    savecolourmap_menu = (Menu) xv_create ( (Xv_object) NULL, MENU,
					   MENU_GEN_PIN_WINDOW, base_frame,
					   "Save Colourmap Menu",
					   NULL );
    menu_item = (Menu_item) xv_create ( (Xv_object) NULL, MENUITEM,
				       MENU_STRING, "Write to file",
				       MENU_NOTIFY_PROC, write_colourmap_proc,
				       MENU_RELEASE,
				       NULL );
    xv_set (savecolourmap_menu, MENU_APPEND_ITEM, menu_item, NULL);
    xv_set (savecolourmap_menu, MENU_DEFAULT_ITEM, menu_item, NULL);
#ifdef dummy
    menu_item = (Menu_item) xv_create ( (Xv_object) NULL, MENUITEM,
				       MENU_STRING, "Attach to arrayfile",
				       MENU_NOTIFY_PROC,
				       attach_colourmap_proc,
				       MENU_RELEASE,
				       NULL );
    xv_set (savecolourmap_menu, MENU_APPEND_ITEM, menu_item, NULL);
#endif

    /*  Create the zoom menu  */
    zoom_menu = (Menu) xv_create ( (Xv_object) NULL, MENU,
				  MENU_GEN_PIN_WINDOW, base_frame,
				  "Zoom Menu",
				  NULL );
    menu_item = (Menu_item) xv_create ( (Xv_object) NULL, MENUITEM,
				       MENU_STRING, "Zoom Horizontal",
				       MENU_NOTIFY_PROC, zoom_horizontal,
				       MENU_RELEASE,
				       NULL );
    xv_set (zoom_menu, MENU_APPEND_ITEM, menu_item, NULL);
    menu_item = (Menu_item) xv_create ( (Xv_object) NULL, MENUITEM,
				       MENU_STRING, "Zoom Vertical",
				       MENU_NOTIFY_PROC, zoom_vertical,
				       MENU_RELEASE,
				       NULL );
    xv_set (zoom_menu, MENU_APPEND_ITEM, menu_item, NULL);
    menu_item = (Menu_item) xv_create ( (Xv_object) NULL, MENUITEM,
				       MENU_STRING, "Zoom 2D",
				       MENU_NOTIFY_PROC, zoom_2d,
				       MENU_RELEASE,
				       NULL );
    xv_set (zoom_menu, MENU_APPEND_ITEM, menu_item, NULL);
    xv_set (zoom_menu, MENU_DEFAULT_ITEM, menu_item, NULL);
    menu_item = (Menu_item) xv_create ( (Xv_object) NULL, MENUITEM,
				       MENU_STRING, "Unzoom",
				       MENU_NOTIFY_PROC, unzoom,
				       MENU_RELEASE,
				       NULL );
    xv_set (zoom_menu, MENU_APPEND_ITEM, menu_item, NULL);

    /*  Install a panel  */
    panel = (Panel) xv_create (base_frame, PANEL, NULL);
    /*  Install buttons on the panel  */
    (void) xv_create (panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Files...",
		      PANEL_NOTIFY_PROC, scan_directory,
		      NULL);
    (void) xv_create (panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Colourmap",
		      PANEL_ITEM_MENU, colourmap_menu,
		      NULL);
    (void) xv_create (panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Save Colourmap",
		      PANEL_ITEM_MENU, savecolourmap_menu,
		      NULL);
    (void) xv_create (panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Colourbar",
		      PANEL_NOTIFY_PROC, show_colourbar_frame,
		      NULL);
    (void) xv_create (panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Track",
		      PANEL_NOTIFY_PROC, show_track_frame,
		      NULL);
    (void) xv_create (panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Zoom",
		      PANEL_ITEM_MENU, zoom_menu,
		      NULL);
    (void) xv_create (panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Clear Crosshairs",
		      PANEL_NOTIFY_PROC, clear_crosshairs,
		      NULL);
    (void) xv_create (panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Show Crosshair Position",
		      PANEL_NOTIFY_PROC, show_crosshair_positions,
		      NULL);
    (void) xv_create (panel, PANEL_TOGGLE,
		      PANEL_CHOICE_STRINGS, "Maintain Aspect Ratio", NULL,
		      PANEL_VALUE, 0,
		      PANEL_NOTIFY_PROC, choice_notify_proc,
		      NULL);
    (void) xv_create (panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, "Quit",
		      PANEL_NOTIFY_PROC, quit,
		      NULL);
    window_fit_height (panel);

    /*  Install a canvas  */
    canvas = (Canvas) xv_create (base_frame, CANVAS,
				 CANVAS_REPAINT_PROC,
				 image_window_repaint_proc,
				 CANVAS_X_PAINT_WINDOW, TRUE,
				 CANVAS_FIXED_IMAGE, FALSE,
				 CANVAS_AUTO_CLEAR, TRUE,
				 CANVAS_RESIZE_PROC, canvas_resize_proc,
				 CANVAS_RETAINED, FALSE,
				 NULL);
    /*  Determine canvas paint window  */
    if ( ( cnv_paint_window = canvas_paint_window (canvas) )
	== (Xv_Window) NULL )
    {
	(void) fprintf (stderr, "Error getting canvas paint window\n");
	exit (RV_UNDEF_ERROR);
    }
    /*  Install the event procedure on the paint window  */
    xv_set (cnv_paint_window, WIN_EVENT_PROC, paint_window_event,
	    WIN_CONSUME_EVENTS, WIN_MOUSE_BUTTONS, LOC_DRAG, NULL,
	    NULL);

    /*  Create the command frame  */
    cmd_frame = (Frame) xv_create (base_frame, FRAME_CMD,
				   FRAME_LABEL, "kview_2d File Selector",
				   NULL);
    /*  Command frames have panels already created by default: get it  */
    panel = (Panel) xv_get (cmd_frame, FRAME_CMD_PANEL);
    /*  Create the scrolling list of files  */
    directory_list = (Panel_item) xv_create (panel, PANEL_LIST,
					     PANEL_CHOOSE_ONE, TRUE,
					     PANEL_LIST_DISPLAY_ROWS, 20,
					     PANEL_LIST_WIDTH, 200,
					     PANEL_NEXT_COL, 50,
					     PANEL_NOTIFY_PROC, file_select,
					     NULL);
    if (conn_controlled_by_cm_tool () != TRUE) scan_directory (NULL, NULL);
    window_fit (panel);
    window_fit (cmd_frame);

    /*  Create the colourbar frame  */
    colourbar_frame = (Frame) xv_create (base_frame, FRAME_CMD,
					 FRAME_LABEL,
					 "kview_2d Colourbar Window",
					 FRAME_SHOW_RESIZE_CORNER, TRUE,
					 XV_WIDTH,
					 DEFAULT_COLOURBAR_FRAME_WIDTH,
					 XV_HEIGHT,
					 DEFAULT_COLOURBAR_FRAME_HEIGHT,
					 NULL);
    /*  Size pushpin panel only to the required size (vertical size only)  */
    /*  Must be done before canvas is created  */
    window_fit_height ( (Frame) xv_get (colourbar_frame, FRAME_CMD_PANEL) );
    /*  Install a canvas  */
    canvas = (Canvas) xv_create (colourbar_frame, CANVAS,
				 CANVAS_REPAINT_PROC, colourbar_repaint_proc,
				 CANVAS_X_PAINT_WINDOW, TRUE,
				 CANVAS_FIXED_IMAGE, FALSE,
				 CANVAS_AUTO_CLEAR, TRUE,
				 CANVAS_RESIZE_PROC, colourbar_resize_proc,
				 CANVAS_RETAINED, FALSE,
				 NULL);
    window_fit_height (colourbar_frame);
    /*  Determine canvas paint window  */
    if ( ( colourbar_paint_window = canvas_paint_window (canvas) )
	== (Xv_Window) NULL )
    {
	(void) fprintf (stderr, "Error getting canvas paint window\n");
	exit (RV_UNDEF_ERROR);
    }

    /*  Create the track window frame  */
    track_frame = (Frame) xv_create (base_frame, FRAME_CMD,
				     FRAME_LABEL, "kview_2d Track Window",
				     FRAME_SHOW_RESIZE_CORNER, TRUE,
				     XV_WIDTH, DEFAULT_TRACK_FRAME_WIDTH,
				     XV_HEIGHT, DEFAULT_TRACK_FRAME_HEIGHT,
				     FRAME_DONE_PROC, dismiss_track_frame,
				     NULL);
    /*  Size pushpin panel only to the required size (vertical size only)  */
    /*  Must be done before canvas is created  */
    window_fit_height ( (Panel) xv_get (track_frame, FRAME_CMD_PANEL) );
    /*  Install a canvas  */
    canvas = (Canvas) xv_create (track_frame, CANVAS,
				 CANVAS_REPAINT_PROC, track_frame_repaint_proc,
				 CANVAS_X_PAINT_WINDOW, TRUE,
				 CANVAS_FIXED_IMAGE, FALSE,
				 CANVAS_AUTO_CLEAR, FALSE,
				 CANVAS_RESIZE_PROC, track_frame_resize_proc,
				 CANVAS_RETAINED, FALSE,
				 NULL);
    window_fit_height (track_frame);
    /*  Determine canvas paint window  */
    if ( ( track_paint_window = canvas_paint_window (canvas) )
	== (Xv_Window) NULL )
    {
	(void) fprintf (stderr, "Error getting canvas paint window\n");
	exit (RV_UNDEF_ERROR);
    }

    /*  Create icon  */
    xv_set (base_frame, FRAME_ICON, create_icon (base_frame), NULL);

    /*  Set frame destruction function  */
    (void) notify_interpose_destroy_func (base_frame, base_frame_destroy_func);

    /*  Enter the main loop (Notifier)  */
    xv_main_loop (base_frame);
}   /*  End Function kview_2d_xview  */

void clear_crosshairs (item, event)
/*  This routine will clear the crosshairs.
*/
Panel_item item;
Event *event;
{
    extern struct dual_crosshair_type crosshairs;

    if (crosshairs.num_crosshairs == 0)
    {
	return;
    }
    /*  Remove the first crosshair  */
    draw_crosshair (crosshairs.first.x, crosshairs.first.y);
    if (crosshairs.num_crosshairs > 1)
    {
	/*  Remove second crosshair  */
	draw_crosshair (crosshairs.second.x, crosshairs.second.y);
    }
    crosshairs.num_crosshairs = 0;
}   /*  End Function clear_crosshairs  */

void zoom_horizontal (item, event)
/*  This routine will zoom the plot horizontally.
*/
Panel_item item;
Event *event;
{
    int width, height;
    struct win_scale_type win_scale;
    extern flag image_auto_x, image_auto_y, image_auto_v;
    extern flag maintain_aspect_ratio;
    extern KWorldCanvas image_worldcanvas;
    extern struct dual_crosshair_type crosshairs;

    if (crosshairs.num_crosshairs < 2)
    {
	/*  Clear the crosshairs (not enough to zoom)  */
	clear_crosshairs ();
	return;
    }
    if (crosshairs.first.x == crosshairs.second.x)
    {
	/*  Can't do an infinite zoom  */
	clear_crosshairs ();
	return;
    }
    /*  Zoom horizontal  */
    canvas_get_size (image_worldcanvas, &width, &height, &win_scale);
    image_auto_x = FALSE;
    if (crosshairs.first.x < crosshairs.second.x)
    {
	win_scale.x_min = crosshairs.first.x;
	win_scale.x_max = crosshairs.second.x;
    }
    else
    {
	win_scale.x_min = crosshairs.second.x;
	win_scale.x_max = crosshairs.first.x;
    }
    clear_crosshairs ();
    viewimg_control_autoscaling (image_worldcanvas,
				 image_auto_x, image_auto_y, image_auto_v,
				 TRUE, TRUE, maintain_aspect_ratio);
    /*  Resize the canvas  */
    if (canvas_resize (image_worldcanvas, &win_scale, FALSE) != TRUE)
    {
	(void) fprintf (stderr, "Error resizing window\n");
    }
}   /*  End Function zoom_horizontal  */

void zoom_vertical (item, event)
/*  This routine will zoom the plot vertically.
*/
Panel_item item;
Event *event;
{
    int width, height;
    struct win_scale_type win_scale;
    extern flag image_auto_x, image_auto_y, image_auto_v;
    extern flag maintain_aspect_ratio;
    extern KWorldCanvas image_worldcanvas;
    extern struct dual_crosshair_type crosshairs;

    if (crosshairs.num_crosshairs < 2)
    {
	/*  Clear the crosshairs (not enough to zoom)  */
	clear_crosshairs ();
	return;
    }
    if (crosshairs.first.y == crosshairs.second.y)
    {
	/*  Can't do an infinite zoom  */
	clear_crosshairs ();
	return;
    }
    /*  Zoom vertical  */
    canvas_get_size (image_worldcanvas, &width, &height, &win_scale);
    image_auto_y = FALSE;
    if (crosshairs.first.y < crosshairs.second.y)
    {
	win_scale.y_min = crosshairs.first.y;
	win_scale.y_max = crosshairs.second.y;
    }
    else
    {
	win_scale.y_min = crosshairs.second.y;
	win_scale.y_max = crosshairs.first.y;
    }
    clear_crosshairs ();
    viewimg_control_autoscaling (image_worldcanvas,
				 image_auto_x, image_auto_y, image_auto_v,
				 TRUE, TRUE, maintain_aspect_ratio);
    /*  Resize the canvas  */
    if (canvas_resize (image_worldcanvas, &win_scale, FALSE) != TRUE)
    {
	(void) fprintf (stderr, "Error resizing window\n");
    }
}   /*  End Function zoom_vertical  */

void zoom_2d (item, event)
/*  This routine will zoom the plot in both dimensions.
*/
Panel_item item;
Event *event;
{
    int width, height;
    struct win_scale_type win_scale;
    extern flag image_auto_x, image_auto_y, image_auto_v;
    extern flag maintain_aspect_ratio;
    extern KWorldCanvas image_worldcanvas;
    extern struct dual_crosshair_type crosshairs;

    if (crosshairs.num_crosshairs < 2)
    {
	/*  Clear the crosshairs (not enough to zoom)  */
	clear_crosshairs ();
	return;
    }
    if (crosshairs.first.x == crosshairs.second.x)
    {
	/*  Can't do an infinite zoom  */
	clear_crosshairs ();
	return;
    }
    if (crosshairs.first.y == crosshairs.second.y)
    {
	/*  Can't do an infinite zoom  */
	clear_crosshairs ();
	return;
    }
    /*  Zoom 2D  */
    canvas_get_size (image_worldcanvas, &width, &height, &win_scale);
    image_auto_x = FALSE;
    image_auto_y = FALSE;
    if (crosshairs.first.x < crosshairs.second.x)
    {
	win_scale.x_min = crosshairs.first.x;
	win_scale.x_max = crosshairs.second.x;
    }
    else
    {
	win_scale.x_min = crosshairs.second.x;
	win_scale.x_max = crosshairs.first.x;
    }
    if (crosshairs.first.y < crosshairs.second.y)
    {
	win_scale.y_min = crosshairs.first.y;
	win_scale.y_max = crosshairs.second.y;
    }
    else
    {
	win_scale.y_min = crosshairs.second.y;
	win_scale.y_max = crosshairs.first.y;
    }
    clear_crosshairs ();
    viewimg_control_autoscaling (image_worldcanvas,
				 image_auto_x, image_auto_y, image_auto_v,
				 TRUE, TRUE, maintain_aspect_ratio);
    /*  Resize the canvas  */
    if (canvas_resize (image_worldcanvas, &win_scale, FALSE) != TRUE)
    {
	(void) fprintf (stderr, "Error resizing window\n");
    }
}   /*  End Function zoom_2d  */

void unzoom (item, event)
/*  This routine will unzoom the plot.
*/
Panel_item item;
Event *event;
{
    extern flag image_auto_x, image_auto_y, image_auto_v;
    extern flag maintain_aspect_ratio;
    extern KWorldCanvas image_worldcanvas;

    clear_crosshairs ();
    /*  Unzoom  */
    image_auto_x = TRUE;
    image_auto_y = TRUE;
    if (!image_auto_v) (void) fprintf (stderr, "\nNO AUTO_V SCALING!!!\n\n");
    viewimg_control_autoscaling (image_worldcanvas,
				 image_auto_x, image_auto_y, image_auto_v,
				 TRUE, TRUE, maintain_aspect_ratio);
    /*  Redraw the canvas  */
    if (canvas_resize (image_worldcanvas, NULL, FALSE) != TRUE)
    {
	(void) fprintf (stderr, "Error refreshing window\n");
    }
}   /*  End Function unzoom  */

void show_crosshair_positions (item, event)
/*  This routine will display the crosshair position(s).
*/
Panel_item item;
Event *event;
{
    extern int canvas_width;
    extern int canvas_height;
    extern Xv_Window cnv_paint_window;
    extern struct dual_crosshair_type crosshairs;
    static char txt1[STRING_LENGTH];
    static char txt2[STRING_LENGTH];
    static char txt3[STRING_LENGTH];
    static char txt4[STRING_LENGTH];

    if (crosshairs.num_crosshairs < 1)
    {
	return;
    }
    if (crosshairs.num_crosshairs < 2)
    {
	/*  Only one crosshair to show position of  */
	(void) sprintf (txt1, "Crosshair: abscissa: %g  ordinate: %g",
			crosshairs.first.x, crosshairs.first.y);
	show_error_message (txt1);
	return;
    }
    /*  Two crosshairs to show position of  */
    (void) sprintf (txt1, "First crosshair: abscissa: %g  ordinate: %g",
		    crosshairs.first.x, crosshairs.first.y);
    (void) sprintf (txt2, "Second crosshair: abscissa: %g  ordinate: %g",
		    crosshairs.second.x, crosshairs.second.y);
    (void) sprintf (txt3, "Abscissa difference: %f",
		    fabs (crosshairs.second.x - crosshairs.first.x) );
    (void) sprintf (txt4, "Ordinate difference: %f",
		    fabs (crosshairs.second.y - crosshairs.first.y) );
    (void) notice_prompt (cnv_paint_window, NULL,
			  NOTICE_FOCUS_XY, canvas_width / 2, canvas_height / 2,
			  NOTICE_MESSAGE_STRINGS, txt1, txt2, txt3, txt4, NULL,
			  NOTICE_BUTTON_YES, "Continue",
			  NOTICE_NO_BEEPING, TRUE,
			  NULL);
}   /*  End Function show_crosshair_positions  */

int quit (item, event)
/*  This routine will save the size of the base frame in external variables and
    will then destroy any XView objects and cause the notifier to stop.
    Control is then returned to the  main  function.
*/
Panel_item item;
Event *event;
{
    extern Frame base_frame;
    static char function_name[] = "quit";

    (void) fprintf (stderr, "Removing shared memory segments...");
/*
    remove_shared_images ();
*/
    (void) fprintf (stderr, "\tremoved shared memory segments.\n");
    if (xv_destroy_safe (base_frame) != XV_OK)
    {
	(void) fprintf (stderr, "Error destroying base frame\n");
	a_prog_bug (function_name);
    }
    notify_stop ();
    return (XV_OK);
}   /*  End Function quit  */

void image_window_repaint_proc (canvas, paint_window, display, win, rects)
/*  This routine will plot the data.
*/
Canvas canvas;
Xv_Window paint_window;
Display *display;
Window win;
Xv_xrectlist *rects;
{
    Kdisplay dpy_handle;
    KWorldCanvas unit_canvas;
    XWindowAttributes window_attributes;
    XGCValues gcvalues;
    unsigned int num_ccels;
    struct win_scale_type win_scale;
    unsigned long *pixel_values;
    unsigned long *cms_colours;
    extern Kcolourmap image_cmap;
    extern int canvas_width;
    extern int canvas_height;
    extern KPixCanvas image_pixcanvas;
    extern KWorldCanvas image_worldcanvas;
    extern unsigned long special_colours[NUM_SPECIAL_COLOURS];
    extern struct win_scale_type plotted_scale;
    extern struct dual_crosshair_type crosshairs;
    extern GC image_gc;
    extern GC crosshair_gc;
    static flag first_time = TRUE;
    static char function_name[] = "image_window_repaint_proc";

    if (canvas_width < 1)
    {
	/*  Get around bug in XView which does not call the resize procedure
	    the first time  */
	canvas_width = (int) xv_get (paint_window, XV_WIDTH);
    }
    if (canvas_height < 1)
    {
	/*  Get around bug in XView which does not call the resize procedure
	    the first time  */
	canvas_height = (int) xv_get (paint_window, XV_HEIGHT);
    }
    if (first_time == TRUE)
    {
	/* First time  */
	/*  No graphics context created yet  */
	cms_colours = (unsigned long *) xv_get (paint_window,
						WIN_X_COLOR_INDICES);
	gcvalues.background = cms_colours[(int) xv_get (paint_window,
							WIN_BACKGROUND_COLOR)];
	gcvalues.foreground = cms_colours[(int) xv_get (paint_window,
							WIN_FOREGROUND_COLOR)];
	image_gc = XCreateGC (display, win, GCForeground | GCBackground,
			      &gcvalues);
	/*  Initialise colourmap  */
	XGetWindowAttributes (display, win, &window_attributes);
	if (window_attributes.colormap == None)
        {
            (void) fprintf (stderr, "Error: window has no colourmap\n");
            a_prog_bug (function_name);
        }
	/*  No graphics context for crosshairs created yet  */
	gcvalues.foreground = BlackPixel ( display, DefaultScreen (display) );
	gcvalues.background = WhitePixel ( display, DefaultScreen (display) );
	gcvalues.function = GXinvert;
	crosshair_gc = XCreateGC (display, win,
				  GCForeground | GCBackground | GCFunction,
				  &gcvalues);
	if ( ( dpy_handle = xc_get_dpy_handle (display,
					       window_attributes.colormap) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error getting display handle\n");
	    a_prog_bug (function_name);
        }
	if ( ( image_cmap = kcmap_create (DEFAULT_COLOURMAP_NAME,
					  MAX_COLOURS, TRUE,
					  dpy_handle) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error creating main colourmap\n");
	    a_prog_bug (function_name);
        }
	num_ccels = kcmap_get_pixels (image_cmap, &pixel_values);
	(void) fprintf (stderr, "num colours: %u\n", num_ccels);
	/*  Create the pixel canvas  */
	if ( ( image_pixcanvas = kwin_create_x (display, win, image_gc,
						0, 0,
						canvas_width, canvas_height) )
	    == NULL )
	{
	    exit (RV_UNDEF_ERROR);
	}
	/*  Create the unit canvas  */
	win_scale.x_min = 0.0;
	win_scale.x_max = 1.0;
	win_scale.y_min = 0.0;
	win_scale.y_max = 1.0;
	if ( ( unit_canvas = canvas_create (image_pixcanvas, image_cmap,
					    &win_scale) )
	    == NULL )
	{
	    exit (RV_UNDEF_ERROR);
	}
	canvas_register_position_event_func (unit_canvas,
					     unit_canvas_position_func,
					     (void *) image_cmap);
	/*  Create the main world canvas  */
	plotted_scale.x_log = FALSE;
	plotted_scale.y_log = FALSE;
	plotted_scale.z_scale = K_INTENSITY_SCALE_LINEAR;
	plotted_scale.conv_type = CONV1_REAL;
	plotted_scale.min_sat_pixel = special_colours[SPECIAL_COLOUR_MIN_SAT_INDEX];
	plotted_scale.max_sat_pixel = special_colours[SPECIAL_COLOUR_MAX_SAT_INDEX];
	plotted_scale.blank_pixel = special_colours[SPECIAL_COLOUR_BLANK_INDEX];
	if ( ( image_worldcanvas = canvas_create (image_pixcanvas, image_cmap,
						  &plotted_scale) )
	    == NULL )
	{
	    exit (RV_UNDEF_ERROR);
	}
	canvas_register_position_event_func (image_worldcanvas,
					     crosshair_click_consumer,
					     (void *) NULL);
	viewimg_register_position_event_func (image_worldcanvas,
					      track_canvas_event,
					      (void *) image_worldcanvas);
	first_time = FALSE;
    }
    kwin_resize (image_pixcanvas, FALSE, 0, 0, canvas_width, canvas_height);
    /*  Draw crosshairs (if neccessary)  */
    if (crosshairs.num_crosshairs > 0)
    {
	draw_crosshair (crosshairs.first.x, crosshairs.first.y);
	if (crosshairs.num_crosshairs > 1)
	{
	    draw_crosshair (crosshairs.second.x, crosshairs.second.y);
	}
    }
}   /*  End Function image_window_repaint_proc  */

void canvas_resize_proc (canvas, width, height)
/*  This routine is called whenever the canvas is resized. It stores the canvas
    width and height.
    It does NOT call the canvas repaint procedure. This is done by the
    notifier.
*/
Canvas canvas;
int width;
int height;
{
    extern int canvas_width;
    extern int canvas_height;
    extern Frame base_frame;
    extern struct dual_crosshair_type crosshairs;

    canvas_width = width;
    canvas_height = height;
    /*  Clear the crosshairs  */
/*
    crosshairs.num_crosshairs = 0;
*/
}   /*  End Function canvas_resize_proc  */

void paint_window_event (window, event)
/*  This routine is called whenever a mouse button is pressed in the
    paint window.
    The routine updates the global crosshair structure.
    The routine returns nothing.
*/
Xv_Window window;
Event *event;
{
    unsigned int event_code = K_CANVAS_EVENT_UNDEFINED;
    extern KPixCanvas image_pixcanvas;
    extern KWorldCanvas image_worldcanvas;
    extern int canvas_width;
    extern int canvas_height;
    extern int track_win_width;
    extern int track_win_height;
    extern unsigned int mouse_mode;
    extern Xv_Window track_paint_window;
    extern struct dual_crosshair_type crosshairs;
    static flag adjust_button_down = FALSE;
    static flag select_button_down = FALSE;
    static char function_name[] = "paint_window_event";

    switch ( event_action (event) )
    {
      case ACTION_SELECT:
	if (event_is_up (event) == TRUE)
	{
	    /*  Button released: ignore  */
	    select_button_down = FALSE;
	    return;
	}
	else
	{
	    select_button_down = TRUE;
	}
	event_code = K_CANVAS_EVENT_LEFT_MOUSE_CLICK;
	break;
      case ACTION_ADJUST:
	if (event_is_down (event) == TRUE)
	{
	    adjust_button_down = TRUE;
	    event_code = K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK;
	}
	else
	{
	    adjust_button_down = FALSE;
	}
	break;
      case ACTION_MENU:
	break;
      case LOC_DRAG:
	if (adjust_button_down == TRUE)
	{
	    event_code = K_CANVAS_EVENT_MIDDLE_MOUSE_DRAG;
	}
	else if (select_button_down == TRUE)
	{
	    if (mouse_mode == MOUSE_MODE_EDIT)
	    {
		event_code = K_CANVAS_EVENT_LEFT_MOUSE_DRAG;
	    }
	}
	break;
      case LOC_MOVE:
	event_code = K_CANVAS_EVENT_POINTER_MOVE;
	break;
      default:
	return;
/*
	break;
*/
    }
    if (event_code == K_CANVAS_EVENT_UNDEFINED) return;
    if (kwin_process_position_event (image_pixcanvas,
				     event_x (event), event_y (event),
				     TRUE, event_code,
				     (void *) NULL) != TRUE)
    {
	(void) fprintf (stderr, "Paint window event not consumed\n");
    }
}   /*  End Function paint_window_event  */

flag crosshair_click_consumer (canvas, x, y, event_code, e_info, f_info)
/*  This routine is a position event consumer for a world canvas.
    The canvas is given by  canvas  .
    The horizontal world co-ordinate of the event will be given by  x  .
    The vertical world co-ordinate of the event will be given by  y  .
    The arbitrary event code is given by  event_code  .
    The arbitrary event information is pointed to by  e_info  .
    The arbitrary function information pointer is pointed to by  f_info  .
    The routine returns TRUE if the event was consumed, else it return
    FALSE indicating that the event is still to be processed.
*/
KWorldCanvas canvas;
double x;
double y;
unsigned int event_code;
void *e_info;
void **f_info;
{
    extern unsigned int mouse_mode;
    extern struct dual_crosshair_type crosshairs;
    static char function_name[] = "crosshair_click_consumer";

    if (event_code != K_CANVAS_EVENT_LEFT_MOUSE_CLICK) return (FALSE);
    if (mouse_mode != MOUSE_MODE_CROSSHAIR) return (FALSE);
    switch (crosshairs.num_crosshairs)
    {
      case 0:
	/*  Set the first crosshairs  */
	crosshairs.first.x = x;
	crosshairs.first.y = y;
	++crosshairs.num_crosshairs;
	/*  Draw the first crosshairs  */
	draw_crosshair (crosshairs.first.x, crosshairs.first.y);
	break;
      case 1:
	/*  Set the second crosshairs  */
	crosshairs.second.x = x;
	crosshairs.second.y = y;
	++crosshairs.num_crosshairs;
	/*  Draw the second crosshairs  */
	draw_crosshair (crosshairs.second.x, crosshairs.second.y);
	break;
      case 2:
	/*  Remove the old first crosshairs on the canvas  */
	draw_crosshair (crosshairs.first.x, crosshairs.first.y);
	/*  Set the first crosshairs  */
	crosshairs.first.x = x;
	crosshairs.first.y = y;
	++crosshairs.num_crosshairs;
	/*  Draw the new first crosshairs  */
	draw_crosshair (crosshairs.first.x, crosshairs.first.y);
	break;
      case 3:
	/*  Remove the old second crosshairs on the canvas  */
	draw_crosshair (crosshairs.second.x, crosshairs.second.y);
	/*  Set the second crosshairs  */
	crosshairs.second.x = x;
	crosshairs.second.y = y;
	crosshairs.num_crosshairs = 2;
	/*  Draw the new second crosshairs  */
	draw_crosshair (crosshairs.second.x, crosshairs.second.y);
	break;
      default:
	(void) fprintf (stderr, "Number of crosshairs: %u is too large\n",
			crosshairs.num_crosshairs);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function crosshair_click_consumer  */

Icon create_icon (frame)
/*  This routine will create the application icon.
    The frame which the icon is to be owned by must be in  frame  .
    The icon will contain the name of the application at the bottom, and will
    contain a plot of a cubic function.
*/
Frame frame;
{
    int icon_width;
    int icon_height;
    int icon_text_height = 13;
    int num_icon_sizes;
    Display *display;
    Window window;
    Pixmap icon_pixmap;
    Icon icon;
    Server_image image;
    XIconSize *icon_size_list;
    extern Xv_Window cnv_paint_window;
    extern char module_name[STRING_LENGTH + 1];
    static char function_name[] = "create_icon";

    /*  Determine the display (needed later)  */
    if ( ( display = (Display *) xv_get (frame, XV_DISPLAY) ) == NULL )
    {
	(void) fprintf (stderr, "Error getting display ID\n");
	a_prog_bug (function_name);
    }
    /*  Determine X window ID of canvas paint window (to get icon size)  */
    window = xv_get (cnv_paint_window, XV_XID);
    /*  Determine window manager preferred icon sizes  */
    if (XGetIconSizes (display, window, &icon_size_list, &num_icon_sizes) == 0)
    {
	/*  No window manager preferences: use appliaction default  */
	icon_width = DEFAULT_ICON_WIDTH;
	icon_height = DEFAULT_ICON_HEIGHT;
    }
    else
    {
	/*  Just use the first icon size  */
	icon_width = icon_size_list[0].max_width;
	icon_height = icon_size_list[0].max_height;
	(void) fprintf (stderr, "Prefferred icon size: %d by %d\n",
			icon_width, icon_height);
    }
    image = (Server_image) xv_create ( (Xv_object) NULL, SERVER_IMAGE,
				      XV_WIDTH, icon_width,
				      XV_HEIGHT, icon_height -icon_text_height,
				      SERVER_IMAGE_DEPTH, 1,
				      NULL );
    /*  Extract pixmap ID for icon  */
    if ( ( icon_pixmap = (Pixmap) xv_get (image, XV_XID) ) == (Pixmap) NULL )
    {
	(void) fprintf (stderr, "Error getting XID of icon\n");
	a_prog_bug (function_name);
    }
    ic_write_kimage_icon (display, icon_pixmap, icon_width,
			    icon_height - icon_text_height - 1);
    icon = (Icon) xv_create (frame, ICON,
			     XV_WIDTH, icon_width,
			     XV_HEIGHT, icon_height,
			     XV_LABEL, module_name,
			     ICON_IMAGE, image,
			     NULL);
    return (icon);
}   /*  End Function create_icon  */

void show_error_message (string)
/*  This routine will display an error message from the centre of the canvas
    paint window using the NOTICE package.
    The string to write must be pointed to by  string  .
    The routine returns nothing.
*/
char *string;
{
    extern Xv_Window cnv_paint_window;
    extern int canvas_width;
    extern int canvas_height;

    (void) notice_prompt (cnv_paint_window, NULL,
			  NOTICE_FOCUS_XY, canvas_width / 2, canvas_height / 2,
			  NOTICE_MESSAGE_STRINGS, string, NULL,
			  NOTICE_BUTTON_YES, "Continue",
			  NULL);
}   /*  End Function show_error_message  */

void cmap_menu_proc (menu, menu_item)
/*  This routine will register the selection of a colourmap from the menu.
*/
Menu menu;
Menu_item menu_item;
{
    char *new_colourmap;
    multi_array *multi_desc;
    Menu_item new_menu_item;
    extern Kcolourmap image_cmap;
    extern char *choosen_colourmap;
    extern char *cmap_file;
    static char function_name[] = "cmap_menu_proc";

    new_colourmap = (char *) xv_get (menu_item, MENU_STRING);
    if (strcmp (new_colourmap, "Slave") == 0)
    {
	/*  Slave colourmap: try to connect  */
	if (attempt_slave_cmap_connection (FALSE) == TRUE)
	{
	    return;
	}
	/*  Failed: try to connect with the full protocol  */
	(void) attempt_slave_cmap_connection (TRUE);
	return;
    }
    if (strcmp (new_colourmap, "Read") == 0)
    {
	/*  Read colourmap  */
	if ( ( multi_desc = dsxfr_get_multi (cmap_file, FALSE, K_CH_MAP_NEVER,
					     FALSE) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error reading colourmap file\n");
	    return;
	}
	if (kcmap_copy_from_struct (image_cmap, (*multi_desc).headers[0],
				    (*multi_desc).data[0]) != TRUE)
	{
	    (void) fprintf (stderr, "Error getting colourmap data\n");
	    return;
	}
	ds_dealloc_multi (multi_desc);
	return;
    }
    choosen_colourmap = new_colourmap;
    if (kcmap_change (image_cmap, new_colourmap, 0, TRUE) != TRUE)
    {
	(void) fprintf (stderr, "Error changing to colourmap: \"%s\"\n",
			new_colourmap);
    }
}  /*  End Function cmap_menu_proc  */

void show_colourbar_frame (item, event)
/*  This routine will show the colourbar frame.
*/
Panel_item item;
Event *event;
{
    extern Frame colourbar_frame;

    xv_set (colourbar_frame, XV_SHOW, TRUE, NULL);
}   /*  End Function show_colourbar_frame  */

void colourbar_repaint_proc (canvas, paint_window, display, win, rects)
/*  This routine will plot the colourbar.
*/
Canvas canvas;
Xv_Window paint_window;
Display *display;
Window win;
Xv_xrectlist *rects;
{
    extern Kcolourmap image_cmap;
    extern int colourbar_win_width;
    extern int colourbar_win_height;
    extern Frame colourbar_frame;
    static KPixCanvas pixcanvas = NULL;

    /*  Set the frame busy attribute  */
    xv_set (colourbar_frame, FRAME_BUSY, TRUE, NULL);
    if (colourbar_win_width < 1)
    {
	/*  Get around bug in XView which does not call the resize procedure
	    the first time  */
	colourbar_win_width = (int) xv_get (paint_window, XV_WIDTH);
    }
    if (colourbar_win_height < 1)
    {
	/*  Get around bug in XView which does not call the resize procedure
	    the first time  */
	colourbar_win_height = (int) xv_get (paint_window, XV_HEIGHT);
    }
    if (pixcanvas == NULL)
    {
	if ( ( pixcanvas = kwin_create_x (display, win,
					  DefaultGC ( display,
						     DefaultScreen (display) ),
					  0, 0,
					  colourbar_win_width,
					  colourbar_win_height) )
	    == NULL )
	{
	    exit (RV_UNDEF_ERROR);
	}
	kwin_register_refresh_func (pixcanvas, refresh_colourbar,
				    (void *) image_cmap);
	kcmap_register_resize_func (image_cmap, colourbar_cmap_resize_func,
				    (void *) pixcanvas);
    }
    kwin_resize (pixcanvas, FALSE, 0, 0,
		 colourbar_win_width, colourbar_win_height);
    /*  Unset the frame busy attribute  */
    xv_set (colourbar_frame, FRAME_BUSY, FALSE, NULL);
}   /*  End Function colourbar_repaint_proc  */

void colourbar_resize_proc (canvas, width, height)
/*  This routine is called whenever the colourbar canvas is resized.
    It stores the canvas width and height.
    It does NOT call the canvas repaint procedure. This is done by the
    notifier.
*/
Canvas canvas;
int width;
int height;
{
    extern int colourbar_win_width;
    extern int colourbar_win_height;

    colourbar_win_width = width;
    colourbar_win_height = height;
}   /*  End Function colourbar_resize_proc  */

void show_track_frame ()
/*  This routine will show the track frame and set the event mask on the main
    canvas paint window.
    The routine returns nothing.
*/
{
    extern Frame track_frame;
    extern Xv_Window cnv_paint_window;

    /*  Show the track frame  */
    (void) xv_set (track_frame, XV_SHOW, TRUE, NULL);
    /*  Install the mouse move event procedure on the paint window  */
    xv_set (cnv_paint_window, WIN_EVENT_PROC, paint_window_event,
	    WIN_CONSUME_EVENTS, LOC_MOVE, NULL,
	    NULL);
}   /*  End Function show_track_frame  */

void dismiss_track_frame (track_frame)
/*  This routine will dismiss the track frame and clear the event mask on the
    main canvas paint window.
    The track frame must be given by  track_frame  .
    The routine returns nothing.
*/
Frame track_frame;
{
    extern Xv_Window cnv_paint_window;

    /*  Show the track frame  */
    (void) xv_set (track_frame, XV_SHOW, FALSE, NULL);
    /*  Install the mouse move event procedure on the paint window  */
    xv_set (cnv_paint_window, WIN_EVENT_PROC, paint_window_event,
	    WIN_IGNORE_EVENTS, LOC_MOVE, NULL,
	    NULL);
}   /*  End Function dismiss_track_frame  */

void track_frame_repaint_proc (canvas, paint_window, display, win, rects)
/*  This routine will display the track information.
*/
Canvas canvas;
Xv_Window paint_window;
Display *display;
Window win;
Xv_xrectlist *rects;
{
    extern int track_win_width;
    extern int track_win_height;

    if (track_win_width < 1)
    {
	/*  Get around bug in XView which does not call the resize procedure
	    the first time  */
	track_win_width = (int) xv_get (paint_window, XV_WIDTH);
    }
    if (track_win_height < 1)
    {
	/*  Get around bug in XView which does not call the resize procedure
	    the first time  */
	track_win_height = (int) xv_get (paint_window, XV_HEIGHT);
    }
    draw_track_window (display, win, track_win_width, track_win_height);
}   /*  End Function track_frame_repaint_proc  */

void track_frame_resize_proc (canvas, width, height)
/*  This routine is called whenever the track frame canvas is resized.
    It stores the canvas width and height.
    It does NOT call the canvas repaint procedure. This is done by the
    notifier.
*/
Canvas canvas;
int width;
int height;
{
    extern int track_win_width;
    extern int track_win_height;

    track_win_width = width;
    track_win_height = height;
}   /*  End Function track_frame_resize_proc  */

Notify_value base_frame_destroy_func (client, status)
/*  This routine is called whenever the base frame is about to be closed.
*/
Notify_client client;
Destroy_status status;
{
    static char function_name[] = "base_frame_destroy_func";

    /*  Do clean-up operations  */
/*
    remove_shared_images ();
*/
    switch (status)
    {
      case DESTROY_CLEANUP:
	return ( notify_next_destroy_func (client, status) );
	break;
      case DESTROY_CHECKING:
	break;
      case DESTROY_SAVE_YOURSELF:
	(void) fprintf (stderr, "SaveYourself\n");
	break;
      case DESTROY_PROCESS_DEATH:
	(void) fprintf (stderr, "ProcessDeath\n");
	break;
      default:
	(void) fprintf (stderr, "Illegal destroy status: %d\n", status);
	a_prog_bug (function_name);
	break;
    }
    (void) fprintf (stderr, "Removed shared memory segments\n");
    return (NOTIFY_DONE);
}   /*  End Function base_frame_destroy_func  */

void write_colourmap_proc (item, event)
/*  This routine will write the current colourmap to a file.
*/
Panel_item item;
Event *event;
{
    multi_array *multi_desc;
    extern Kcolourmap image_cmap;
    extern char *cmap_file;
    static char function_name[] = "write_colourmap_proc";

    if ( ( multi_desc = ds_alloc_multi (1) ) == NULL )
    {
	m_error_notify (function_name, "multi_array");
	return;
    }
    if (kcmap_copy_to_struct (image_cmap, (*multi_desc).headers,
			      (*multi_desc).data) != TRUE)
    {
	(void) fprintf (stderr, "Error copying colour data\n");
	ds_dealloc_multi (multi_desc);
	return;
    }
    if (dsxfr_put_multi (cmap_file, multi_desc) != TRUE)
    {
	(void) fprintf (stderr, "Error writing colourmap\n");
    }
    ds_dealloc_multi (multi_desc);
    return;
#ifdef dummy
    write_colourmap ();
#endif
}  /*  End Function write_colourmap_proc  */

void attach_colourmap_proc (item, event)
/*  This routine will attach the current colourmap to an arrayfile.
*/
Panel_item item;
Event *event;
{
#ifdef dummy
    attach_colourmap ();
#endif
}  /*  End Function attach_colourmap_proc  */

static void scan_directory (item, event)
/*  This routine will show the command frame
*/
Panel_item item;
Event *event;
{
    KDir directory;
    KFileInfo *entry;
    char *ch;
    extern Panel_item directory_list;
    extern Frame cmd_frame;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static int entry_count = 0;

    xv_set (cmd_frame, XV_SHOW, TRUE, NULL);
    if (entry_count > 0)
    {
	/*  Remove old entries  */
	while (--entry_count > -1)
	{
	    xv_set (directory_list, PANEL_LIST_DELETE, entry_count, NULL);
	}
    }
    entry_count = 0;
    xv_set (directory_list,
	    PANEL_LIST_INSERT, entry_count,
	    PANEL_LIST_STRING, entry_count, "connection",
	    NULL);
    ++entry_count;
    if ( ( directory = dir_open (".") ) == NULL )
    {
	(void) fprintf (stderr, "Error reading directory\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    while ( (entry = dir_read (directory, KDIR_NO_DOTS) ) != NULL )
    {
	if ( ( ch = strrchr ( (*entry).filename, '.' ) ) != NULL )
	{
	    if (strcmp (ch, ".kf") == 0)
	    {
		/*  Karma data file  */
		xv_set (directory_list,
			PANEL_LIST_INSERT, entry_count,
			PANEL_LIST_STRING, entry_count, (*entry).filename,
			NULL);
		++entry_count;
	    }
	}
    }
    (void) dir_close (directory);
}   /*  End Function scan_directory  */

char *get_selected_arrayfile ()
/*  This routine will return the current selected arrayfile.
*/
{
    int index;
    char *arrayfile;
    extern Panel_item directory_list;

    index = (int) xv_get (directory_list, PANEL_LIST_FIRST_SELECTED);
    arrayfile = (char *) xv_get (directory_list, PANEL_LIST_STRING, index);
    return (arrayfile);
}   /*  End Function get_selected_arrayfile  */

static int file_select (item, string, client_data, op, event, row)
/*  This routine will load an arrayfile.
*/
Panel_item item;
char *string;
Xv_opaque client_data;
Panel_list_op op;
Event *event;
int row;
{
    if (op != PANEL_LIST_OP_SELECT) return (XV_OK);
    if (load (string) != TRUE) return (XV_ERROR);
    return (XV_OK);
}   /*  End Function file_select  */

void kview_2d_init (display)
/*  This routine will initialise the kview_2d module.
    The display the module is connected to must be pointed to by  display  .
    The routine returns nothing.
*/
Display *display;
{
    Kdisplay dpy_handle;
    int def_port_number;
    unsigned int server_port_number;
    extern char module_name[STRING_LENGTH + 1];

    kcmap_init (xc_alloc_colours, xc_free_colours, xc_store_colours,
		xc_get_location);
    /*  Register channel managers (Notifier version)  */
    conn_register_managers (notify_chm_manage, notify_chm_unmanage,
			    register_server_exit);
    /*  Get default port number  */
    if ( ( def_port_number = r_get_def_port ( module_name,
					     DisplayString (display) ) ) < 0 )
    {
	(void) fprintf (stderr, "Could not get default port number\n");
	return;
    }
    server_port_number = def_port_number;
    if (conn_become_server (&server_port_number, CONN_MAX_INSTANCES) != TRUE)
    {
	(void) fprintf (stderr, "Module not operating as Karma server\n");
    }
    else
    {
	(void) fprintf (stderr, "Port allocated: %d\n", server_port_number);
	/*  Register the protocols  */
	dsxfr_register_connection_limits (1, -1);
	dsxfr_register_read_func (new_data_on_connection);
	dsxfr_register_close_func (connection_closed);
    }
}   /*  End Function kview_2d_init  */

static void register_server_exit ()
/*  This routine will register a message from a Karma client to exit.
    The routine returns nothing.
*/
{
    quit (NULL, NULL);
}   /*  End Function register_server_exit  */

static void choice_notify_proc (item, value, event)
/*  This routine will register an event on the choice items.
    The routine returns nothing.
*/
Panel_item item;
int value;
Event *event;
{
    extern flag image_auto_x, image_auto_y, image_auto_v;
    extern flag maintain_aspect_ratio;
    extern KWorldCanvas image_worldcanvas;

    maintain_aspect_ratio = (value == 0) ? FALSE : TRUE;
    viewimg_control_autoscaling (image_worldcanvas,
				 image_auto_x, image_auto_y, image_auto_v,
				 TRUE, TRUE, maintain_aspect_ratio);
    /*  Resize the canvas  */
    if (canvas_resize (image_worldcanvas, NULL, FALSE) != TRUE)
    {
	(void) fprintf (stderr, "Error resizing window\n");
    }
}   /*  End Function choice_notify_proc  */

static flag load (arrayfile)
/*  This routine will load an arrayfile.
*/
char *arrayfile;
{
    iarray image;
    iarray image_red, image_green, image_blue;
    char *cmap_packet;
    multi_array *multi_desc;
    packet_desc *cmap_pack_desc;
    extern KWorldCanvas image_worldcanvas;
    extern ViewableImage vimage;
    extern Kcolourmap image_cmap;
    static char function_name[] = "load";

    if ( ( multi_desc = dsxfr_get_multi (arrayfile, FALSE,
					 K_CH_MAP_IF_AVAILABLE, FALSE) )
	== NULL )
    {
	(void) fprintf (stderr, "Error getting arrayfile: \"%s\"\n",
			arrayfile);
	return (FALSE);
    }
    if ( (*multi_desc).num_arrays > 1 )
    {
	if ( ( image = iarray_get_from_multi_array (multi_desc, "Frame", 2,
						    (char **) NULL, NULL) )
	    == NULL )
	{
	    (void) fprintf (stderr,
			    "Error getting Intelligent Array: Frame\n");
	    ds_dealloc_multi (multi_desc);
	    return (FALSE);
	}
	(void) fprintf (stderr, "Read: \"%s\" with: %u structures\n",
			arrayfile, (*multi_desc).num_arrays);
    }
    else
    {
	if ( ( image = iarray_get_from_multi_array (multi_desc, NULL, 2,
						    (char **) NULL, NULL) )
	    == NULL )
	{
	    (void) fprintf (stderr,
			    "Error getting PseudoColour Intelligent Array\n");
	    (void) fprintf (stderr, "Trying TrueColour...\n");
	    if ( ( image_red =
		  iarray_get_from_multi_array (multi_desc, NULL,
					       2, (char **) NULL,
					       "Red Intensity") )
		== NULL )
	    {
		(void) fprintf (stderr, "Error getting red array\n");
		ds_dealloc_multi (multi_desc);
		return (FALSE);
	    }
	    if (iarray_type (image_red) != K_UBYTE)
	    {
		(void) fprintf (stderr, "Red array must be of type K_UBYTE\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		return (FALSE);
	    }
	    if ( ( image_green =
		  iarray_get_from_multi_array (multi_desc, NULL,
					       2, (char **) NULL,
					       "Green Intensity") )
		== NULL )
	    {
		(void) fprintf (stderr, "Error getting green array\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		return (FALSE);
	    }
	    if (iarray_type (image_green) != K_UBYTE)
	    {
		(void) fprintf (stderr,
				"Green array must be of type K_UBYTE\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		iarray_dealloc (image_green);
		return (FALSE);
	    }
	    if ( (*image_red).arr_desc != (*image_green).arr_desc )
	    {
		(void) fprintf (stderr,
				"Green array descriptor different than red\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		iarray_dealloc (image_green);
		return (FALSE);
	    }
	    if ( ( image_blue =
		  iarray_get_from_multi_array (multi_desc, NULL,
					       2, (char **) NULL,
					       "Blue Intensity") )
		== NULL )
	    {
		(void) fprintf (stderr, "Error getting blue array\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		iarray_dealloc (image_green);
		return (FALSE);
	    }
	    if (iarray_type (image_blue) != K_UBYTE)
	    {
		(void) fprintf (stderr,
				"Blue array must be of type K_UBYTE\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		iarray_dealloc (image_green);
		iarray_dealloc (image_blue);
		return (FALSE);
	    }
	    if ( (*image_red).arr_desc != (*image_blue).arr_desc )
	    {
		(void) fprintf (stderr,
				"Blue array descriptor different than red\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		iarray_dealloc (image_green);
		iarray_dealloc (image_blue);
		return (FALSE);
	    }
	    /*  Have RGB component arrays: create PseudoColour array  */
	    if ( ( image = iarray_create_2D (iarray_dim_length (image_red, 0),
					     iarray_dim_length (image_red, 1),
					     K_UBYTE) ) == NULL )
	    {
		m_error_notify (function_name, "PseudoColour image");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		iarray_dealloc (image_green);
		iarray_dealloc (image_blue);
		return (FALSE);
	    }
	    if (imc_24to8 (iarray_dim_length (image_red, 0) *
			   iarray_dim_length (image_red, 1),
			   (*image_red).data, (*image_green).data,
			   (*image_blue).data,
			   ds_get_packet_size
			   ( (* (*image_red).arr_desc ).packet ),
			   (*image).data, 1, MAX_COLOURS, 0,
			   &cmap_pack_desc, &cmap_packet) != TRUE)
	    {
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		iarray_dealloc (image_green);
		iarray_dealloc (image_blue);
		iarray_dealloc (image);
		return (FALSE);
	    }
	    iarray_dealloc (image_red);
	    iarray_dealloc (image_green);
	    iarray_dealloc (image_blue);
	    if (kcmap_copy_from_struct (image_cmap, cmap_pack_desc,cmap_packet)
		!= TRUE)
	    {
		(void) fprintf (stderr, "Error changing colourmap\n");
	    }
	}
    }
    if (vimage != NULL) viewimg_destroy (vimage);
    if ( ( vimage = viewimg_create_from_iarray (image_worldcanvas, image,
						FALSE) ) == NULL )
    {
        (void) fprintf (stderr, "Error getting ViewableImage from Iarray\n");
	iarray_dealloc (image);
	return (FALSE);
    }
    iarray_dealloc (image);
    if (viewimg_make_active (vimage) != TRUE)
    {
        (void) fprintf (stderr, "Error making ViewableImage active\n");
        return (FALSE);
    }
    load_cmap_for_frame (multi_desc);
    ds_dealloc_multi (multi_desc);
    return (TRUE);
}   /*  End Function load  */

static void new_data_on_connection (first_time_data)
/*  This routine is called when new data arrives.
    If data is arriving the first time, then  first_time_data  will be TRUE.
    The routine returns nothing.
*/
flag first_time_data;
{
    (void) fprintf (stderr, "new data...\n");
    (void) load ("connection");
}   /*  End Function new_data_on_connection  */

static void connection_closed (data_deallocated)
/*  This routine is called when the "multi_array" connection closes.
    If there was data on the connection, the value of  data_deallocated  will
    be TRUE.
    The routine returns nothing.
*/
flag data_deallocated;
{
    extern ViewableImage vimage;

    (void) fprintf (stderr, "conn close...\n");
    if (vimage != NULL) viewimg_destroy (vimage);
    (void) fprintf (stderr, "Destroyed viewable image...\n");
    vimage = NULL;
}   /*  End Function connection_closed  */

static void load_cmap_for_frame (multi_desc)
/*  This routine will attempt to load the colourmap for a frame.
    The data structure to get the colourmap from must be pointed to by
    multi_desc  .
    The routine returns nothing.
*/
multi_array *multi_desc;
{
    unsigned int cmap_index;
    unsigned int dummy;
    char txt[STRING_LENGTH];
    extern Kcolourmap image_cmap;
    static char function_name[] = "load_cmap_for_frame";

    /*  Check if colourmap update required  */
    if ( (*multi_desc).num_arrays > 1 )
    {
	/*  Check colourmap  */
	switch ( dummy = ds_f_array_name (multi_desc, "RGBcolourmap",
					  (char **) NULL, &cmap_index) )
	{
	  case IDENT_NOT_FOUND:
	    (void) fprintf (stderr, "Colourmap not found for frame\n");
	    break;
	  case IDENT_GEN_STRUCT:
	    /*  Got it!  */
	    if (kcmap_copy_from_struct (image_cmap,
					(*multi_desc).headers[cmap_index],
					(*multi_desc).data[cmap_index])
		!= TRUE)
	    {
		(void) fprintf (stderr, "Error changing colourmap\n");
	    }
	    break;
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr,
			    "Multiple RGBcolourmap structures found\n");
	    return;
/*
	    break;
*/
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value: %u from: ds_f_array_name\n",
			    dummy);
	    a_prog_bug (function_name);
	    break;
	}
    }
}   /*  End Function load_cmap_for_frame  */
