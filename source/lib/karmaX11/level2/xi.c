/*LINTLIBRARY*/
/*PREFIX:"xi_"*/
/*  xi.c

    This code provides XImage creation and manipulation routines.

    Copyright (C) 1992,1993  Richard Gooch

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

    Last updated by Richard Gooch   10-MAY-1993: Added #ifdef to preclude
  definition of  shmat  on SGImips.


*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <karma_xi.h>
#include "os.h"
#ifdef HAS_SYSV_SHARED_MEMORY
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif
#include "karma.h"
#include <karma_r.h>
#include <karma_m.h>

#define SHARED_MEMORY_TEST_BUFFER_SIZE 16384

/*  External functions  */
#if defined(HAS_SYSV_SHARED_MEMORY) && !defined(ARCH_SGImips)
extern char *shmat ();
#endif

/*  Private functions  */
#ifdef HAS_SYSV_SHARED_MEMORY
static Bool check_if_shm_event ();
#endif  /*  HAS_SYSV_SHARED_MEMORY  */

/*PUBLIC_FUNCTION*/
flag xi_check_shared_images_available (display)
/*  This routine will determine if shared memory XImage structures may be used
    with a particular server.
    The X server must be pointed to by  display  .
    The routine returns TRUE if shared images are available, FALSE if not.
*/
Display *display;
{
#ifdef HAS_SYSV_SHARED_MEMORY
    flag local;
    char *display_string;
    char *Xserver_host;
    XShmSegmentInfo shminfo;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "xi_check_shared_images_available";

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
XImage *xi_create_image (display, window, image_width, image_height, share)
/*  This routine will create an image structure for a window.
    The display must be given by  display  and the window must be given by
    window  .
    The width of the desired image in pixels must be given by  image_width  .
    The height of the desired image in pixels must be given by  image_height  .
    If the value of the logical pointed to by  share  is TRUE, the routine will
    attempt to use a shared memory segment for the image data (as per the
    MIT-Shared Memory Extension to the X Protocol). If the routine did in fact
    allocate a shared memory segment, the value of TRUE will be written to the
    storage pointed to by  share  ,else FALSE will be written here.
    Do NOT request a shared image if the routine  shared_images_available
    did not return TRUE.
    The routine returns a pointer to the image structure on success,
    else it returns NULL.
*/
Display *display;
Window window;
unsigned int image_width;
unsigned int image_height;
flag *share;
{
    unsigned int image_bytes;
    unsigned int bits_per_line;
    unsigned int pad_bits;
    char *image_data;
    XWindowAttributes window_attributes;
#ifdef HAS_SYSV_SHARED_MEMORY
    XImage *ximage;
    XShmSegmentInfo *shminfo;
#endif
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "xi_create_image";

    if ( (display == NULL) || (share == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Get window attributes (to get visual structure)  */
    XGetWindowAttributes (display, window, &window_attributes);

    if (*share == TRUE)
    {
	/*  Shared memory images requested  */
#ifdef HAS_SYSV_SHARED_MEMORY
	if ( ( shminfo = (XShmSegmentInfo *) m_alloc (sizeof *shminfo) )
	    == NULL )
	{
	    m_error_notify (function_name, "shared memory segment info");
	    return (NULL);
	}
	if ( ( ximage = XShmCreateImage (display, window_attributes.visual,
					 window_attributes.depth, ZPixmap,
					 NULL,
					 shminfo, image_width, image_height) )
	    == NULL )
	{
	    m_error_notify (function_name, "XShmImage structure");
	    return (NULL);
	}
	if ( ( (*shminfo).shmid = shmget (IPC_PRIVATE,
					  (int) ( (*ximage).bytes_per_line *
						 image_height ),
					  IPC_CREAT | 0777) ) < 0 )
	{
	    (void) fprintf (stderr,
			    "Error creating shared memory segment of size: %d bytes\t%s\n",
			    (*ximage).bytes_per_line * image_height,
			    sys_errlist[errno]);
	    XDestroyImage (ximage);
	    (void) fprintf (stderr,
			    "Falling back to normal memory for this image\n");
	    m_free ( (char *) shminfo );
	}
	else
	{
	    /*  Must allow the server to write to the segment,
		otherwise there will be an error when detaching the segment
		from the process.
		Probably a bug in SunOS.  */
	    (*shminfo).readOnly = False;
	    /*  Make the X server attach  */
	    if (XShmAttach (display, shminfo) != True)
	    {
		(void) fprintf (stderr, "Error attaching\n");
		XDestroyImage (ximage);
		if (shmctl ( (*shminfo).shmid, IPC_RMID,
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
	    /*  Attach locally  */
	    if ( (int) ( (*shminfo).shmaddr = (*ximage).data =
			(char *) shmat ( (*shminfo).shmid, (char *) 0, 0 ) )
		== -1 )
	    {
		(void) XShmDetach (display, shminfo);
		(void) fprintf (stderr,
				"Error attaching to shared memory segment\t%s\n",
				sys_errlist[errno]);
		XDestroyImage (ximage);
		if (shmctl ( (*shminfo).shmid, IPC_RMID,
			    (struct shmid_ds *) NULL) != 0)
		{
		    (void) fprintf (stderr,
				    "Error removing shared memory segment\t%s\n",
				    sys_errlist[errno]);
		}
		m_free ( (char *) shminfo );
		return (NULL);
	    }
	    /*  Now remove the segment (really queing the removal)  */
	    if (shmctl ( (*shminfo).shmid, IPC_RMID, (struct shmid_ds *) NULL)
		!= 0)
	    {
		(void) fprintf (stderr,
				"Error removing shared memory segment: %d\t%s\n",
				(*shminfo).shmid, sys_errlist[errno]);
	    }
	    return (ximage);
	}

#else  /*  HAS_SYSV_SHARED_MEMORY  */
	/*  Shared memory not supported on this platform  */
	(void) fprintf (stderr,
			"Operating system does not have shared memory segments\n");
#endif  /*  HAS_SYSV_SHARED_MEMORY  */
    }
    /*  Shared memory images not available or not requested  */
    *share = FALSE;
    bits_per_line = image_width * window_attributes.depth;
    if ( (pad_bits = bits_per_line % 32) != 0 )
    {
	/*  Must pad out line to 32 bit boundary  */
	pad_bits = 32 - pad_bits;
    }
    image_bytes = ( (bits_per_line + pad_bits) / 8 ) * image_height;
    /*  Allocate actual image storage area  */
    if ( ( image_data = m_alloc (image_bytes) ) == NULL )
    {
	m_error_notify (function_name, "XImage storage");
	return (NULL);
    }
    return ( XCreateImage (display, window_attributes.visual,
			   window_attributes.depth, ZPixmap, 0,
			   image_data, image_width, image_height, 32, 0) );
}   /*  End Function xi_create_image  */

/*PUBLIC_FUNCTION*/
void xi_destroy_image (display, ximage, shared_memory)
/*  This routine will destroy all the memory allocated for an XImage structure.
    The display must be pointed to by  display  .
    The XImage structure must be pointed to by  ximage  .
    If the value of  shared_memory  is TRUE, then the routine assumes the
    XImage data is a shared memory segment, and will attempt to remove it.
    The routine returns nothing.
*/
Display *display;
XImage *ximage;
flag shared_memory;
{
#ifdef HAS_SYSV_SHARED_MEMORY
    XShmSegmentInfo *shminfo;
#endif
    ERRNO_TYPE errno;
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
	shminfo = (XShmSegmentInfo *) (*ximage).obdata;
	/*  Tell X server to detach  */
	(void) XShmDetach (display, shminfo);
	/*  Detach shared memory segment  */
	if (shmdt ( (*shminfo).shmaddr ) != 0)
	{
	    (void) fprintf (stderr,
			    "Error detaching shared memory segment at address: %x\t%s\n",
			    (*shminfo).shmaddr, sys_errlist[errno]);
	}
	m_free ( (char *) shminfo );
	/*  Remove image  */
	(*ximage).obdata = NULL;
	(*ximage).data = NULL;
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
	(void) fprintf (stderr, "Ximage being destroyed: %x\n", ximage);
	m_free ( (char *) (*ximage).data );
	(*ximage).data = NULL;
	XDestroyImage (ximage);
    }
}   /*  End Function xi_destroy_image  */

/*PUBLIC_FUNCTION*/
void xi_put_image (display, drawable, gc, ximage, src_x, src_y, dest_x, dest_y,
		   width, height, shared_memory, wait)
/*  This routine will write an XImage structure to a drawable.
    The display must be pointed to by  display  .
    The ID of the drawable must be given by  drawable  .
    The graphics context must be given by  gc  .
    The image structure must be pointed to by  ximage  .
    The upper left corner of the subimage in the image must be given by  src_x
    and  src_y  .
    The upper left corner of the subimage will be positioned at the drawable
    co-ordinates given by  dest_x  and  dest_y  .
    The width and height of the subimage must be given by  width  and  height
    ,respectively.
    If the XImage structure resides in a shared memory segment, the value of
    shared_memory  must be TRUE, else it should be FALSE.
    If the XImage structure resides in a shared memory segment and the routine
    should wait for the X server to copy the contents from the segment, then
    the value of  wait  should be TRUE.
    The routine returns nothing.
*/
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
#ifdef HAS_SYSV_SHARED_MEMORY
    XEvent event;
    static int CompletionType = -1;
#endif
    static char function_name[] = "xi_put_image";

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
    if (shared_memory == TRUE)
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
static Bool check_if_shm_event (display, event, CompletionType)
Display *display;
XEvent *event;
int *CompletionType;
{
    if (*CompletionType == (*event).type)
    {
	return (True);
    }
    else
    {
	return (False);
    }
}   /*  End Function check_if_shm_event */
#endif  /*  HAS_SYSV_SHARED_MEMORY  */
