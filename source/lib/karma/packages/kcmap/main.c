/*LINTLIBRARY*/
/*  main.c

    This code provides Kcolourmap objects.

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

    This file contains the various utility routines for managing read/write
    colourcells in pseudocolour colourmaps.


    Written by      Richard Gooch   23-FEB-1993

    Updated by      Richard Gooch   4-MAR-1993

    Updated by      Richard Gooch   4-APR-1993: Took account of change to
  conn_register_server_protocol  and  conn_register_client_protocol  .

    Updated by      Richard Gooch   13-APR-1993: Moved from level 7 to level 5.

    Updated by      Richard Gooch   8-AUG-1993: Fixed bug in
  kcmap_copy_from_struct  which could result in a memory leak on error.

    Updated by      Richard Gooch   24-AUG-1993: Fixed bug  change_cmap_size
  which allocated oversized data structure when not all colours allocated.

    Updated by      Richard Gooch   21-SEP-1993: Improved error message in
  kcmap_change  when colourmap resize fails.

    Updated by      Richard Gooch   12-NOV-1993: Added colourmap functions
  submitted by Tom Oosterlo.

    Updated by      Richard Gooch   22-NOV-1993: Added colourmap function
  submitted by Jeanne Young.

    Updated by      Richard Gooch   30-NOV-1993: Added warning message in
  kcmap_copy_from_struct  if reordering done.

    Updated by      Richard Gooch   20-JUL-1994: Added some CONST casts.

    Updated by      Richard Gooch   19-AUG-1994: Created  kcmap_get_attributes
  and  kcmap_set_attributes  .

    Updated by      Richard Gooch   22-AUG-1994: Added support for
  KCMAP_ATT_INVERT  attribute.

    Updated by      Richard Gooch   26-AUG-1994: Moved  typedef  of Kcolourmap
  class to header.

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/kcmap/main.c

    Updated by      Richard Gooch   29-NOV-1994: Made use of  c_  package.

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Updated by      Richard Gooch   3-JAN-1995: Fixed  kcmap_modify  to
  correctly reverse colourmap.

    Updated by      Richard Gooch   31-JAN-1995: Added <cf_mandelbrot>.

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   7-MAY-1995: Placate gcc -Wall

    Updated by      Richard Gooch   1-SEP-1995: Created <kwin_va_create>.

    Updated by      Richard Gooch   28-OCT-1995: Added *_DPY_HANDLE attribute.

    Updated by      Richard Gooch   30-MAR-1996: Moved remaing functions to new
  documentation style.

    Updated by      Richard Gooch   30-APR-1996: Fixed experimental direct
  colourmap support.

    Updated by      Richard Gooch   18-MAY-1996: Fixed software colourmap
  support.

    Updated by      Richard Gooch   5-JUN-1996: Added colour scaling attributes
  and made use of them.

    Updated by      Richard Gooch   23-JUN-1996: Added <cf_greyscale3> to the
  list.

    Last updated by Richard Gooch   1-OCT-1996: Added <cf_rgb2> to the list.


*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <karma.h>
#include <karma_kcmap.h>
#include <karma_conn.h>
#include <karma_dsrw.h>
#include <karma_pio.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_cf.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_c.h>


#define MAGIC_NUMBER (unsigned int) 2140872384
#define PROTOCOL_VERSION (unsigned int) 0
#define MAX_INTENSITY 65535

#define VERIFY_COLOURMAP(cmap) if (cmap == NULL) \
{(void) fprintf (stderr, "NULL colourmap passed\n"); \
 a_prog_bug (function_name); } \
if (cmap->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid colourmap object\n"); \
 a_prog_bug (function_name); }

#define CMAP_FUNC_TYPE_RGB       (unsigned int) 0
#define CMAP_FUNC_TYPE_CIE       (unsigned int) 1
#define CMAP_FUNC_TYPE_GREYSCALE (unsigned int) 2

struct colourmap_type
{
    unsigned int magic_number;
    Kdisplay dpy_handle;
    unsigned int (*alloc_func) ();
    void (*free_func) ();
    void (*store_func) ();
    void (*location_func) ();
    unsigned int size;
    unsigned long *pixel_values;
    unsigned short *intensities;
    packet_desc *top_pack_desc;
    char *top_packet;
    struct cmap_func_type *modify_func;
    KCallbackList resize_list;
    Connection master;
    flag full_cmap;
    flag modifiable;
    flag reverse;
    flag invert;
    flag software;
    flag direct_visual;
    unsigned short red_scale;
    unsigned short green_scale;
    unsigned short blue_scale;
};

struct cmap_func_type
{
    unsigned int type;
    char *name;
    void (*func) (unsigned int num_cells, unsigned short *reds,
		  unsigned short *greens, unsigned short *blues,
		  unsigned int stride, double x, double y, void *var_param);
    unsigned int min_cells;
    unsigned int max_cells;
    struct cmap_func_type *next;
};


/*  Private data  */
static unsigned int (*obsolete_alloc_ccells_func) () = NULL;
static void (*obsolete_free_ccells_func) () = NULL;
static void (*obsolete_store_ccells_func) () = NULL;
static void (*obsolete_get_location_func) () = NULL;

static struct cmap_func_type *cmap_functions = NULL;
static Kcolourmap shareable_colourmap = NULL;
static Kcolourmap slaveable_colourmap = NULL;


/*  Private functions  */
STATIC_FUNCTION (void initialise, () );
static struct cmap_func_type *get_cmap_function (/* name */);
static flag register_new_cmap_indices_slave (/* connection, info */);
static flag register_new_full_cmap_slave (/* connection, info */);
static flag verify_indices_slave_cmap_connection (/* info */);
static flag verify_full_slave_cmap_connection (/* info */);
static flag register_cmap_indices_connection (/* connection, info */);
static flag register_full_cmap_connection (/* connection, info */);
static void register_cmap_connection_close (/* connection, info */);
static flag write_cmap_indices (/* connection, cmap */);
static flag read_cmap_indices (/* connection, cmap */);
static flag write_full_cmap (/* connection, cmap */);
static flag read_full_cmap (/* connection, cmap */);
static void notify_cmap_resize (/* cmap */);
static void notify_cmap_modify (/* cmap */);
static flag change_cmap_size (/* cmap, num_cells, tolerant, notify,
			       colour_array, pack_desc, packet */);


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
Kcolourmap kcmap_va_create (CONST char *name, unsigned int num_cells,
			    flag tolerant,
			    Kdisplay dpy_handle, unsigned int (*alloc_func) (),
			    void (*free_func) (), void (*store_func) (),
			    void (*location_func) (), ...)
