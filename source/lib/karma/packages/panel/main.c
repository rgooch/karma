/*LINTLIBRARY*/
/*  main.c

    This code provides a user control panel (parameter manipulation).

    Copyright (C) 1993-1996  Richard Gooch

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

    Updated by      Richard Gooch   22-NOV-1993: Cleaned up some display
  output.

    Updated by      Richard Gooch   23-APR-1994: Added  PIA_CHOICE_COMMENTS
  attribute.

    Updated by      Richard Gooch   21-MAY-1994: Added "show_protocols"
  action.

    Updated by      Richard Gooch   22-MAY-1994: Added  panel_create_group  .

    Updated by      Richard Gooch   26-AUG-1994: Moved  typedef  of classes to
  header.

    Updated by      Richard Gooch   3-NOV-1994: Removed unused  group  typedef.

    Updated by      Richard Gooch   21-NOV-1994: Tidied up  VERIFY_CONTROLPANEL
  macro.

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/panel/main.c

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   14-JUN-1995: Made use of <ex_uint>.

    Updated by      Richard Gooch   21-JAN-1996: Switched to <stdarg.h> and new
  documentation format. Support PIA_TOP_OF_PANEL attribute.

    Updated by      Richard Gooch   29-JAN-1996: Added "show_version" function.

    Updated by      Richard Gooch   20-FEB-1996: Fixed bug in test for
  PIA_MAX_VALUE value greater than PIA_MIN_VALUE value.

    Updated by      Richard Gooch   12-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_panel.h>
#include <karma_conn.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>


#define MAGIC_NUMBER (unsigned int) 798345329

#define PANEL_STACK_SIZE 100

#define VERIFY_CONTROLPANEL(pnl) if (pnl == NULL) \
{(void) fprintf (stderr, "NULL control panel passed\n"); \
 a_prog_bug (function_name); } \
if (pnl->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid control panel object\n"); \
 a_prog_bug (function_name); }

struct controlpanel_type
{
    unsigned int magic_number;
    flag group;
    KPanelItem first_item;
    KPanelItem last_item;
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
    char **choice_comments;
    unsigned int max_array_length;
    unsigned int min_array_length;
    unsigned int *curr_array_length;
    double min_value;
    double max_value;
    /*  Pointer to the next item in the panel  */
    KPanelItem prev;
    KPanelItem next;
/*
    void (*func) ();
*/
};

typedef struct
{
    char *field_name;
    char *field_comment;
    unsigned int field_type;
    char *field_value;
} field;


/*  Local functions  */


/*  Private data  */
static KControlPanel panel_stack[PANEL_STACK_SIZE];
static int panel_stack_index = -1;


/*  Private functions  */
STATIC_FUNCTION (flag panel_process_command, (KControlPanel panel, char *cmd,
					      flag (*unknown_func) (),
					      FILE *fp) );
STATIC_FUNCTION (void panel_show_items, (KControlPanel panel,
					 flag screen_display, FILE *fp,
					 flag verbose) );
STATIC_FUNCTION (char *decode_datum, (KPanelItem item, void *value_ptr,
				      char *string, flag *failed) );
STATIC_FUNCTION (void show_datum, (KPanelItem item, FILE *fp,
				   void *value_ptr) );
STATIC_FUNCTION (void abort_func, (char *cmd) );
STATIC_FUNCTION (void add_conn_func, (char *cmd) );
STATIC_FUNCTION (void process_choice_item, (KPanelItem item, char *cmd) );
STATIC_FUNCTION (void show_choice_item, (KPanelItem item, FILE *fp,
					 flag screen_display, flag verbose) );
STATIC_FUNCTION (void decode_array, (KPanelItem item, char *string,flag add) );
STATIC_FUNCTION (void show_array, (KPanelItem item, flag screen_display,
				   FILE *fp, flag verbose) );
STATIC_FUNCTION (unsigned int get_size_of_type, (unsigned int type) );
STATIC_FUNCTION (flag test_minmax, (KPanelItem item, double value) );
STATIC_FUNCTION (void show_protocols_func, (char *cmd) );
STATIC_FUNCTION (void show_version_func, (char *cmd) );
STATIC_FUNCTION (void decode_group, (KPanelItem item, char *string,flag add) );
STATIC_FUNCTION (void show_group, (KPanelItem item, flag screen_display,
				   FILE *fp, flag verbose) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KControlPanel panel_create (flag blank)
/*  [SUMMARY] Create a control panel object.
    <blank> If TRUE, then the routine will create a blank form, else it will
    add some internally defined panel items.
    [RETURNS] A KControlPanel object on success, else NULL.
*/
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
    panel->magic_number = MAGIC_NUMBER;
    panel->group = FALSE;
    panel->first_item = NULL;
    panel->last_item = NULL;
    if (blank) return (panel);
    panel_add_item (panel, "quit", "exit panel", PIT_EXIT_FORM, NULL, PIA_END);
    panel_add_item (panel, "exit", "exit panel", PIT_EXIT_FORM, NULL, PIA_END);
    panel_add_item (panel, "add_connection",
		    "hostname port_number protocol_name",
		    PIT_FUNCTION, (void *) add_conn_func,
		    PIA_END);
    panel_add_item (panel, "show_version",
		    "this will show version information",
		    PIT_FUNCTION, (void *) show_version_func,
		    PIA_END);
    panel_add_item (panel, "show_protocols",
		    "this will show all supported protocols",
		    PIT_FUNCTION, (void *) show_protocols_func,
		    PIA_END);
    panel_add_item (panel, "abort", "abort without saving panel values",
		    PIT_FUNCTION, (void *) abort_func,
		    PIA_END);
    return (panel);
}   /*  End Function panel_create  */

