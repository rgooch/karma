/*LINTLIBRARY*/
/*PREFIX:"kcmap_"*/
/*  cmap.c

    This code provides Kcolourmap objects.

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

    Last updated by Richard Gooch   21-SEP-1993: Improved error message in
  kcmap_change  when colourmap resize fails.


*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <karma.h>
#include <karma_conn.h>
#include <karma_dsrw.h>
#include <karma_ds.h>
#include <karma_cf.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>

typedef struct colourmap_type * Kcolourmap;

#define KCOLOURMAP_DEFINED
#include <karma_kcmap.h>

#define MAGIC_NUMBER (unsigned int) 2140872384
#define PROTOCOL_VERSION (unsigned int) 0

#define VERIFY_COLOURMAP(cmap) if (cmap == NULL) \
{(void) fprintf (stderr, "NULL colourmap passed\n"); \
 a_prog_bug (function_name); } \
if ( (*cmap).magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid colourmap object\n"); \
 a_prog_bug (function_name); }

#define CMAP_FUNC_TYPE_RGB (unsigned int) 0
#define CMAP_FUNC_TYPE_CIE (unsigned int) 1

struct colourmap_type
{
    unsigned int magic_number;
    Kdisplay dpy_handle;
    unsigned int size;
    unsigned long *pixel_values;
    unsigned short *intensities;
    packet_desc *top_pack_desc;
    char *top_packet;
    char *modify_func_name;
    struct resize_func *first_resize_func;
    struct resize_func *last_resize_func;
    Connection master;
    flag full_cmap;
    flag modifiable;
};

struct cmap_func_type
{
    unsigned int type;
    char *name;
    void (*func) ();
    unsigned int min_cells;
    unsigned int max_cells;
    struct cmap_func_type *next;
};

struct resize_func
{
    void (*func) ();
    void *info;
    struct resize_func *next;
};

/*  Private data  */
static unsigned int (*alloc_ccells_func) () = NULL;
static void (*free_ccells_func) () = NULL;
static void (*store_ccells_func) () = NULL;
static void (*get_location_func) () = NULL;

static struct cmap_func_type *cmap_functions = NULL;
static Kcolourmap shareable_colourmap = NULL;
static Kcolourmap slaveable_colourmap = NULL;


/*  Local functions  */
static struct cmap_func_type *get_cmap_function (/* name */);
static flag register_new_cmap_indices_slave (/* connection, info */);
static flag register_new_full_cmap_slave (/* connection, info */);
static flag verify_slave_cmap_connection (/* info */);
static flag register_cmap_indices_connection (/* connection, info */);
static flag register_full_cmap_connection (/* connection, info */);
static void register_cmap_connection_close (/* connection, info */);
static flag write_cmap_indices (/* connection, cmap */);
static flag read_cmap_indices (/* connection, cmap */);
static flag write_full_cmap (/* connection, cmap */);
static flag read_full_cmap (/* connection, cmap */);
static void transmit_cmap_resize (/* cmap */);
static void transmit_cmap_modify (/* cmap */);
static flag change_cmap_size (/* cmap, num_cells, tolerant, notify,
			       colour_array, pack_desc, packet */);
static void call_resize_funcs (/* cmap */);


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void kcmap_init (alloc_func, free_func, store_func, location_func)
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
    flag tolerant;
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
unsigned int (*alloc_func) ();
void (*free_func) ();
void (*store_func) ();
void (*location_func) ();
{
    extern unsigned int (*alloc_ccells_func) ();
    extern void (*free_ccells_func) ();
    extern void (*store_ccells_func) ();
    extern void (*get_location_func) ();
    static char function_name[] = "kcmap_init";

    if (alloc_ccells_func != NULL)
    {
	(void) fprintf (stderr, "Initialisation already performed\n");
	a_prog_bug (function_name);
    }
    alloc_ccells_func = alloc_func;
    free_ccells_func = free_func;
    store_ccells_func = store_func;
    get_location_func = location_func;
    kcmap_add_RGB_func ("Greyscale1", cf_greyscale1, 0, 0);
    kcmap_add_RGB_func ("Greyscale2", cf_greyscale2, 0, 0);
    kcmap_add_RGB_func ("Random Grey", cf_random_grey, 0, 0);
    kcmap_add_RGB_func ("Random Pseudocolour", cf_random_pseudocolour, 0, 0);
    kcmap_add_RGB_func ("Glynn Rogers1", cf_rainbow1, 0, 0);
    kcmap_add_RGB_func ("Glynn Rogers2", cf_rainbow2, 0, 0);
    kcmap_add_RGB_func ("Glynn Rogers3", cf_rainbow3, 0, 0);
    kcmap_add_RGB_func ("Cyclic 1", cf_cyclic1, 0, 0);
    kcmap_add_RGB_func ("Velocity: Compensating Tones",
			  cf_velocity_compensating_tones, 0, 0);
    kcmap_add_RGB_func ("Compressed Colourmap 3R2G2B",
			  cf_compressed_colourmap_3r2g2b, 128, 128);
    conn_register_server_protocol ("colourmap_indices", PROTOCOL_VERSION, 0,
				   register_new_cmap_indices_slave,
				   ( flag (*) () ) NULL, ( void (*) () ) NULL);
    conn_register_client_protocol ("colourmap_indices", PROTOCOL_VERSION, 1,
				   verify_slave_cmap_connection,
				   register_cmap_indices_connection,
				   read_cmap_indices,
				   register_cmap_connection_close);
    conn_register_server_protocol ("full_colourmap", PROTOCOL_VERSION, 0,
				   register_new_full_cmap_slave,
				   ( flag (*) () ) NULL,
				   ( void (*) () ) NULL);
    conn_register_client_protocol ("full_colourmap", PROTOCOL_VERSION, 1,
				   verify_slave_cmap_connection,
				   register_full_cmap_connection,
				   read_full_cmap,
				   register_cmap_connection_close);
}   /*  End Function kcmap_init  */