/*  [SUMMARY] Create a high level colourmap.
    [PURPOSE] This routine will create a high level colourmap. The colourmap
    may be associated with a hardware colourmap, or it may be a software-only
    colourmap. In either case, storage for an array of pixel values and arrays
    of colour components is maintained within the colourmap object.
    <name> The name of the function used to initialise the colour values. If
    this is NULL, the default "Greyscale1" function is used.
    <num_cells> The initial number of colourcells to allocate. This must not be
    less than 2.
    <tolerant> If TRUE the routine will try to allocate as many colourcells as
    possible (up to <<num_cells>>), else it will fail if it could not
    allocate all required colourcells.
    <dpy_handle> The low level display handle. The meaning of this value
    depends on the lower level graphics library used.
    <alloc_func> The function which must be called in order to allocate
    colourcells. See the [<xc>] routines for examples. The prototype function
    is [<KCMAP_PROTO_alloc_func>].
    <free_func> The function which must be called to free colourcells. The
    The prototype function is [<KCMAP_PROTO_free_func>].
    <store_func> The function which is used to store colours into a low level
    colourmap. The prototype function is [<KCMAP_PROTO_store_func>].
    <location_func> The function which is used to determine the location of a
    display. The prototype function is [<KCMAP_PROTO_location_func>].
    [NOTE] If the above routines are NULL, the colourmap created is assumed to
    be a software colourmap, otherwise it is considered to be a
    hardware/virtual colourmap. A software colourmap, while maintaining storage
    for an array of pixel values, does not generate the pixel values. The
    [<kcmap_get_pixels>] routine may be used to modify the array of pixel
    values in a software colourmap.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. This list must be terminated with the value
    KCMAP_ATT_END. See [<KCMAP_ATTRIBUTES>] for a list of defined attributes.
    [RETURNS] A colourmap on success, else NULL.
*/
{
    va_list argp;
    Kcolourmap cmap;
    unsigned int min_cells;
    unsigned int att_key;
    unsigned int num_null;
    struct cmap_func_type *cmap_func;
    extern Kcolourmap shareable_colourmap;
    extern Kcolourmap slaveable_colourmap;
    static char *def_name = "Greyscale1";
    static char function_name[] = "kcmap_va_create";

    va_start (argp, location_func);
    initialise ();
    /*  It's all or nothing...  */
    num_null = 0;
    if (alloc_func == NULL) ++num_null;
    if (free_func == NULL) ++num_null;
    if (store_func == NULL) ++num_null;
    if (location_func == NULL) ++num_null;
    if ( (num_null != 0) && (num_null != 4) )
    {
	(void) fprintf (stderr, "Number of NULL functions: %u\n", num_null);
	a_prog_bug (function_name);
    }
    if (num_cells < 2)
    {
	(void) fprintf (stderr, "Must specify colourmap size of at least 2\n");
	a_prog_bug (function_name);
    }
    if (name == NULL) name = def_name;
    /*  Verify if colourmap function exists  */
    if ( (cmap_func = get_cmap_function (name) ) == NULL )
    {
	(void) fprintf (stderr, "Colourmap function: \"%s\" does not exist\n",
			name);
	a_prog_bug (function_name);
    }
    if ( (cmap_func->min_cells > 1) && (num_cells < cmap_func->min_cells) )
    {
	(void) fprintf (stderr, "Requested number of cells: %u is less than\n",
			num_cells);
	(void) fprintf (stderr,
			"minimum number of cells: %u for colourmap function: %s\n",
			cmap_func->min_cells, cmap_func->name);
	return (NULL);
    }
    if ( (cmap_func->max_cells > 1) && (num_cells > cmap_func->max_cells) )
    {
	(void) fprintf (stderr,
			"Requested number of cells: %u is greater than\n",
			num_cells);
	(void) fprintf (stderr,
			"maximum number of cells: %u for colourmap function: %s\n",
			cmap_func->max_cells, cmap_func->name);
	return (NULL);
    }
    if ( ( cmap = (Kcolourmap) m_alloc (sizeof *cmap) ) == NULL )
    {
	m_error_notify (function_name, "colourmap");
	return (NULL);
    }
    cmap->magic_number = MAGIC_NUMBER;
    cmap->dpy_handle = dpy_handle;
    if (num_null > 0)
    {
	cmap->alloc_func = ( unsigned int (*) () ) NULL;
	cmap->free_func = ( void (*) () ) NULL;
	cmap->store_func = ( void (*) () ) NULL;
	cmap->location_func = ( void (*) () ) NULL;
    }
    else
    {
	cmap->alloc_func = alloc_func;
	cmap->free_func = free_func;
	cmap->store_func = store_func;
	cmap->location_func = location_func;
    }
    cmap->size = 0;
    cmap->pixel_values = NULL;
    cmap->intensities = NULL;
    min_cells = tolerant ? cmap_func->min_cells : num_cells;
    if ( !change_cmap_size (cmap, num_cells, min_cells, FALSE,
			    (unsigned short *) NULL,
			    (packet_desc *) NULL, (char *) NULL) )
    {
	m_error_notify (function_name, "array of pixel values");
	cmap->magic_number = 0;
	m_free ( (char *) cmap );
	return (NULL);
    }
    cmap->modify_func = cmap_func;
    cmap->resize_list = NULL;
    cmap->master = NULL;
    cmap->modifiable = TRUE;
    /* Must initialise the reverse flag prior to  kcmap_modify  being called */
    cmap->reverse = FALSE;
    cmap->invert = FALSE;
    cmap->software = (num_null > 0) ? TRUE : FALSE;
    cmap->direct_visual = FALSE;
    cmap->red_scale = MAX_INTENSITY;
    cmap->green_scale = MAX_INTENSITY;
    cmap->blue_scale = MAX_INTENSITY;
    if (shareable_colourmap == NULL)
    {
	/*  Register this colourmap as the shareable one  */
	shareable_colourmap = cmap;
    }
    if (slaveable_colourmap == NULL)
    {
	/*  Register this colourmap as the slaveable one  */
	slaveable_colourmap = cmap;
    }
    while ( ( att_key = va_arg (argp, unsigned int) ) != KCMAP_ATT_END )
    {
	switch (att_key)
	{
	  case KCMAP_ATT_DIRECT_VISUAL:
	    cmap->direct_visual = va_arg (argp, flag);
	    break;
	  case KCMAP_ATT_RED_SCALE:
	    cmap->red_scale = va_arg (argp, unsigned short);
	    break;
	  case KCMAP_ATT_GREEN_SCALE:
	    cmap->green_scale = va_arg (argp, unsigned short);
	    break;
	  case KCMAP_ATT_BLUE_SCALE:
	    cmap->blue_scale = va_arg (argp, unsigned short);
	    break;
	  default:
	    (void) fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    if (cmap->direct_visual)
    {
	kcmap_modify_direct_type (cmap,
				  (double) 0.5, (double) 0.5, (void *) NULL,
				  (double) 0.5, (double) 0.5, (void *) NULL,
				  (double) 0.5, (double) 0.5, (void *) NULL);
    }
    else kcmap_modify (cmap, (double) 0.5, (double) 0.5, (void *) NULL);
    return (cmap);
}   /*  End Function kcmap_va_create  */

/*OBSOLETE_FUNCTION*/
void kcmap_init ( unsigned int (*alloc_func) (), void (*free_func) (),
		 void (*store_func) (), void (*location_func) () )
/*  This routine will initialise the high level colourmap package. This must be
    called before any other  kcmap_  routines.
    The function which must be called in order to allocate colourcells must be
    pointed to by  alloc_func  .See the  xc_  routines.
    The interface to this routine is as follows:

    unsigned int alloc_func (num_cells, pixel_values, min_cells, dpy_handle)
    *   This routine will allocate a number of colourcells in a low level
        colourmap (eg. using the Xlib routine XAllocColorCells).
	The number of colourcells to allocate must be given by  num_cells  .
	The pixel values allocated will be written to the array pointed to by
	pixel_values  .
	The minimum number of colourcells to allocate must be given by
	min_cells  .The routine will try to allocate at least this number of
	colourcells.
	The low level display handle must be given by  dpy_handle  .The meaning
	of this value depends on the lower level graphics library used.
	The routine returns the number of colourcells allocated.
    *
    unsigned int num_cells;
    unsigned long *pixel_values;
    unsigned int min_cells;
    Kdisplay dpy_handle;

    The function which must be called to free colourcells must be pointed to by
    free_func  .
    The interface to this routine is as follows:

    void free_func (num_cells, pixel_values, dpy_handle)
    *   This routine will free a number of colourcells in a low level colourmap
        The number of colourcells to free must be given by  num_cells  .
        The pixel values (colourcells) to free must be pointed to by
	pixel_values  .
        The low level display handle must be given by  dpy_handle  .The meaning
	of this value depends on the lower level graphics library used.
	The routine returns nothing.
    *
    unsigned int num_cells;
    unsigned long *pixel_values;
    Kdisplay dpy_handle;

    The function which is used to store colours into a low level colourmap must
    be pointed to by  store_func  .
    The interface to this routine is as follows:

    void store_func (num_cells, pixel_values, reds, greens, blues, stride,
                     dpy_handle)
    *   This routine will store colours into a low level colourmap.
        The number of colourcells to store must be given by  num_cells  .
	The pixel values must be pointed to by  pixel_values  .
	The red intensity values must be pointed to by  reds  .
	The green intensity values must be pointed to by  greens  .
	The blue intensity values must be pointed to by  blues  .
	The stride (in unsigned shorts) between intensity values in each array
	must be given by  stride  .
	The low level display handle must be given by  dpy_handle  .The	meaning
	of this value depends on the lower level graphics library used.
	The routine returns nothing.
    *
    unsigned int num_cells;
    unsigned long *pixel_values;
    unsigned short *reds;
    unsigned short *greens;
    unsigned short *blues;
    unsigned int stride;
    Kdisplay dpy_handle;

    The function which is used to determine the location of a display must be
    pointed to by  location_func  .
    The interface to this routine is as follows:

    void location_func (dpy_handle, serv_hostaddr, serv_display_num)
    *   This routine will determine the location of the graphics display being
        used.
	The low level display handle must be given by  dpy_handle  .The	meaning
	of this value depends on the lower level graphics library used.
	The Internet address of the host on which the display is running will
	be written to the storage pointed to by  serv_hostaddr  .
	The number of the display will be written to the storage pointed to by
	serv_display_num  .
	The routine returns nothing.
    *
    Kdisplay dpy_handle;
    unsigned long *serv_hostaddr;
    unsigned long *serv_display_num;

    The routine returns nothing.
*/
{
    extern unsigned int (*obsolete_alloc_ccells_func) ();
    extern void (*obsolete_free_ccells_func) ();
    extern void (*obsolete_store_ccells_func) ();
    extern void (*obsolete_get_location_func) ();
    static char function_name[] = "kcmap_init";

    if (obsolete_alloc_ccells_func != NULL)
    {
	(void) fprintf (stderr, "Initialisation already performed\n");
	a_prog_bug (function_name);
    }
    (void) fprintf (stderr,
		    "WARNING: the <%s> routine will be removed in Karma ",
		    function_name);
    (void)fprintf(stderr,
		  "version 2.0\nUse the <kcmap_va_create> routine instead.\n");
    obsolete_alloc_ccells_func = alloc_func;
    obsolete_free_ccells_func = free_func;
    obsolete_store_ccells_func = store_func;
    obsolete_get_location_func = location_func;
}   /*  End Function kcmap_init  */

/*PUBLIC_FUNCTION*/
void kcmap_add_RGB_func (CONST char *name, void (*func) (),
			 unsigned int min_cells, unsigned int max_cells)
/*  [SUMMARY] Register a new colourmap function.
    [PURPOSE] This routine will register a named function which will compute
    RGB intensity values for a colourmap. This function is typically called in
    response to a call to [<kcmap_modify>].
    <name> The name of the colourmap function.
    <func> The function which is used to compute the RGB values. The prototype
    function is [<KCMAP_PROTO_colour_func>].
    <min_cells> The minimum number of colourcells that must be allocated for
    this function to work. If this is less than 2, no minimum is defined.
    <max_cells> The maximum number of colourcells that may be allocated for
    this function to work. If this is less than 2, no maximum is defined.
    [RETURNS] Nothing.
*/
{
    struct cmap_func_type *new_entry;
    extern struct cmap_func_type *cmap_functions;
    static char function_name[] = "kcmap_add_RGB_func";

    if ( ( new_entry = (struct cmap_func_type *) m_alloc (sizeof *new_entry) )
	== NULL )
    {
	m_abort (function_name, "new function entry");
    }
    new_entry->type = CMAP_FUNC_TYPE_RGB;
    if ( ( new_entry->name = st_dup (name) ) == NULL )
    {
	m_abort (function_name, "new function entry name");
    }
    new_entry->func = func;
    new_entry->min_cells = min_cells;
    new_entry->max_cells = max_cells;
    /*  Insert at top of list  */
    new_entry->next = cmap_functions;
    cmap_functions = new_entry;
}   /*  End Function kcmap_add_RGB_func  */

/*EXPERIMENTAL_FUNCTION*/
void kcmap_add_grey_func (CONST char *name, void (*func) (),
			  unsigned int min_cells, unsigned int max_cells)
/*  [SUMMARY] Register a new colourmap function.
    [PURPOSE] This routine will register a named function which will compute
    RGB intensity values for a colourmap. This function is typically called in
    response to a call to [<kcmap_modify>].
    <name> The name of the colourmap function.
    <func> The function which is used to compute the RGB values. The prototype
    function is [<KCMAP_PROTO_grey_func>].
    <min_cells> The minimum number of colourcells that must be allocated for
    this function to work. If this is less than 2, no minimum is defined.
    <max_cells> The maximum number of colourcells that may be allocated for
    this function to work. If this is less than 2, no maximum is defined.
    [RETURNS] Nothing.
*/
{
    struct cmap_func_type *new_entry;
    extern struct cmap_func_type *cmap_functions;
    static char function_name[] = "kcmap_add_grey_func";

    if ( ( new_entry = (struct cmap_func_type *) m_alloc (sizeof *new_entry) )
	== NULL )
    {
	m_abort (function_name, "new function entry");
    }
    new_entry->type = CMAP_FUNC_TYPE_GREYSCALE;
    if ( ( new_entry->name = st_dup (name) ) == NULL )
    {
	m_abort (function_name, "new function entry name");
    }
    new_entry->func = func;
    new_entry->min_cells = min_cells;
    new_entry->max_cells = max_cells;
    /*  Insert at top of list  */
    new_entry->next = cmap_functions;
    cmap_functions = new_entry;
}   /*  End Function kcmap_add_grey_func  */

/*OBSOLETE_FUNCTION*/
Kcolourmap kcmap_create (CONST char *name, unsigned int num_cells,
			 flag tolerant, Kdisplay dpy_handle)
/*  This routine will create a high level colourmap.
    The function used to initialise the colour values must be pointed to by
    name  .If this is NULL, the default "Greyscale1" function is used.
    The initial number of colourcells to allocate must be given by  num_cells
    This must not be less than 2.
    If the flag  tolerant  is TRUE, then the routine will try to allocate
    as many colourcells as possible (up to  num_cells  ), else it will
    fail if it could not allocatate all required colourcells.
    The low level display handle must be given by  dpy_handle  .The meaning of
    this value depends on the lower level graphics library used.
    The routine returns a colourmap on success, else it returns NULL.
*/
{
    extern unsigned int (*obsolete_alloc_ccells_func) ();
    extern void (*obsolete_free_ccells_func) ();
    extern void (*obsolete_store_ccells_func) ();
    extern void (*obsolete_get_location_func) ();
    static char function_name[] = "kcmap_create";

    if (obsolete_alloc_ccells_func == NULL)
    {
	(void) fprintf (stderr,
			"Lower level display routines not registered yet\n");
	a_prog_bug (function_name);
    }
    (void) fprintf (stderr,
		    "WARNING: the <%s> routine will be removed in Karma ",
		    function_name);
    (void)fprintf(stderr,
		  "version 2.0\nUse the <kcmap_va_create> routine instead.\n");
    return ( kcmap_va_create (name, num_cells, tolerant, dpy_handle,
			      obsolete_alloc_ccells_func,
			      obsolete_free_ccells_func,
			      obsolete_store_ccells_func,
			      obsolete_get_location_func,
			      KCMAP_ATT_END) );
}   /*  End Function kcmap_create  */

/*PUBLIC_FUNCTION*/
KCallbackFunc kcmap_register_resize_func (Kcolourmap cmap,
					  void (*resize_func) (), void *info)
/*  [SUMMARY] Register a colourmap resize function.
    [PURPOSE] This routine will register a resize function for a high level
    colourmap. The resize function will be called whenever the colourmap is
    resized. If the colourmap is a software colourmap, the resize function is
    called whenever the colour values change.
    Many resize functions may be registered per colourmap. The first
    function registered is the first function called upon resize.
    <cmap> The colourmap.
    <resize_func> The function which is called when the colourmap is resized.
    The prototype function is [<KCMAP_PROTO_resize_func>].
    <info> The initial arbitrary colourmap information pointer.
    [RETURNS] A KCallbackFunc object.
*/
{
    static char function_name[] = "kcmap_register_resize_func";

    VERIFY_COLOURMAP (cmap);
    return ( c_register_callback (&cmap->resize_list,
				  ( flag (*) () ) resize_func,
				  cmap, info, TRUE, NULL, FALSE,
				  FALSE) );
}   /*  End Function kcmap_register_resize_func  */

/*PUBLIC_FUNCTION*/
flag kcmap_change (Kcolourmap cmap, CONST char *new_name,
		   unsigned int num_cells, flag tolerant)
/*  [SUMMARY] Change active colourmap function.
    [PURPOSE] This routine will change the active function (algorithm) used to
    calculate the colours in a colourmap and the size of the colourmap.
    <cmap> The colourmap object.
    <new_name> The new function name. If this is NULL then the active function
    is not changed.
    <num_cells> The number of colourcells required for the colourmap. If this
    is less then 2 the number of cells is not changed.
    <tolerant> If TRUE, then the routine will try to allocate as many
    colourcells as possible (up to  num_cells  ), else it will fail if it could
    not allocatate all required colourcells.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int orig_num_cells = num_cells;
    unsigned int min_cells;
    struct cmap_func_type *cmap_func;
    static char function_name[] = "kcmap_change";

    VERIFY_COLOURMAP (cmap);
    if ( (new_name == NULL) && (num_cells < 2) )
    {
	/*  No change to colourmap function or resize  */
	return (TRUE);
    }
    if ( (cmap->master != NULL) && (new_name != NULL) )
    {
	/*  Slave colourmap: close connection  */
	if ( !conn_close (cmap->master) )
	{
	    (void) fprintf (stderr, "Error closing slave connection\n");
	    return (FALSE);
	}
    }
    if (cmap->master != NULL)
    {
	(void) fprintf (stderr, "Attempt to resize a slave colourmap\n");
	a_prog_bug (function_name);
    }
    cmap->modifiable = TRUE;
    if (new_name == NULL)
    {
	cmap_func = cmap->modify_func;
    }
    else
    {
	/*  Get new colourmap function  */
	if ( ( cmap_func = get_cmap_function (new_name) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Colourmap function: \"%s\" does not exist\n",
			    new_name);
	    a_prog_bug (function_name);
	}
    }
    if (num_cells < 2)
    {
	/*  No change in size requested: check for function limits  */
	min_cells = cmap_func->min_cells;
	if (cmap_func->min_cells > 1)
	{
	    /*  Lower limit specified  */
	    if (cmap->size < cmap_func->min_cells)
	    {
		num_cells = cmap_func->min_cells;
	    }
	}
	if (cmap_func->max_cells > 1)
	{
	    /*  Upper limit specified  */
	    if (cmap->size > cmap_func->max_cells)
	    {
		num_cells = cmap_func->max_cells;
	    }
	}
    }
    else
    {
	/*  Change specified  */
	if ( (cmap_func->min_cells > 1) && 
	    (num_cells < cmap_func->min_cells) )
	{
	    (void) fprintf (stderr,
			    "Requested number of cells: %u is less than\n",
			    num_cells);
	    (void) fprintf (stderr,
			    "minimum number of cells: %u for colourmap function: %s\n",
			    cmap_func->min_cells, cmap_func->name);
	    return (FALSE);
	}
	if ( (cmap_func->max_cells > 1) && 
	    (num_cells > cmap_func->max_cells) )
	{
	    (void) fprintf (stderr,
			    "Requested number of cells: %u is greater than\n",
			    num_cells);
	    (void) fprintf (stderr,
			    "maximum number of cells: %u for colourmap function: %s\n",
			    cmap_func->max_cells, cmap_func->name);
	    return (FALSE);
	}
	/*  num_cells  is within legal range  */
	min_cells = tolerant ? cmap_func->min_cells : num_cells;
    }
    if ( !change_cmap_size (cmap, num_cells, min_cells, TRUE,
			    (unsigned short *) NULL,
			    (packet_desc *) NULL, (char *) NULL) )
    {
	if (num_cells > 1)
	{
	    (void) fprintf (stderr,
			    "%s: Could not allocate colourmap of size: %u\n",
			    function_name, num_cells);
	    (void) fprintf (stderr, "Original  num_cells: %u\n",
			    orig_num_cells);
	}
	return (FALSE);
    }
    cmap->modify_func = cmap_func;
    if (cmap->direct_visual)
    {
	kcmap_modify_direct_type (cmap,
				  (double) 0.5, (double) 0.5, (void *) NULL,
				  (double) 0.5, (double) 0.5, (void *) NULL,
				  (double) 0.5, (double) 0.5, (void *) NULL);
    }
    else kcmap_modify (cmap, (double) 0.5, (double) 0.5, (void *) NULL);
    return (TRUE);
}   /*  End Function kcmap_change  */

