#include <karma_dsproc.h>
# 1 "" 
void dsproc_object (object, array_names, num_arrays, save_unproc_data,
		    pre_process, process_array, post_process, mmap_option)
char *object;
char *array_names[];
unsigned int num_arrays;
flag save_unproc_data;
flag (*pre_process) ();
flag (*process_array) ();
flag (*post_process) ();
unsigned int mmap_option;
{
}