/*PUBLIC_FUNCTION*/
KControlPanel panel_create_group ()
/*  [SUMMARY] Create a panel to contain a group.
    [PURPOSE] This routine will create a control panel object which is a
    container for a group of items. Panel items may be added to this panel
    object, and the panel object itself may be later added as panel item to
    another panel object (of type PIT_GROUP).
    [RETURNS] A KControlPanel object on success, else NULL.
*/
{
    KControlPanel panel;
    static char function_name[] = "panel_create_group";

    if ( ( panel = (KControlPanel) m_alloc (sizeof *panel) ) == NULL )
    {
	m_error_notify (function_name, "control panel");
	return (NULL);
    }
    panel->magic_number = MAGIC_NUMBER;
    panel->group = TRUE;
    panel->first_item = NULL;
    panel->last_item = NULL;
    return (panel);
}   /*  End Function panel_create_group  */

/*PUBLIC_FUNCTION*/
void panel_add_item (KControlPanel panel, char *name, char *comment,
		     unsigned int type, void *value_ptr, ...)
/*  [SUMMARY] Add an item to a panel.
    [PURPOSE] This routine will add a panel item to a KControlPanel object.
    Each panel item has a number of "attributes". First come the "core"
    attributes, follwed by the optional attributes.
    Below are the core attributes:
    <panel> The control panel to add to.
    <name> The name of the panel item.
    <comment> the comment (eg. name of the units: "(km/sec)").
    <type> The type of the panel item. See [<DS_KARMA_DATA_TYPES>] and
    [<PANEL_ITEM_TYPES>] for legal values.
    <value_ptr> A pointer to the panel item data storage.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with the value PIA_END.
    See [<PANEL_ATTRIBUTES>] for a list of defined attributes.
    [RETURNS] Nothing.
*/
{
    va_list argp;
    KPanelItem item;
    KPanelItem curr_item;
    KPanelItem dup_item;
    KControlPanel group;
    flag top_of_panel = TRUE;
    unsigned int count, arr_len;
    unsigned int att_key;
    char *new_string;
    char **string_ptr;
    static char function_name[] = "panel_add_item";

    va_start (argp, value_ptr);
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
      case PIT_CHOICE_INDEX:
	if (panel->group)
	{
	    (void) fprintf (stderr,
			    "Cannot add type PIT_CHOICE_INDEX to a group\n");
	    a_prog_bug (function_name);
	}
	/*  Fall through  */
      case PIT_FUNCTION:
      case PIT_FLAG:
	if (value_ptr == NULL)
	{
	    (void) fprintf (stderr, "NULL value pointer passed\n");
	    a_prog_bug (function_name);
	}
	/*  Fall through  */
      case PIT_EXIT_FORM:
	/*  Fine  */
	break;
      case PIT_GROUP:
	if (panel->group)
	{
	    (void) fprintf (stderr, "Cannot add a group to a group\n");
	    a_prog_bug (function_name);
	}
	group = (KControlPanel) value_ptr;
	VERIFY_CONTROLPANEL (group);
	break;
      case K_VSTRING:
	if (value_ptr == NULL)
	{
	    (void) fprintf (stderr, "NULL value pointer passed\n");
	    a_prog_bug (function_name);
	}
	break;
      case K_FSTRING:
	(void) fprintf (stderr, "Fixed string not yet implemented\n");
	a_prog_bug (function_name);
	break;
      default:
	if ( !ds_element_is_named (type) )
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
    item->choice_strings = NULL;
    item->choice_comments = NULL;
    item->curr_array_length = NULL;
    item->min_array_length = 0;
    item->max_array_length = 0;
    item->min_value = -TOOBIG;
    item->max_value = TOOBIG;
    item->prev = NULL;
    item->next = NULL;
    if ( ( item->name = st_dup (name) ) == NULL )
    {
	m_abort (function_name, "panel item name");
    }
    if ( (int) strlen (name) >= 19 ) item->name[19] = '\0';
    if ( ( item->comment = st_dup (comment) ) == NULL )
    {
	m_abort (function_name, "panel item comment string");
    }
    item->type = type;
    item->value_ptr = value_ptr;
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != PIA_END )
    {
	switch (att_key)
	{
	  case PIA_NUM_CHOICE_STRINGS:
	    if ( ( item->num_choice_strings =
		  va_arg (argp, unsigned int) ) == 0 )
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
	    if ( ( item->choice_strings = va_arg (argp, char **) )
		== NULL )
	    {
		(void) fprintf (stderr,
				"NULL choice string array pointer passed\n");
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
	  case PIA_CHOICE_COMMENTS:
	    if ( ( item->choice_comments = va_arg (argp, char **) )
		== NULL )
	    {
		(void) fprintf (stderr,
				"NULL choice comment array pointer passed\n");
		a_prog_bug (function_name);
	    }
	    if (type != PIT_CHOICE_INDEX)
	    {
		(void) fprintf (stderr,
				"Cannot specify choice comments for type: %u\n",
				type);
		a_prog_bug (function_name);
	    }
	    break;
	  case PIA_ARRAY_LENGTH:
	  case PIA_ARRAY_MIN_LENGTH:
	  case PIA_ARRAY_MAX_LENGTH:
	    if (panel->group)
	    {
		(void) fprintf (stderr,
				"Cannot specify array length for group item\n");
		a_prog_bug (function_name);
	    }
	    switch (type)
	    {
	      case PIT_FUNCTION:
	      case PIT_EXIT_FORM:
	      case PIT_CHOICE_INDEX:
		(void) fprintf (stderr,
				"Cannot specify array length for type: %u\n",
				type);
		a_prog_bug (function_name);
		break;
	      case PIT_FLAG:
	      case PIT_GROUP:
		break;
	      default:
		if ( !ds_element_is_named (type) )
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
		item->curr_array_length = va_arg (argp,unsigned int *);
		break;
	      case PIA_ARRAY_MIN_LENGTH:
		item->min_array_length = va_arg (argp, unsigned int);
		break;
	      case PIA_ARRAY_MAX_LENGTH:
		item->max_array_length = va_arg (argp, unsigned int);
		break;
	    }
	    break;
	  case PIA_MIN_VALUE:
	    if ( !ds_element_is_legal (type) || !ds_element_is_atomic (type) ||
		ds_element_is_complex (type) )
	    {
		(void) fprintf (stderr,
				"Cannot specify minimum value for non-real item\n");
		a_prog_bug (function_name);
	    }
	    item->min_value = va_arg (argp, double);
	    break;
	  case PIA_MAX_VALUE:
	    if ( !ds_element_is_legal (type) || !ds_element_is_atomic (type) ||
		ds_element_is_complex (type) )
	    {
		(void) fprintf (stderr,
				"Cannot specify maximum value for non-real item\n");
		a_prog_bug (function_name);
	    }
	    item->max_value = va_arg (argp, double);
	    break;
	  case PIA_TOP_OF_PANEL:
	    top_of_panel = va_arg (argp, flag);
	    FLAG_VERIFY (top_of_panel);
	    break;
	  default:
	    (void) fprintf (stderr, "Unknown panel item attribute key: %u\n",
			    att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    /*  Check for enough extra information for type  */
    switch (type)
    {
      case PIT_CHOICE_INDEX:
	if (item->num_choice_strings == 0)
	{
	    (void) fprintf (stderr, "Zero choice strings for choice type\n");
	    a_prog_bug (function_name);
	}
	if (item->choice_strings == NULL)
	{
	    (void) fprintf (stderr,
			    "NULL choice string pointer for choice type\n");
	    a_prog_bug (function_name);
	}
	if (*(unsigned int *) value_ptr >= item->num_choice_strings)
	{
	    (void) fprintf (stderr,
			    "Initial choice index: %u not less than num_choices: %u\n",
			    *(unsigned int *) value_ptr,
			    item->num_choice_strings);
	    a_prog_bug (function_name);
	}
	break;
      default:
	break;
    }
    /*  Check for valid array specifications  */
    if (item->max_array_length < item->min_array_length)
    {
	(void) fprintf (stderr,
			"Maximum array length: %u less than minimum: %u\n",
			item->max_array_length, item->min_array_length);
	a_prog_bug (function_name);
    }
    /*  Check for valid minimum and maximum  */
    if ( (item->min_value > -TOOBIG) && (item->max_value < TOOBIG) &&
	(item->min_value >= item->max_value) )
    {
	(void) fprintf (stderr,
			"Maximum value: %e not greater than minimum: %e\n",
			item->max_value, item->min_value);
	a_prog_bug (function_name);
    }
    /*  Make sure we don't reference externally defined strings  */
    if (type == K_VSTRING)
    {
	arr_len = (item->max_array_length < 1) ? 1 : *item->curr_array_length;
	string_ptr = (char **) value_ptr;
	for (count = 0; count < arr_len; ++count)
	{
	    if (string_ptr[count] == NULL)
	    {
		if ( ( new_string = st_dup ("") ) == NULL )
		{
		    m_error_notify (function_name, "string value");
		    return;
		}
	    }
	    else
	    {
		if ( ( new_string = st_dup (string_ptr[count]) ) == NULL )
		{
		    m_error_notify (function_name, "string value");
		    return;
		}
	    }
	    string_ptr[count] = new_string;
	}
    }
    /*  Add panel item to list  */
    /*  Check for existing panel item of the same name  */
    for (curr_item = panel->first_item, dup_item = NULL;
	 curr_item != NULL;
	 curr_item = curr_item->next)
    {
	if (strcmp (curr_item->name, name) == 0)
	{
	    /*  Match!  */
	    dup_item = curr_item;
	    curr_item = NULL;
	}
    }
    if (dup_item != NULL)
    {
	/*  Remove this one  */
	if (dup_item->prev == NULL)
	{
	    /*  First entry in list  */
	    panel->first_item = dup_item->next;
	}
	else
	{
	    /*  Not first entry  */
	    dup_item->prev->next = dup_item->next;
	}
	if (dup_item->next == NULL)
	{
	    panel->last_item = dup_item->prev;
	}
	else
	{
	    dup_item->next->prev = dup_item->prev;
	}
	/*  Deallocate it  */
	m_free (dup_item->name);
	m_free (dup_item->comment);
	m_free ( (char *) dup_item );
    }
    if (panel->first_item == NULL)
    {
	/*  First entry  */
	panel->first_item = item;
	panel->last_item = item;
	return;
    }
    if (top_of_panel)
    {
	item->next = panel->first_item;
	panel->first_item->prev = item;
	panel->first_item = item;
    }
    else
    {
	item->prev = panel->last_item;
	panel->last_item->next = item;
	panel->last_item = item;
    }	
}   /*  End Function panel_add_item  */

/*PUBLIC_FUNCTION*/
void panel_push_onto_stack (KControlPanel panel)
/*  [SUMMARY] Push a control panel object onto the control panel stack.
    <panel> The control panel.
    [RETURNS] Nothing.
*/
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
/*  [SUMMARY] Pop last pushed control panel object from control panel stack.
    [RETURNS] Nothing.
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
flag panel_process_command_with_stack (char *cmd, flag (*unknown_func) (),
				       FILE *fp)
/*  [SUMMARY] Process a command.
    [PURPOSE] This routine will process a command, using the top control panel
    on the stack to interpret it.
    <cmd> The command.
    <unknown_func> The function that is called when the command is not
    understood. If this is NULL, then a message is displayed if the command is
    not understood. The prototype function is [<PANEL_PROTO_decode_func>].
    <fp> Output messages are directed here.
    [RETURNS] TRUE if more commands should be processed, else FALSE, indicating
    that the control panel's "exit" command was entered.
*/
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

static flag panel_process_command (KControlPanel panel, char *cmd,
				   flag (*unknown_func) (), FILE *fp)
/*  [PURPOSE] This routine will process a command, using a given control panel.
    <panel> The control panel.
    <cmd> The command.
    <unknown_func> The function that is called when the command is not
    understood. If this is NULL, then a message is displayed if the command is
    not understood. The interface to this routine is as follows:
    [<pre>]
    flag unknown_func (char *cmd, FILE *fp)
    *   [PURPOSE] This routine will process a command.
        <cmd> The command.
	<fp> Output messages are directed here.
	[RETURNS] TRUE if more commands should be processed, else FALSE,
	indicating that the "exit" command was entered.
    *
    [</pre>]
    <fp> Output messages are directed here.
    [RETURNS] TRUE if more commands should be processed, else FALSE, indicating
    that the control panel's "exit" command was entered.
*/
{
    KPanelItem item, curr_item;
    flag array_add = FALSE;
    int pname_len;
    flag dummy;
    char *word1, *pname, *cmd_ptr;
    void (*func_ptr) ();
    static char function_name[] = "panel_process_command";

    VERIFY_CONTROLPANEL (panel);
    if (cmd == NULL)
    {
	(void) fprintf (stderr, "NULL command pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Extract panel item name  */
    if ( ( word1 = ex_word (cmd, &cmd_ptr) ) == NULL )
    {
	m_abort (function_name, "panel item name");
    }
    /*  Decode a few special panel items  */
    if (strcmp (word1, "??") == 0)
    {
	/*  Print panel items  */
	panel_show_items (panel, TRUE, fp, TRUE);
	m_free (word1);
	return (TRUE);
    }
    if (strcmp (word1, "?") == 0)
    {
	/*  Print panel items  */
	panel_show_items (panel, TRUE, fp, FALSE);
	m_free (word1);
	return (TRUE);
    }
    if (strcmp (word1, "?#") == 0)
    {
	/*  Print panel items  */
	panel_show_items (panel, FALSE, fp, FALSE);
	m_free (word1);
	return (TRUE);
    }
    /*  Determine if array addition syntax  */
    if (word1[0] == '+')
    {
	pname = word1 + 1;
	array_add = TRUE;
    }
    else
    {
	pname = word1;
	array_add = FALSE;
    }
    /*  Loop through trying to find panel item  */
    pname_len = strlen (pname);
    for (curr_item = panel->first_item, item = NULL; curr_item != NULL;
	 curr_item = curr_item->next)
    {
	if (strncmp (pname, curr_item->name, pname_len) == 0)
	{
	    if (item != NULL)
	    {
		(void) fprintf (stderr, "Ambiguous panel item name: \"%s\"\n",
				pname);
		m_free (word1);
		return (TRUE);
	    }
	    item = curr_item;
	}
    }
    m_free (word1);
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
    if ( array_add && (item->max_array_length < 1) )
    {
	(void) fprintf (stderr,
			"Item: \"%s\" is not an array: cannot use '+'\n",
			item->name);
	return (TRUE);
    }
    switch (item->type)
    {
      case PIT_FUNCTION:
	func_ptr = ( void (*) () ) item->value_ptr;
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
      case PIT_GROUP:
	decode_group (item, cmd_ptr, array_add);
	return (TRUE);
/*
	break;
*/
      default:
	/*  Process other types later  */
	break;
    }
    /*  No actions or group parameters at this point  */
    /*  Check if array of values  */
    if (item->max_array_length > 0)
    {
	decode_array (item, cmd_ptr, array_add);
	return (TRUE);
    }
    /*  Single instance item  */
    switch (item->type)
    {
      case PIT_FLAG:
	(void) decode_datum (item, item->value_ptr, cmd_ptr, &dummy);
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
	if ( ds_element_is_named (item->type) )
	{
	    (void) decode_datum (item, item->value_ptr, cmd_ptr, &dummy);
	    return (TRUE);
	}
	(void) fprintf (stderr, "Illegal panel item type: %u\n", item->type);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function panel_process_command  */

static void panel_show_items (KControlPanel panel, flag screen_display,
			      FILE *fp, flag verbose)
/*  [PURPOSE] This routine will show the panel items for a form.
    <panel> The control panel.
    <screen_display> If TRUE, then the comment strings and action panel items
    are displayed.
    <fp> Output messages are directed here.
    <verbose> If TRUE, then the output is verbose.
    [RETURNS] Nothing.
*/
{
    KPanelItem item;
    static char function_name[] = "panel_show_items";

    VERIFY_CONTROLPANEL (panel);
    FLAG_VERIFY (screen_display);
    if (screen_display) (void) fprintf (fp, "\n");
    for (item = panel->first_item; item != NULL; item = item->next)
    {
	/*  Display name and value  */
	switch (item->type)
	{
	  case PIT_FUNCTION:
	  case PIT_EXIT_FORM:
	    if (screen_display)
	    {
		(void) fprintf (fp, "%-40s#%s\n",
				item->name, item->comment);
	    }
	    continue;
/*
	    break;
*/
	  case PIT_GROUP:
	    show_group (item, screen_display, fp, verbose);
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
	if (item->max_array_length > 0)
	{
	    show_array (item, screen_display, fp, verbose);
	    continue;
	}
	/*  Single instance item  */
	switch (item->type)
	{
	  case PIT_FLAG:
	    (void) fprintf (fp, "%-20s%-20s", item->name,
			    (*(flag *) item->value_ptr) ? "on" : "off");
	    break;
	  case PIT_CHOICE_INDEX:
	    show_choice_item (item, fp, screen_display, verbose);
	    continue;

/*
	    break;
*/
	  default:
	    if ( !ds_element_is_named (item->type) )
	    {
		(void) fprintf (stderr, "Illegal panel item type: %u\n",
				item->type);
		a_prog_bug (function_name);
	    }
	    (void) fprintf (fp, "%-20s", item->name);
	    show_datum (item, fp, NULL);
	}
	/*  This should come last  */
	if (screen_display)
	{
	    /*  Display comment  */
	    (void) fprintf (fp, "#%s\n", item->comment);
	}
	else
	{
	    (void) fprintf (fp, "\n");
	}
    }
}   /*  End Function panel_show_items  */


/*  Private functions follow  */

static char *decode_datum (KPanelItem item, void *value_ptr, char *string,
			   flag *failed)
/*  [PURPOSE] This routine will decode a single value (ie. atomic, string,
    flag, choice) from a command string.
    <item> The panel item.
    <value_ptr> The value pointer.
    <string> The command string.
    <failed> If the routine failed, the value of TRUE will be written here,
    else FALSE is written here.
    [RETURNS] A pointer to the next argument.
*/
{
    unsigned int type = item->type;
    char *name = item->name;
    FString *fstring;
    char *new_string;
    long ivalue[2];
    double dvalue[2];
    static char function_name[] = "decode_datum";

    *failed = TRUE;
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
	if ( test_minmax (item, dvalue[0]) ) return (NULL);
	*(float *) value_ptr = dvalue[0];
	break;
      case K_DOUBLE:
	dvalue[0] = ex_float (string, &string);
	if ( test_minmax (item, dvalue[0]) ) return (NULL);
	*(double *) value_ptr = dvalue[0];
	break;
      case K_BYTE:
	ivalue[0] = ex_int (string, &string);
	if ( test_minmax (item, (double) ivalue[0]) ) return (NULL);
	*(char *) value_ptr = ivalue[0];
	break;
      case K_INT:
	ivalue[0] = ex_int (string, &string);
	if ( test_minmax (item, (double) ivalue[0]) ) return (NULL);
	*(int *) value_ptr = ivalue[0];
	break;
      case K_SHORT:
	ivalue[0] = ex_int (string, &string);
	if ( test_minmax (item, (double) ivalue[0]) ) return (NULL);
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
	if ( test_minmax (item, (double) ivalue[0]) ) return (NULL);
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
	ivalue[0] = ex_uint (string, &string);
	if ( test_minmax (item, (double) ivalue[0]) ) return (NULL);
	*(unsigned char *) value_ptr = ivalue[0];
	break;
      case K_UINT:
	ivalue[0] = ex_uint (string, &string);
	if ( test_minmax (item, (double) ivalue[0]) ) return (NULL);
	*(unsigned int *) value_ptr = ivalue[0];
	break;
      case K_USHORT:
	ivalue[0] = ex_uint (string, &string);
	if ( test_minmax (item, (double) ivalue[0]) ) return (NULL);
	*(unsigned short *) value_ptr = ivalue[0];
	break;
      case K_ULONG:
	ivalue[0] = ex_uint (string, &string);
	if ( test_minmax (item, (double) ivalue[0]) ) return (NULL);
	*(unsigned long *) value_ptr = ivalue[0];
	break;
      case K_UBCOMPLEX:
	ivalue[0] = ex_uint (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_uint (string, &string);
	*(unsigned char *) value_ptr = ivalue[0];
	*( (unsigned char *) value_ptr + 1 ) = ivalue[1];
        break;
      case K_UICOMPLEX:
	ivalue[0] = ex_uint (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_uint (string, &string);
	*(unsigned int *) value_ptr = ivalue[0];
	*( (unsigned int *) value_ptr + 1 ) = ivalue[1];
        break;
      case K_USCOMPLEX:
	ivalue[0] = ex_uint (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_uint (string, &string);
	*(unsigned short *) value_ptr = ivalue[0];
	*( (unsigned short *) value_ptr + 1 ) = ivalue[1];
        break;
      case K_ULCOMPLEX:
	ivalue[0] = ex_uint (string, &string);
	if (string == NULL)
	{
	    (void) fprintf (stderr,
			    "Imaginary component missing for panel item: \"%s\"\n",
			    name);
	    return (NULL);
	}
	ivalue[1] = ex_uint (string, &string);
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
	if (strlen (new_string) > fstring->max_len)
	{
	    (void) fprintf (stderr,
			    "String value for panel item: \"%s\" too long\n",
			    name);
	    m_free (new_string);
	    return (NULL);
	}
	(void) strncpy (fstring->string, new_string, fstring->max_len);
        break;
      case PIT_FLAG:
	*(flag *) value_ptr = ex_on_or_off (&string);
	break;
      default:
	(void) fprintf (stderr, "Illegal panel item type: %u\n", type);
	a_prog_bug (function_name);
	break;
    }
    *failed = FALSE;
    return (string);
}   /*  End Function decode_datum  */

static void show_datum (KPanelItem item, FILE *fp, void *value_ptr)
/*  [PURPOSE] This routine will show a single value (ie. atomic or string).
    <panel> The panel item.
    <fp> Output messages are directed here.
    <value_ptr> A pointer to the value. If this is NULL, then the value pointer
    registered with the item is used.
    [RETURNS] Nothing.
*/
{
    int pad_len;
    FString *fstring;
    char padding[STRING_LENGTH];
    static char function_name[] = "show_datum";

    if (value_ptr == NULL) value_ptr = item->value_ptr;
    switch (item->type)
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
	(void) fprintf (fp, "%-20ld", *(long *) value_ptr);
	break;
      case K_LCOMPLEX:
	(void) fprintf ( fp, "%-20ld %-20ld",
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
	(void) fprintf (fp, "%-20lu", *(unsigned long *) value_ptr);
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
	(void) fprintf ( fp, "%-20lu %-20lu",
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
	if ( ( pad_len = 18 - strlen (fstring->string) ) < 1 )
	{
	    pad_len = 1;
	}
	padding[pad_len] = '\0';
	while (pad_len > 0) padding[--pad_len] = ' ';
	(void) fprintf (fp, "\"%s\"%s", fstring->string, padding);
	break;
      default:
	(void) fprintf (stderr, "Illegal panel item type: %u\n", item->type);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function show_datum  */

static void abort_func (char *cmd)
/*  [PURPOSE] This routine will exit the process.
    [RETURNS] Does not return.
*/
{
    exit (RV_ABORT);
}   /*  End Function abort_func  */

static void add_conn_func (char *cmd)
/*  [PURPOSE] This routine will add a connection.
    <cmd> The connection to add.
    [RETURNS] Nothing.
*/
{
    char *host;
    char *protocol;
    unsigned int port;

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
    port = ex_uint (cmd, &cmd);
    if ( (cmd == NULL) || (*cmd == '\0') ||
	( ( protocol = ex_str (cmd, &cmd) ) == NULL ) )
    {
	(void) fprintf (stderr, "Must also specify protocol\n");
	m_free (host);
	return;
    }
    if ( !conn_attempt_connection (host, port, protocol) )
    {
	(void) fprintf (stderr,
			"Error adding \"%s\" connection\n", protocol);
    }
    m_free (host);
    m_free (protocol);
}   /*  End Function add_conn_func  */

static void process_choice_item (KPanelItem item, char *cmd)
/*  [PURPOSE] This routine will process a choice index item.
    <item> The panel item.
    <cmd> The command.
    [RETURNS] Nothing.
*/
{
    unsigned int count, index;
    int len;
    char *choice;

    if ( ( choice = ex_str (cmd, &cmd) ) == NULL )
    {
	(void) fprintf (stderr,
			"String value missing for panel item: \"%s\"\n",
			item->name);
	return;
    }
    if (cmd != NULL)
    {
	(void) fprintf (stderr, "Too many arguments\n");
	m_free (choice);
	return;
    }
    len = strlen (choice);
    for (count = 0, index = item->num_choice_strings;
	 count < item->num_choice_strings;
	 ++count)
    {
	if (strncmp (choice, item->choice_strings[count], len) == 0)
	{
	    if (index < item->num_choice_strings)
	    {
		(void) fprintf (stderr, "Ambiguous choice: \"%s\"\n", choice);
		m_free (choice);
		return;
	    }
	    index = count;
	}
    }
    if (index >= item->num_choice_strings)
    {
	(void) fprintf (stderr, "Unknown choice: \"%s\" for item: \"%s\"\n",
			choice, item->name);
	m_free (choice);
	return;
    }
    m_free (choice);
    *(unsigned int *) item->value_ptr = index;
}   /*  End Function process_choice_item  */

static void show_choice_item (KPanelItem item, FILE *fp, flag screen_display,
			      flag verbose)
/*  [PURPOSE] This routine will show the value of a choice index item.
    <item> The panel item.
    <fp> The output messages are directed here.
    <screen_display> If TRUE, then the comment strings and action panel items
    are displayed.
    <verbose> If TRUE, then the output is verbose.
    [RETURNS] Nothing.
*/
{
    unsigned int count, index;
    char **choices, **choice_comments;

    choices = (char **) item->choice_strings;
    choice_comments = (char **) item->choice_comments;
    index = *(unsigned int *) item->value_ptr;
    if (screen_display)
    {
	(void) fprintf (fp, "%-20s%-20s#%s\n",
			item->name, choices[index], item->comment);
	if (!verbose) return;
	(void) fprintf (fp, "  Choices:\n");
	for (count = 0; count < item->num_choice_strings; ++count)
	{
	    if ( ( choice_comments != NULL ) &&
		( choice_comments[count] != NULL ) )
	    {
		(void) fprintf (fp, "    %-36s#%s\n",
				choices[count], choice_comments[count]);
	    }
	    else
	    {
		(void) fprintf (fp, "    %s\n", choices[count]);
	    }
	}
	return;
    }
    (void) fprintf (fp, "%-20s%-20s\n", item->name, choices[index]);
}   /*  End Function show_choice_item  */

static void decode_array (KPanelItem item, char *string, flag add)
/*  [PURPOSE] This routine will decode an array of values (ie. atomic, string,
    flag, choice) from a command string.
    <item> The panel item.
    <string> The command string.
    <add> If TRUE the array will be added to.
    [RETURNS] Nothing.
*/
{
    flag failed;
    unsigned int array_count;
    unsigned int type;
    char *value_ptr;
/*
    static char function_name[] = "decode_array";
*/

    type = item->type;
    value_ptr = item->value_ptr;
    if (add) value_ptr += get_size_of_type (type) * *item->curr_array_length;
    array_count = add ? *item->curr_array_length : 0;
    while (string != NULL)
    {
	if (array_count >= item->max_array_length)
	{
	    *item->curr_array_length = array_count;
	    if (string != NULL)
	    {
		(void) fprintf (stderr,
				"Ignored trailing arguments: \"%s\" for panel item: \"%s\"\n",
				string, item->name);
	    }
	    return;
	}
	string = decode_datum (item, value_ptr, string, &failed);
	/*  Memory leak for string types if user doesn't give at least as many
	    strings as there were last time. FIX LATER  */
	if (failed) return;
	value_ptr += get_size_of_type (type);
	++array_count;
    }
    if (array_count < item->min_array_length)
    {
	(void) fprintf (stderr,
			"Not enough values given for array: need: %d\n",
			(int) item->min_array_length);
	return;
    }
    *item->curr_array_length = array_count;
}   /*  End Function decode_array  */

static void show_array (KPanelItem item, flag screen_display, FILE *fp,
			flag verbose)
/*  [PURPOSE] This routine will display an array of values (ie. atomic, string,
    flag, choice).
    <choice> The panel item.
    <screen_display> If TRUE, then the comment strings are displayed.
    <fp> Output messages are directed here.
    <verbose> If TRUE then the output is verbose.
    [RETURNS] Nothing.
*/
{
    FString *fstring;
    char *value_ptr;
    unsigned int array_count;
    static char function_name[] = "show_array";

    if (screen_display)
    {
	if (item->max_array_length == item->min_array_length)
	{
	    (void) fprintf (fp, "%s[%d]    ",
			    item->name, (int) item->min_array_length);
	}
	else
	{
	    (void) fprintf (fp, "%s[%d..%d]    ",
			    item->name, (int) item->min_array_length,
			    (int) item->max_array_length);
	}
    }
    else
    {
	(void) fprintf (fp, "%s    ", item->name);
    }
    value_ptr = item->value_ptr;
    for ( array_count = 0; array_count < *item->curr_array_length;
	 ++array_count, value_ptr += get_size_of_type (item->type) )
    {
	switch (item->type)
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
	    (void) fprintf (fp, "%ld  ", *(long *) value_ptr);
	    break;
	  case K_LCOMPLEX:
	    (void) fprintf ( fp, "%ld   %ld  ",
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
	    (void) fprintf (fp, "%lu  ", *(unsigned long *) value_ptr);
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
	    (void) fprintf ( fp, "%lu   %lu  ",
			    *(unsigned long *) value_ptr,
			    *( (unsigned long *) value_ptr + 1 ) );
	    break;
	  case K_VSTRING:
	    (void) fprintf (fp, "\"%s\"  ", *(char **) value_ptr);
	    break;
	  case K_FSTRING:
	    fstring = (FString *) value_ptr;
	    (void) fprintf (fp, "\"%s\"  ", fstring->string);
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal panel item type: %u\n",
			    item->type);
	    a_prog_bug (function_name);
	    break;
	}
    }
    if (screen_display)
    {
	(void) fprintf (fp, "  #%s\n", item->comment);
    }
    else
    {
	(void) fprintf (fp, "\n");
    }
}   /*  End Function show_array  */

static unsigned int get_size_of_type (unsigned int type)
/*  [PURPOSE] This routine will determine the size of a parameter item.
    <type> The parameter item type.
    [RETURNS] The size (in bytes).
*/
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
	if ( !ds_element_is_named (type) )
	{
	    (void) fprintf (stderr, "Illegal panel item type: %u\n", type);
	    a_prog_bug (function_name);
	}
	size = host_type_sizes[type];
	break;
    }
    return (size);
}   /*  End Function get_size_of_type  */

static flag test_minmax (KPanelItem item, double value)
/*  [PURPOSE] This routine will determine is a value is outside the range
    allowed.
    <item> The panel item.
    <value> The value.
    [RETURNS] TRUE if the value is outside the allowed range, else FALSE.
*/
{
    if ( (item->min_value > -TOOBIG) && (value < item->min_value) )
    {
	if ( (item->type == K_FLOAT) || (item->type == K_DOUBLE) )
	{
	    (void) fprintf (stderr, "Value: %e is less than minimum: %e\n",
			    value, item->min_value);
	}
	else
	{
	    (void) fprintf (stderr, "Value: %ld is less than minimum: %ld\n",
			    (long) value, (long) item->min_value);
	}
	return (TRUE);
    }
    if ( (item->max_value < TOOBIG) && (value > item->max_value) )
    {
	if ( (item->type == K_FLOAT) || (item->type == K_DOUBLE) )
	{
	    (void) fprintf (stderr, "Value: %e is grater than maximum: %e\n",
			    value, item->max_value);
	}
	else
	{
	    (void) fprintf (stderr,
			    "Value: %ld is greater than maximum: %ld\n",
			    (long) value, (long) item->max_value);
	}
	return (TRUE);
    }
    return (FALSE);
}   /*  End Function test_minmax  */

static void show_protocols_func (char *cmd)
/*  [PURPOSE] This routine will show the supported protocols.
    <cmd> Ignored.
    [RETURNS] Nothing.
*/
{
    char **strings, **ptr;

    if ( ( strings = conn_extract_protocols () ) == NULL ) return;
    for (ptr = strings; *ptr != NULL; ++ptr)
    {
	(void) fprintf (stderr, "%s\n", *ptr);
    }
    for (ptr = strings; *ptr != NULL; ++ptr) m_free (*ptr);
    m_free ( (char *) strings );
}   /*  End Function show_protocols_func  */

static void show_version_func (char *cmd)
/*  [PURPOSE] This routine will show the library and module version.
    <cmd> Ignored.
    [RETURNS] Nothing.
*/
{
    extern char module_name[STRING_LENGTH + 1];
    extern char module_version_date[STRING_LENGTH + 1];
    extern char module_lib_version[STRING_LENGTH + 1];
    extern char karma_library_version[STRING_LENGTH + 1];

    (void) fprintf (stderr, "module: \"%s\" version: \"%s\"\n",
		    module_name, module_version_date);
    (void) fprintf (stderr, "Module running with Karma library version: %s\n",
		    karma_library_version);
    (void) fprintf (stderr, "Module compiled with library version: %s\n",
		    module_lib_version);
}   /*  End Function show_version_func  */

static void show_group (KPanelItem item, flag screen_display, FILE *fp,
			flag verbose)
/*  [PURPOSE] This routine will display a group item.
    <panel> The panel item.
    <screen_display> If TRUE, then the comment strings are displayed.
    <fp> Output messages are directed here.
    <verbose> If TRUE then the output is verbose.
    [RETURNS] Nothing.
*/
{
    KControlPanel group;
    KPanelItem gitem;
    unsigned int num_groups;
    FString *fstring;
    char *value_ptr;
    unsigned int array_count;
    char txt[STRING_LENGTH];
    static char function_name[] = "show_group";

    if (item->type != PIT_GROUP)
    {
	(void) fprintf (stderr, "Item is not of type PIT_GROUP\n");
	a_prog_bug (function_name);
    }
    group = (KControlPanel) item->value_ptr;
    if (!group->group)
    {
	(void) fprintf (stderr, "Group KControlPanel is not a group\n");
	a_prog_bug (function_name);
    }
    if (screen_display)
    {
	if (item->max_array_length < 1)
	{
	    (void) strcpy (txt, item->name);
	}
	else if (item->max_array_length == item->min_array_length)
	{
	    (void) sprintf (txt, "%s[%d]",
			    item->name, (int) item->min_array_length);
	}
	else
	{
	    (void) sprintf (txt, "%s[%d..%d]",
			    item->name, (int) item->min_array_length,
			    (int) item->max_array_length);
	}
	(void) fprintf (fp, "%-40s#%s\n", txt, item->comment);
	/*  Show group items  */
	for (gitem = group->first_item; gitem != NULL; gitem = gitem->next)
	{
	    (void) fprintf (fp, "  %-9s", gitem->name);
	}
	(void) fprintf (fp, "\n");
    }
    else
    {
	(void) fprintf (fp, "%s    ", item->name);
    }
    num_groups = (item->max_array_length < 1) ? 1 : *item->curr_array_length;
    /*  Loop through all the instances of group  */
    for (array_count = 0; array_count < num_groups; ++array_count)
    {
	/*  Loop through all group items  */
	for (gitem = group->first_item; gitem != NULL; gitem = gitem->next)
	{
	    value_ptr = gitem->value_ptr;
	    value_ptr += array_count * get_size_of_type (gitem->type);
	    switch (gitem->type)
	    {
	      case K_FLOAT:
		(void) fprintf (fp, "  %-9.3e", *(float *) value_ptr);
		break;
	      case K_DOUBLE:
		(void) fprintf (fp, "  %-9.3e", *(double *) value_ptr);
		break;
	      case K_BYTE:
		(void) fprintf (fp, "  %-9d", *(char *) value_ptr);
		break;
	      case K_INT:
		(void) fprintf (fp, "  %-9d", *(int *) value_ptr);
		break;
	      case K_SHORT:
		(void) fprintf (fp, "  %-9d", *(short *) value_ptr);
		break;
	      case K_COMPLEX:
		(void) fprintf ( fp, "  %-4e %-4e",
				*(float *) value_ptr,
				*( (float *) value_ptr + 1 ) );
		break;
	      case K_DCOMPLEX:
		(void) fprintf ( fp, "  %-4e %-4e",
				*(double *) value_ptr,
				*( (double *) value_ptr + 1 ) );
		break;
	      case K_BCOMPLEX:
		(void) fprintf ( fp, "  %-4d %-4d",
				*(char *) value_ptr,
				*( (char *) value_ptr + 1 ) );
		break;
	      case K_ICOMPLEX:
		(void) fprintf ( fp, "  %-4d %-4d",
				*(int *) value_ptr,
				*( (int *) value_ptr + 1 ) );
		break;
	      case K_SCOMPLEX:
		(void) fprintf ( fp, "  %-4d %-4d",
				*(short *) value_ptr,
				*( (short *) value_ptr + 1 ) );
		break;
	      case K_LONG:
		(void) fprintf (fp, "  %-9ld", *(long *) value_ptr);
		break;
	      case K_LCOMPLEX:
		(void) fprintf ( fp, "  %-4ld %-4ld",
				*(long *) value_ptr,
				*( (long *) value_ptr + 1 ) );
		break;
	      case K_UBYTE:
		(void) fprintf (fp, "  %-9u", *(unsigned char *) value_ptr);
		break;
	      case K_UINT:
		(void) fprintf (fp, "  %-9u", *(unsigned int *) value_ptr);
		break;
	      case K_USHORT:
		(void) fprintf (fp, "  %-9u", *(unsigned short *) value_ptr);
		break;
	      case K_ULONG:
		(void) fprintf (fp, "  %-9lu", *(unsigned long *) value_ptr);
		break;
	      case K_UBCOMPLEX:
		(void) fprintf ( fp, "  %-4u %-4u",
				*(unsigned char *) value_ptr,
				*( (unsigned char *) value_ptr + 1 ) );
		break;
	      case K_UICOMPLEX:
		(void) fprintf ( fp, "  %-4u %-4u",
				*(unsigned int *) value_ptr,
				*( (unsigned int *) value_ptr + 1 ) );
		break;
	      case K_USCOMPLEX:
		(void) fprintf ( fp, "  %-4u %-4u",
				*(unsigned short *) value_ptr,
				*( (unsigned short *) value_ptr + 1 ) );
		break;
	      case K_ULCOMPLEX:
		(void) fprintf ( fp, "  %-4lu %-4lu",
				*(unsigned long *) value_ptr,
				*( (unsigned long *) value_ptr + 1 ) );
		break;
	      case K_VSTRING:
		(void) sprintf (txt, "\"%s\"", *(char **) value_ptr);
		(void) fprintf (fp, "  %-9s", txt);
		break;
	      case K_FSTRING:
		fstring = (FString *) value_ptr;
		(void) sprintf (txt, "\"%s\"", fstring->string);
		(void) fprintf (fp, "  %-9s", txt);
		break;
	      default:
		(void) fprintf (stderr, "Illegal panel item type: %u\n",
				item->type);
		a_prog_bug (function_name);
		break;
	    }
	}
	if (screen_display) (void) fprintf (fp, "\n");
    }
    if (!screen_display) (void) fprintf (fp, "\n");
}   /*  End Function show_group  */

static void decode_group (KPanelItem item, char *string, flag add)
/*  [PURPOSE] This routine will decode a group item.
    <item> The panel item.
    <string> The command string.
    <add> If TRUE the array is to be added to.
    [RETURNS] Nothing.
*/
{
    KControlPanel group;
    KPanelItem gitem;
    flag failed;
    unsigned int max_groups;
    char *value_ptr;
    unsigned int array_count;
    static char function_name[] = "decode_group";

    if (item->type != PIT_GROUP)
    {
	(void) fprintf (stderr, "Item is not of type PIT_GROUP\n");
	a_prog_bug (function_name);
    }
    group = (KControlPanel) item->value_ptr;
    if (!group->group)
    {
	(void) fprintf (stderr, "Group KControlPanel is not a group\n");
	a_prog_bug (function_name);
    }
    if (item->max_array_length < 1)
    {
	max_groups = 1;
	array_count = 0;
    }
    else
    {
	max_groups = item->max_array_length;
	array_count = add ? *item->curr_array_length : 0;
    }
    /*  Loop through all the instances of group  */
    while (string != NULL)
    {
	if (array_count >= max_groups)
	{
	    if (item->max_array_length > 0)
	    {
		*item->curr_array_length = array_count;
	    }
	    if (string != NULL)
	    {
		(void) fprintf (stderr,
				"Ignored trailing arguments: \"%s\" for panel item: \"%s\"\n",
				string, item->name);
	    }
	    return;
	}
	/*  Loop through all group items  */
	for (gitem = group->first_item; gitem != NULL; gitem = gitem->next)
	{
	    value_ptr = gitem->value_ptr;
	    value_ptr += array_count * get_size_of_type (gitem->type);
	    if (string == NULL)
	    {
		(void) fprintf (stderr, "Group not completed\n");
		return;
	    }
	    string = decode_datum (gitem, value_ptr, string, &failed);
	    if (failed) return;
	}
	++array_count;
    }
    if (item->max_array_length < 1) return;
    if (array_count < item->min_array_length)
    {
	(void) fprintf (stderr,
			"Not enough values given for array: need: %d\n",
			(int) item->min_array_length);
	return;
    }
    *item->curr_array_length = array_count;
}   /*  End Function decode_group  */