/*PUBLIC_FUNCTION*/
void kcmap_modify (Kcolourmap cmap, double x, double y, void *var_param)
/*  [SUMMARY] Change colours in a colourmap.
    [PURPOSE] This routine will call the active colour compute function to
    change the colourmap colours in a colourmap.
    <cmap> The colourmap object.
    <x> A parameter used to compute the colour values.
    <y> A parameter used to compute the colour values.
    <var_param> A parameter used to compute the colour values.
    [NOTE] If the REVERSE attribute for the colourmap is set, the colourmap is
    reversed.
    [RETURNS] Nothing.
*/
{
    unsigned short max = MAX_INTENSITY;
    unsigned short red, green, blue;
    unsigned int count, far;
    float iscale;
    unsigned short *intensities;
    struct cmap_func_type *cmap_func;
    static char function_name[] = "kcmap_modify";

    VERIFY_COLOURMAP (cmap);
    if (cmap->direct_visual)
    {
	(void) fprintf (stderr,
			"Cannot modify a direct visual type colourmap this way\n");
	a_prog_bug (function_name);
    }
    if (!cmap->modifiable)
    {
	/*  This colourmap is a not modifiable: ignore  */
	return;
    }
    cmap_func = cmap->modify_func;
    if ( (x < 0.0) || (x > 1.0) )
    {
	(void) fprintf (stderr, "x value: %e  outside range 0.0 to 1.0\n",
			x);
	a_prog_bug (function_name);
    }
    if ( (y < 0.0) || (y > 1.0) )
    {
	(void) fprintf (stderr, "y value: %e  outside range 0.0 to 1.0\n",
			y);
	a_prog_bug (function_name);
    }
    intensities = cmap->intensities;
    (*cmap_func->func) (cmap->size, 
			intensities, intensities + 1, intensities + 2, 3,
			x, y, var_param);
    if (cmap->reverse)
    {
	/*  Reverse the colourmap  */
	for (count = 0; count < cmap->size / 2; ++count)
	{
	    far = cmap->size - count - 1;
	    red = intensities[far * 3];
	    green = intensities[far * 3 + 1];
	    blue = intensities[far * 3 + 2];
	    intensities[far * 3] = intensities[count * 3];
	    intensities[far * 3 + 1] = intensities[count * 3 + 1];
	    intensities[far * 3 + 2] = intensities[count * 3 + 2];
	    intensities[count * 3] = red;
	    intensities[count * 3 + 1] = green;
	    intensities[count * 3 + 2] = blue;
	}
    }
    if (cmap->invert)
    {
	/*  Reverse the colourmap  */
	for (count = 0; count < cmap->size; ++count)
	{
	    intensities[count * 3] = max - intensities[count * 3];
	    intensities[count * 3 + 1] = max - intensities[count * 3 + 1];
	    intensities[count * 3 + 2] = max - intensities[count * 3 + 2];
	}
    }
    if (cmap->red_scale != MAX_INTENSITY)
    {
	/*  Scale the red intensities  */
	iscale = (float) cmap->red_scale / (float) MAX_INTENSITY;
	for (count = 0; count < cmap->size; ++count)
	{
	    intensities[count * 3] = (intensities[count * 3] * iscale);
	}
    }
    if (cmap->green_scale != MAX_INTENSITY)
    {
	/*  Scale the green intensities  */
	iscale = (float) cmap->green_scale / (float) MAX_INTENSITY;
	for (count = 0; count < cmap->size; ++count)
	{
	    intensities[count * 3 + 1] = (intensities[count * 3 + 1] * iscale);
	}
    }
    if (cmap->blue_scale != MAX_INTENSITY)
    {
	/*  Scale the blue intensities  */
	iscale = (float) cmap->blue_scale / (float) MAX_INTENSITY;
	for (count = 0; count < cmap->size; ++count)
	{
	    intensities[count * 3 + 2] = (intensities[count * 3 + 2] * iscale);
	}
    }
    if (cmap->store_func != NULL)
    {
	(*cmap->store_func) (cmap->size, cmap->pixel_values,
			     intensities, intensities + 1, intensities + 2, 3,
			     cmap->dpy_handle);
    }
    /*  Transmit this colourmap to any slaves of it  */
    notify_cmap_modify (cmap);
}   /*  End Function kcmap_modify  */

