#include <karma_cm.h>
# 1 "" 
flag cm_manage (pid, stop_func, term_func, exit_func)
int pid;
void (*stop_func) ();
void (*term_func) ();
void (*exit_func) ();
{
    return ( (flag) 0 );
}
void cm_unmanage (pid)
int pid;
{
}
void cm_poll (block)
flag block;
{
}
void cm_poll_silent (block)
flag block;
{
}
