/*LINTLIBRARY*/
/*  main.c

    This code provides XImage creation and manipulation routines.

    Copyright (C) 1992-1996  Richard Gooch

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

    This file contains the various utility routines for creating XImage data
  structures.


    Written by      Richard Gooch   21-SEP-1992

    Updated by      Richard Gooch   4-DEC-1992

    Updated by      Richard Gooch   10-MAY-1993: Added #ifdef to preclude
  definition of  shmat  on SGImips.

    Updated by      Richard Gooch   30-NOV-1993: Added test for XI_DISABLE_SHM
  environment variable.

    Updated by      Richard Gooch   10-AUG-1994: Fixed bug for VM XImages which
  allocated image data first (bad assumptions about pixel size made: broke with
  24 bit display).

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   9-NOV-1994: Modified to  #ifdef OS_SunOS
  for  shmat  declaration.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/xi/main.c

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Updated by      Richard Gooch   15-DEC-1994: Added notice when shared
  memory XImages are disabled by the XI_DISABLE_SHM environment variable.

    Updated by      Richard Gooch   8-SEP-1995: Created <xi_create_shm_image>.

    Updated by      Richard Gooch   21-FEB-1996: Only display message about
  XI_DISABLE_SHM environment variable once.

    Updated by      Richard Gooch   16-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <karma_xi.h>
#include <os.h>
#ifdef HAS_SYSV_SHARED_MEMORY
#  include <sys/ipc.h>
#  include <sys/shm.h>
#  include <X11/extensions/XShm.h>
#endif
#include <errno.h>
#include <karma.h>
#include <karma_r.h>
#include <karma_m.h>
#include <karma_a.h>


#define SHARED_MEMORY_TEST_BUFFER_SIZE 16384

/*  External functions: looks like SunOS is the odd one out  */
#ifdef OS_SunOS
extern char *shmat ();
#endif
EXTERN_FUNCTION (int XShmGetEventBase, (Display *display) );


/*  Private functions  */
#ifdef HAS_SYSV_SHARED_MEMORY
static Bool check_if_shm_event ();
#endif  /*  HAS_SYSV_SHARED_MEMORY  */
STATIC_FUNCTION (XImage *create_unshared_image,
		 (Display *display, XWindowAttributes window_attributes,
		  unsigned int width, unsigned int height, flag *share) );


/*  Public routines follow  */