/*PUBLIC_FUNCTION*/
void kcmap_modify_direct_type (Kcolourmap cmap,
			       double red_x, double red_y, void *red_var_param,
			       double green_x, double green_y,
			       void *green_var_param,
			       double blue_x, double blue_y,
			       void *blue_var_param)
/*  [SUMMARY] Change colours in a colourmap.
    [PURPOSE] This routine will call the active colour compute function to
    change the colourmap colours in a colourmap.
    <cmap> The colourmap object.
    <red_x> A parameter used to compute the red component colour values.
    <red_y> A parameter used to compute the red component colour values.
    <red_var_param> A parameter used to compute the red component colour
    values.
    <green_x> A parameter used to compute the green component colour values.
    <green_y> A parameter used to compute the green component colour values.
    <green_var_param> A parameter used to compute the green component colour
    values.
    <blue_x> A parameter used to compute the blue component colour values.
    <blue_y> A parameter used to compute the blue component colour values.
    <blue_var_param> A parameter used to compute the blue component colour
    values.
    [NOTE] If the REVERSE attribute for the colourmap is set, the colourmap is
    reversed.
    [RETURNS] Nothing.
*/
{
    unsigned short max = MAX_INTENSITY;
    unsigned short red, green, blue;
    unsigned int count, far;
    unsigned short *intensities;
    struct cmap_func_type *cmap_func;
    static char function_name[] = "kcmap_modify_direct_type";

    VERIFY_COLOURMAP (cmap);
    if (!cmap->direct_visual)
    {
	(void) fprintf (stderr,
			"Cannot modify a regular type colourmap this way\n");
	a_prog_bug (function_name);
    }
    if (!cmap->modifiable)
    {
	/*  This colourmap is a not modifiable: ignore  */
	return;
    }
    cmap_func = cmap->modify_func;
    if ( (red_x < 0.0) || (red_x > 1.0) )
    {
	(void) fprintf (stderr, "red_x value: %e  outside range 0.0 to 1.0\n",
			red_x);
	a_prog_bug (function_name);
    }
    if ( (red_y < 0.0) || (red_y > 1.0) )
    {
	(void) fprintf (stderr, "red_y value: %e  outside range 0.0 to 1.0\n",
			red_y);
	a_prog_bug (function_name);
    }
    if ( (green_x < 0.0) || (green_x > 1.0) )
    {
	(void) fprintf (stderr, "green_x value: %e  outside range 0.0 to 1.0\n",
			green_x);
	a_prog_bug (function_name);
    }
    if ( (green_y < 0.0) || (green_y > 1.0) )
    {
	(void) fprintf (stderr, "green_y value: %e  outside range 0.0 to 1.0\n",
			green_y);
	a_prog_bug (function_name);
    }
    if ( (blue_x < 0.0) || (blue_x > 1.0) )
    {
	(void) fprintf (stderr, "blue_x value: %e  outside range 0.0 to 1.0\n",
			blue_x);
	a_prog_bug (function_name);
    }
    if ( (blue_y < 0.0) || (blue_y > 1.0) )
    {
	(void) fprintf (stderr, "blue_y value: %e  outside range 0.0 to 1.0\n",
			blue_y);
	a_prog_bug (function_name);
    }
    intensities = cmap->intensities;
    (*cmap_func->func) (cmap->size, intensities, NULL, NULL, 3,
			red_x, red_y, red_var_param);
    (*cmap_func->func) (cmap->size, NULL, intensities + 1, NULL, 3,
			green_x, green_y, green_var_param);
    (*cmap_func->func) (cmap->size, NULL, NULL, intensities + 2, 3,
			blue_x, blue_y, blue_var_param);
    if (cmap->reverse)
    {
	/*  Reverse the colourmap  */
	for (count = 0; count < cmap->size / 2; ++count)
	{
	    far = cmap->size - count - 1;
	    red = intensities[far * 3];
	    green = intensities[far * 3 + 1];
	    blue = intensities[far * 3 + 2];
	    intensities[far * 3] = intensities[count * 3];
	    intensities[far * 3 + 1] = intensities[count * 3 + 1];
	    intensities[far * 3 + 2] = intensities[count * 3 + 2];
	    intensities[count * 3] = red;
	    intensities[count * 3 + 1] = green;
	    intensities[count * 3 + 2] = blue;
	}
    }
    if (cmap->invert)
    {
	/*  Reverse the colourmap  */
	for (count = 0; count < cmap->size; ++count)
	{
	    intensities[count * 3] = max - intensities[count * 3];
	    intensities[count * 3 + 1] = max - intensities[count * 3 + 1];
	    intensities[count * 3 + 2] = max - intensities[count * 3 + 2];
	}
    }
    if (!cmap->software)
    {
	(*cmap->store_func) (cmap->size, cmap->pixel_values,
			     intensities, intensities + 1, intensities + 2, 3,
			     cmap->dpy_handle);
    }
    /*  Transmit this colourmap to any slaves of it  */
    notify_cmap_modify (cmap);
}   /*  End Function kcmap_modify_direct_type  */

/*PUBLIC_FUNCTION*/
CONST char **kcmap_list_funcs ()
/*  [SUMMARY] Get list of colourmap functions.
    [PURPOSE] This routine will get the array of supported colour function
    names. This array is dynamically allocated, and should be freed using
    [<m_free>]. The array is terminated with a NULL pointer.
    [NOTE] The names in the array are statically allocated.
    [RETURNS] A pointer to the array.
*/
{
    int count;
    unsigned int num_funcs;
    struct cmap_func_type *entry;
    char **names;
    extern struct cmap_func_type *cmap_functions;
    static char function_name[] = "kcmap_list_funcs";
 
    for (num_funcs = 0, entry = cmap_functions; entry != NULL;
	 ++num_funcs, entry = entry->next);
    if (num_funcs < 1)
    {
	(void) fprintf (stderr, "No colourmap functions!\n");
	a_prog_bug (function_name);
    }
    if ( ( names = (char **) m_alloc ( sizeof *names * (num_funcs + 1) ) )
	== NULL )
    {
	m_abort (function_name, "array of name pointers");
    }
    for (count = num_funcs - 1, entry = cmap_functions; entry != NULL;
	 --count, entry = entry->next)
    {
	names[count] = entry->name;
    }
    names[num_funcs] = NULL;
    return ( (CONST char **) names );
}   /*  End Function kcmap_list_funcs  */

/*PUBLIC_FUNCTION*/
CONST char **kcmap_get_funcs_for_cmap (Kcolourmap cmap)
/*  [SUMMARY] Get list of colourmap functions compatible with a colourmap.
    [PURPOSE] This routine will get the array of supported colour function
    names. This array is dynamically allocated, and should be freed using
    [<m_free>]. The array is terminated with a NULL pointer. Only functions
    which are compatible with the colourmap are returned.
    <cmap> The colourmap object.
    [NOTE] The names in the array are statically allocated.
    [RETURNS] A pointer to the array.
*/
{
    int count;
    unsigned int num_funcs;
    struct cmap_func_type *entry;
    char **names;
    extern struct cmap_func_type *cmap_functions;
    static char function_name[] = "kcmap_get_funcs_for_cmap";

    /*  Count number of functions  */
    for (entry = cmap_functions, num_funcs = 0; entry != NULL;
	 entry = entry->next)
    {
	if ( (entry->type == CMAP_FUNC_TYPE_GREYSCALE) ||
	     !cmap->direct_visual ) ++num_funcs;
    }
    if (num_funcs < 1)
    {
	(void) fprintf (stderr, "No colourmap functions!\n");
	a_prog_bug (function_name);
    }
    if ( ( names = (char **) m_alloc ( sizeof *names * (num_funcs + 1) ) )
	== NULL )
    {
	m_abort (function_name, "array of name pointers");
    }
    for (count = num_funcs - 1, entry = cmap_functions; entry != NULL;
	 entry = entry->next)
    {
	if ( (entry->type != CMAP_FUNC_TYPE_GREYSCALE) &&
	     cmap->direct_visual ) continue;
	names[count--] = entry->name;
    }
    names[num_funcs] = NULL;
    return ( (CONST char **) names );
}   /*  End Function kcmap_get_funcs_for_cmap  */

/*PUBLIC_FUNCTION*/
CONST char *kcmap_get_active_func (Kcolourmap cmap)
/*  [SUMMARY] Get active colourmap function.
    [PURPOSE] This routine will get the name of the active colour function for
    a colourmap.
    <cmap> The colourmap object.
    [RETURNS] A pointer to the name of the colour function. This name must not
    be freed.
*/
{
    static char function_name[] = "kcmap_get_active_func";

    VERIFY_COLOURMAP (cmap);
    return (cmap->modify_func->name);
}   /*  End Function kcmap_get_active_func  */

/*PUBLIC_FUNCTION*/
unsigned int kcmap_get_pixels (Kcolourmap cmap, unsigned long **pixel_values)
/*  [SUMMARY] Get pixel values in a colourmap.
    [PURPOSE] This routine will determine the number of colourcells in a
    colourmap and the pixel values allocated.
    <cmap> The colourmap object.
    <pixel_values> A pointer to the array of pixel values will be written here.
    If this is NULL, nothing is written here. The pixel values may be modified
    if the colourmap is a software colourmap. If the pixel values are changed,
    the [<kcmap_notify_pixels_changed>] routine should be called.
    [RETURNS] The number of colourcells allocated.
*/
{
    static char function_name[] = "kcmap_get_pixels";

    VERIFY_COLOURMAP (cmap);
    if (pixel_values != NULL) *pixel_values = cmap->pixel_values;
    return (cmap->size);
}   /*  End Function kcmap_get_pixels  */

/*PUBLIC_FUNCTION*/
void kcmap_notify_pixels_changed (Kcolourmap cmap)
/*  [SUMMARY] Notify that pixel values have been changed.
    [PURPOSE] This routine posts a notification that the pixel values in a
    software colourmap have changed. The resize functions registered with
    [<kcmap_register_resize_func>] are usually called.
    <cmap> The colourmap object. This must be a software colourmap.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "kcmap_notify_pixels_changed";

    if (!cmap->software)
    {
	(void) fprintf (stderr, "Not a software colourmap!\n");
	a_prog_bug (function_name);
    }
    (void) c_call_callbacks (cmap->resize_list, NULL);
}   /*  End Function kcmap_notify_pixels_changed  */

