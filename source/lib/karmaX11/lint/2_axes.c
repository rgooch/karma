#include <karma_ax.h>
# 1 "" 
flag ax_plot_dressing (display, window, gc,
		       title_string, abscissa_label, ordinate_label,
		       title_font_name, axes_font_name, scale_font_name,
		       win_scale, max_log_cycles, error_notify_func)
Display *display;
Window window;
GC gc;
char *title_string;
char *abscissa_label;
char *ordinate_label;
char *title_font_name;
char *axes_font_name;
char *scale_font_name;
struct win_scale_type *win_scale;
unsigned int max_log_cycles;
void (*error_notify_func) ();
{
    return ( (flag) 0 );
}
flag ax_choose_scale (min, max, log, new_scale, max_log_cycles,
		      error_notify_func)
double min;
double max;
flag log;
struct scale_type *new_scale;
unsigned int max_log_cycles;
void (*error_notify_func) ();
{
    return ( (flag) 0 );
}
flag ax_draw_ordinate_scale (display, window, gc, font_info, win_scale,
			     max_log_cycles, error_notify_func)
Display *display;
Window window;
GC gc;
XFontStruct *font_info;
struct win_scale_type *win_scale;
unsigned int max_log_cycles;
void (*error_notify_func) ();
{
    return ( (flag) 0 );
}
flag ax_draw_abscissa_scale (display, window, gc, font_info, scale_offset,
			     win_scale, max_log_cycles, error_notify_func)
Display *display;
Window window;
GC gc;
XFontStruct *font_info;
int scale_offset;
struct win_scale_type *win_scale;
unsigned int max_log_cycles;
void (*error_notify_func) ();
{
    return ( (flag) 0 );
}
void ax_set_zoom_h_info (x1, x2, abs_zoomed, win_scale)
int x1;
int x2;
flag *abs_zoomed;
struct win_scale_type *win_scale;
{
}
void ax_set_zoom_v_info (y1, y2, ord_zoomed, win_scale)
int y1;
int y2;
flag *ord_zoomed;
struct win_scale_type *win_scale;
{
}
flag ax_unset_zoom_info (abs_zoomed, ord_zoomed)
flag *abs_zoomed;
flag *ord_zoomed;
{
    return ( (flag) 0 );
}
flag ax_verify_crosshair_location (x, y, win_scale)
int x;
int y;
struct win_scale_type *win_scale;
{
    return ( (flag) 0 );
}
double ax_choose_ord_intvl (input_interval)
double input_interval;
{
    return ( (double) 0 );
}
double ax_pixel_to_abscissa (x, win_scale)
int x;
struct win_scale_type *win_scale;
{
    return ( (double) 0 );
}
double ax_pixel_to_ordinate (y, win_scale)
int y;
struct win_scale_type *win_scale;
{
    return ( (double) 0 );
}