/*PUBLIC_FUNCTION*/
void kcmap_add_RGB_func (name, func, min_cells, max_cells)
/*  This routine will register a named function which will compute RGB
    intensity values for a colourmap. This function is typically called in
    response to a call to  kcmap_modify  .
    The name of the colourmap function must be pointed to by  name  .
    The function which is used to compute the RGB values must be pointed to by
    func  .
    The interface to this routine is as follows:

    void func (num_cells, reds, greens, blues, stride, x, y, var_param)
    *   This routine will write RGB colour intensity values to a number of
        colourcells. This routine is called in response to a call to
	kcmap_modify  .
        The number of colour cells to modify must be given by  num_cells  .
	The red intensity values must be pointed to by  reds  .
	The green intensity values must be pointed to by  greens  .
	The blue intensity values must be pointed to by  blues  .
	The stride (in unsigned shorts) between intensity values in each array
	must be given by  stride  .
	The parameters used to compute the colour values must be given by
	x  ,  y  and  var_param  .
	The routine returns nothing.
    *
    unsigned int num_cells;
    unsigned short *reds;
    unsigned short *greens;
    unsigned short *blues;
    unsigned int stride;
    double x;
    double y;
    void *var_param;

    The minimum number of colourcells that must be allocated for this function
    to work must be given by  min_cells  .If this is less than 2, no minimum is
    defined.
    The maximum number of colourcells that must be allocated for this function
    to work must be given by  max_cells  .If this is less than 2, no maximum is
    defined.
    The routine returns nothing.
*/
char *name;
void (*func) ();
unsigned int min_cells;
unsigned int max_cells;
{
    struct cmap_func_type *new_entry;
    extern struct cmap_func_type *cmap_functions;
    static char function_name[] = "kcmap_add_RGB_func";

    if ( ( new_entry = (struct cmap_func_type *) m_alloc (sizeof *new_entry) )
	== NULL )
    {
	m_abort (function_name, "new function entry");
    }
    (*new_entry).type = CMAP_FUNC_TYPE_RGB;
    if ( ( (*new_entry).name = st_dup (name) ) == NULL )
    {
	m_abort (function_name, "new function entry name");
    }
    (*new_entry).func = func;
    (*new_entry).min_cells = min_cells;
    (*new_entry).max_cells = max_cells;
    /*  Insert at top of list  */
    (*new_entry).next = cmap_functions;
    cmap_functions = new_entry;
}   /*  End Function kcmap_add_RGB_func  */

/*PUBLIC_FUNCTION*/
Kcolourmap kcmap_create (name, num_cells, tolerant, dpy_handle)
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
char *name;
unsigned int num_cells;
flag tolerant;
Kdisplay dpy_handle;
{
    Kcolourmap cmap;
    unsigned int min_cells;
    struct cmap_func_type *cmap_func;
    extern Kcolourmap shareable_colourmap;
    extern Kcolourmap slaveable_colourmap;
    extern unsigned int (*alloc_ccells_func) ();
    static char *def_name = "Greyscale1";
    static char function_name[] = "kcmap_create";

    if (alloc_ccells_func == NULL)
    {
	(void) fprintf (stderr,
			"Lower level display routines not registered yet\n");
	a_prog_bug (function_name);
    }
    if (num_cells < 2)
    {
	(void) fprintf (stderr, "Must specify colourmap size of at least 2\n");
	a_prog_bug (function_name);
    }
    if (name == NULL) name = def_name;
    /*  Verify if colourmap function exists  */
    if ( ( cmap_func = get_cmap_function (name) ) == NULL )
    {
	(void) fprintf (stderr, "Colourmap function: \"%s\" does not exist\n",
			name);
	a_prog_bug (function_name);
    }
    if ( ( (*cmap_func).min_cells > 1 ) && 
	(num_cells < (*cmap_func).min_cells) )
    {
	(void) fprintf (stderr, "Requested number of cells: %u is less than\n",
			num_cells);
	(void) fprintf (stderr,
			"minimum number of cells: %u for colourmap function: %s\n",
			(*cmap_func).min_cells, (*cmap_func).name);
	return (NULL);
    }
    if ( ( (*cmap_func).max_cells > 1 ) && 
	(num_cells > (*cmap_func).max_cells) )
    {
	(void) fprintf (stderr,
			"Requested number of cells: %u is greater than\n",
			num_cells);
	(void) fprintf (stderr,
			"maximum number of cells: %u for colourmap function: %s\n",
			(*cmap_func).max_cells, (*cmap_func).name);
	return (NULL);
    }
    if ( ( cmap = (Kcolourmap) m_alloc (sizeof *cmap) ) == NULL )
    {
	m_error_notify (function_name, "colourmap");
	return (NULL);
    }
    (*cmap).dpy_handle = dpy_handle;
    (*cmap).size = 0;
    (*cmap).pixel_values = NULL;
    (*cmap).intensities = NULL;
    (*cmap).magic_number = MAGIC_NUMBER;
    if (tolerant)
    {
	min_cells = (*cmap_func).min_cells;
    }
    else
    {
	min_cells = num_cells;
    }
    if (change_cmap_size (cmap, num_cells, min_cells, FALSE,
			  (unsigned short *) NULL,
			  (packet_desc *) NULL, (char *) NULL) != TRUE)
    {
	m_error_notify (function_name, "array of pixel values");
	(*cmap).magic_number = 0;
	m_free ( (char *) cmap );
	return (NULL);
    }
    (*cmap).modify_func_name = (*cmap_func).name;
    (*cmap).first_resize_func = NULL;
    (*cmap).last_resize_func = NULL;
    (*cmap).master = NULL;
    (*cmap).modifiable = TRUE;
    kcmap_modify (cmap, (double) 0.5, (double) 0.5, (void *) NULL);
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
    return (cmap);
}   /*  End Function kcmap_create  */

