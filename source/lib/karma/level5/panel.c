/*LINTLIBRARY*/
/*PREFIX:"panel_"*/
/*  panel.c

    This code provides a user control panel (parameter manipulation).

    Copyright (C) 1993  Richard Gooch

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

    This file contains the various utility routines for supporting a generic
    user interface.


    Written by      Richard Gooch   1-OCT-1993

    Updated by      Richard Gooch   5-OCT-1993

    Updated by      Richard Gooch   9-OCT-1993: Added  PIT_CHOICE_INDEX  type.

    Updated by      Richard Gooch   16-NOV-1993: Added  PIA_ARRAY_LENGTH
  PIA_ARRAY_MIN_LENGTH  and  PIA_ARRAY_MAX_LENGTH  attributes.

    Last updated by Richard Gooch   22-NOV-1993: Cleaned up some display
  output.


*/

#include <stdio.h>
#include <string.h>
#include <varargs.h>
#include <karma.h>
#include <karma_conn.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>

typedef struct controlpanel_type * KControlPanel;
typedef struct panelitem_type * KPanelItem;

#define KPANEL_INTERNAL
#include <karma_panel.h>

#define MAGIC_NUMBER (unsigned int) 798345329

#define PANEL_STACK_SIZE 100

#define VERIFY_CONTROLPANEL(panel) if (panel == NULL) \
{(void) fprintf (stderr, "NULL control panel passed\n"); \
 a_prog_bug (function_name); } \
