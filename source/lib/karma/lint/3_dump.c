#include <karma_dmp.h>
# 1 "" 
void dmp_multi_desc (fp, multi_desc, comments)
FILE *fp;
multi_array *multi_desc;
flag comments;
{
}
void dmp_packet_desc (fp, pack_desc, comments)
FILE *fp;
packet_desc *pack_desc;
flag comments;
{
}
void dmp_element_desc (fp, type, desc, comments)
FILE *fp;
unsigned int type;
char *desc;
flag comments;
{
}
void dmp_array_desc (fp, arr_desc, comments)
FILE *fp;
array_desc *arr_desc;
flag comments;
{
}
void dmp_dim_desc (fp, dimension, comments)
FILE *fp;
dim_desc *dimension;
flag comments;
{
}
void dmp_multi_data (fp, multi_desc, comments)
FILE *fp;
multi_array *multi_desc;
flag comments;
{
}
void dmp_packet (fp, pack_desc, packet, comments)
FILE *fp;
packet_desc *pack_desc;
char *packet;
flag comments;
{
}
void dmp_element (fp, type, desc, element, comments)
FILE *fp;
unsigned int type;
char *desc;
char *element;
flag comments;
{
}
void dmp_array (fp, arr_desc, array, comments)
FILE *fp;
array_desc *arr_desc;
char *array;
flag comments;
{
}
void dmp_list (fp, pack_desc, list_head, comments)
FILE *fp;
packet_desc *pack_desc;
list_header *list_head;
flag comments;
{
}
void dmp_flag (fp, logical, comment_string, comments)
FILE *fp;
flag logical;
char comment_string[];
flag comments;
{
}