/*PUBLIC_FUNCTION*/
void kcmap_register_resize_func (cmap, resize_func, info)
/*  This routine will register a resize function for a high level colourmap.
    The resize function will be called whenever the colourmap is resized.
    Many resize functions may be registered per colourmap. The first
    function registered is the first function called upon resize.
    The colourmap must be given by  cmap  .
    The function which is called when the colourmap is resized must be pointed
    to by  resize_func  .
    The interface to this routine is as follows:

    void resize_func (cmap, info)
    *   This routine registers a change in the size of a colourmap.
        The colourmap must be given by  cmap  .
	The arbitrary colourmap information pointer is pointed to by  info  .
	The routine returns nothing.
    *
    Kcolourmap cmap;
    void **info;

    The initial arbitrary colourmap information pointer must be given by  info
    The routine returns nothing.
*/
Kcolourmap cmap;
void (*resize_func) ();
void *info;
{
    struct resize_func *new_func;
    static char function_name[] = "kcmap_register_resize_func";

    VERIFY_COLOURMAP (cmap);
    if (resize_func == NULL) return;
    if ( ( new_func = (struct resize_func *) m_alloc (sizeof *new_func) )
	== NULL )
    {
	m_abort (function_name, "resize function entry");
    }
    (*new_func).func = resize_func;
    (*new_func).info = info;
    (*new_func).next = NULL;
    if ( (*cmap).first_resize_func == NULL )
    {
	/*  Create list of resize functions  */
	(*cmap).first_resize_func = new_func;
    }
    else
    {
	/*  Append to list of resize functions  */
	(* (*cmap).last_resize_func ).next = new_func;
    }
    (*cmap).last_resize_func = new_func;
}   /*  End Function kcmap_register_resize_func  */

/*PUBLIC_FUNCTION*/
flag kcmap_change (cmap, new_name, num_cells, tolerant)
/*  This routine will change the active function (algorithm) used to calculate
    the colours in a colourmap and the size of the colourmap.
    The colourmap must be given by  cmap  .
    The new function name must be pointed to by  new_name  .If this is NULL
    then the active function is not changed.
    The number of colourcells required for the colourmap must be given by
    num_cells  .If this is less then 2 the number of cells is not changed.
    If the flag  tolerant  is TRUE, then the routine will try to allocate
    as many colourcells as possible (up to  num_cells  ), else it will
    fail if it could not allocatate all required colourcells.
    The routine returns TRUE on success, else it returns FALSE.
*/
Kcolourmap cmap;
char *new_name;
unsigned int num_cells;
flag tolerant;
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
    if ( ( (*cmap).master != NULL ) && (new_name != NULL) )
    {
	/*  Slave colourmap: close connection  */
	if (conn_close ( (*cmap).master ) != TRUE)
	{
	    (void) fprintf (stderr, "Error closing slave connection\n");
	    return (FALSE);
	}
    }
    if ( (*cmap).master != NULL )
    {
	(void) fprintf (stderr, "Attempt to resize a slave colourmap\n");
	a_prog_bug (function_name);
    }
    (*cmap).modifiable = TRUE;
    if (new_name == NULL)
    {
	/*  Get old colourmap function  */
	if ( ( cmap_func = get_cmap_function ( (*cmap).modify_func_name ) )
	    == NULL )
	{
	    (void) fprintf (stderr,
			    "Colourmap function: \"%s\" does not exist\n",
			    (*cmap).modify_func_name);
	    a_prog_bug (function_name);
	}
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
	min_cells = (*cmap_func).min_cells;
	if ( (*cmap_func).min_cells > 1 )
	{
	    /*  Lower limit specified  */
	    if ( (*cmap).size < (*cmap_func).min_cells )
	    {
		num_cells = (*cmap_func).min_cells;
	    }
	}
	if ( (*cmap_func).max_cells > 1 )
	{
	    /*  Upper limit specified  */
	    if ( (*cmap).size > (*cmap_func).max_cells )
	    {
		num_cells = (*cmap_func).max_cells;
	    }
	}
    }
    else
    {
	/*  Change specified  */
	if ( ( (*cmap_func).min_cells > 1 ) && 
	    (num_cells < (*cmap_func).min_cells) )
	{
	    (void) fprintf (stderr, "Requested number of cells: %u is less than\n",
			    num_cells);
	    (void) fprintf (stderr,
			    "minimum number of cells: %u for colourmap function: %s\n",
			    (*cmap_func).min_cells, (*cmap_func).name);
	    return (FALSE);
	}
	if ( ( (*cmap_func).max_cells > 1 ) && 
	    (num_cells > (*cmap_func).max_cells) )
	{
	    (void) fprintf (stderr,
			    "Requested number of cells: %u is greater than\n",
			    num_cells);
	    (void) fprintf (stderr,
			    "maximum number of cells: %u for colourmap function: %s\n",
			    (*cmap_func).max_cells, (*cmap_func).name);
	    return (FALSE);
	}
	/*  num_cells  is within legal range  */
	if (tolerant)
	{
	    min_cells = (*cmap_func).min_cells;
	}
	else
	{
	    min_cells = num_cells;
	}
    }
    if (change_cmap_size (cmap, num_cells, min_cells, TRUE,
			  (unsigned short *) NULL,
			  (packet_desc *) NULL, (char *) NULL) != TRUE)
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
    (*cmap).modify_func_name = (*cmap_func).name;
    kcmap_modify (cmap, (double) 0.5, (double) 0.5, (void *) NULL);
    return (TRUE);
}   /*  End Function kcmap_change  */

