#include <karma_xi.h>
# 1 "" 
flag xi_check_shared_images_available (display)
Display *display;
{
    return ( (flag) 0 );
}
XImage *xi_create_image (display, window, image_width, image_height, share)
Display *display;
Window window;
unsigned int image_width;
unsigned int image_height;
flag *share;
{
    return ( (XImage *) 0 );
}
void xi_destroy_image (display, ximage, shared_memory)
Display *display;
XImage *ximage;
flag shared_memory;
{
}
void xi_put_image (display, drawable, gc, ximage, src_x, src_y, dest_x, dest_y,
		   width, height, shared_memory, wait)
Display *display;
Drawable drawable;
GC gc;
XImage *ximage;
int src_x;
int src_y;
int dest_x;
int dest_y;
unsigned int width;
unsigned int height;
flag shared_memory;
flag wait;
{
}
