/*LINTLIBRARY*/
/*  decrypt.c

    This code provides PGP (Pretty Good Privacy) support.

    Copyright (C) 1994,1995  Richard Gooch

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

    Last updated by Richard Gooch   23-JAN-1995: Fixed test when opening
  channels.


*/

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <karma.h>
#include <karma_pgp.h>
#include <karma_ch.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>


#define MIN_BUF_SIZE 4096


/*  Declarations of private functions follow  */
STATIC_FUNCTION (flag connect_to_pgpdaemon,
		 (char *pgppath, Channel *to_ch, Channel *from_ch) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
char *pgp_decrypt (CONST char *ciphertext, unsigned int ciphertext_length,
		   unsigned int *plaintext_length)
/*  [PURPOSE] Decrypt a block of data using PGP and PGPdaemon.
    <ciphertext> The input ciphertext data.
    <ciphertext_length> The length of the input ciphertext data.
    <plaintext_length> The length of the decrypted data is written here.
    [RETURNS] A pointer to the plaintext data on success, else NULL.
*/
{
    Channel to_ch, from_ch;
    flag bool_val;
    uaddr buf_size = MIN_BUF_SIZE;
    uaddr buf_pos;
    char ch;
    char *buffer, *new_buf;
    char *pgppath;
    char frompipe_filename[STRING_LENGTH];
    extern char *sys_errlist[];
    static char function_name[] = "pgp_decrypt";

    if ( ( pgppath = r_getenv ("PGPPATH") ) == NULL )
    {
	(void) fprintf (stderr, "No PGPPATH environment variable set\n");
	return (NULL);
    }
    if ( ( buffer = m_alloc (buf_size) ) == NULL )
    {
	m_error_notify (function_name, "ciphertext buffer");
	return (NULL);
    }
    if ( !connect_to_pgpdaemon (pgppath, &to_ch, &from_ch) )
    {
	(void) fprintf (stderr, "Error connecting to PGPdaemon\n");
	m_free (buffer);
	return (NULL);
    }
    if (ch_write (to_ch, "DECRYPT\n", 8) < 8)
    {
	(void) fprintf (stderr, "Error writing request code\t%s\n",
			sys_errlist[errno]);
	m_free (buffer);
	return (NULL);
    }
    if ( !ch_flush (to_ch) )
    {
	(void) fprintf (stderr, "Error flushing buffer\t%s\n",
			sys_errlist[errno]);
	m_free (buffer);
	return (NULL);
    }
    /*  Check if OK so far  */
    if (ch_read (from_ch, (char *) &bool_val, sizeof bool_val)
	< sizeof bool_val)
    {
	(void) fprintf (stderr,
			"Error reading response flag from PGPdaemon\t%s\n",
			sys_errlist[errno]);
	m_free (buffer);
	return (NULL);
    }
    if (!bool_val)
    {
	(void) fprintf (stderr,
			"PGPdaemon refused to decrypt: probably has no passphrase\n");
	m_free (buffer);
	return (NULL);
    }
    if (ch_write (to_ch, ciphertext, ciphertext_length) != ciphertext_length)
    {
	(void) fprintf (stderr, "Error writing ciphertext\t%s\n",
			sys_errlist[errno]);
	m_free (buffer);
	return (NULL);
    }
    if ( !ch_close (to_ch) )
    {
	(void) fprintf (stderr, "Error closing buffer\t%s\n",
			sys_errlist[errno]);
	m_free (buffer);
	return (NULL);
    }
    /*  Read in decrypted data  */
    buf_pos = 0;
    while (ch_read (from_ch, &ch, 1) == 1)
    {
	if (buf_pos >= buf_size)
	{
	    if ( ( new_buf = m_alloc (buf_size * 2) ) == NULL )
	    {
		m_error_notify (function_name, "plaintext buffer");
		m_clear (buffer, buf_size);
		(void) ch_close (from_ch);
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
    (void) ch_close (from_ch);
    *plaintext_length = buf_pos;
    return (buffer);
}   /*  End Function pgp_decrypt  */

static flag connect_to_pgpdaemon (char *pgppath,
				  Channel *to_ch, Channel *from_ch)
/*  This routine will connect to the PGPdaemon.
    The value of the PGPPATH environment variable must be pointed to by
    pgppath  .
    The channel to use when writing to PGPdaemon will be written to the
    storage pointed to by  to_ch  .
    The channel to use when reading from PGPdaemon will be written to the
    storage pointed to by  from_ch  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    int daemon_pid, randval;
    FILE *fp;
    struct stat statbuf;
    char hostname[STRING_LENGTH];
    char pid_filename[STRING_LENGTH];
    char topipe_filename[STRING_LENGTH];
    char frompipe_filename[STRING_LENGTH];
    extern char *sys_errlist[];

    r_gethostname (hostname, STRING_LENGTH);
    /*  Worry about communication files  */
    (void) sprintf (pid_filename, "%s/.pgpd.PID.%s", pgppath, hostname);
    (void) sprintf (topipe_filename, "%s/.pgpd.input.%s", pgppath, hostname);
    (void) sprintf (frompipe_filename, "%s/.pgpd.output.%s", pgppath,hostname);
    if ( ( fp = fopen (pid_filename, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening: \"%s\"\t%s\n",
			pid_filename, sys_errlist[errno]);
	return (FALSE);
    }
    if (fscanf (fp, "%d", &daemon_pid) != 1)
    {
	(void) fprintf (stderr, "Error reading: \"%s\"\t%s\n",
			pid_filename, sys_errlist[errno]);
	return (FALSE);
    }
    (void) fclose (fp);
    if (stat (topipe_filename, &statbuf) != 0)
    {
	(void) fprintf (stderr, "Error stat'ing file: \"%s\"\t%s\n",
			topipe_filename, sys_errlist[errno]);
	return (FALSE);
    }
    if ( !S_ISFIFO (statbuf.st_mode) )
    {
	(void) fprintf (stderr, "File: \"%s\" is not a named pipe\n",
			topipe_filename);
	return (FALSE);
    }
    if (getuid () != statbuf.st_uid)
    {
	(void) fprintf (stderr, "File: \"%s\" is not owned by you\n",
			topipe_filename);
	return (FALSE);
    }
    if (stat (frompipe_filename, &statbuf) != 0)
    {
	(void) fprintf (stderr, "Error stat'ing file: \"%s\"\t%s\n",
			frompipe_filename, sys_errlist[errno]);
	return (FALSE);
    }
    if ( !S_ISFIFO (statbuf.st_mode) )
    {
	(void) fprintf (stderr, "File: \"%s\" is not a named pipe\n",
			frompipe_filename);
	return (FALSE);
    }
    if (getuid () != statbuf.st_uid)
    {
	(void) fprintf (stderr, "File: \"%s\" is not owned by you\n",
			frompipe_filename);
	return (FALSE);
    }
    if ( (statbuf.st_mode & (S_IRWXG | S_IRWXO) ) != 0 )
    {
	(void) fprintf (stderr, "File: \"%s\" is accessible by others\n",
			frompipe_filename);
	return (FALSE);
    }
    /*  Send signal to daemon  */
    if (kill (daemon_pid, SIGIO) == -1)
    {
	(void) fprintf (stderr, "Error signalling daemon\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if ( ( *from_ch = ch_open_file (frompipe_filename, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening named pipe\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if (ch_read (*from_ch, (char *) &randval,sizeof randval) != sizeof randval)
    {
	(void) fprintf (stderr, "Error reading from named pipe\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (*from_ch);
	return (FALSE);
    }
    if ( ( *to_ch = ch_open_file (topipe_filename, "W") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening named pipe\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (*from_ch);
	return (FALSE);
    }
    if (ch_write (*to_ch, (char *) &randval, sizeof randval) != sizeof randval)
    {
	(void) fprintf (stderr, "Error writing to named pipe\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (*from_ch);
	(void) ch_close (*to_ch);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function connect_to_pgpdaemon  */
