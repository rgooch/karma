#include <karma_panel.h>
# 1 "" 
KControlPanel panel_create (blank)
flag blank;
{
    return ( (KControlPanel) 0 );
}
void panel_add_item (panel, name, comment, type, value_ptr, va_alist)
KControlPanel panel;
char *name;
char *comment;
unsigned int type;
void *value_ptr;
va_dcl
{
}
void panel_push_onto_stack (panel)
KControlPanel panel;
{
}
void panel_pop_from_stack ()
{
}
flag panel_process_command_with_stack (cmd, unknown_func, fp)
char *cmd;
flag (*unknown_func) ();
FILE *fp;
{
    return ( (flag) 0 );
}