if ( (*panel).magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid control panel object\n"); \
 a_prog_bug (function_name); }

struct controlpanel_type
{
    unsigned int magic_number;
    KPanelItem first_item;
};

struct panelitem_type
{
    char *name;
    char *comment;
    unsigned int type;
    void *value_ptr;
    /*  Various attributes (this will grow)  */
    unsigned int num_choice_strings;
    char **choice_strings;
    unsigned int max_array_length;
    unsigned int min_array_length;
    unsigned int *curr_array_length;
    /*  Pointer to the next item in the panel  */
    KPanelItem prev;
    KPanelItem next;
/*
    void (*func) ();
*/
};


/*  Local functions  */
void panel_add_item ();


/*  Private data  */
static KControlPanel panel_stack[PANEL_STACK_SIZE];
static int panel_stack_index = -1;


/*  Private functions  */
static flag panel_process_command (/* panel, cmd */);          /*  temporary */
static void panel_show_items (/* panel, screen_display, fp, verbose */);
static char *decode_datum (/* string, type, name, value_ptr */);
static void show_datum (/* item, fp, value_ptr */);
static void abort_func (/* cmd */);
static void add_conn_func (/* cmd */);
static void process_choice_item (/* item, cmd */);
static void show_choice_item (/* item, fp, screen_display, verbose */);
static void decode_array (/* string, item */);
static void show_array (/* item, screen_display, fp, verbose */);
static unsigned int get_size_of_type (/* type */);


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KControlPanel panel_create (blank)
/*  This routine will create a control panel object.
    If the value of  blank  is TRUE, then the routine will create a blank form,
    else it will add some internally defined panel items.
    The routine returns a KControlPanel object on success,
    else it returns NULL.
*/
flag blank;
{
    KControlPanel panel;
    static char function_name[] = "panel_create";

    FLAG_VERIFY (blank);
    if (FALSE != 0)
    {
	(void) fprintf (stderr, "Constant FALSE has value: %d  not 0\n",
			FALSE);
	a_prog_bug (function_name);
    }
    if ( ( panel = (KControlPanel) m_alloc (sizeof *panel) ) == NULL )
    {
	m_error_notify (function_name, "control panel");
	return (NULL);
    }
    (*panel).magic_number = MAGIC_NUMBER;
    (*panel).first_item = NULL;
    if (blank) return (panel);
    panel_add_item (panel, "quit", "exit panel", PIT_EXIT_FORM, NULL, PIA_END);
    panel_add_item (panel, "exit", "exit panel", PIT_EXIT_FORM, NULL, PIA_END);
    panel_add_item (panel, "add_connection",
		    "hostname port_number protocol_name",
		    PIT_FUNCTION, add_conn_func, PIA_END);
    panel_add_item (panel, "abort", "abort without saving panel values",
		    PIT_FUNCTION, abort_func, PIA_END);
    return (panel);
}   /*  End Function panel_create  */

/*PUBLIC_FUNCTION*/
void panel_add_item (panel, name, comment, type, value_ptr, va_alist)
/*  This routine will add a panel item to a KControlPanel object. Each
    panel item has a number of "attributes". First come the "core" attributes,
    follwed by the optional attributes.
    Below are the core attributes:
    The control panel to add to must be given by  panel  .
    The name of the panel item must be pointed to by  name  .
    The comment (eg. name of the units: "(km/sec)") must be pointed to by
    comment  .
    The type of the panel item must be given by  type  .
    The panel item data storage must be pointed by  value_ptr  .
    The optional attributes are given as pairs of attribute-key attribute-value
    pairs. The last argument must be PIA_END. The attributes are passed using
    varargs, and are given by  va_alist  .
    The routine returns nothing.
*/
KControlPanel panel;
char *name;
char *comment;
unsigned int type;
void *value_ptr;
va_dcl
{
    KPanelItem item;
    KPanelItem curr_item;
    KPanelItem dup_item;
    va_list arg_pointer;
    unsigned int att_key;
    char *new_string;
    static char function_name[] = "panel_add_item";

    va_start (arg_pointer);
    VERIFY_CONTROLPANEL (panel);
    if (name == NULL)
    {
	(void) fprintf (stderr, "NULL name pointer passed\n");
	a_prog_bug (function_name);
    }
    if (comment == NULL)
    {
	(void) fprintf (stderr, "NULL comment pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check for valid type  */
    switch (type)
    {
      case PIT_FUNCTION:
      case PIT_FLAG:
      case PIT_CHOICE_INDEX:
	if (value_ptr == NULL)
	{
	    (void) fprintf (stderr, "NULL value pointer passed\n");
	    a_prog_bug (function_name);
	}
      case PIT_EXIT_FORM:
	/*  Fine  */
	break;
      case K_VSTRING:
	if (value_ptr == NULL)
	{
	    (void) fprintf (stderr, "NULL value pointer passed\n");
	    a_prog_bug (function_name);
	}
	if (*(char **) value_ptr == NULL)
	{
	    if ( ( new_string = st_dup ("") ) == NULL )
	    {
		m_error_notify (function_name, "string value");
		return;
	    }
	}
	else
	{
	    if ( ( new_string = st_dup (*(char **) value_ptr) ) == NULL )
	    {
		m_error_notify (function_name, "string value");
		return;
	    }
	}
	*(char **) value_ptr = new_string;
        break;
      default:
	if (ds_element_is_named (type) != TRUE)
	{
	    (void) fprintf (stderr, "Illegal panel item type: %u\n", type);
	    a_prog_bug (function_name);
	}
	if (value_ptr == NULL)
	{
	    (void) fprintf (stderr, "NULL value pointer passed\n");
	    a_prog_bug (function_name);
	}
	break;
    }
    if ( ( item = (KPanelItem) m_alloc (sizeof *item) ) == NULL )
    {
	m_abort (function_name, "panel item object");
    }
    /*  Initialise everything to zero (all attribute flags set FALSE)  */
    m_clear ( (char *) item, sizeof *item );
    /*  Paranoia  */
    (*item).curr_array_length = NULL;
    (*item).min_array_length = 0;
    (*item).max_array_length = 0;
    (*item).prev = NULL;
    (*item).next = NULL;
    if ( ( (*item).name = st_dup (name) ) == NULL )
    {
	m_abort (function_name, "panel item name");
    }
    if ( ( (*item).comment = st_dup (comment) ) == NULL )
    {
	m_abort (function_name, "panel item comment string");
    }
    (*item).type = type;
    (*item).value_ptr = value_ptr;
    while ( ( att_key = va_arg (arg_pointer, unsigned int) ) != PIA_END )
    {
	switch (att_key)
	{
	  case PIA_NUM_CHOICE_STRINGS:
	    if ( ( (*item).num_choice_strings =
		  va_arg (arg_pointer, unsigned int) ) == 0 )
	    {
		(void) fprintf (stderr,
				"Number of choice strings must be greater than zero\n");
		a_prog_bug (function_name);
	    }
	    if (type != PIT_CHOICE_INDEX)
	    {
		(void) fprintf (stderr,
				"Cannot specify number of choice strings for type: %u\n",
				type);
		a_prog_bug (function_name);
	    }
	    break;
	  case PIA_CHOICE_STRINGS:
	    if ( ( (*item).choice_strings = va_arg (arg_pointer, char **) )
		== NULL )
	    {
		(void) fprintf (stderr, "NULL choice string pointer passed\n");
		a_prog_bug (function_name);
	    }
	    if (type != PIT_CHOICE_INDEX)
	    {
		(void) fprintf (stderr,
				"Cannot specify choice strings for type: %u\n",
				type);
		a_prog_bug (function_name);
	    }
	    break;
	  case PIA_ARRAY_LENGTH:
	  case PIA_ARRAY_MIN_LENGTH:
	  case PIA_ARRAY_MAX_LENGTH:
	    switch (type)
	    {
	      case PIT_FUNCTION:
	      case PIT_EXIT_FORM:
		(void) fprintf (stderr,
				"Cannot specify array length for type: %u\n",
				type);
		a_prog_bug (function_name);
		break;
	      case PIT_FLAG:
	      case PIT_CHOICE_INDEX:
		break;
	      default:
		if (ds_element_is_named (type) != TRUE)
		{
		    (void) fprintf (stderr,
				    "Cannot specify array length for type: %u\n",
				    type);
		    a_prog_bug (function_name);
		}
		break;
	    }
	    switch (att_key)
	    {
	      case PIA_ARRAY_LENGTH:
		(*item).curr_array_length =va_arg (arg_pointer,unsigned int *);
		break;
	      case PIA_ARRAY_MIN_LENGTH:
		(*item).min_array_length = va_arg (arg_pointer, unsigned int);
		break;
	      case PIA_ARRAY_MAX_LENGTH:
		(*item).max_array_length = va_arg (arg_pointer, unsigned int);
		break;
	    }
	    break;
	  default:
	    (void) fprintf (stderr, "Unknown panel item attribute key: %u\n",
			    att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (arg_pointer);
    /*  Check for enough extra information for type  */
    switch (type)
    {
      case PIT_CHOICE_INDEX:
	if ( (*item).num_choice_strings == 0 )
	{
	    (void) fprintf (stderr, "Zero choice strings for choice type\n");
	    a_prog_bug (function_name);
	}
	if ( (*item).choice_strings == NULL )
	{
	    (void) fprintf (stderr,
			    "NULL choice string pointer for choice type\n");
	    a_prog_bug (function_name);
	}
	if (*(unsigned int *) value_ptr >= (*item).num_choice_strings)
	{
	    (void) fprintf (stderr,
			    "Initial choice index: %u not less than num_choices: %u\n",
			    *(unsigned int *) value_ptr,
			    (*item).num_choice_strings);
	    a_prog_bug (function_name);
	}
	break;
      default:
	break;
    }
    /*  Check for valid array specifications  */
    if ( (*item).max_array_length < (*item).min_array_length )
    {
	(void) fprintf ( stderr,
			"Maximum array length: %u less than minimum: %u\n",
			(*item).max_array_length, (*item).min_array_length );
	a_prog_bug (function_name);
    }
    /*  Insert panel item at head of list  */
    if ( (*panel).first_item == NULL )
    {
	/*  First entry  */
	(*panel).first_item = item;
	return;
    }
    /*  Check for existing panel item of the same name  */
    for (curr_item = (*panel).first_item, dup_item = NULL;
	 curr_item != NULL;
	 curr_item = (*curr_item).next)
    {
	if (strcmp ( (*curr_item).name, name ) == 0)
	{
	    /*  Match!  */
	    dup_item = curr_item;
	    curr_item = NULL;
	}
    }
    if (dup_item != NULL)
    {
	/*  Remove this one  */
	if ( (*dup_item).next != NULL )
	{
	    (* (*dup_item).next ).prev = (*dup_item).prev;
	}
	if ( (*dup_item).prev == NULL )
	{
	    /*  First entry in list  */
	    (*panel).first_item = NULL;
	}
	else
	{
	    /*  Not first entry  */
	    (* (*dup_item).prev ).next = (*dup_item).next;
	}
	/*  Deallocate it  */
	m_free ( (*dup_item).name );
	m_free ( (*dup_item).comment );
	m_free ( (char *) dup_item );
    }
    (*item).next = (*panel).first_item;
    (*panel).first_item = item;
    if ( (*item).next != NULL )
    {
	(* (*item).next ).prev = item;
    }
}   /*  End Function panel_add_item  */

/*PUBLIC_FUNCTION*/
void panel_push_onto_stack (panel)
/*  This routine will push a control panel object onto the control panel
    stack.
    The control panel must be given by  panel  .
    The routine returns nothing.
*/
KControlPanel panel;
{
    extern KControlPanel panel_stack[PANEL_STACK_SIZE];
    extern int panel_stack_index;
    static char function_name[] = "panel_push_onto_stack";

    VERIFY_CONTROLPANEL (panel);
    if (panel_stack_index >= PANEL_STACK_SIZE)
    {
	(void) fprintf (stderr,
			"Too many control panels already on stack\n");
	a_prog_bug (function_name);
    }
    panel_stack[++panel_stack_index] = panel;
}   /*  End Function panel_push_onto_stack  */

/*PUBLIC_FUNCTION*/
void panel_pop_from_stack ()
/*  This routine will pop the last pushed control panel object from the
    control panel stack.
    The routine returns nothing.
*/
{
    extern KControlPanel panel_stack[PANEL_STACK_SIZE];
    extern int panel_stack_index;
    static char function_name[] = "panel_pop_from_stack";

    if (panel_stack_index < 0)
    {
	(void) fprintf (stderr, "No control panels on stack\n");
	a_prog_bug (function_name);
    }
    --panel_stack_index;
}   /*  End Function panel_pop_from_stack  */

/*PUBLIC_FUNCTION*/
flag panel_process_command_with_stack (cmd, unknown_func, fp)
/*  This routine will process a command, using the top control panel on the
    stack to interpret it.
    The command must be pointed to by  cmd  .
    If the command is not understood, then the command will be passed to the
    function pointed to by  unknown_func  .If this is NULL, then a message is
    displayed if the command is not understood.
    The interface to this function is given below:

    flag unknown_func (cmd, fp)
    *   This routine will process a command.
        The command must be pointed to by  cmd  .
	Output messages are directed to  fp  .
	The routine returns TRUE if more commands should be processed,
	else it returns FALSE, indicating that the "exit" command was entered.
    *
    char *cmd;
    FILE *fp;

    Output messages are directed to  fp  .
    The routine returns TRUE if more commands should be processed,
    else it returns FALSE, indicating that the control panel's "exit" command
    was entered.
*/
char *cmd;
flag (*unknown_func) ();
FILE *fp;
{
    KControlPanel panel;
    char *arg1, *arg2;
    extern KControlPanel panel_stack[PANEL_STACK_SIZE];
    extern int panel_stack_index;
    static char function_name[] = "panel_process_command_with_stack";

    if (cmd == NULL)
    {
	(void) fprintf (stderr, "NULL command pointer passed\n");
	a_prog_bug (function_name);
    }
    if (*cmd == '\0') return (TRUE);
    /*  Extract panel item name  */
    if ( ( arg1 = ex_word (cmd, &arg2) ) == NULL )
    {
	m_abort (function_name, "panel item name");
    }
    if (panel_stack_index < 0)
    {
	/*  No control panels on stack  */
	if (unknown_func == NULL)
	{
	    (void) fprintf (stderr, "Command: \"%s\" not understood\n", cmd);
	    m_free (arg1);
	    return (TRUE);
	}
	if (strcmp (arg1, "?#") == 0)
	{
	    /*  Transition period: support  ez_decode  too  */
	    m_free (arg1);
	    return ( (*unknown_func) ("?!", fp) );
	}
	m_free (arg1);
	return ( (*unknown_func) (cmd, fp) );
    }
    if (strcmp (arg1, "-") == 0)
    {
	if (unknown_func == NULL)
	{
	    (void) fprintf (stderr,
			    "No  unknown_func  to process escaped command\n");
	    m_free (arg1);
	    return (TRUE);
	}
	m_free (arg1);
	return ( (*unknown_func) (arg2, fp) );
    }
    m_free (arg1);
    panel = panel_stack[panel_stack_index];
    if ( panel_process_command (panel, cmd, unknown_func, fp) ) return (TRUE);
    /*  Pop up control panel from stack (unless top form)  */
    if (panel_stack_index > 0)
    {
	--panel_stack_index;
	return (TRUE);
    }
    return (FALSE);
}   /*  End Function panel_process_command_with_stack  */


/*  Temporary private functions follow (will probably be made public later)  */

static flag panel_process_command (panel, cmd, unknown_func, fp)
/*  This routine will process a command, using a given control panel.
    The control panel must be given by  panel  .
    The command must be pointed to by  cmd  .
    If the command is not understood, then the command will be passed to the
    function pointed to by  unknown_func  .If this is NULL, then a message is
    displayed if the command is not understood.
    The interface to this function is given below:

    flag unknown_func (cmd, fp)
    *   This routine will process a command.
        The command must be pointed to by  cmd  .
	Output messages are directed to  fp  .
	The routine returns TRUE if more commands should be processed,
	else it returns FALSE, indicating that the "exit" command was entered.
    *
    char *cmd;
    FILE *fp;

    Output messages are directed to  fp  .
    The routine returns TRUE if more commands should be processed,
    else it returns FALSE, indicating that the control panel's "exit" command
    was entered.
*/
KControlPanel panel;
char *cmd;
flag (*unknown_func) ();
FILE *fp;
{
    KPanelItem item, curr_item;
    int pname_len;
    flag onoff;
    char *pname, *cmd_ptr, *cmd_ptr2;
    void (*func_ptr) ();
    static char function_name[] = "panel_process_command";

    VERIFY_CONTROLPANEL (panel);
    if (cmd == NULL)
    {
	(void) fprintf (stderr, "NULL command pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Extract panel item name  */
    if ( ( pname = ex_word (cmd, &cmd_ptr) ) == NULL )
    {
	m_abort (function_name, "panel item name");
    }
    /*  Decode a few special panel items  */
    if (strcmp (pname, "??") == 0)
    {
	/*  Print panel items  */
	panel_show_items (panel, TRUE, fp, TRUE);
	m_free (pname);
	return (TRUE);
    }
    if (strcmp (pname, "?") == 0)
    {
	/*  Print panel items  */
	panel_show_items (panel, TRUE, fp, FALSE);
	m_free (pname);
	return (TRUE);
    }
    if (strcmp (pname, "?#") == 0)
    {
	/*  Print panel items  */
	panel_show_items (panel, FALSE, fp, FALSE);
	m_free (pname);
	return (TRUE);
    }
    /*  Loop through trying to find panel item  */
    pname_len = strlen (pname);
    for (curr_item = (*panel).first_item, item = NULL; curr_item != NULL;
	 curr_item = (*curr_item).next)
    {
	if (strncmp (pname, (*curr_item).name, pname_len) == 0)
	{
	    if (item != NULL)
	    {
		(void) fprintf (stderr, "Ambiguous panel item name: \"%s\"\n",
				pname);
		m_free (pname);
		return (TRUE);
	    }
	    item = curr_item;
	}
    }
    m_free (pname);
    if (item == NULL)
    {
	/*  Panel Item not found  */
	if (unknown_func == NULL)
	{
	    (void) fprintf (stderr, "Command: \"%s\" not understood\n", cmd);
	    return (TRUE);
	}
	return ( (*unknown_func) (cmd, fp) );
    }
    /*  Continue decoding panel item  */
    switch ( (*item).type )
    {
      case PIT_FUNCTION:
	func_ptr = (*item).value_ptr;
	(*func_ptr) (cmd_ptr);
	return (TRUE);
/*
	break;
*/
      case PIT_EXIT_FORM:
	return (FALSE);
/*
	break;
*/
      default:
	/*  Process other types later  */
	break;
    }
    /*  No actions or group parameters at this point  */
    /*  Check if array of values  */
    if ( (*item).max_array_length > 0 )
    {
	decode_array (cmd_ptr, item);
	return (TRUE);
    }
    /*  Single instance item  */
    switch ( (*item).type )
    {
      case PIT_FLAG:
	(void) decode_datum (cmd_ptr, (*item).type, (*item).name,
			     (*item).value_ptr);
	return (TRUE);
/*
	break;
*/
      case PIT_CHOICE_INDEX:
	process_choice_item (item, cmd_ptr);
	return (TRUE);
/*
	break;
*/
      default:
	if ( ds_element_is_named ( (*item).type ) )
	{
	    (void) decode_datum (cmd_ptr, (*item).type, (*item).name,
				 (*item).value_ptr);
	    return (TRUE);
	}
	(void) fprintf (stderr, "Illegal panel item type: %u\n", (*item).type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function panel_process_command  */

static void panel_show_items (panel, screen_display, fp, verbose)
/*  This routine will show the panel items for a form.
    The control panel must be given by  panel  .
    If the value of  screen_display  is TRUE, then the comment strings and
    action panel items are displayed.
    Output messages are directed to  fp  .
    If the value of  verbose  is TRUE, then the output is verbose.
    The routine returns nothing.
*/
KControlPanel panel;
flag screen_display;
FILE *fp;
flag verbose;
{
    KPanelItem item;
    static char function_name[] = "panel_show_items";

    VERIFY_CONTROLPANEL (panel);
    FLAG_VERIFY (screen_display);
    if (screen_display) (void) fprintf (fp, "\n");
    for (item = (*panel).first_item; item != NULL; item = (*item).next)
    {
	/*  Display name and value  */
	switch ( (*item).type )
	{
	  case PIT_FUNCTION:
	  case PIT_EXIT_FORM:
	    if (screen_display)
	    {
		(void) fprintf (fp, "%-20s%-20s#%s\n",
				(*item).name, "", (*item).comment);
	    }
	    continue;
/*
	    break;
*/
	  default:
	    /*  Process other types later  */
	    break;
	}
	/*  No actions or group parameters at this point  */
	/*  Check if array of values  */
	if ( (*item).max_array_length > 0 )
	{
	    show_array (item, screen_display, fp, verbose);
	    continue;
	}
	/*  Single instance item  */
	switch ( (*item).type )
	{
	  case PIT_FLAG:
	    (void) fprintf (fp, "%-20s%-20s", (*item).name,
			    (*(flag *) (*item).value_ptr) ? "on" : "off");
	    break;
	  case PIT_CHOICE_INDEX:
	    show_choice_item (item, fp, screen_display, verbose);
	    continue;

/*
	    break;
*/
	  default:
	    if (ds_element_is_named ( (*item).type ) != TRUE)
	    {
		(void) fprintf (stderr, "Illegal panel item type: %u\n",
				(*item).type);
		a_prog_bug (function_name);
	    }
	    (void) fprintf (fp, "%-20s", (*item).name);
	    show_datum (item, fp, NULL);
	}
	/*  This should come last  */
	if (screen_display)
	{
	    /*  Display comment  */
	    (void) fprintf (fp, "#%s\n", (*item).comment);
	}
	else
	{
	    (void) fprintf (fp, "\n");
	}
    }
}   /*  End Function panel_show_items  */


/*  Private functions follow  */

static char *decode_datum (string, type, name, value_ptr)
/*  This routine will decode a single value (ie. atomic, string, flag, choice)
    from a command string.
    The command string must be pointed to by  string  .
    The type of the panel item must be given by  type  .
    The name of the panel item must be given by  name  .
    The value pointer must be given by  value_ptr  .
    The routine returns a pointer to the next argument.
*/
char *string;
unsigned int type;
char *name;
void *value_ptr;
{
    FString *fstring;
    char *new_string;
    double ivalue[2];
    double dvalue[2];
    static char function_name[] = "decode_datum";

    if (string == NULL)
    {
	(void) fprintf (stderr, "value missing for panel item: \"%s\"\n",
			name);
	return (NULL);
    }
    switch (type)
    {
      case K_FLOAT:
	dvalue[0] = ex_float (string, &string);
	*(float *) value_ptr = dvalue[0];
	break;
      case K_DOUBLE:
	dvalue[0] = ex_float (string, &string);
	*(double *) value_ptr = dvalue[0];
	break;
      case K_BYTE:
	ivalue[0] = ex_int (string, &string);
	*(char *) value_ptr = ivalue[0];
	break;
      case K_INT:
	ivalue[0] = ex_int (string, &string);
	*(int *) value_ptr = ivalue[0];
	break;
      case K_SHORT:
	ivalue[0] = ex_int (string, &string);
	*(short *) value_ptr = ivalue[0];
	break;
      case K_COMPLEX:
	dvalue[0] = ex_float (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	dvalue[1] = ex_float (string, &string);
	*(float *) value_ptr = dvalue[0];
	*( (float *) value_ptr + 1 ) = dvalue[1];
	break;
      case K_DCOMPLEX:
	dvalue[0] = ex_float (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	dvalue[1] = ex_float (string, &string);
	*(double *) value_ptr = dvalue[0];
	*( (double *) value_ptr + 1 ) = dvalue[1];
	break;
      case K_BCOMPLEX:
	ivalue[0] = ex_int (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_int (string, &string);
	*(char *) value_ptr = ivalue[0];
	*( (char *) value_ptr + 1 ) = ivalue[1];
	break;
      case K_ICOMPLEX:
	ivalue[0] = ex_int (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_int (string, &string);
	*(int *) value_ptr = ivalue[0];
	*( (int *) value_ptr + 1 ) = ivalue[1];
	break;
      case K_SCOMPLEX:
	ivalue[0] = ex_int (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_int (string, &string);
	*(short *) value_ptr = ivalue[0];
	*( (short *) value_ptr + 1 ) = ivalue[1];
	break;
      case K_LONG:
	ivalue[0] = ex_int (string, &string);
	*(long *) value_ptr = ivalue[0];
	break;
      case K_LCOMPLEX:
	ivalue[0] = ex_int (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_int (string, &string);
	*(long *) value_ptr = ivalue[0];
	*( (long *) value_ptr + 1 ) = ivalue[1];
        break;
      case K_UBYTE:
	ivalue[0] = ex_int (string, &string);
	*(unsigned char *) value_ptr = ivalue[0];
	break;
      case K_UINT:
	ivalue[0] = ex_int (string, &string);
	*(unsigned int *) value_ptr = ivalue[0];
	break;
      case K_USHORT:
	ivalue[0] = ex_int (string, &string);
	*(unsigned short *) value_ptr = ivalue[0];
	break;
      case K_ULONG:
	ivalue[0] = ex_int (string, &string);
	*(unsigned long *) value_ptr = ivalue[0];
	break;
      case K_UBCOMPLEX:
	ivalue[0] = ex_int (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_int (string, &string);
	*(unsigned char *) value_ptr = ivalue[0];
	*( (unsigned char *) value_ptr + 1 ) = ivalue[1];
        break;
      case K_UICOMPLEX:
	ivalue[0] = ex_int (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_int (string, &string);
	*(unsigned int *) value_ptr = ivalue[0];
	*( (unsigned int *) value_ptr + 1 ) = ivalue[1];
        break;
      case K_USCOMPLEX:
	ivalue[0] = ex_int (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_int (string, &string);
	*(unsigned short *) value_ptr = ivalue[0];
	*( (unsigned short *) value_ptr + 1 ) = ivalue[1];
        break;
      case K_ULCOMPLEX:
	ivalue[0] = ex_int (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_int (string, &string);
	*(unsigned long *) value_ptr = ivalue[0];
	*( (unsigned long *) value_ptr + 1 ) = ivalue[1];
        break;
      case K_VSTRING:
	if ( ( new_string = ex_str (string, &string) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "String value missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	if (*(char **) value_ptr != NULL)
	{
	    m_free (*(char **) value_ptr);
	}
	*(char **) value_ptr = new_string;
        break;
      case K_FSTRING:
	if ( ( new_string = ex_str (string, &string) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "String value missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	fstring = value_ptr;
	if (strlen (new_string) > (*fstring).max_len)
	{
	    (void) fprintf (stderr,
			    "String value for panel item: \"%s\" too long\n",
			    name);
	    m_free (new_string);
	    return (NULL);
	}
	(void) strncpy ( (*fstring).string, new_string, (*fstring).max_len );
        break;
      case PIT_FLAG:
	*(flag *) value_ptr = ex_on_or_off (&string);
	break;
      default:
	(void) fprintf (stderr, "Illegal panel item type: %u\n", type);
	a_prog_bug (function_name);
	break;
    }
    return (string);
}   /*  End Function decode_datum  */

static void show_datum (item, fp, value_ptr)
/*  This routine will show a single value (ie. atomic or string).
    The panel item must be given by  item  .
    Output messages are directed to  fp  .
    The value must be pointed to by  value_ptr  .If this is NULL, then the
    value pointer registered with the item is used.
    The routine returns nothing.
*/
KPanelItem item;
FILE *fp;
void *value_ptr;
{
    int pad_len;
    FString *fstring;
    char padding[STRING_LENGTH];
    static char function_name[] = "show_datum";

    if (value_ptr == NULL) value_ptr = (*item).value_ptr;
    switch ( (*item).type )
    {
      case K_FLOAT:
	(void) fprintf (fp, "%-20e", *(float *) value_ptr);
	break;
      case K_DOUBLE:
	(void) fprintf (fp, "%-20e", *(double *) value_ptr);
	break;
      case K_BYTE:
	(void) fprintf (fp, "%-20d", *(char *) value_ptr);
	break;
      case K_INT:
	(void) fprintf (fp, "%-20d", *(int *) value_ptr);
	break;
      case K_SHORT:
	(void) fprintf (fp, "%-20d", *(short *) value_ptr);
	break;
      case K_COMPLEX:
	(void) fprintf ( fp, "%-20e %-20e",
			*(float *) value_ptr, *( (float *) value_ptr + 1 ) );
	break;
      case K_DCOMPLEX:
	(void) fprintf ( fp, "%-20e %-20e",
			*(double *) value_ptr, *( (double *) value_ptr + 1 ) );
	break;
      case K_BCOMPLEX:
	(void) fprintf ( fp, "%-20d %-20d",
			*(char *) value_ptr, *( (char *) value_ptr + 1 ) );
	break;
      case K_ICOMPLEX:
	(void) fprintf ( fp, "%-20d %-20d",
			*(int *) value_ptr, *( (int *) value_ptr + 1 ) );
	break;
      case K_SCOMPLEX:
	(void) fprintf ( fp, "%-20d %-20d",
			*(short *) value_ptr, *( (short *) value_ptr + 1 ) );
	break;
      case K_LONG:
	(void) fprintf (fp, "%-20d", *(long *) value_ptr);
	break;
      case K_LCOMPLEX:
	(void) fprintf ( fp, "%-20d %-20d",
			*(long *) value_ptr, *( (long *) value_ptr + 1 ) );
	break;
      case K_UBYTE:
	(void) fprintf (fp, "%-20u", *(unsigned char *) value_ptr);
	break;
      case K_UINT:
	(void) fprintf (fp, "%-20u", *(unsigned int *) value_ptr);
	break;
      case K_USHORT:
	(void) fprintf (fp, "%-20u", *(unsigned short *) value_ptr);
	break;
      case K_ULONG:
	(void) fprintf (fp, "%-20u", *(unsigned long *) value_ptr);
	break;
      case K_UBCOMPLEX:
	(void) fprintf ( fp, "%-20u %-20u",
			*(unsigned char *) value_ptr,
			*( (unsigned char *) value_ptr + 1 ) );
	break;
      case K_UICOMPLEX:
	(void) fprintf ( fp, "%-20u %-20u",
			*(unsigned int *) value_ptr,
			*( (unsigned int *) value_ptr + 1 ) );
	break;
      case K_USCOMPLEX:
	(void) fprintf ( fp, "%-20u %-20u",
			*(unsigned short *) value_ptr,
			*( (unsigned short *) value_ptr + 1 ) );
	break;
      case K_ULCOMPLEX:
	(void) fprintf ( fp, "%-20u %-20u",
			*(unsigned long *) value_ptr,
			*( (unsigned long *) value_ptr + 1 ) );
	break;
      case K_VSTRING:
	if ( ( pad_len = 18 - strlen (*(char **) value_ptr) ) < 1 )
	{
	    pad_len = 1;
	}
	padding[pad_len] = '\0';
	while (pad_len > 0) padding[--pad_len] = ' ';
	(void) fprintf (fp, "\"%s\"%s", *(char **) value_ptr, padding);
	break;
      case K_FSTRING:
	fstring = value_ptr;
	if ( ( pad_len = 18 - strlen ( (*fstring).string ) ) < 1 )
	{
	    pad_len = 1;
	}
	padding[pad_len] = '\0';
	while (pad_len > 0) padding[--pad_len] = ' ';
	(void) fprintf (fp, "\"%s\"%s", (*fstring).string, padding);
	break;
      default:
	(void) fprintf (stderr, "Illegal panel item type: %u\n", (*item).type);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function show_item  */

static void abort_func (cmd)
char *cmd;
{
    exit (RV_ABORT);
}   /*  End Function abort_func  */

static void add_conn_func (cmd)
char *cmd;
{
    char *host;
    char *protocol;
    int port;

    /*  User wants to add a connection  */
    if ( ( host = ex_str (cmd, &cmd) ) == NULL )
    {
	(void) fprintf (stderr,
			"Must specify hostname, port number and protocol\n");
	return;
    }
    if ( (cmd == NULL) || (*cmd == '\0') )
    {
	(void) fprintf (stderr,
			"Must also specify port number and protocol\n");
	m_free (host);
	return;
    }
    port = ex_int (cmd, &cmd);
    if ( (cmd == NULL) || (*cmd == '\0') ||
	( ( protocol = ex_str (cmd, &cmd) ) == NULL ) )
    {
	(void) fprintf (stderr, "Must also specify protocol\n");
	m_free (host);
	return;
    }
    if (conn_attempt_connection (host, (unsigned int) port, protocol) != TRUE)
    {
	(void) fprintf (stderr,
			"Error adding \"%s\" connection\n", protocol);
    }
    m_free (host);
    m_free (protocol);
}   /*  End Function add_conn_func  */

static void process_choice_item (item, cmd)
/*  This routine will process a choice index item.
    The panel item must be given by  item  .
    The command must be pointed to by  cmd  .
    The routine returns nothing.
*/
KPanelItem item;
char *cmd;
{
    unsigned int count, index;
    int len;
    char *choice;

    if ( ( choice = ex_str (cmd, &cmd) ) == NULL )
    {
	(void) fprintf (stderr,
			"String value missing for panel item: \"%s\"\n",
			(*item).name);
	return;
    }
    if (cmd != NULL)
    {
	(void) fprintf (stderr, "Too many arguments\n");
	m_free (choice);
	return;
    }
    len = strlen (choice);
    for (count = 0, index = (*item).num_choice_strings;
	 count < (*item).num_choice_strings;
	 ++count)
    {
	if (strncmp (choice, (*item).choice_strings[count], len) == 0)
	{
	    if (index < (*item).num_choice_strings)
	    {
		(void) fprintf (stderr, "Ambiguous choice: \"%s\"\n", choice);
		m_free (choice);
		return;
	    }
	    index = count;
	}
    }
    if (index >= (*item).num_choice_strings)
    {
	(void) fprintf (stderr, "Unknown choice: \"%s\" for item: \"%s\"\n",
			choice, (*item).name);
	m_free (choice);
	return;
    }
    m_free (choice);
    *(unsigned int *) (*item).value_ptr = index;
}   /*  End Function process_choice_item  */

static void show_choice_item (item, fp, screen_display, verbose)
/*  This routine will show the value of a choice index item.
    The panel item must be given by  item  .
    The output messages are directed to  fp  .
    If the value of  screen_display  is TRUE, then the comment strings and
    action panel items are displayed.
    If the value of  verbose  is TRUE, then the output is verbose.
    The routine returns nothing.
*/
KPanelItem item;
FILE *fp;
flag screen_display;
flag verbose;
{
    unsigned int count, index;
    char **choices;

    choices = (char **) (*item).choice_strings;
    index = *(unsigned int *) (*item).value_ptr;
    if (screen_display)
    {
	(void) fprintf (fp, "%-20s%-20s#%s\n",
			(*item).name, choices[index], (*item).comment);
	if (!verbose) return;
	(void) fprintf (fp, "  Choices:\n");
	for (count = 0; count < (*item).num_choice_strings; ++count)
	{
	    (void) fprintf (fp, "    %s\n", choices[count]);
	}
	return;
    }
    (void) fprintf (fp, "%-20s%-20s\n", (*item).name, choices[index]);
}   /*  End Function show_choice_item  */

static void decode_array (string, item)
/*  This routine will decode an array of values (ie. atomic, string, flag,
    choice) from a command string.
    The command string must be pointed to by  string  .
    The panel item must be given by  item  .
    The routine returns nothing.
*/
char *string;
KPanelItem item;
{
    char *value_ptr;
    unsigned int array_count;
    unsigned int type;
    static char function_name[] = "decode_array";

    value_ptr = (*item).value_ptr;
    type = (*item).type;
    array_count = 0;
    while (string != NULL)
    {
	string = decode_datum (string, type, (*item).name, value_ptr);
	value_ptr += get_size_of_type (type);
	++array_count;
	if (array_count >= (*item).max_array_length)
	{
	    *(*item).curr_array_length = array_count;
	    return;
	}
    }
    if (array_count < (*item).min_array_length)
    {
	(void) fprintf (stderr,
			"Not enough values given for array: need: %d\n",
			(int) (*item).min_array_length);
	return;
    }
    *(*item).curr_array_length = array_count;
    if (string != NULL)
    {
	(void) fprintf (stderr,
			"Ignored trailing arguments: \"%s\" for panel item: \"%s\"\n",
			string, (*item).name);
    }
}   /*  End Function decode_array  */

static void show_array (item, screen_display, fp, verbose)
/*  This routine will display an array of values (ie. atomic, string, flag,
    choice).
    The panel item must be given by  item  .
    If the value of  screen_display  is TRUE, then the comment strings are
    displayed.
    Output messages are directed to  fp  .
    If the value of  verbose  is TRUE, then the output is verbose.
    The routine returns nothing.
*/
KPanelItem item;
flag screen_display;
FILE *fp;
flag verbose;
{
    FString *fstring;
    char *value_ptr;
    unsigned int array_count;
    static char function_name[] = "show_array";

    if (screen_display)
    {
	if ( (*item).max_array_length == (*item).min_array_length )
	{
	    (void) fprintf (fp, "%s[%d]    ",
			    (*item).name, (int) (*item).min_array_length);
	}
	else
	{
	    (void) fprintf (fp, "%s[%d..%d]    ",
			    (*item).name, (int) (*item).min_array_length,
			    (int) (*item).max_array_length);
	}
    }
    else
    {
	(void) fprintf (fp, "%s    ", (*item).name);
    }
    value_ptr = (*item).value_ptr;
    for (array_count = 0; array_count < *(*item).curr_array_length;
	 ++array_count)
    {
	switch ( (*item).type )
	{
	  case K_FLOAT:
	    (void) fprintf (fp, "%e  ", *(float *) value_ptr);
	    break;
	  case K_DOUBLE:
	    (void) fprintf (fp, "%e  ", *(double *) value_ptr);
	    break;
	  case K_BYTE:
	    (void) fprintf (fp, "%d  ", *(char *) value_ptr);
	    break;
	  case K_INT:
	    (void) fprintf (fp, "%d  ", *(int *) value_ptr);
	    break;
	  case K_SHORT:
	    (void) fprintf (fp, "%d  ", *(short *) value_ptr);
	    break;
	  case K_COMPLEX:
	    (void) fprintf ( fp, "%e   %e  ",
			    *(float *) value_ptr, *( (float *) value_ptr + 1 ) );
	    break;
	  case K_DCOMPLEX:
	    (void) fprintf ( fp, "%e   %e  ",
			    *(double *) value_ptr, *( (double *) value_ptr + 1 ) );
	    break;
	  case K_BCOMPLEX:
	    (void) fprintf ( fp, "%d   %d  ",
			    *(char *) value_ptr, *( (char *) value_ptr + 1 ) );
	    break;
	  case K_ICOMPLEX:
	    (void) fprintf ( fp, "%d   %d  ",
			    *(int *) value_ptr, *( (int *) value_ptr + 1 ) );
	    break;
	  case K_SCOMPLEX:
	    (void) fprintf ( fp, "%d   %d  ",
			    *(short *) value_ptr, *( (short *) value_ptr + 1 ) );
	    break;
	  case K_LONG:
	    (void) fprintf (fp, "%d  ", *(long *) value_ptr);
	    break;
	  case K_LCOMPLEX:
	    (void) fprintf ( fp, "%d   %d  ",
			    *(long *) value_ptr, *( (long *) value_ptr + 1 ) );
	    break;
	  case K_UBYTE:
	    (void) fprintf (fp, "%u  ", *(unsigned char *) value_ptr);
	    break;
	  case K_UINT:
	    (void) fprintf (fp, "%u  ", *(unsigned int *) value_ptr);
	    break;
	  case K_USHORT:
	    (void) fprintf (fp, "%u  ", *(unsigned short *) value_ptr);
	    break;
	  case K_ULONG:
	    (void) fprintf (fp, "%u  ", *(unsigned long *) value_ptr);
	    break;
	  case K_UBCOMPLEX:
	    (void) fprintf ( fp, "%u   %u  ",
			    *(unsigned char *) value_ptr,
			    *( (unsigned char *) value_ptr + 1 ) );
	    break;
	  case K_UICOMPLEX:
	    (void) fprintf ( fp, "%u   %u  ",
			    *(unsigned int *) value_ptr,
			    *( (unsigned int *) value_ptr + 1 ) );
	    break;
	  case K_USCOMPLEX:
	    (void) fprintf ( fp, "%u   %u  ",
			    *(unsigned short *) value_ptr,
			    *( (unsigned short *) value_ptr + 1 ) );
	    break;
	  case K_ULCOMPLEX:
	    (void) fprintf ( fp, "%u   %u  ",
			    *(unsigned long *) value_ptr,
			    *( (unsigned long *) value_ptr + 1 ) );
	    break;
	  case K_VSTRING:
	    (void) fprintf (fp, "\"%s\"  ", *(char **) value_ptr);
	    break;
	  case K_FSTRING:
	    fstring = (FString *) value_ptr;
	    (void) fprintf (fp, "\"%s\"  ", (*fstring).string);
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal panel item type: %u\n", (*item).type);
	    a_prog_bug (function_name);
	    break;
	}
	value_ptr += get_size_of_type ( (*item).type );
    }
    if (screen_display)
    {
	(void) fprintf (fp, "  #%s\n", (*item).comment);
    }
    else
    {
	(void) fprintf (fp, "\n");
    }
}   /*  End Function show_array  */

static unsigned int get_size_of_type (type)
/*  This routine will determine the size of a parameter item.
    The parameter item type must be given by  type  .
    The routine returns the size (in bytes).
*/
unsigned int type;
{
    unsigned int size;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "get_size_of_type";

    switch (type)
    {
      case PIT_FUNCTION:
      case PIT_FLAG:
	size = 0;
	break;
      case PIT_CHOICE_INDEX:
	size = sizeof (unsigned int);
	break;
      case PIT_EXIT_FORM:
	size = 0;
	break;
      default:
	if (ds_element_is_named (type) != TRUE)
	{
	    (void) fprintf (stderr, "Illegal panel item type: %u\n", type);
	    a_prog_bug (function_name);
	}
	size = host_type_sizes[type];
	break;
    }
    return (size);
}   /*  End Function get_size_of_type  */