/*PUBLIC_FUNCTION*/
unsigned long kcmap_get_pixel (Kcolourmap cmap, unsigned int index)
/*  [SUMMARY] This routine will get a numbered pixel value from a colourmap.
    <cmap> The colourmap object.
    <index> The index of the pixel.
    [RETURNS] The pixel value.
*/
{
    static char function_name[] = "kcmap_get_pixel";

    VERIFY_COLOURMAP (cmap);
    if (cmap->software)
    {
	(void) fprintf (stderr, "No pixels in a software colourmap!\n");
	a_prog_bug (function_name);
    }
    if (index >= cmap->size)
    {
	(void) fprintf (stderr,
			"index: %u  is not less than colourmap size: %u\n",
			index, cmap->size);
	a_prog_bug (function_name);
    }
    return (cmap->pixel_values[index]);
}   /*  End Function kcmap_get_pixel  */

/*PUBLIC_FUNCTION*/
void kcmap_prepare_for_slavery (Kcolourmap cmap)
/*  [SUMMARY] Allow colourmap to be slaved.
    [PURPOSE] This routine will register a colourmap to be the choosen
    colourmap for subsequent attempts to open a slave colourmap connection. In
    order to make the colourmap a slave, a subsequent call to
    [<conn_attempt_connection>] must be made.
    <cmap> The colourmap object.
    [RETURNS] Nothing.
*/
{
    extern Kcolourmap slaveable_colourmap;
    static char function_name[] = "kcmap_prepare_for_slavery";

    VERIFY_COLOURMAP (cmap);
    slaveable_colourmap = cmap;
}   /*  End Function kcmap_prepare_for_slavery  */

/*PUBLIC_FUNCTION*/
flag kcmap_copy_to_struct (Kcolourmap cmap, packet_desc **top_pack_desc,
			   char **top_packet)
/*  [SUMMARY] Copy colour data from a colourmap.
    [PURPOSE] This routine will copy the colour data in a colourmap into a
    newly allocated Karma data structure. This data structure may be
    subsequently deallocated.
    <cmap> The colourmap object.
    <top_pack_desc> The pointer to the top level packet descriptor that is
    allocated will be written here.
    <top_packet> The pointer to the top level packet that is allocated will be
    written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "kcmap_copy_to_struct";

    VERIFY_COLOURMAP (cmap);
    if (cmap->intensities == NULL)
    {
	(void) fprintf (stderr, "Colourmap has no colour information\n");
	return (FALSE);
    }
    if ( ( *top_pack_desc = ds_copy_desc_until (cmap->top_pack_desc, NULL ) )
	== NULL )
    {
	m_error_notify (function_name, "top level packet descriptor");
	*top_packet = NULL;
	return (FALSE);
    }
    if ( ( *top_packet = ds_alloc_data (cmap->top_pack_desc, FALSE, TRUE) )
	== NULL )
    {
	m_error_notify (function_name, "top level packet");
	ds_dealloc_packet (*top_pack_desc, NULL);
	*top_pack_desc = NULL;
	return (FALSE);
    }
    if ( !ds_copy_data (cmap->top_pack_desc, cmap->top_packet,
			*top_pack_desc, *top_packet) )
    {
	(void) fprintf (stderr, "Data structure copy not identical\n");
	a_prog_bug (function_name);
    }
    return (TRUE);
}   /*  End Function kcmap_copy_to_struct  */

/*PUBLIC_FUNCTION*/
flag kcmap_copy_from_struct (Kcolourmap cmap, packet_desc *top_pack_desc,
			     char *top_packet)
