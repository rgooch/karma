#include <karma_xt_chm.h>
# 1 "" 
void xt_chm_register_app_context (context)
XtAppContext context;
{
}
flag xt_chm_manage (channel, info, input_func, close_func, output_func,
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
void xt_chm_unmanage (channel)
Channel channel;
{
}
