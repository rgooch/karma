#include <karma_chm.h>
# 1 "" 
flag chm_manage (channel, info, input_func, close_func, output_func,
		 exception_func)
Channel channel;
void *info;
flag (*input_func) ();
void (*close_func) ();
flag (*output_func) ();
flag (*exception_func) ();
{
    return ( (flag) 0 );
}
void chm_unmanage (channel)
Channel channel;
{
}
void chm_poll (timeout_ms)
long timeout_ms;
{
}
