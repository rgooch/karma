#include <karma_dir.h>
# 1 "" 
KDir dir_open (dirname)
char *dirname;
{
    return ( (KDir) 0 );
}
KFileInfo *dir_read (dir, skip_control)
KDir dir;
unsigned int skip_control;
{
    return ( (KFileInfo *) 0 );
}
void dir_close (dir)
KDir dir;
{
}
