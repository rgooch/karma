#include <karma_module.h>
# 1 "" 
void module_run (argc, argv, name_string, version_string, decode_func,
		 max_incoming, max_outgoing, server)
int argc;
char **argv;
char *name_string;
char *version_string;
flag (*decode_func) ();
int max_incoming;
int max_outgoing;
flag server;
{
}
void module_process_argvs (argc, argv, unknown_func)
int argc;
char **argv;
flag (*unknown_func) ();
{
}