/*PUBLIC_FUNCTION*/
void kcmap_modify (cmap, x, y, var_param)
/*  This routine will call the active colour compute function to change the
    colourmap colours in a colourmap.
    The colourmap must be given by  cmap  .
    The parameters used to compute the colour values must be given by
    x  ,  y  and  var_param  .
    The routine returns nothing.
*/
Kcolourmap cmap;
double x;
double y;
void *var_param;
{
    struct cmap_func_type *cmap_func;
    extern void (*store_ccells_func) ();
    static char function_name[] = "kcmap_modify";

    VERIFY_COLOURMAP (cmap);
    if (!(*cmap).modifiable)
    {
	/*  This colourmap is a not modifiable: ignore  */
	return;
    }
    if ( ( cmap_func = get_cmap_function ( (*cmap).modify_func_name ) )
	== NULL )
    {
	(void) fprintf (stderr,
			"Colourmap function: \"%s\" does not exist\n",
			(*cmap).modify_func_name);
	a_prog_bug (function_name);
    }
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
    (* (*cmap_func).func ) ( (*cmap).size, 
			    (*cmap).intensities, (*cmap).intensities + 1,
			    (*cmap).intensities + 2, 3,
			    x, y, var_param );
    (*store_ccells_func) ( (*cmap).size, (*cmap).pixel_values,
			  (*cmap).intensities, (*cmap).intensities + 1,
			  (*cmap).intensities + 2, 3,
			  (*cmap).dpy_handle );
    /*  Transmit this colourmap to any slaves of it  */
    transmit_cmap_modify (cmap);
}   /*  End Function kcmap_modify  */

/*PUBLIC_FUNCTION*/
char **kcmap_list_funcs ()
/*  This routine will return a pointer to an array of supported colour function
    names. This array is dynamically allocated, and should be freed using
    m_free  .
    NOTE: the names in the array are statically allocated.
    The array is terminated with a NULL pointer.
*/
{
    int count;
    unsigned int num_funcs;
    struct cmap_func_type *entry;
    char **names;
    extern struct cmap_func_type *cmap_functions;
    static char function_name[] = "kcmap_list_funcs";
 
    for (num_funcs = 0, entry = cmap_functions; entry != NULL;
	 ++num_funcs, entry = (*entry).next);
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
	 --count, entry = (*entry).next)
    {
	names[count] = (*entry).name;
    }
    names[num_funcs] = NULL;
    return (names);
}   /*  End Function kcmap_list_funcs  */

/*PUBLIC_FUNCTION*/
char *kcmap_get_active_func (cmap)
/*  This routine will get the name of the active colour function for a
    colourmap.
    The colourmap must be given by  cmap  .
    The routine returns a pointer to the name of the colour function. This
    name must not be freed.
*/
Kcolourmap cmap;
{
    static char function_name[] = "kcmap_get_active_func";

    VERIFY_COLOURMAP (cmap);
    return ( (*cmap).modify_func_name );
}   /*  End Function kcmap_get_active_func  */

/*PUBLIC_FUNCTION*/
unsigned int kcmap_get_pixels (cmap, pixel_values)
/*  This routine will determine the number of colourcells in a colourmap.
    The colourmap must be given by  cmap  .
    The routine will write a pointer to the array of pixel values to the 
    storage pointed to by  pixel_values  .If this is NULL, nothing is written
    here.
    The routine returns the number of colourcells allocated.
*/
Kcolourmap cmap;
unsigned long **pixel_values;
{
    static char function_name[] = "kcmap_get_pixels";

    VERIFY_COLOURMAP (cmap);
    if (pixel_values != NULL)
    {
	*pixel_values = (*cmap).pixel_values;
    }
    return ( (*cmap).size );
}   /*  End Function kcmap_get_pixels  */

/*PUBLIC_FUNCTION*/
unsigned long kcmap_get_pixel (cmap, index)
/*  This routine will get a numbered pixel value from a colourmap.
    The colourmap must be given by  cmap  .
    The index of the pixel must be given by  index  .
    The routine returns the pixel value.
*/
Kcolourmap cmap;
unsigned int index;
{
    static char function_name[] = "kcmap_get_pixel";

    VERIFY_COLOURMAP (cmap);
    if (index >= (*cmap).size)
    {
	(void) fprintf (stderr,
			"index: %u  is not less than colourmap size: %u\n",
			index, (*cmap).size);
	a_prog_bug (function_name);
    }
    return ( (*cmap).pixel_values[index] );
}   /*  End Function kcmap_get_pixel  */

/*PUBLIC_FUNCTION*/
void kcmap_prepare_for_slavery (cmap)
/*  This routine will register a colourmap to be the choosen colourmap for
    subsequent attempts to open a slave colourmap connection.
    In order to make the colourmap a slave, a subsequent call to
    conn_attempt_connection  must be made.
    The colourmap must be given by  cmap  .
    The routine returns nothing.
*/
Kcolourmap cmap;
{
    extern Kcolourmap slaveable_colourmap;
    static char function_name[] = "kcmap_prepare_for_slavery";

    VERIFY_COLOURMAP (cmap);
    slaveable_colourmap = cmap;
}   /*  End Function kcmap_prepare_for_slavery  */