/*  [SUMMARY] Copy colour data into a colourmap.
    [PURPOSE] This routine will copy the colour data in a Karma data structure
    into a colourmap. If the colourmap changes size, then the <<resize_func>>
    registered is called.
    <cmap> The colourmap object.
    <top_pack_desc> The top level packet descriptor.
    <top_packet> The top level packet.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag reordering_done;
    unsigned int colourmap_size;
    unsigned short *colour_array;
    packet_desc *pack_desc;
    char *packet;
    static char function_name[] = "kcmap_copy_from_struct";

    VERIFY_COLOURMAP (cmap);
    if ( ( pack_desc = ds_copy_desc_until (top_pack_desc, NULL) )
	== NULL )
    {
	m_error_notify (function_name, "top level packet descriptor");
	return (FALSE);
    }
    if ( ( packet = ds_alloc_data (top_pack_desc, FALSE, TRUE) ) == NULL )
    {
	m_error_notify (function_name, "top level packet");
	ds_dealloc_packet (pack_desc, NULL);
	return (FALSE);
    }
    if ( !ds_copy_data (top_pack_desc, top_packet, pack_desc, packet) )
    {
	(void) fprintf (stderr, "Data structure copy not identical\n");
	a_prog_bug (function_name);
    }
    /*  Now get a handle to the colourmap  */
    if ( ( colour_array = ds_cmap_find_colourmap (pack_desc, packet,
						  &colourmap_size,
						  &reordering_done,
						  (CONST char **) NULL,
						  (double *) NULL,
						  (unsigned int) 0) ) == NULL )
    {
	(void) fprintf (stderr, "Could not find colourmap\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    if (reordering_done) (void) fprintf (stderr, "Colourmap was reordered\n");
    if (cmap->master != NULL)
    {
	/*  Slave colourmap: close connection  */
	if ( !conn_close (cmap->master) )
	{
	    (void) fprintf (stderr, "Error closing slave connection\n");
	    ds_dealloc_packet (pack_desc, packet);
	    return (FALSE);
	}
    }
    if ( !change_cmap_size (cmap, colourmap_size, colourmap_size, TRUE,
			    colour_array, pack_desc, packet) )
    {
	(void) fprintf (stderr, "Could not reallocate colourmap\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    if (cmap->store_func != NULL)
    {
	(*cmap->store_func) (cmap->size, cmap->pixel_values,
			     cmap->intensities, cmap->intensities + 1,
			     cmap->intensities + 2, 3,
			     cmap->dpy_handle);
    }
    cmap->modifiable = FALSE;
    /*  Transmit this colourmap to any slaves of it  */
    notify_cmap_modify (cmap);
    return (TRUE);
}   /*  End Function kcmap_copy_from_struct  */

/*PUBLIC_FUNCTION*/
unsigned short *kcmap_get_rgb_values (Kcolourmap cmap, unsigned int *size)
/*  [SUMMARY] Get colour data from a colourmap.
    [PURPOSE] This routine will return the RGB values in a colourmap. The
    colour values are arranged in packets of Red, Green and Blue values.
    <cmap> The colourmap object.
    <size> The routine will write the size of the colourmap here.
    [RETURNS] A pointer to a dynamically allocated array. This must be freed
    with [<m_free>]. On failure it returns NULL.
*/
{
    unsigned short *intensities;
    static char function_name[] = "kcmap_get_rgb_values";

    VERIFY_COLOURMAP (cmap);
    if (cmap->intensities == NULL)
    {
	(void) fprintf (stderr, "Colourmap has no colour information\n");
	return (NULL);
    }
    if ( ( intensities = (unsigned short *)
	  m_alloc (sizeof *intensities * 3 * cmap->size) ) == NULL )
    {
	m_error_notify (function_name, "array of intensities");
	return (NULL);
    }
    m_copy ( (char *) intensities, (char *) cmap->intensities,
	    sizeof *intensities * 3 * cmap->size );
    *size = cmap->size;
    return (intensities);
}   /*  End Function kcmap_get_rgb_values  */

/*PUBLIC_FUNCTION*/
void kcmap_get_attributes (Kcolourmap cmap, ...)
/*  [SUMMARY] This routine will get the attributes for a colourmap.
    <cmap> The colourmap.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. This list must be terminated with the value
    KCMAP_ATT_END. See [<KCMAP_ATTRIBUTES>] for a list of defined attributes.
    [RETURNS] Nothing.
*/
{
    va_list argp;
    unsigned int att_key;
    static char function_name[] = "kcmap_get_attributes";

    va_start (argp, cmap);
    VERIFY_COLOURMAP (cmap);
    while ( ( att_key = va_arg (argp, unsigned int) ) != KCMAP_ATT_END )
    {
	switch (att_key)
	{
	  case KCMAP_ATT_REVERSE:
	    *( va_arg (argp, flag *) ) = cmap->reverse;
	    break;
	  case KCMAP_ATT_INVERT:
	    *( va_arg (argp, flag *) ) = cmap->invert;
	    break;
	  case KCMAP_ATT_SOFTWARE:
	    *( va_arg (argp, flag *) ) = cmap->software;
	    break;
	  case KCMAP_ATT_DPY_HANDLE:
	    *( va_arg (argp, Kdisplay *) ) = cmap->dpy_handle;
	    break;
	  case KCMAP_ATT_DIRECT_VISUAL:
	    *( va_arg (argp, flag *) ) = cmap->direct_visual;
	    break;
	  case KCMAP_ATT_SIZE:
	    (* va_arg (argp, unsigned int *) ) = cmap->size;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
}   /*  End Function kcmap_get_attributes  */

/*PUBLIC_FUNCTION*/
void kcmap_set_attributes (Kcolourmap cmap, ...)
/*  [SUMMARY] This routine will set the attributes for a colourmap.
    <cmap> The colourmap.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. This list must be terminated with the value
    KCMAP_ATT_END. See [<KCMAP_ATTRIBUTES>] for a list of defined attributes.
    [NOTE] The colourmap is not recomputed: the effect is delayed.
    [RETURNS] Nothing.
*/
{
    va_list argp;
    flag bool;
    unsigned int att_key;
    static char function_name[] = "kcmap_set_attributes";

    va_start (argp, cmap);
    VERIFY_COLOURMAP (cmap);
    while ( ( att_key = va_arg (argp, unsigned int) ) != KCMAP_ATT_END )
    {
	switch (att_key)
	{
	  case KCMAP_ATT_REVERSE:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    cmap->reverse = bool;
	    break;
	  case KCMAP_ATT_INVERT:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    cmap->invert = bool;
	    break;
	  case KCMAP_ATT_RED_SCALE:
	    cmap->red_scale = va_arg (argp, unsigned short);
	    break;
	  case KCMAP_ATT_GREEN_SCALE:
	    cmap->green_scale = va_arg (argp, unsigned short);
	    break;
	  case KCMAP_ATT_BLUE_SCALE:
	    cmap->blue_scale = va_arg (argp, unsigned short);
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
}   /*  End Function kcmap_set_attributes  */


/*  Private routines follow  */

static void initialise ()
/*  [SUMMARY] This routine will initialise the package.
    [RETURNS] Nothing.
*/
{
    static flag initialised = FALSE;

    if (initialised) return;
    initialised = TRUE;
    kcmap_add_grey_func ("Greyscale1", cf_greyscale1, 0, 0);
    kcmap_add_grey_func ("Greyscale2", cf_greyscale2, 0, 0);
    kcmap_add_grey_func ("Greyscale3", cf_greyscale3, 0, 0);
    kcmap_add_grey_func ("Random Grey", cf_random_grey, 0, 0);
    kcmap_add_RGB_func ("Random Pseudocolour", cf_random_pseudocolour, 0, 0);
    kcmap_add_RGB_func ("mirp", cf_mirp, 0, 0);
    kcmap_add_RGB_func ("Glynn Rogers1", cf_rainbow1, 0, 0);
    kcmap_add_RGB_func ("Glynn Rogers2", cf_rainbow2, 0, 0);
    kcmap_add_RGB_func ("Glynn Rogers3", cf_rainbow3, 0, 0);
    kcmap_add_RGB_func ("Cyclic 1", cf_cyclic1, 0, 0);
    kcmap_add_RGB_func ("Velocity: Compensating Tones",
			  cf_velocity_compensating_tones, 0, 0);
    kcmap_add_RGB_func ("Compressed Colourmap 3R2G2B",
			  cf_compressed_colourmap_3r2g2b, 128, 128);

    kcmap_add_RGB_func ("Background", cf_background, 0, 0);
    kcmap_add_RGB_func ("Heat",       cf_heat,       0, 0);
    kcmap_add_RGB_func ("Isophot",    cf_isophot,    0, 0);
    kcmap_add_grey_func ("Mono",       cf_mono,       0, 0);
    kcmap_add_RGB_func ("Mousse",     cf_mousse,     0, 0);
    kcmap_add_RGB_func ("Rainbow",    cf_rainbow,    0, 0);
    kcmap_add_RGB_func ("Random",     cf_random,     0, 0);
    kcmap_add_RGB_func ("RGB",        cf_rgb,        0, 0);
    kcmap_add_RGB_func ("Ronekers",   cf_ronekers,   0, 0);
    kcmap_add_RGB_func ("Smooth",     cf_smooth,     0, 0);
    kcmap_add_RGB_func ("Staircase",  cf_staircase,  0, 0);
    kcmap_add_RGB_func ("Velocity Field",  cf_rgb2,  0, 0);
    kcmap_add_RGB_func ("Mandelbrot", cf_mandelbrot, 0, 0);
    conn_register_server_protocol ("colourmap_indices", PROTOCOL_VERSION, 0,
				   register_new_cmap_indices_slave,
				   ( flag (*) () ) NULL, ( void (*) () ) NULL);
    conn_register_client_protocol ("colourmap_indices", PROTOCOL_VERSION, 1,
				   verify_indices_slave_cmap_connection,
				   register_cmap_indices_connection,
				   read_cmap_indices,
				   register_cmap_connection_close);
    conn_register_server_protocol ("full_colourmap", PROTOCOL_VERSION, 0,
				   register_new_full_cmap_slave,
				   ( flag (*) () ) NULL,
				   ( void (*) () ) NULL);
    conn_register_client_protocol ("full_colourmap", PROTOCOL_VERSION, 1,
				   verify_full_slave_cmap_connection,
				   register_full_cmap_connection,
				   read_full_cmap,
				   register_cmap_connection_close);
}   /*  End Function initialise  */

static struct cmap_func_type *get_cmap_function (char *name)
/*  This routine will get the named colourmap function
    The name must be pointed to by  name  .
    The routine returns a pointer to a struct on success, else it returns NULL
    (indicating there is no colourmap function with that name).
*/
{
    struct cmap_func_type *entry;
    extern struct cmap_func_type *cmap_functions;

    for (entry = cmap_functions; entry != NULL; entry = entry->next)
    {
	if (strcmp (name, entry->name) == 0)
	{
	    /*  Found it  */
	    return (entry);
	}
    }
    return (NULL);
}   /*  End Function get_cmap_function  */


/*  Routines related to incoming colourmap connections  */

static flag register_new_cmap_indices_slave (Connection connection,void **info)
/*  This routine will register the connection of a new colourmap_indices client
    The connection must be given by  connection  .
    Any appropriate information is pointed to by  info  (unused).
    The routine will write the host Internet address and display number of the
    X Windows server the module is connected to to the connection and will read
    a 1 byte flag from the client. If this flag is TRUE, the routine will then
    write the colourmap indices to the connection, else it will return FALSE,
    indicating that the connection is to be closed.
    The routine returns TRUE on success, else it returns FALSE (indicating that
    the connection should be closed).
*/
{
    Channel channel;
    unsigned long serv_display_num;
    unsigned long serv_hostaddr;
    char accepted;
    char false = FALSE;
    char true = TRUE;
    extern Kcolourmap shareable_colourmap;
    extern char *sys_errlist[];

    channel = conn_get_channel (connection);
    if ( (shareable_colourmap == NULL) ||
	(shareable_colourmap->master != NULL) ||
	shareable_colourmap->software )
    {
	/*  Cannot service requests: tell client  */
	if (ch_write (channel, &false, 1) < 1)
	{
	    (void) fprintf (stderr, "Error writing rejection\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if ( !ch_flush (channel) )
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
	}
	return (FALSE);
    }
    if (ch_write (channel, &true, 1) < 1)
    {
	(void) fprintf (stderr, "Error writing acceptance\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Determine X server we are connected to  */
    (*shareable_colourmap->location_func) (shareable_colourmap->dpy_handle,
					   &serv_hostaddr, &serv_display_num);
    /*  Tell client what X Windows display we are connected to  */
    if ( !pio_write32 (channel, serv_hostaddr) )
    {
	return (FALSE);
    }
    if ( !pio_write32 (channel, serv_display_num) )
    {
	return (FALSE);
    }
    if ( !ch_flush (channel) )
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Wait for client's response (acceptance or rejection)  */
    if (ch_read (channel, &accepted, 1) < 1)
    {
	(void) fprintf (stderr, "Error reading acceptance\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if (!accepted)
    {
	/*  Connection rejected by client  */
	return (FALSE);
    }
    /*  Client is happy about this connection: send pixel values  */
    if ( !write_cmap_indices (connection, shareable_colourmap) )
    {
	(void) fprintf (stderr, "Error writing pixels\n");
	return (FALSE);
    }
    *info = (void *) shareable_colourmap;
    return (TRUE);
}   /*  End Function register_new_cmap_indices_slave  */

static flag register_new_full_cmap_slave (Connection connection, void **info)
/*  This routine will register the connection of a new full_colourmap client
    The connection must be given by  connection  .
    Any appropriate information is pointed to by  info  (unused).
    The routine will write the colourmap colours to the connection.
    The routine returns TRUE on success, else it returns FALSE (indicating that
    the connection should be closed).
*/
{
    Channel channel;
    char false = FALSE;
    char true = TRUE;
    extern Kcolourmap shareable_colourmap;
    extern char *sys_errlist[];

    channel = conn_get_channel (connection);
    if ( (shareable_colourmap == NULL) ||
	(shareable_colourmap->master != NULL) )
    {
	/*  Cannot service requests: tell client  */
	if (ch_write (channel, &false, 1) < 1)
	{
	    (void) fprintf (stderr, "Error writing rejection\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if ( !ch_flush (channel) )
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
	}
	return (FALSE);
    }
    if (ch_write (channel, &true, 1) < 1)
    {
	(void) fprintf (stderr, "Error writing acceptance\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if ( !write_full_cmap (connection, shareable_colourmap) )
    {
	(void) fprintf (stderr, "Error writing colourcell definitions\n");
	return (FALSE);
    }
    *info = (void *) shareable_colourmap;
    return (TRUE);
}   /*  End Function register_new_full_cmap_slave  */


/*  Routines related to outgoing colourmap connections  */

static flag verify_indices_slave_cmap_connection (void **info)
/*   This routine will validate whether it is appropriate to open a slave
     colourmap indicies connection.
     The routine will write the slaveable colourmap to the storage
     pointed to by  info  .The pointer value written here will be passed
     to the other routines.
     The routine returns TRUE if the connection should be attempted,
     else it returns FALSE (indicating the connection should be aborted).
     NOTE: Even if this routine is called and returns TRUE, there is no
     guarantee that the connection will be subsequently opened.
*/
{
    extern Kcolourmap slaveable_colourmap;

    if (slaveable_colourmap == NULL)
    {
	(void) fprintf (stderr, "No slaveable colourmap registered\n");
	return (FALSE);
    }
    if (slaveable_colourmap->master != NULL)
    {
	(void) fprintf (stderr, "Slaveable colourmap is already a slave\n");
	return (FALSE);
    }
    if (slaveable_colourmap->software)
    {
	(void) fprintf (stderr,
			"Slaveable colourmap is a software colourmap\n");
	return (FALSE);
    }
    *info = (void *) slaveable_colourmap;
    return (TRUE);
}   /*  End Function verify_indices_slave_cmap_connection  */

static flag verify_full_slave_cmap_connection (void **info)
/*   This routine will validate whether it is appropriate to open a slave
     full colourmap connection.
     The routine will write the slaveable colourmap to the storage
     pointed to by  info  .The pointer value written here will be passed
     to the other routines.
     The routine returns TRUE if the connection should be attempted,
     else it returns FALSE (indicating the connection should be aborted).
     NOTE: Even if this routine is called and returns TRUE, there is no
     guarantee that the connection will be subsequently opened.
*/
{
    extern Kcolourmap slaveable_colourmap;

    if (slaveable_colourmap == NULL)
    {
	(void) fprintf (stderr, "No slaveable colourmap registered\n");
	return (FALSE);
    }
    if (slaveable_colourmap->master != NULL)
    {
	(void) fprintf (stderr, "Slaveable colourmap is already a slave\n");
	return (FALSE);
    }
    *info = (void *) slaveable_colourmap;
    return (TRUE);
}   /*  End Function verify_full_slave_cmap_connection  */

static flag register_cmap_indices_connection (Connection connection,
					      void **info)
/*  This routine will register the connection to a  colourmap_indices  server
    The connection must be given by  connection  .
    The colourmap to enslave must be pointed to by  info  .
    The routine will write the host Internet address and display number of the
    X Windows server the module is connected to to the connection and will read
    a 1 byte flag from the client. If this flag is TRUE, the routine will then
    write the colourmap indices to the connection, else it will return FALSE,
    indicating that the connection is to be closed.
    The routine returns TRUE on success, else it returns FALSE (indicating that
    the connection should be closed).
*/
{
    char true = TRUE;
    char false = FALSE;
    char server_happy;
    Channel channel;
    Kcolourmap cmap;
    unsigned int num_cells;
    unsigned long my_display_num;
    unsigned long my_display_inet_addr;
    unsigned long server_display_inet_addr;
    unsigned long server_display_num;
    extern char *sys_errlist[];
    static char function_name[] = "register_cmap_indices_connection";

    channel = conn_get_channel (connection);
    cmap = (Kcolourmap) *info;
    VERIFY_COLOURMAP (cmap);
    num_cells = cmap->size;
    /*  See if server can cope with it  */
    if (ch_read (channel, &server_happy, 1) < 1)
    {
	(void) fprintf (stderr, "Error reading server acceptance\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if (!server_happy)
    {
	return (FALSE);
    }
    /* Now see if server is connected to the same X Windows display as we are*/
    /*  Server should soon write the  X Windows display server Internet address
	and display number it is connected to.  */
    if ( !pio_read32 (channel, &server_display_inet_addr) )
    {
	(void) fprintf (stderr,
			"Error reading Internet address of X server of colourmap server\n");
	return (FALSE);
    }
    if ( !pio_read32 (channel, &server_display_num) )
    {
	(void) fprintf (stderr,
			"Error reading display number for colourmap server\n");
	return (FALSE);
    }
    /*  Server has said which X Display it is using: let's find out what one
	we are using  */
    (*cmap->location_func) (cmap->dpy_handle,
			    &my_display_inet_addr, &my_display_num);
    /*  Now compare my display address and number with that of
	colourmap server  */
    if ( (server_display_inet_addr != my_display_inet_addr) ||
	(server_display_num != (unsigned long) my_display_num) )
    {
	/*  We are connected to a different X server  */
	/*  Tell server we are closing this connection: send FALSE  */
	if (ch_write (channel, &false, 1) < 1)
	{
	    (void) fprintf (stderr, "Error writing to connection\t%s\n",
			    sys_errlist[errno]);
	}
	/*  Close connection: we will try for a more advanced protocol later */
	return (FALSE);
    }
    /*  We are connected to the same X server: send TRUE  */
    if (ch_write (channel, &true, 1) < 1)
    {
	(void) fprintf (stderr, "Error writing to connection\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if ( !ch_flush (channel) )
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Now have a valid connection to the colourmap server, irrespective of
	whether it is connected to the same X Windows display as us or not  */
    cmap->master = connection;
    cmap->full_cmap = FALSE;
    cmap->modifiable = FALSE;
    /*  No colourcells need be allocated: free any  */
    if (cmap->size > 0)
    {
	(*cmap->free_func) (cmap->size, cmap->pixel_values, cmap->dpy_handle);
    }
    if ( !read_cmap_indices (connection, info) )
    {
	(void) fprintf (stderr, "Error reading pixels\n");
	(void) kcmap_change (cmap, cmap->modify_func->name, num_cells, TRUE);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function register_cmap_indices_connection  */

static flag register_full_cmap_connection (Connection connection, void **info)
/*  This routine will register the connection to a  full_colourmap  server
    The connection must be given by  connection  .
    The colourmap to enslave must be pointed to by  info  .
    The routine will write the host Internet address and display number of the
    The routine returns TRUE on success, else it returns FALSE (indicating that
    the connection should be closed).
*/
{
    char server_happy;
    Channel channel;
    Kcolourmap cmap;
    unsigned int num_cells;
    extern char *sys_errlist[];
    static char function_name[] = "register_full_cmap_connection";

    channel = conn_get_channel (connection);
    cmap = (Kcolourmap) *info;
    VERIFY_COLOURMAP (cmap);
    num_cells = cmap->size;
    /*  See if server can cope with it  */
    if (ch_read (channel, &server_happy, 1) < 1)
    {
	(void) fprintf (stderr, "Error reading server acceptance\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if (!server_happy)
    {
	return (FALSE);
    }
    /*  Now have a valid connection to the colourmap server, irrespective of
	whether it is connected to the same X Windows display as us or not  */
    cmap->master = connection;
    cmap->full_cmap = TRUE;
    cmap->modifiable = FALSE;
    if ( !read_full_cmap (connection, info) )
    {
	(void) fprintf (stderr, "Error reading full colourmap\n");
	(void) kcmap_change (cmap, cmap->modify_func->name, num_cells, TRUE);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function register_full_cmap_connection  */

static void register_cmap_connection_close (Connection connection, void *info)
/*  This routine will register the closure of the connection to the colourmap
    server.
    The connection must be given by  connection  .
    The colourmap to emancipate must be pointed to by  info  .
    The routine returns nothing.
*/
{
    unsigned int num_cells;
    Kcolourmap cmap;
    static char function_name[] = "register_cmap_connection_close";

    cmap = (Kcolourmap) info;
    VERIFY_COLOURMAP (cmap);
    if (cmap->master != connection)
    {
	(void) fprintf (stderr, "Invalid connection for colourmap object\n");
	a_prog_bug (function_name);
    }
    cmap->master = NULL;
    num_cells = cmap->size;
    cmap->modifiable = TRUE;
    if (!cmap->full_cmap)
    {
	/*  Colourmap indicies: deallocate and change  */
	m_free ( (char *) cmap->pixel_values );
	cmap->pixel_values = NULL;
	cmap->size = 0;
	(void) kcmap_change (cmap, (char *) NULL, num_cells, TRUE);
    }
    /*  Everything deallocated  */
}   /*  End Function register_cmap_connection_close  */

static flag write_cmap_indices (Connection connection, Kcolourmap cmap)
/*  This routine will write an array of pixel values to a connection.
    The connection must be given by  connection  .
    The colourmap must be given by  cmap  .
    The routine returns TRUE on sucess, else it returns FALSE.
*/
{
    Channel channel;
    unsigned int num_pixels;
    unsigned int pixel_count;
    unsigned long *pixel_values;
    extern char *sys_errlist[];
    static char function_name[] = "write_cmap_indices";

    VERIFY_COLOURMAP (cmap);
    channel = conn_get_channel (connection);
    num_pixels = kcmap_get_pixels (cmap, &pixel_values);
    /*  Write 4 bytes indicicating how many pixel values are coming  */
    if ( !pio_write32 (channel, (unsigned long) num_pixels) )
    {
	(void) fprintf (stderr, "Error writing number of pixels\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Write pixel values  */
    for (pixel_count = 0; pixel_count < num_pixels; ++pixel_count)
    {
	if ( !pio_write32 (channel, pixel_values[pixel_count]) )
	{
	    (void) fprintf (stderr,
			    "Error writing pixel value: %u to channel\t%s\n",
			    pixel_count, sys_errlist[errno]);
	    return (FALSE);
	}
    }
    if ( !ch_flush (channel) )
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  All pixels sent  */
    return (TRUE);
}   /*  End Function write_cmap_indices  */

static flag read_cmap_indices (Connection connection, void **info)
/*  This routine will read an array of pixel values from a connection.
    The connection must be given by  connection  .
    The colourmap must be pointed to by  info  .
    The routine returns TRUE on sucess, else it returns FALSE.
*/
{
    Channel channel;
    Kcolourmap cmap;
    unsigned int pixel_count;
    unsigned long pixels_to_read;
    unsigned long *pixel_values;
    extern char *sys_errlist[];
    static char function_name[] = "read_cmap_indices";

    cmap = (Kcolourmap) *info;
    VERIFY_COLOURMAP (cmap);
    channel = conn_get_channel (connection);
    if (cmap->intensities != NULL)
    {
	ds_dealloc_packet (cmap->top_pack_desc, cmap->top_packet);
	cmap->intensities = NULL;
	cmap->top_pack_desc = NULL;
	cmap->top_packet = NULL;
    }
    /*  Read number of pixel values to read  */
    if (pio_read32 (channel, &pixels_to_read) == FALSE)
    {
	(void) fprintf (stderr, "Error reading number of pixels to read\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Allocate storage for read of pixel values  */
    if ( ( pixel_values = (unsigned long *)
	  m_alloc (sizeof *pixel_values * pixels_to_read) )
	== NULL )
    {
	m_error_notify (function_name, "array of pixel values");
	return (FALSE);
    }
    /*  Read in array of pixels  */
    for (pixel_count = 0; pixel_count < pixels_to_read; ++pixel_count)
    {
	if ( !pio_read32 (channel, pixel_values + pixel_count) )
	{
	    (void) fprintf (stderr, "Error reading pixel value: %u\t%s\n",
			    pixel_count, sys_errlist[errno]);
	    m_free ( (char *) pixel_values );
	    return (FALSE);
	}
    }
    m_free ( (char *) cmap->pixel_values );
    cmap->pixel_values = pixel_values;
    cmap->size = pixels_to_read;
    (void) c_call_callbacks (cmap->resize_list, NULL);
    return (TRUE);
}   /*  End Function read_cmap_indices  */

static flag write_full_cmap (Connection connection, Kcolourmap cmap)
/*  This routine will write an array of pixel colours to a connection.
    The connection must be given by  connection  .
    The colourmap must be given by  cmap  .
    The routine will write the colourmap using the Karma colourmap format
    (which utilises the Karma general data structure format).
    The routine returns TRUE on sucess, else it returns FALSE.
*/
{
    Channel channel;
    extern char *sys_errlist[];
    static char function_name[] = "write_full_cmap";

    VERIFY_COLOURMAP (cmap);
    channel = conn_get_channel (connection);
    /*  Write colourmap  */
    dsrw_write_packet_desc (channel, cmap->top_pack_desc);
    dsrw_write_packet (channel, cmap->top_pack_desc, cmap->top_packet);
    if ( !ch_flush (channel) )
    {
	(void) fprintf (stderr, "Error writing Karma colourmap\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  All colours sent  */
    return (TRUE);
}   /*  End Function write_full_cmap  */

static flag read_full_cmap (Connection connection, void **info)
/*  This routine will read an array of pixel colours from a connection.
    The connection must be given by  connection  .
    The colourmap must be pointed to by  info  .
    The routine returns TRUE on sucess, else it returns FALSE.
*/
{
    Channel channel;
    Kcolourmap cmap;
    unsigned int colourmap_size;
    unsigned short *colour_array;
    char *packet;
    packet_desc *pack_desc;
    static char function_name[] = "read_full_cmap";

    cmap = (Kcolourmap) *info;
    VERIFY_COLOURMAP (cmap);
    channel = conn_get_channel (connection);
    /*  Read a single Karma general data structure  */
    if ( ( pack_desc = dsrw_read_packet_desc (channel) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading Karma colourmap descriptor\n");
	return (FALSE);
    }
    if ( ( packet = ds_alloc_data (pack_desc, TRUE, TRUE) ) == NULL )
    {
	m_error_notify (function_name, "Karma colourmap data");
	ds_dealloc_packet (pack_desc, (char *) NULL);
	return (FALSE);
    }
    if ( !dsrw_read_packet (channel, pack_desc, packet) )
    {
	(void) fprintf (stderr, "Error reading Karma colourmap data\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    /*  Now get a handle to the colourmap  */
    if ( ( colour_array = ds_cmap_find_colourmap (pack_desc, packet,
						  &colourmap_size,
						  (flag *) NULL,
						  (CONST char **) NULL,
						  (double *) NULL,
						  (unsigned int) 0) ) == NULL )
    {
	(void) fprintf (stderr, "Could not find colourmap\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    if ( !change_cmap_size (cmap, colourmap_size, colourmap_size, TRUE,
			    colour_array, pack_desc, packet) )
    {
	(void) fprintf (stderr, "Could not reallocate colourmap\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    (*cmap->store_func) (cmap->size, cmap->pixel_values,
			 cmap->intensities, cmap->intensities + 1,
			 cmap->intensities + 2, 3,
			 cmap->dpy_handle);
    return (TRUE);
}   /*  End Function read_full_cmap  */

static void notify_cmap_resize (Kcolourmap cmap)
/*  This routine will transmit a colourmap resize to all colourmap_indices
    clients.
    The colourmap must be given by  cmap  .
    The routine returns nothing.
*/
{
    Connection connection;
    Kcolourmap conn_cmap;
    unsigned int num_connections;
    unsigned int connection_count;
    static char function_name[] = "notify_cmap_resize";

    VERIFY_COLOURMAP (cmap);
    num_connections = conn_get_num_serv_connections ("colourmap_indices");
    for (connection_count = 0; connection_count < num_connections;
	 ++connection_count)
    {
	if ( ( connection = conn_get_serv_connection ("colourmap_indices",
						      connection_count) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error getting connection: %u\n",
			    connection_count);
	    a_prog_bug (function_name);
	}
	if ( ( conn_cmap = (Kcolourmap) conn_get_connection_info (connection) )
	    == NULL )
	{
	    (void) fprintf (stderr,
			    "colourmap_indices connection has no colourmap\n");
	    a_prog_bug (function_name);
	}
	if (conn_cmap != cmap) continue;
	(void) write_cmap_indices (connection, cmap);
    }
}   /*  End Function notify_cmap_resize  */

static void notify_cmap_modify (Kcolourmap cmap)
/*  This routine will transmit a change in the colourmap colours to all
    full_colourmap clients.
    The colourmap must be given by  cmap  .
    The routine returns nothing.
*/
{
    Connection connection;
    Kcolourmap conn_cmap;
    unsigned int num_connections;
    unsigned int connection_count;
    static char function_name[] = "notify_cmap_modify";

    VERIFY_COLOURMAP (cmap);
    num_connections = conn_get_num_serv_connections ("full_colourmap");
    for (connection_count = 0; connection_count < num_connections;
	 ++connection_count)
    {
	if ( ( connection = conn_get_serv_connection ("full_colourmap",
						      connection_count) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error getting connection: %u\n",
			    connection_count);
	    a_prog_bug (function_name);
	}
	if ( ( conn_cmap = (Kcolourmap) conn_get_connection_info (connection) )
	    == NULL )
	{
	    (void) fprintf (stderr,
			    "full_colourmap connection has no colourmap\n");
	    a_prog_bug (function_name);
	}
	if (conn_cmap != cmap) continue;
	(void) write_full_cmap (connection, cmap);
    }
}   /*  End Function notify_cmap_modify  */

static flag change_cmap_size (Kcolourmap cmap, unsigned int num_cells,
			      unsigned int min_cells, flag notify,
			      unsigned short *colour_array,
			      packet_desc *pack_desc, char *packet)
/*  This routine will change the size of a colourmap. The  resize_func  IS
    called on a successfull size change.
    The colourmap must be given by  cmap  .
    The number of colourcells required for the colourmap must be given by
    num_cells  .If this is less then 2 the number of cells is not changed.
    The minimum number of colourcells that MUST be allocated must be given by
    min_cells  .If this is less than 2, no lower limit is imposed.
    If the  notify  flag is TRUE, then the resize function registered with the
    colourmap is called if the colourmap actually changed size.
    If  colour_array  is not NULL, then the routine will use the allocation
    pointed to by  colour_array  ,  pack_desc  and  packet  for the colour
    intensities.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int num_to_alloc;
    unsigned int num_allocated;
    char *top_packet;
    unsigned short *intensities;
    unsigned long *pixel_values;
    packet_desc *top_pack_desc;
    static char function_name[] = "change_cmap_size";

    VERIFY_COLOURMAP (cmap);
    if (num_cells == cmap->size)
    {
	/*  No change  */
	if (colour_array != NULL)
	{
	    ds_dealloc_packet (cmap->top_pack_desc, cmap->top_packet);
	    cmap->top_pack_desc = pack_desc;
	    cmap->top_packet = packet;
	    cmap->intensities = colour_array;
	}
	return (TRUE);
    }
    if (num_cells < 2)
    {
	/*  No change wanted  */
	if (colour_array != NULL)
	{
	    ds_dealloc_packet (cmap->top_pack_desc, cmap->top_packet);
	    cmap->top_pack_desc = pack_desc;
	    cmap->top_packet = packet;
	    cmap->intensities = colour_array;
	}
	return (TRUE);
    }
    if (num_cells < min_cells)
    {
	(void) fprintf (stderr, "num_cells: %u is less than min_cells: %u\n",
			num_cells, min_cells);
	a_prog_bug (function_name);
    }
    /*  Change size  */
    if ( ( pixel_values = (unsigned long *)
	  m_alloc (sizeof *pixel_values * num_cells) )
	== NULL )
    {
	m_error_notify (function_name, "array of pixel values");
	return (FALSE);
    }
    if (num_cells < cmap->size)
    {
	/*  Reduce size  */
	m_copy ( (char *) pixel_values, (char *) cmap->pixel_values,
		sizeof *pixel_values * num_cells );
	if (colour_array == NULL)
	{
	    /*  Allocate new data structure  */
	    if ( ( intensities =
		  ds_cmap_alloc_colourmap (num_cells, (multi_array **) NULL,
					   &top_pack_desc,
					   &top_packet) )
		== NULL )
	    {
		m_error_notify (function_name, "array of intensities");
		return (FALSE);
	    }
	}
	else
	{
	    intensities = colour_array;
	    top_pack_desc = pack_desc;
	    top_packet = packet;
	}
	if (cmap->free_func != NULL)
	{
	    (*cmap->free_func) (cmap->size - num_cells,
				cmap->pixel_values + num_cells,
				cmap->dpy_handle);
	}
	ds_dealloc_packet (cmap->top_pack_desc, cmap->top_packet);
	cmap->size = num_cells;
	cmap->top_pack_desc = top_pack_desc;
	cmap->top_packet = top_packet;
	cmap->intensities = intensities;
    }
    else
    {
	/*  Increase size  */
	num_to_alloc = num_cells - cmap->size;
	m_copy ( (char *) pixel_values, (char *) cmap->pixel_values,
		sizeof *pixel_values * cmap->size );
	/*  Allocate colourcells first to see how big data structure
	    needs to be  */
	if (min_cells > 1) min_cells -= cmap->size;
	if (min_cells < 1) min_cells = 1;
	if (cmap->alloc_func == NULL) num_allocated = num_to_alloc;
	else if ( ( num_allocated =
	      (*cmap->alloc_func) (num_to_alloc,
				   pixel_values + cmap->size, min_cells,
				   cmap->dpy_handle) )
	    < min_cells )
	{
	    /*  Failed  */
	    (void) fprintf (stderr, "Could not allocate: %u colourcells\n",
			    num_to_alloc);
	    m_free ( (char *) pixel_values );
	    return (FALSE);
	}
	/*  Size has increased  */
	if (colour_array == NULL)
	{
	    /*  Allocate new data structure  */
	    if ( ( intensities =
		  ds_cmap_alloc_colourmap (num_allocated + cmap->size,
					   (multi_array **) NULL,
					   &top_pack_desc, &top_packet) )
		== NULL )
	    {
		m_error_notify (function_name, "array of intensities");
		if (cmap->free_func != NULL)
		{
		    (*cmap->free_func) (num_allocated,
					pixel_values + cmap->size,
					cmap->dpy_handle);
		}
		m_free ( (char *) pixel_values );
		return (FALSE);
	    }
	}
	else
	{
	    intensities = colour_array;
	    top_pack_desc = pack_desc;
	    top_packet = packet;
	}
	cmap->size += num_allocated;
	if (cmap->intensities != NULL)
	{
	    ds_dealloc_packet (cmap->top_pack_desc, cmap->top_packet);
	}
	cmap->top_pack_desc = top_pack_desc;
	cmap->top_packet = top_packet;
	cmap->intensities = intensities;
    }
    if (cmap->pixel_values != NULL)
    {
	m_free ( (char *) cmap->pixel_values );
    }
    cmap->pixel_values = pixel_values;
    /*  Colourmap has been resized (colours not computed yet)  */
    if (notify) (void) c_call_callbacks (cmap->resize_list, NULL);
    /*  Transmit this colourmap to any slaves of it  */
    notify_cmap_resize (cmap);
    return (TRUE);
}   /*  End Function change_cmap_size  */

#ifdef dummy
unsigned short *ds-cmap_read_colourmap (arrayfile, size, reordering_done)
/*  This routine will read in a Karma arrayfile and find a single colourmap.
    The name of the arrayfile must be pointed to by  arrayfile  .
    The size of the colourmap will be written to storage pointed to by  size  .
    The routine is tolerant of incorrect ordering of the intensity elements.
    If they are ordered incorrectly, the data will be re-ordered, and the
    value TRUE will be written to the storage pointed to by  reordering_done  ,
    else the value of FALSE will be written here. If this is NULL, nothing is
    written here.
    The routine returns a pointer to the colourmap on success,
    else it returns NULL.
*/
char *arrayfile;
unsigned int *size;
flag *reordering_done;
{
    unsigned int *array_indices;
    unsigned int num_found;
    multi_array *multi_desc;

    if ( ( multi_desc = dsx*fr_get_multi (arrayfile, FALSE, K_CH_MAP_NEVER) )
	== NULL )
    {
	return (NULL);
    }
    if ( ( array_indices = ds-cmap_get_all_colourmaps (multi_desc, &num_found,
						      reordering_done,
						      (char **) NULL,
						      (double *) NULL,
						      (unsigned int) 0) )
	== NULL )
    {
	return (NULL);
    }
    m_free ( (char *) array_indices );
    if (num_found != 1)
    {
	(void) fprintf (stderr,
			"More than one colourmap in arrayfile: \"%s\"\n",
			arrayfile);
	return (NULL);
    }
    return ( ds-cmap_find_colourmap (multi_desc->headers[0],
				     multi_desc->data[0], size,
				     (flag *) NULL,
				     (char **) NULL, (double *) NULL,
				     (unsigned int) 0) );
}   /*  End Function ds-cmap_read_colourmap  */
#endif
