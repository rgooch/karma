#include <karma_dsxfr.h>
# 1 "" 
void dsxfr_register_connection_limits (max_incoming, max_outgoing)
int max_incoming;
int max_outgoing;
{
}
flag dsxfr_put_multi (object, multi_desc)
char *object;
multi_array *multi_desc;
{
    return ( (flag) 0 );
}
multi_array *dsxfr_get_multi (object, cache, mmap_option, writeable)
char *object;
flag cache;
unsigned int mmap_option;
flag writeable;
{
    return ( (multi_array *) 0 );
}
void dsxfr_register_read_func (read_func)
void (*read_func) ();
{
}
void dsxfr_register_close_func (close_func)
void (*close_func) ();
{
}
