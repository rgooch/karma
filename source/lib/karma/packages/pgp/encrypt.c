/*LINTLIBRARY*/
/*  encrypt.c

    This code provides PGP (Pretty Good Privacy) support.

    Copyright (C) 1994-1996  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*

    This file contains the various utility routines for supporting PGP.


    Written by      Richard Gooch   2-DEC-1994

    Updated by      Richard Gooch   3-DEC-1994

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Last updated by Richard Gooch   13-APR-1996: Changed to new documentation
  format.


*/

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <karma.h>
#include <karma_pgp.h>
#include <karma_ch.h>
#include <karma_rp.h>
#include <karma_cm.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>
#include <os.h>


#define MIN_BUF_SIZE 4096


/*  Declarations of private functions follow  */
#ifdef CAN_FORK
STATIC_FUNCTION (int spawn_job,
		 (char *path, char *argv[],
		  Channel *in_ch, Channel *out_ch, Channel *err_ch) );
#endif


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
char *pgp_encrypt (CONST char *plaintext, unsigned int plaintext_length,
		   CONST char **recipients, unsigned int num_recipients,
		   unsigned int *ciphertext_length, flag ascii_armour)
/*  [SUMMARY] Encrypt a block of data using PGP.
    <plaintext> The input plaintext data.
    <plaintext_length> The length of the input plaintext data.
    <recipients> The list of recipients to encrypt for.
    <num_recipients> The number of recipients in the recipient list.
    <ciphertext_length> The length of the encrypted data is written here.
    <ascii_armour> If TRUE, the ciphertext is ASCII armoured, suitable for
    transmitting through Email.
    [RETURNS] A pointer to the ciphertext data on success, else NULL.
*/
{
    Channel in_ch = NULL;
    Channel out_ch = NULL;
    Channel err_ch;
    uaddr buf_size = MIN_BUF_SIZE;
    uaddr buf_pos;
    int child_pid;
    char ch;
    char *buffer, *new_buf;
    char **argv;
    extern char *sys_errlist[];
    static char function_name[] = "pgp_encrypt";

    if ( ( buffer = m_alloc (buf_size) ) == NULL )
    {
	m_error_notify (function_name, "ciphertext buffer");
	return (NULL);
    }
    if ( ( argv = (char **) m_alloc ( sizeof *argv * (num_recipients + 4) ) )
	== NULL )
    {
	m_error_notify (function_name, "argv buffer");
	return (NULL);
    }
    argv[0] = "pgp";
    argv[1] = ascii_armour ? "-feat" : "-fe";
    argv[2] = "+batchmode";
    m_copy ( (char *) (argv + 3), (char *) recipients,
	    sizeof *argv * num_recipients );
    argv[num_recipients + 3] = NULL;
    if ( ( err_ch = ch_open_file ("/dev/null", "w") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening /dev/null\t%s\n",
			sys_errlist[errno]);
	m_free ( (char *) argv );
	return (NULL);
    }
    if ( ( child_pid = spawn_job ("pgp",argv, &in_ch, &out_ch, &err_ch) ) < 0 )
    {
	m_free ( (char *) argv );
	return (NULL);
    }
    m_free ( (char *) argv );
    (void) cm_manage (child_pid, ( void (*) () ) NULL, ( void (*) () ) NULL,
		      ( void (*) () ) NULL);
    if (ch_write (in_ch, plaintext, plaintext_length) != plaintext_length)
    {
	(void) fprintf (stderr, "Error writing plaintext\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (in_ch);
	(void) ch_close (out_ch);
	(void) kill (child_pid, SIGKILL);
	cm_poll (TRUE);
	return (NULL);
    }
    if ( !ch_close (in_ch) )
    {
	(void) fprintf (stderr, "Error flushing plaintext\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (out_ch);
	(void) kill (child_pid, SIGKILL);
	cm_poll (TRUE);
	return (NULL);
    }
    /*  Read in encrypted data  */
    buf_pos = 0;
    while (ch_read (out_ch, &ch, 1) == 1)
    {
	if (buf_pos >= buf_size)
	{
	    if ( ( new_buf = m_alloc (buf_size * 2) ) == NULL )
	    {
		m_error_notify (function_name, "ciphertext buffer");
		m_clear (buffer, buf_size);
		(void) ch_close (out_ch);
		(void) kill (child_pid, SIGKILL);
		cm_poll (TRUE);
		return (NULL);
	    }
	    m_copy (new_buf, buffer, buf_size);
	    m_clear (buffer, buf_size);
	    m_free (buffer);
	    buffer = new_buf;
	    buf_size *= 2;
	}
	buffer[buf_pos++] = ch;
    }
    (void) ch_close (out_ch);
    cm_poll (TRUE);
    *ciphertext_length = buf_pos;
    return (buffer);
}   /*  End Function pgp_encrypt  */


/*  Private functions follow  */

#ifdef CAN_FORK
static int spawn_job (char *path, char *argv[],
		      Channel *in_ch, Channel *out_ch, Channel *err_ch)
/*  This routine will fork(2) and execvp(2) a process.
    The file to execute must be pointed to by  path  .
    The NULL terminated list of arguments which will be passed to  main  must
    be pointed to by  argv  .
    The input channel (fd = 0) for the process must be pointed to by
    in_ch  .If the value here is NULL, then a pipe to the process is
    opened and the writeable end is written to the storage pointed to by  in_ch
    The standard output channel (fd = 1) for the process must be
    pointed to by  out_ch  .If the value here is NULL, then a pipe to
    the process is opened and the readable end is written to the storage
    pointed to by  out_ch  .
    The standard error output channel (fd = 2) for the process must be
    pointed to by  err_ch  .If the value here is NULL, then a pipe to
    the process is opened and the readable end is written to the storage
    pointed to by  err_ch  .
    The routine returns the child process ID on success, else it returns -1.
*/
{
    int child_pid;
    Channel in_read_ch, in_write_ch;
    Channel out_read_ch, out_write_ch;
    Channel err_read_ch, err_write_ch;
    extern char *sys_errlist[];

    if (*in_ch == NULL)
    {
	/*  Open a pipe  */
	if ( !ch_create_pipe (&in_read_ch, &in_write_ch) )
	{
	    (void) fprintf (stderr, "Could not open input pipe\t%s\n",
			    sys_errlist[errno]);
	    return (-1);
	}
    }
    if (*out_ch == NULL)
    {
	/*  Open a pipe  */
	if ( !ch_create_pipe (&out_read_ch, &out_write_ch) )
	{
	    (void) fprintf (stderr, "Could not open output pipe\t%s\n",
			    sys_errlist[errno]);
	    return (-1);
	}
    }
    if (*err_ch == NULL)
    {
	/*  Open a pipe  */
	if ( !ch_create_pipe (&err_read_ch, &err_write_ch) )
	{
	    (void) fprintf (stderr, "Could not open error output pipe\t%s\n",
			    sys_errlist[errno]);
	    return (-1);
	}
    }
    /*  Fork and exec  */
    switch ( child_pid = fork () )
    {
      case 0:
	/*  Child: exec  */
	rp_destroy_all ();
	if (*in_ch == NULL)
	{
	    (void) ch_close (in_write_ch);
	    dup2 (ch_get_descriptor (in_read_ch), 0);
	}
	else
	{
	    dup2 (ch_get_descriptor (*in_ch), 0);
	}
	if (*out_ch == NULL)
	{
	    (void) ch_close (out_read_ch);
	    dup2 (ch_get_descriptor (out_write_ch), 1);
	}
	else
	{
	    dup2 (ch_get_descriptor (*out_ch), 1);
	}
	if (*err_ch == NULL)
	{
	    (void) ch_close (err_read_ch);
	    dup2 (ch_get_descriptor (err_write_ch), 2);
	}
	else
	{
	    dup2 (ch_get_descriptor (*err_ch), 2);
	}
	(void) execvp (path, argv);
	(void) fprintf (stderr, "Could not exec: \"%s\"\t%s\n",
			path, sys_errlist[errno]);
	exit (1);
	break;
      case -1:
	/*  Error  */
	(void) fprintf (stderr, "Could not fork\t%s\n", sys_errlist[errno]);
	return (-1);
/*
	break;
*/
      default:
	/*  Parent  */
	break;
    }
    /*  Parent only  */
    if (*in_ch == NULL)
    {
	(void) ch_close (in_read_ch);
	*in_ch = in_write_ch;
    }
    if (*out_ch == NULL)
    {
	(void) ch_close (out_write_ch);
	*out_ch = out_read_ch;
    }
    if (*err_ch == NULL)
    {
	(void) ch_close (err_write_ch);
	*err_ch = err_read_ch;
    }
    return (child_pid);
}   /*  End Function spawn_job  */
#endif  /*  CAN_FORK  */
