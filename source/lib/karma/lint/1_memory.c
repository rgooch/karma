#include <karma_m.h>
# 1 "" 
void m_clear (memory, length)
char *memory;
unsigned int length;
{
}
void m_copy (dest, source, length)
char *dest;
char *source;
unsigned int length;
{
}
void m_copy_blocks (dest, source, dest_stride, source_stride, block_size,
		    num_blocks)
char *dest;
char *source;
unsigned int dest_stride;
unsigned int source_stride;
unsigned int block_size;
unsigned int num_blocks;
{
}
void m_fill (dest, stride, source, size, num)
char *dest;
unsigned int stride;
char *source;
unsigned int size;
unsigned int num;
{
}
flag m_cmp (block1, block2, length)
char *block1;
char *block2;
unsigned int length;
{
    return ( (flag) 0 );
}
