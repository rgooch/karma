#include <karma_pio.h>
# 1 "" 
flag pio_write64 (channel, data)
Channel channel;
unsigned long data;
{
    return ( (flag) 0 );
}
flag pio_read64 (channel, data)
Channel channel;
unsigned long *data;
{
    return ( (flag) 0 );
}
flag pio_write32 (channel, data)
Channel channel;
unsigned long data;
{
    return ( (flag) 0 );
}
flag pio_read32 (channel, data)
Channel channel;
unsigned long *data;
{
    return ( (flag) 0 );
}
flag pio_write16 (channel, data)
Channel channel;
unsigned long data;
{
    return ( (flag) 0 );
}
flag pio_read16 (channel, data)
Channel channel;
unsigned long *data;
{
    return ( (flag) 0 );
}
flag pio_write_float (channel, data)
Channel channel;
float data;
{
    return ( (flag) 0 );
}
flag pio_read_float (channel, data)
Channel channel;
float *data;
{
    return ( (flag) 0 );
}
flag pio_write_double (channel, data)
Channel channel;
double data;
{
    return ( (flag) 0 );
}
flag pio_read_double (channel, data)
Channel channel;
double *data;
{
    return ( (flag) 0 );
}
flag pio_write32s (channel, data)
Channel channel;
long data;
{
    return ( (flag) 0 );
}
flag pio_read32s (channel, data)
Channel channel;
long *data;
{
    return ( (flag) 0 );
}
flag pio_write16s (channel, data)
Channel channel;
long data;
{
    return ( (flag) 0 );
}
flag pio_read16s (channel, data)
Channel channel;
long *data;
{
    return ( (flag) 0 );
}
flag pio_write_swap (channel, data, length)
Channel channel;
char *data;
unsigned int length;
{
    return ( (flag) 0 );
}
flag pio_read_swap (channel, data, length)
Channel channel;
char *data;
unsigned int length;
{
    return ( (flag) 0 );
}
flag pio_write_string (channel, string)
Channel channel;
char *string;
{
    return ( (flag) 0 );
}
char *pio_read_string (channel, length)
Channel channel;
unsigned int *length;
{
    return ( (char *) 0 );
}