/*PUBLIC_FUNCTION*/
flag xi_check_shared_images_available (Display *display)
/*  [SUMMARY] Test if shared memory XImages may be use with an X server.
    <display> The X server.
    [RETURNS] TRUE if shared images are available, else FALSE.
*/
{
#ifdef HAS_SYSV_SHARED_MEMORY
    flag local;
    char *display_string;
    char *Xserver_host;
    XShmSegmentInfo shminfo;
    extern char *sys_errlist[];
    static char *env_name = "XI_DISABLE_SHM";
    static flag first_time = TRUE;
    static flag env_disable = FALSE;
    static char function_name[] = "xi_check_shared_images_available";

    if (first_time)
    {
	first_time = FALSE;
	if (r_getenv (env_name) != NULL)
	{
	    (void) fprintf (stderr,
			    "Environment variable: \"%s\" found: SHM XImages disabled\n",
			    env_name);
	    env_disable = TRUE;
	    return (FALSE);
	}
    }
    if (env_disable) return (FALSE);
    /*  Check if X server is running on the same machine  */
    if ( ( display_string = DisplayString (display) ) == NULL )
    {
	(void) fprintf (stderr, "Error getting my DISPLAY string\n");
	a_prog_bug (function_name);
    }
    if ( ( Xserver_host = r_get_host_from_display (display_string) )
	== NULL )
    {
	(void) fprintf (stderr, "Error getting hostname of X server\n");
	a_prog_bug (function_name);
    }
    if (r_get_inet_addr_from_host (Xserver_host, &local) == 0 )
    {
	(void) fprintf (stderr,
			"Error getting Internet address of X server\n");
	a_prog_bug (function_name);
    }
    if ( (local != TRUE) || (XShmQueryExtension (display) != True) )
    {
	/*  Shared memory images not available for this X server  */
	return (FALSE);
    }
    /*  Shared memory images available for this X server  */
    /*  Test to see if a shared memory segment can be allocated: this is
	necessary as the kernel may be configured without shared memory  */
    if ( ( shminfo.shmid = shmget (IPC_PRIVATE,
				   SHARED_MEMORY_TEST_BUFFER_SIZE,
				   IPC_CREAT | 0777) ) < 0 )
    {
	return (FALSE);
    }
    /*  Shared memory is turned on in this kernel  */
    if (shmctl (shminfo.shmid, IPC_RMID, (struct shmid_ds *) NULL) != 0)
    {
	(void) fprintf (stderr,
			"Error removing shared memory segment\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);

#else  /*  HAS_SYSV_SHARED_MEMORY  */
    return (FALSE);
#endif  /*  HAS_SYSV_SHARED_MEMORY  */
}   /*  End Function xi_check_shared_images_available  */

/*PUBLIC_FUNCTION*/
XImage *xi_create_image (Display *display, Window window,
			 unsigned int image_width, unsigned int image_height,
			 flag *share)
/*  [SUMMARY] Create an XImage structure for a window.
    <display> The X display.
    <window> The X window ID.
    <image_width> The width of the desired image in pixels.
    <image_height> The height of the desired image in pixels.
    <share> If this points to the value TRUE, the routine will attempt to use
    a shared memory segment for the image data (as per the MIT-Shared Memory
    Extension to the X Protocol). If the routine did in fact
    allocate a shared memory segment, the value of TRUE will be written here,
    else FALSE will be written here.
    [NOTE] Do NOT request a shared image if the routine
    [<xi_shared_images_available>] did not return TRUE.
    [RETURNS] A pointer to the image structure on success, else NULL.
*/
{
    XImage *ximage;
    XWindowAttributes window_attributes;
    static char function_name[] = "xi_create_image";

    if ( (display == NULL) || (share == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Get window attributes (to get visual structure)  */
    XGetWindowAttributes (display, window, &window_attributes);
    if (!*share)
    {
	return ( create_unshared_image (display, window_attributes,
					image_width, image_height, share) );
    }
    /*  Shared memory images requested  */
    if ( ( ximage = xi_create_shm_image (display, window_attributes.visual,
					 window_attributes.depth,
					 image_width, image_height, FALSE) )
	== NULL )
    {
	(void) fprintf (stderr,
			"Falling back to normal memory for this image\n");
	return ( create_unshared_image (display, window_attributes,
					image_width, image_height, share) );
    }
    return (ximage);
}   /*  End Function xi_create_image  */

/*PUBLIC_FUNCTION*/
XImage *xi_create_shm_image (Display *display, Visual *visual, int depth,
			     unsigned int width, unsigned int height,
			     flag quiet)
/*  [SUMMARY] Create a shared-memory XImage structure.
    <display> The X11 display handle.
    <visual> The Visual pointer.
    <depth> The depth.
    <width> The width of the image.
    <height> The height of the image.
    <quiet> If TRUE, no error messages are displayed if no shared memory XImage
    could be created.
    [RETURNS] An XImage pointer on success, else NULL.
*/
{
    unsigned int image_bytes;
    XImage *ximage;
#ifdef HAS_SYSV_SHARED_MEMORY
    XShmSegmentInfo *shminfo;
#endif
    extern char *sys_errlist[];
    static char function_name[] = "xi_create_shm_image";

    if (display == NULL)
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
#ifdef HAS_SYSV_SHARED_MEMORY
    if ( ( shminfo = (XShmSegmentInfo *) m_alloc (sizeof *shminfo) )
	== NULL )
    {
	m_error_notify (function_name, "shared memory segment info");
	return (NULL);
    }
    if ( ( ximage = XShmCreateImage (display, visual, depth, ZPixmap, NULL,
				     shminfo, width, height) ) == NULL )
    {
	m_error_notify (function_name, "XShmImage structure");
	return (NULL);
    }
    image_bytes = ximage->bytes_per_line * height;
    if ( ( shminfo->shmid = shmget (IPC_PRIVATE, (int) image_bytes,
				    IPC_CREAT | 0777) ) == -1 )
    {
	if (!quiet)
	{
	    (void) fprintf (stderr,
			    "Error creating shared memory segment of size: %d bytes\t%s\n",
			    ximage->bytes_per_line * height,
			    sys_errlist[errno]);
	}
	XDestroyImage (ximage);
	m_free ( (char *) shminfo );
	return (NULL);
    }
    /*  I first had the server attach, and then attached locally.
	But the anal retentive Dec Alpha-OSF/1 system seems to have
	some limit on number of shared memory segments, which caused
	the X server to die. The idea is to let the client catch the failure
	message first, since the X server drops the connection to the client if
	it couldn't attach.
	This idea works sometimes only.  */
    /*  Must allow the server to write to the segment,
	otherwise there will be an error when detaching the segment
	from the process.
	Probably a bug in SunOS.  */
    shminfo->readOnly = False;
    /*  Attach locally  */
    if ( (long) ( shminfo->shmaddr = ximage->data =
		 (char *) shmat ( shminfo->shmid, (char *) 0, 0 ) )
	== -1 )
    {
	if (!quiet)
	{
	    (void) fprintf (stderr,
			    "Error attaching to shared memory segment\t%s\n",
			    sys_errlist[errno]);
	}
	XDestroyImage (ximage);
	if (shmctl ( shminfo->shmid, IPC_RMID,
		    (struct shmid_ds *) NULL) != 0)
	{
	    (void) fprintf (stderr,
			    "Error removing shared memory segment\t%s\n",
			    sys_errlist[errno]);
	}
	m_free ( (char *) shminfo );
	return (NULL);
    }
    /*  Make the X server attach  */
    if (XShmAttach (display, shminfo) != True)
    {
	if (!quiet) (void) fprintf (stderr, "Error attaching\n");
	XDestroyImage (ximage);
	if (shmdt ( shminfo->shmaddr ) == -1)
	{
	    (void) fprintf (stderr,
			    "Error detaching shared memory segment\t%s\n",
			    sys_errlist[errno]);
	}
	if (shmctl ( shminfo->shmid, IPC_RMID,
		    (struct shmid_ds *) NULL ) != 0)
	{
	    (void) fprintf (stderr,
			    "Error removing shared memory segment\t%s\n",
			    sys_errlist[errno]);
	}
	m_free ( (char *) shminfo );
	return (NULL);
    }
    (void) XSync (display, False);
    /*  Now remove the segment (really queing the removal)  */
    if (shmctl ( shminfo->shmid, IPC_RMID, (struct shmid_ds *) NULL) != 0)
    {
	(void) fprintf (stderr,
			"Error removing shared memory segment: %d\t%s\n",
			shminfo->shmid, sys_errlist[errno]);
	(void) XShmDetach (display, shminfo);
	XDestroyImage (ximage);
	m_free ( (char *) shminfo );
	return (NULL);
    }
    return (ximage);

#else  /*  HAS_SYSV_SHARED_MEMORY  */
    if (quiet) return (NULL);
    /*  Shared memory not supported on this platform  */
    (void) fprintf (stderr,
		    "Operating system does not have shared memory segments\n");
    return (NULL);
#endif  /*  HAS_SYSV_SHARED_MEMORY  */
}   /*  End Function xi_create_shm_image  */

/*PUBLIC_FUNCTION*/
void xi_destroy_image (Display *display, XImage *ximage, flag shared_memory)
/*  [SUMMARY] Destroy all the memory allocated for an XImage structure.
    <display> The X display.
    <ximage> A pointer to the XImage structure.
    <shared_memory> If TRUE, then the routine assumes the XImage data is a
    shared memory segment, and will attempt to remove it.
    [RETURNS] Nothing.
*/
{
#ifdef HAS_SYSV_SHARED_MEMORY
    XShmSegmentInfo *shminfo;
#endif
    extern char *sys_errlist[];
    static char function_name[] = "xi_destroy_image";

    if (display == NULL)
    {
	(void) fprintf (stderr, "NULL display pointer passed\n");
	a_prog_bug (function_name);
    }
    if (ximage == NULL)
    {
	(void) fprintf (stderr, "NULL XImage pointer passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (shared_memory);
    if (shared_memory == TRUE)
    {
	/*  Shared memory segment claimed to have been used  */
#ifdef HAS_SYSV_SHARED_MEMORY
	shminfo = (XShmSegmentInfo *) ximage->obdata;
	/*  Tell X server to detach  */
	(void) XShmDetach (display, shminfo);
	/*  Detach shared memory segment  */
	if (shmdt ( shminfo->shmaddr ) != 0)
	{
	    (void) fprintf (stderr,
			    "Error detaching shared memory segment at address: %p\t%s\n",
			    shminfo->shmaddr, sys_errlist[errno]);
	}
	m_free ( (char *) shminfo );
	/*  Remove image  */
	ximage->obdata = NULL;
	ximage->data = NULL;
	XDestroyImage (ximage);

#else  /*  HAS_SYSV_SHARED_MEMORY  */
	(void) fprintf (stderr,
			"Operating system does not have shared memory segments\n");
	a_prog_bug (function_name);
#endif  /*  HAS_SYSV_SHARED_MEMORY  */
    }
    else
    {
	/*  Regular image  */
	(void) fprintf (stderr, "Ximage being destroyed: %p\n", ximage);
	m_free ( (char *) ximage->data );
	ximage->data = NULL;
	XDestroyImage (ximage);
    }
}   /*  End Function xi_destroy_image  */

/*PUBLIC_FUNCTION*/
void xi_put_image (Display *display, Drawable drawable, GC gc, XImage *ximage,
		   int src_x, int src_y, int dest_x, int dest_y,
		   unsigned int width, unsigned int height, flag shared_memory,
		   flag wait)
/*  [SUMMARY] Write an XImage structure to a drawable.
    <display> The X display.
    <drawable> The ID of the drawable.
    <gc> The graphics context.
    <ximage> The image structure.
    <src_x> The horizontal position of the upper left corner of the subimage.
    <src_y> The vertical position of the upper left corner of the subimage.
    <dest_x> The horizontal position in the drawable where the upper left
    corner will be drawn.
    <dest_y> The vertical position in the drawable where the upper left
    corner will be drawn.
    <width> The width of the subimage.
    <height> The height of the subimage.
    <shared_memory> If TRUE the routine assmumes the XImage structure resides
    in a shared memory segment.
    <wait> If TRUE and the XImage structure resides in a shared memory segment
    the routine will wait for the X server to copy the contents from the
    segment.
    [RETURNS] Nothing.
*/
{
#ifdef HAS_SYSV_SHARED_MEMORY
    XEvent event;
    static int CompletionType = -1;
#else
    static char function_name[] = "xi_put_image";
#endif

#ifdef HAS_SYSV_SHARED_MEMORY
    if (CompletionType == -1)
    {
	CompletionType = XShmGetEventBase (display) + ShmCompletion;
    }
    if (shared_memory == TRUE)
    {
	XShmPutImage (display, drawable, gc, ximage, src_x, src_y,
		      dest_x, dest_y, width, height,
		      (wait == TRUE) ? True : False);
	if (wait == TRUE)
	{
	    /*  Must wait for completion event  */
	    XIfEvent (display, &event, check_if_shm_event,
		      (char *) &CompletionType);
	}
	/*  Everything OK  */
	return;
    }

#else  /*  HAS_SYSV_SHARED_MEMORY  */
    if (shared_memory)
    {
	(void) fprintf (stderr,
			"Operating system does not have shared memory segments\n");
	a_prog_bug (function_name);
    }
#endif  /*  HAS_SYSV_SHARED_MEMORY  */

    /*  Ordinary XImage  */
    XPutImage (display, drawable, gc, ximage, src_x, src_y, dest_x, dest_y,
	       width, height);
}   /*  End Function xi_put_image  */

#ifdef HAS_SYSV_SHARED_MEMORY
static Bool check_if_shm_event (Display *display, XEvent *event,
				int *CompletionType)
{
    if (*CompletionType == event->type)
    {
	return (True);
    }
    else
    {
	return (False);
    }
}   /*  End Function check_if_shm_event */
#endif  /*  HAS_SYSV_SHARED_MEMORY  */

static XImage *create_unshared_image (Display *display,
				      XWindowAttributes window_attributes,
				      unsigned int width, unsigned int height,
				      flag *share)
/*  This routine will create an unshared XImage structure.
    The display must be pointed to by  display  .
    The window attributes must be given by  window_attributes  .
    The image width must be given by  width  .
    The image height must be given by  height  .
    The value  FALSE  is written to the storage pointed to by  share  .
    The routine returns the XImage structure on success, else it returns NULL.
*/
{
    unsigned int image_bytes;
    char *image_data;
    XImage *ximage;
    static char function_name[] = "create_unshared_image";

    *share = FALSE;
    if ( ( ximage = XCreateImage (display, window_attributes.visual,
				  window_attributes.depth, ZPixmap, 0,
				  NULL, width, height, 32, 0) )
	== NULL ) return (NULL);
    image_bytes = ximage->bytes_per_line * height;
    /*  Allocate actual image storage area  */
    if ( ( image_data = m_alloc (image_bytes) ) == NULL )
    {
	m_error_notify (function_name, "XImage storage");
	XFree (ximage);
	return (NULL);
    }
    ximage->data = image_data;
    return (ximage);
}   /*  End Function create_unshared_image  */