/*PUBLIC_FUNCTION*/
flag kcmap_copy_to_struct (cmap, top_pack_desc, top_packet)
/*  This routine will copy the colour data in a colourmap into a newly
    allocated Karma data structure. This data structure may be subsequently
    deallocated.
    The colourmap must be given by  cmap  .
    The pointer to the top level packet descriptor that is allocated will be
    written to the storage pointed to by  top_pack_desc  .
    The pointer to the top level packet that is allocated will be written to
    the storage pointed to by  top_packet  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Kcolourmap cmap;
packet_desc **top_pack_desc;
char **top_packet;
{
    static char function_name[] = "kcmap_copy_to_struct";

    VERIFY_COLOURMAP (cmap);
    if ( (*cmap).intensities == NULL )
    {
	(void) fprintf (stderr, "Colourmap has no colour information\n");
	return (FALSE);
    }
    if ( ( *top_pack_desc = ds_copy_desc_until ( (*cmap).top_pack_desc,NULL ) )
	== NULL )
    {
	m_error_notify (function_name, "top level packet descriptor");
	*top_packet = NULL;
	return (FALSE);
    }
    if ( ( *top_packet = ds_alloc_data ( (*cmap).top_pack_desc, FALSE, TRUE ) )
	== NULL )
    {
	m_error_notify (function_name, "top level packet");
	ds_dealloc_packet (*top_pack_desc, NULL);
	*top_pack_desc = NULL;
	return (FALSE);
    }
    if (ds_copy_data ( (*cmap).top_pack_desc, (*cmap).top_packet,
		      *top_pack_desc, *top_packet ) != TRUE)
    {
	(void) fprintf (stderr, "Data structure copy not identical\n");
	a_prog_bug (function_name);
    }
    return (TRUE);
}   /*  End Function kcmap_copy_to_struct  */

/*PUBLIC_FUNCTION*/
flag kcmap_copy_from_struct (cmap, top_pack_desc, top_packet)
/*  This routine will copy the colour data in a Karma data structure into a
    colourmap. If the colourmap changes size, then the  resize_func  registered
    is called.
    The colourmap must be given by  cmap  .
    The top level packet descriptor must be pointed to by  top_pack_desc  .
    The top level packet must be pointed to by  top_packet  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Kcolourmap cmap;
packet_desc *top_pack_desc;
char *top_packet;
{
    unsigned int colourmap_size;
    unsigned short *colour_array;
    packet_desc *pack_desc;
    char *packet;
    extern void (*store_ccells_func) ();
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
    if (ds_copy_data (top_pack_desc, top_packet, pack_desc, packet ) != TRUE)
    {
	(void) fprintf (stderr, "Data structure copy not identical\n");
	a_prog_bug (function_name);
    }
    /*  Now get a handle to the colourmap  */
    if ( ( colour_array = ds_cmap_find_colourmap (pack_desc, packet,
						  &colourmap_size,
						  (flag *) NULL,
						  (char **) NULL,
						  (double *) NULL,
						  (unsigned int) 0) ) == NULL )
    {
	(void) fprintf (stderr, "Could not find colourmap\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    if ( (*cmap).master != NULL )
    {
	/*  Slave colourmap: close connection  */
	if (conn_close ( (*cmap).master ) != TRUE)
	{
	    (void) fprintf (stderr, "Error closing slave connection\n");
	    ds_dealloc_packet (pack_desc, packet);
	    return (FALSE);
	}
    }
    if (change_cmap_size (cmap, colourmap_size, colourmap_size, TRUE,
			  colour_array, pack_desc, packet) != TRUE)
    {
	(void) fprintf (stderr, "Could not reallocate colourmap\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    (*store_ccells_func) ( (*cmap).size, (*cmap).pixel_values,
			  (*cmap).intensities, (*cmap).intensities + 1,
			  (*cmap).intensities + 2, 3,
			  (*cmap).dpy_handle );
    (*cmap).modifiable = FALSE;
    /*  Transmit this colourmap to any slaves of it  */
    transmit_cmap_modify (cmap);
    return (TRUE);
}   /*  End Function kcmap_copy_from_struct  */


/*  Private routines follow  */

static struct cmap_func_type *get_cmap_function (name)
/*  This routine will get the named colourmap function
    The name must be pointed to by  name  .
    The routine returns a pointer to a struct on success, else it returns NULL
    (indicating there is no colourmap function with that name).
*/
char *name;
{
    struct cmap_func_type *entry;
    extern struct cmap_func_type *cmap_functions;

    for (entry = cmap_functions; entry != NULL; entry = (*entry).next)
    {
	if (strcmp (name, (*entry).name) == 0)
	{
	    /*  Found it  */
	    return (entry);
	}
    }
    return (NULL);
}   /*  End Function get_cmap_function  */


/*  Routines related to incoming colourmap connections  */

static flag register_new_cmap_indices_slave (connection, info)
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
Connection connection;
void **info;
{
    Channel channel;
    unsigned long serv_display_num;
    unsigned long serv_hostaddr;
    char accepted;
    char false = FALSE;
    char true = TRUE;
    extern Kcolourmap shareable_colourmap;
    extern void (*get_location_func) ();
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    channel = conn_get_channel (connection);
    if ( (shareable_colourmap == NULL) ||
	( (*shareable_colourmap).master != NULL ) )
    {
	/*  Cannot service requests: tell client  */
	if (ch_write (channel, &false, 1) < 1)
	{
	    (void) fprintf (stderr, "Error writing rejection\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if (ch_flush (channel) != TRUE)
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
    (*get_location_func) ( (*shareable_colourmap).dpy_handle,
			  &serv_hostaddr, &serv_display_num );
    /*  Tell client what X Windows display we are connected to  */
    if (pio_write32 (channel, serv_hostaddr) != TRUE)
    {
	return (FALSE);
    }
    if (pio_write32 (channel, serv_display_num) != TRUE)
    {
	return (FALSE);
    }
    if (ch_flush (channel) != TRUE)
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
    if (accepted != TRUE)
    {
	/*  Connection rejected by client  */
	return (FALSE);
    }
    /*  Client is happy about this connection: send pixel values  */
    if (write_cmap_indices (connection, shareable_colourmap) != TRUE)
    {
	(void) fprintf (stderr, "Error writing pixels\n");
	return (FALSE);
    }
    *info = (void *) shareable_colourmap;
    return (TRUE);
}   /*  End Function register_new_cmap_indices_slave  */

static flag register_new_full_cmap_slave (connection, info)
/*  This routine will register the connection of a new full_colourmap client
    The connection must be given by  connection  .
    Any appropriate information is pointed to by  info  (unused).
    The routine will write the colourmap colours to the connection.
    The routine returns TRUE on success, else it returns FALSE (indicating that
    the connection should be closed).
*/
Connection connection;
void **info;
{
    Channel channel;
    char false = FALSE;
    char true = TRUE;
    extern Kcolourmap shareable_colourmap;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    channel = conn_get_channel (connection);
    if ( (shareable_colourmap == NULL) ||
	( (*shareable_colourmap).master != NULL ) )
    {
	/*  Cannot service requests: tell client  */
	if (ch_write (channel, &false, 1) < 1)
	{
	    (void) fprintf (stderr, "Error writing rejection\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if (ch_flush (channel) != TRUE)
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
    if (write_full_cmap (connection, shareable_colourmap)
	!= TRUE)
    {
	(void) fprintf (stderr, "Error writing colourcell definitions\n");
	return (FALSE);
    }
    *info = (void *) shareable_colourmap;
    return (TRUE);
}   /*  End Function register_new_full_cmap_slave  */


/*  Routines related to outgoing colourmap connections  */

static flag verify_slave_cmap_connection (info)
/*   This routine will validate whether it is appropriate to open a slave
     colourmap connection.
     The routine will write the slaveable colourmap to the storage
     pointed to by  info  .The pointer value written here will be passed
     to the other routines.
     The routine returns TRUE if the connection should be attempted,
     else it returns FALSE (indicating the connection should be aborted).
     NOTE: Even if this routine is called and returns TRUE, there is no
     guarantee that the connection will be subsequently opened.
*/
void **info;
{
    extern Kcolourmap slaveable_colourmap;

    if (slaveable_colourmap == NULL)
    {
	(void) fprintf (stderr, "No slaveable colourmap registered\n");
	return (FALSE);
    }
    if ( (*slaveable_colourmap).master != NULL )
    {
	(void) fprintf (stderr, "Slaveable colourmap is already a slave\n");
	return (FALSE);
    }
    *info = (void *) slaveable_colourmap;
    return (TRUE);
}   /*  End Function verify_slave_cmap_connection  */

static flag register_cmap_indices_connection (connection, info)
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
Connection connection;
void **info;
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
    extern void (*free_ccells_func) ();
    extern void (*get_location_func) ();
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "register_cmap_indices_connection";

    channel = conn_get_channel (connection);
    cmap = (Kcolourmap) *info;
    VERIFY_COLOURMAP (cmap);
    num_cells = (*cmap).size;
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
    if (pio_read32 (channel, &server_display_inet_addr) != TRUE)
    {
	(void) fprintf (stderr,
			"Error reading Internet address of X server of colourmap server\n");
	return (FALSE);
    }
    if (pio_read32 (channel, &server_display_num) != TRUE)
    {
	(void) fprintf (stderr,
			"Error reading display number for colourmap server\n");
	return (FALSE);
    }
    /*  Server has said which X Display it is using: let's find out what one
	we are using  */
    (*get_location_func) ( (*cmap).dpy_handle,
			  &my_display_inet_addr, &my_display_num );
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
    if (ch_flush (channel) != TRUE)
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Now have a valid connection to the colourmap server, irrespective of
	whether it is connected to the same X Windows display as us or not  */
    (*cmap).master = connection;
    (*cmap).full_cmap = FALSE;
    (*cmap).modifiable = FALSE;
    /*  No colourcells need be allocated: free any  */
    if ( (*cmap).size > 0 )
    {
	(*free_ccells_func) ( (*cmap).size,
			     (*cmap).pixel_values,
			     (*cmap).dpy_handle );
    }
    if (read_cmap_indices (connection, info) != TRUE)
    {
	(void) fprintf (stderr, "Error reading pixels\n");
	(void) kcmap_change (cmap, (*cmap).modify_func_name, num_cells,TRUE);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function register_cmap_indices_connection  */

static flag register_full_cmap_connection (connection, info)
/*  This routine will register the connection to a  full_colourmap  server
    The connection must be given by  connection  .
    The colourmap to enslave must be pointed to by  info  .
    The routine will write the host Internet address and display number of the
    The routine returns TRUE on success, else it returns FALSE (indicating that
    the connection should be closed).
*/
Connection connection;
void **info;
{
    char server_happy;
    Channel channel;
    Kcolourmap cmap;
    unsigned int num_cells;
    extern void (*free_ccells_func) ();
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "register_full_cmap_connection";

    channel = conn_get_channel (connection);
    cmap = (Kcolourmap) *info;
    VERIFY_COLOURMAP (cmap);
    num_cells = (*cmap).size;
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
    (*cmap).master = connection;
    (*cmap).full_cmap = TRUE;
    (*cmap).modifiable = FALSE;
    if (read_full_cmap (connection, info) != TRUE)
    {
	(void) fprintf (stderr, "Error reading full colourmap\n");
	(void) kcmap_change (cmap, (*cmap).modify_func_name, num_cells,TRUE);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function register_full_cmap_connection  */

static void register_cmap_connection_close (connection, info)
/*  This routine will register the closure of the connection to the colourmap
    server.
    The connection must be given by  connection  .
    The colourmap to emancipate must be pointed to by  info  .
    The routine returns nothing.
*/
Connection connection;
void *info;
{
    unsigned int num_cells;
    Kcolourmap cmap;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "register_cmap_connection_close";

    cmap = (Kcolourmap) info;
    VERIFY_COLOURMAP (cmap);
    if ( (*cmap).master != connection )
    {
	(void) fprintf (stderr, "Invalid connection for colourmap object\n");
	a_prog_bug (function_name);
    }
    (*cmap).master = NULL;
    num_cells = (*cmap).size;
    (*cmap).modifiable = TRUE;
    if ( (*cmap).full_cmap != TRUE )
    {
	/*  Colourmap indicies: deallocate and change  */
	m_free ( (char *) (*cmap).pixel_values );
	(*cmap).pixel_values = NULL;
	(*cmap).size = 0;
	(void) kcmap_change (cmap, (char *) NULL, num_cells, TRUE);
    }
    /*  Everything deallocated  */
}   /*  End Function register_cmap_connection_close  */

static flag write_cmap_indices (connection, cmap)
/*  This routine will write an array of pixel values to a connection.
    The connection must be given by  connection  .
    The colourmap must be given by  cmap  .
    The routine returns TRUE on sucess, else it returns FALSE.
*/
Connection connection;
Kcolourmap cmap;
{
    Channel channel;
    unsigned int num_pixels;
    unsigned int pixel_count;
    unsigned long *pixel_values;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "write_cmap_indices";

    VERIFY_COLOURMAP (cmap);
    channel = conn_get_channel (connection);
    num_pixels = kcmap_get_pixels (cmap, &pixel_values);
    /*  Write 4 bytes indicicating how many pixel values are coming  */
    if (pio_write32 (channel, (unsigned long) num_pixels) != TRUE)
    {
	(void) fprintf (stderr, "Error writing number of pixels\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Write pixel values  */
    for (pixel_count = 0; pixel_count < num_pixels; ++pixel_count)
    {
	if (pio_write32 (channel, pixel_values[pixel_count]) != TRUE)
	{
	    (void) fprintf (stderr,
			    "Error writing pixel value: %u to channel\t%s\n",
			    pixel_count, sys_errlist[errno]);
	    return (FALSE);
	}
    }
    if (ch_flush (channel) != TRUE)
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  All pixels sent  */
    return (TRUE);
}   /*  End Function write_cmap_indices  */

static flag read_cmap_indices (connection, info)
/*  This routine will read an array of pixel values from a connection.
    The connection must be given by  connection  .
    The colourmap must be pointed to by  info  .
    The routine returns TRUE on sucess, else it returns FALSE.
*/
Connection connection;
void **info;
{
    Channel channel;
    Kcolourmap cmap;
    unsigned int pixel_count;
    unsigned long pixels_to_read;
    unsigned long *pixel_values;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "read_cmap_indices";

    cmap = (Kcolourmap) *info;
    VERIFY_COLOURMAP (cmap);
    channel = conn_get_channel (connection);
    if ( (*cmap).intensities != NULL )
    {
	ds_dealloc_packet ( (*cmap).top_pack_desc, (*cmap).top_packet );
	(*cmap).intensities = NULL;
	(*cmap).top_pack_desc = NULL;
	(*cmap).top_packet = NULL;
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
	if (pio_read32 (channel, pixel_values + pixel_count) != TRUE)
	{
	    (void) fprintf (stderr, "Error reading pixel value: %u\t%s\n",
			    pixel_count, sys_errlist[errno]);
	    m_free ( (char *) pixel_values );
	    return (FALSE);
	}
    }
    m_free ( (char *) (*cmap).pixel_values );
    (*cmap).pixel_values = pixel_values;
    (*cmap).size = pixels_to_read;
    call_resize_funcs (cmap);
    return (TRUE);
}   /*  End Function read_cmap_indices  */

static flag write_full_cmap (connection, cmap)
/*  This routine will write an array of pixel colours to a connection.
    The connection must be given by  connection  .
    The colourmap must be given by  cmap  .
    The routine will write the colourmap using the Karma colourmap format
    (which utilises the Karma general data structure format).
    The routine returns TRUE on sucess, else it returns FALSE.
*/
Connection connection;
Kcolourmap cmap;
{
    Channel channel;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "write_full_cmap";

    VERIFY_COLOURMAP (cmap);
    channel = conn_get_channel (connection);
    /*  Write colourmap  */
    dsrw_write_packet_desc (channel, (*cmap).top_pack_desc);
    dsrw_write_packet (channel, (*cmap).top_pack_desc, (*cmap).top_packet);
    if (ch_flush (channel) != TRUE)
    {
	(void) fprintf (stderr, "Error writing Karma colourmap\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  All colours sent  */
    return (TRUE);
}   /*  End Function write_full_cmap  */

static flag read_full_cmap (connection, info)
/*  This routine will read an array of pixel colours from a connection.
    The connection must be given by  connection  .
    The colourmap must be pointed to by  info  .
    The routine returns TRUE on sucess, else it returns FALSE.
*/
Connection connection;
void **info;
{
    Channel channel;
    Kcolourmap cmap;
    unsigned int colourmap_size;
    unsigned short *colour_array;
    char *packet;
    packet_desc *pack_desc;
    extern void (*store_ccells_func) ();
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
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
    if (dsrw_read_packet (channel, pack_desc, packet) != TRUE)
    {
	(void) fprintf (stderr, "Error reading Karma colourmap data\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    /*  Now get a handle to the colourmap  */
    if ( ( colour_array = ds_cmap_find_colourmap (pack_desc, packet,
						  &colourmap_size,
						  (flag *) NULL,
						  (char **) NULL,
						  (double *) NULL,
						  (unsigned int) 0) ) == NULL )
    {
	(void) fprintf (stderr, "Could not find colourmap\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    if (change_cmap_size (cmap, colourmap_size, colourmap_size, TRUE,
			  colour_array, pack_desc, packet) != TRUE)
    {
	(void) fprintf (stderr, "Could not reallocate colourmap\n");
	ds_dealloc_packet (pack_desc, packet);
	return (FALSE);
    }
    (*store_ccells_func) ( (*cmap).size, (*cmap).pixel_values,
			  (*cmap).intensities, (*cmap).intensities + 1,
			  (*cmap).intensities + 2, 3,
			  (*cmap).dpy_handle );
    return (TRUE);
}   /*  End Function read_full_cmap  */

static void transmit_cmap_resize (cmap)
/*  This routine will transmit a colourmap resize to all colourmap_indices
    clients.
    The colourmap must be given by  cmap  .
    The routine returns nothing.
*/
Kcolourmap cmap;
{
    Connection connection;
    Kcolourmap conn_cmap;
    unsigned int num_connections;
    unsigned int connection_count;
    static char function_name[] = "transmit_cmap_resize";

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
}   /*  End Function transmit_cmap_resize  */

static void transmit_cmap_modify (cmap)
/*  This routine will transmit a change in the colourmap colours to all
    full_colourmap clients.
    The colourmap must be given by  cmap  .
    The routine returns nothing.
*/
Kcolourmap cmap;
{
    Connection connection;
    Kcolourmap conn_cmap;
    unsigned int num_connections;
    unsigned int connection_count;
    static char function_name[] = "transmit_cmap_modify";

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
}   /*  End Function transmit_cmap_modify  */

static flag change_cmap_size (cmap, num_cells, min_cells, notify,
			      colour_array, pack_desc, packet)
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
Kcolourmap cmap;
unsigned int num_cells;
unsigned int min_cells;
flag notify;
unsigned short *colour_array;
packet_desc *pack_desc;
char *packet;
{
    flag tolerant = TRUE;
    unsigned int num_to_alloc;
    unsigned int num_allocated;
    char *top_packet;
    unsigned short *intensities;
    unsigned long *pixel_values;
    packet_desc *top_pack_desc;
    extern unsigned int (*alloc_ccells_func) ();
    extern void (*free_ccells_func) ();
    static char function_name[] = "change_cmap_size";

    VERIFY_COLOURMAP (cmap);
    if (num_cells == (*cmap).size)
    {
	/*  No change  */
	if (colour_array != NULL)
	{
	    ds_dealloc_packet ( (*cmap).top_pack_desc, (*cmap).top_packet );
	    (*cmap).top_pack_desc = pack_desc;
	    (*cmap).top_packet = packet;
	    (*cmap).intensities = colour_array;
	}
	return (TRUE);
    }
    if (num_cells < 2)
    {
	/*  No change wanted  */
	if (colour_array != NULL)
	{
	    ds_dealloc_packet ( (*cmap).top_pack_desc, (*cmap).top_packet );
	    (*cmap).top_pack_desc = pack_desc;
	    (*cmap).top_packet = packet;
	    (*cmap).intensities = colour_array;
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
    if (num_cells < (*cmap).size)
    {
	/*  Reduce size  */
	m_copy ( (char *) pixel_values, (char *) (*cmap).pixel_values,
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
	(*free_ccells_func) ( (*cmap).size - num_cells,
			     (*cmap).pixel_values + num_cells,
			     (*cmap).dpy_handle );
	ds_dealloc_packet ( (*cmap).top_pack_desc, (*cmap).top_packet );
	(*cmap).size = num_cells;
	(*cmap).top_pack_desc = top_pack_desc;
	(*cmap).top_packet = top_packet;
	(*cmap).intensities = intensities;
    }
    else
    {
	/*  Increase size  */
	num_to_alloc = num_cells - (*cmap).size;
	m_copy ( (char *) pixel_values, (char *) (*cmap).pixel_values,
		sizeof *pixel_values * (*cmap).size );
	/*  Allocate colourcells first to see how big data structure
	    needs to be  */
	if (min_cells > 1) min_cells -= (*cmap).size;
	if (min_cells < 1) min_cells = 1;
	if ( ( num_allocated =
	      (*alloc_ccells_func) (num_to_alloc,
				    pixel_values + (*cmap).size, min_cells,
				    (*cmap).dpy_handle) )
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
		  ds_cmap_alloc_colourmap (num_allocated + (*cmap).size,
					   (multi_array **) NULL,
					   &top_pack_desc, &top_packet) )
		== NULL )
	    {
		m_error_notify (function_name, "array of intensities");
		(*free_ccells_func) (num_allocated,
				     pixel_values + (*cmap).size,
				     (*cmap).dpy_handle);
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
	(*cmap).size += num_allocated;
	if ( (*cmap).intensities != NULL )
	{
	    ds_dealloc_packet ( (*cmap).top_pack_desc, (*cmap).top_packet );
	}
	(*cmap).top_pack_desc = top_pack_desc;
	(*cmap).top_packet = top_packet;
	(*cmap).intensities = intensities;
    }
    if ( (*cmap).pixel_values != NULL )
    {
	m_free ( (char *) (*cmap).pixel_values );
    }
    (*cmap).pixel_values = pixel_values;
    /*  Colourmap has been resized (colours not computed yet)  */
    if (notify)
    {
	/*  Notify that colourmap has been resized  */
	call_resize_funcs (cmap);
    }
    /*  Transmit this colourmap to any slaves of it  */
    transmit_cmap_resize (cmap);
    return (TRUE);
}   /*  End Function change_cmap_size  */

static void call_resize_funcs (cmap)
/*  This routine will call all registered resize functions for a colourmap.
    The colourmap must be given by  cmap  .
    The routine returns nothing.
*/
Kcolourmap cmap;
{
    struct resize_func *curr_func;
    static char function_name[] = "call_resize_funcs";

    VERIFY_COLOURMAP (cmap);
    /*  Call resize functions  */
    for (curr_func = (*cmap).first_resize_func; curr_func != NULL;
	 curr_func = (*curr_func).next)
    {
	(* (*curr_func).func ) (cmap, &(*curr_func).info);
    }
}   /*  End Function call_resize_funcs  */

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
    return ( ds-cmap_find_colourmap ( (*multi_desc).headers[0],
				    (*multi_desc).data[0], size,
				    (flag *) NULL,
				    (char **) NULL, (double *) NULL,
				    (unsigned int) 0 ) );
}   /*  End Function ds-cmap_read_colourmap  */
#endif
