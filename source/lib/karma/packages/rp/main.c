/*LINTLIBRARY*/
/*  main.c

    This code provides support for managing a random pool of bytes.

    Copyright (C) 1994-1996  Richard Gooch

    I first saw the idea of a random pool of bytes in PGP.

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

/*  This file contains all routines needed for the management of a random pool
  of bytes. This random pool may be used as a source of cryptographically
  strong random numbers.


    Written by      Richard Gooch   23-AUG-1994

    Updated by      Richard Gooch   24-AUG-1994

    Updated by      Richard Gooch   4-OCT-1994: Changed to  lrand48  routine in
  order to avoid having to link with buggy UCB compatibility library in
  Slowaris 2.3

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   21-NOV-1994: Tidied up  VERIFY_RANDPOOL
  macro.

    Updated by      Richard Gooch   25-NOV-1994: Add entropy to key rather than
  pool.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/rp/main.c

    Updated by      Richard Gooch   9-DEC-1994: Fixed bug in  rp_add_time_noise
  where old time values were not updated.

    Updated by      Richard Gooch   25-JAN-1995: Added #ifdef OS_ConvexOS

    Updated by      Richard Gooch   9-APR-1995: Added #include <sys/types.h>

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Last updated by Richard Gooch   13-APR-1996: Changed to new documentation
  format.


*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <math.h>
#ifdef OS_MSDOS
#  include <time.h>
#else
#  include <sys/time.h>
#endif
#include <karma.h>
#include <karma_rp.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_n.h>
#include <karma_c.h>
#include <os.h>

#ifdef OS_ConvexOS
#  define lrand48 random
#endif

#define RANDPOOL_MAGIC_NUMBER 238954390

#define VERIFY_RANDPOOL(rp) if (rp == NULL) \
{(void) fprintf (stderr, "NULL randpool passed\n"); \
 a_prog_bug (function_name); } \
if (rp->magic_number != RANDPOOL_MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid randpool object\n"); \
 a_prog_bug (function_name); }

/*  Internal definition of RandPool object structure type  */
struct randpool_type
{
    unsigned int magic_number;
    unsigned int size;
    unsigned int add_pos;
    unsigned int get_pos;
    unsigned char *pool;
    unsigned int hash_digest_size;
    unsigned int hash_block_size;
    unsigned char *hash_key;        /*  hash_block_size  bytes   */
    unsigned char *iv;              /*  hash_digest_size  bytes  */
    void (*hash_func) ();
    RandPool prev;
    RandPool next;
    KCallbackList destroy_list;
};


/*  Private data follows  */
static RandPool first_randpool = NULL;


/*  Private functions  */
STATIC_FUNCTION (void stir, (RandPool rp) );
STATIC_FUNCTION (void xor_copy,
		 (unsigned char *dest, CONST unsigned char *source,
		  unsigned int length) );


/*  Public routines follow  */

/*PUBLIC_FUNCTION*/
RandPool rp_create ( unsigned int size, unsigned int hash_digest_size,
		    unsigned int hash_block_size, void (*hash_func) () )
/*  [SUMMARY] Create a pool of random bytes.
    [PURPOSE] This routine will create a random pool of bytes to which random
    data may be subsequently added or extracted. The pool is initialised with
    pseudo-random data (which is *not* cryptographically secure). When data is
    added to the pool, the pool is stirred using a supplied hash function.
    Bytes extracted from the pool should be cryptographically secure.
    <size> The size of the pool.
    <hash_digest_size> The size of the hash buffer.
    <hash_block_size> The size of the hash block.
    <hash_func> The hash function. The prototype function is
    [<RP_PROTO_hash_func>].
    [RETURNS] A RandPool object on success, else NULL.
*/
{
    RandPool rp;
    unsigned int count;
    extern RandPool first_randpool;
    static flag first_time = TRUE;
    static char function_name[] = "rp_create";

    if (first_time)
    {
	/*  Initialise the random number generator: it seeds the RNG with
	    the current time (seconds XOR microseconds).  */
	(void) n_uniform ();
	/*  Register function to be called upon processing of  exit(3)  */
#ifdef HAS_ATEXIT
	atexit (rp_destroy_all);
#endif
#ifdef HAS_ON_EXIT
	on_exit (rp_destroy_all, (caddr_t) NULL);
#endif
	first_time = FALSE;
    }
    if (size * 4 < hash_digest_size + hash_block_size)
    {
	size = 4 * (hash_digest_size + hash_block_size);
    }
    if (size % hash_digest_size != 0)
    {
	(void) fprintf (stderr,
			"Pool size: %u must be an integral multiple of hash_digest_size: %u\n",
			size, hash_digest_size);
	a_prog_bug (function_name);
    }
    if (hash_func == NULL)
    {
	(void) fprintf (stderr, "NULL hash function pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( ( rp = (RandPool) m_alloc (sizeof *rp) ) == NULL )
    {
	m_error_notify (function_name, "random pool");
	return (NULL);
    }
    rp->magic_number = RANDPOOL_MAGIC_NUMBER;
    rp->size = size;
    rp->hash_digest_size = hash_digest_size;
    rp->hash_block_size = hash_block_size;
    rp->pool = NULL;
    rp->hash_key = NULL;
    rp->iv = NULL;
    rp->hash_func = hash_func;
    rp->destroy_list = NULL;
    if ( ( rp->pool = (unsigned char *) m_alloc (size) ) == NULL )
    {
	m_error_notify (function_name, "pool of bytes");
	rp_destroy (rp);
	return (NULL);
    }
    /*  Initialise pool  */
    for (count = 0; count < size; ++count) rp->pool[count] =lrand48 () &0xff;
    if ( ( rp->hash_key = (unsigned char *) m_alloc (hash_block_size) )
	== NULL )
    {
	m_error_notify (function_name, "hash key");
	rp_destroy (rp);
	return (NULL);
    }
    /*  Initialise key  */
    for (count = 0; count < hash_block_size; ++count)
    {
	rp->hash_key[count] = lrand48 () & 0xff;
    }
    if ( ( rp->iv = (unsigned char *) m_alloc (hash_digest_size) ) == NULL )
    {
	m_error_notify (function_name, "hash key");
	rp_destroy (rp);
	return (NULL);
    }
    rp->add_pos = 0;
    rp->get_pos = size;  /*  Force stir on get  */
    /*  Add some data  */
    rp_add_time_noise (rp);
    /*  Place randpool object into list  */
    rp->prev = NULL;
    rp->next = first_randpool;
    if (first_randpool != NULL)
    {
	first_randpool->prev = rp;
    }
    first_randpool = rp;
    return (rp);
}   /*  End Function rp_create  */

/*PUBLIC_FUNCTION*/
void rp_add_bytes (RandPool rp, CONST unsigned char *buf, unsigned int length)
/*  [SUMMARY] Add bytes (entropy) to a random pool.
    [PURPOSE] This routine will add bytes of data (entropy) into a pool. The
    pool is then stirred using it's registered hash function in order to
    distribute the bits.
    <rp> The random pool.
    <buf> The bytes to add to the pool.
    <length> The number of bytes to add to the pool.
    [RETURNS] Nothing.
*/
{
    unsigned int space_in_key;
    static char function_name[] = "rp_add_bytes";

    VERIFY_RANDPOOL (rp);
    if (length < 1) return;
    if (buf == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while ( length > (space_in_key = rp->hash_block_size - rp->add_pos) )
    {
	/*  Fill key to the brim and then some  */
	xor_copy (rp->hash_key + rp->add_pos, buf, space_in_key);
	buf += space_in_key;
	length -= space_in_key;
	stir (rp);
    }
    /*  Add bytes to key.  */
    xor_copy (rp->hash_key + rp->add_pos, buf, length);
    rp->add_pos += length;
}   /*  End Function rp_add_bytes  */

/*PUBLIC_FUNCTION*/
void rp_get_bytes (RandPool rp, unsigned char *buf, unsigned int length)
/*  [SUMMARY] Get bytes of data from a random pool.
    <rp> The random pool.
    <buf> The bytes to get from the pool will be written here. These bytes
    should be random an cryptographically secure.
    <length> The number of bytes to get from the pool.
    [RETURNS] Nothing.
*/
{
    unsigned int bytes_in_pool;
    static char function_name[] = "rp_get_bytes";

    VERIFY_RANDPOOL (rp);
    if (length < 1) return;
    if (buf == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while ( length > (bytes_in_pool = rp->size - rp->get_pos) )
    {
	/*  Get all bytes out of pool and then stir  */
	m_copy ( (char *) buf, (char *) rp->pool + rp->get_pos,
		bytes_in_pool );
	buf += bytes_in_pool;
	length -= bytes_in_pool;
	stir (rp);
    }
    /*  Get bytes from pool  */
    m_copy ( (char *) buf, (char *) rp->pool + rp->get_pos, length );
    rp->get_pos += length;
}   /*  End Function rp_get_bytes  */

/*PUBLIC_FUNCTION*/
void rp_destroy (RandPool rp)
/*  [SUMMARY] Destroy a random pool of bytes, erasing all information.
    <rp> The random pool.
    [RETURNS] Nothing.
*/
{
    extern RandPool first_randpool;
    static char function_name[] = "rp_destroy";

    VERIFY_RANDPOOL (rp);
    if (rp->pool != NULL)
    {
	m_clear ( (char *) rp->pool, rp->size );
	m_free ( (char *) rp->pool );
    }
    if (rp->hash_key != NULL)
    {
	m_clear ( (char *) rp->hash_key, rp->hash_block_size );
	m_free ( (char *) rp->hash_key );
    }
    if (rp->iv != NULL)
    {
	m_clear ( (char *) rp->iv, rp->hash_digest_size );
	m_free ( (char *) rp->iv );
    }
    /*  Call any destroy functions  */
    c_call_callbacks (rp->destroy_list, NULL);
    c_destroy_list (rp->destroy_list);
    rp->destroy_list = NULL;
    /*  Remove randpool object from list  */
    if (rp->next != NULL)
    {
	/*  Another entry further in the list  */
	rp->next->prev = rp->prev;
    }
    if (rp->prev != NULL)
    {
	/*  Another entry previous in the list  */
	rp->prev->next = rp->next;
    }
    if (rp == first_randpool)
    {
	/*  Randpool is first in list: make next entry the first  */
	first_randpool = rp->next;
    }
    /*  Kill magic number entry and everything else for safety  */
    m_clear ( (char *) rp, sizeof *rp );
    /*  Deallocate randpool object now that there are no more references  */
    m_free ( (char *) rp );
}   /*  End Function rp_destroy  */

/*PUBLIC_FUNCTION*/
void rp_destroy_all ()
/*  [SUMMARY] Destroy all randpools.
    [PURPOSE] This routine will destroy all randpools. The routine is meant to
    be called from the <<exit(3)>> function. It should also be called by the
    application prior to <<execve(2)>>.
    [RETURNS] Nothing.
*/
{
    extern RandPool first_randpool;

    while (first_randpool != NULL) rp_destroy (first_randpool);
}   /*  End Function rp_destroy_all  */

/*PUBLIC_FUNCTION*/
void rp_add_time_noise (RandPool rp)
/*  [SUMMARY] Add time-based entropy to a randpool.
    [PURPOSE] This routine will add bytes of data (entropy) into a pool,
    derived from the system time. It is suggested that this routine be called
    by various callback routines to assist in the addition of entropy.
    <rp> The random pool.
    [RETURNS] Nothing.
*/
{
#ifdef OS_MSDOS
    time_t ntime;
    static time_t old_ntime = 0;
#else
    struct timeval tv;
    struct timezone tz;
    static struct timeval old_tv = {0, 0};
#endif
    static char function_name[] = "rp_add_time_noise";

    VERIFY_RANDPOOL (rp);
#ifdef OS_MSDOS
    time (&ntime);
    if (ntime == old_ntime) return;
    rp_add_bytes (rp, (unsigned char *) &ntime, sizeof ntime);
    old_ntime = ntime;
#else
    (void) gettimeofday (&tv, &tz);
    if ( (tv.tv_sec == old_tv.tv_sec) && (tv.tv_usec == old_tv.tv_usec) )
    {
	return;
    }
    old_tv.tv_usec = tv.tv_usec;
    old_tv.tv_sec = tv.tv_sec;
    rp_add_bytes (rp, (unsigned char *) &tv, sizeof tv);
#endif
}   /*  End Function rp_add_time_noise  */

/*PUBLIC_FUNCTION*/
void rp_register_destroy_func (RandPool rp, void (*destroy_func) (),void *info)
/*  [SUMMARY] Register randpool destroy callback.
    [PURPOSE] This routine will register a routine which should be called when
    a random pool is destroyed.
    <rp> The random pool.
    <destroy_func> The function which is called when the random pool is
    destroyed. The prototype function is [<RP_PROTO_destroy_func>].
    Multiple destroy functions may be registered, with the first one registered
    being the first one called upon destroy.
    <info> A pointer to the arbitrary information passed to the destroy
    function. This may be NULL.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "rp_register_destroy_func";

    VERIFY_RANDPOOL (rp);
    (void) c_register_callback (&rp->destroy_list,
				( flag (*) () ) destroy_func,
				rp, info, FALSE, NULL, FALSE,
				FALSE);
}   /*  End Function rp_register_destroy_func  */


/*  Private functions follow  */

static void stir (RandPool rp)
/*  This routine will stir a random pool of bytes.
    The random pool must be given by  rp  .
    The routine returns nothing.
*/
{
    unsigned int pool_count, hash_count;
    static char function_name[] = "stir";

    VERIFY_RANDPOOL (rp);
    /*  Copy end of pool into IV  */
    m_copy ( (char *) rp->iv,
	    (char *) rp->pool + rp->size - rp->hash_digest_size,
	    rp->hash_digest_size );
    for (pool_count = 0; pool_count < rp->size;
	 pool_count += rp->hash_digest_size)
    {
	(* rp->hash_func ) (rp->iv, rp->hash_key);
	for (hash_count = 0; hash_count < rp->hash_digest_size; ++hash_count)
	{
	    rp->pool[pool_count + hash_count] ^= rp->iv[hash_count];
	    rp->iv[hash_count] = rp->pool[pool_count + hash_count];
	}
    }
    /*  Copy over new hash key  */
    m_copy ( (char *) rp->hash_key, (char *) rp->pool,
	    rp->hash_block_size );
    for (pool_count = 0; pool_count < rp->size;
	 pool_count += rp->hash_digest_size)
    {
	(* rp->hash_func ) (rp->iv, rp->hash_key);
	for (hash_count = 0; hash_count < rp->hash_digest_size; ++hash_count)
	{
	    rp->pool[pool_count + hash_count] ^= rp->iv[hash_count];
	    rp->iv[hash_count] = rp->pool[pool_count + hash_count];
	}
    }
    /*  Copy over new hash key  */
    m_copy ( (char *) rp->hash_key, (char *) rp->pool,
	    rp->hash_block_size );
    m_clear ( (char *) rp->iv, rp->hash_digest_size );
    rp->add_pos = 0;
    rp->get_pos = rp->hash_block_size;
}   /*  End Function stir  */

static void xor_copy (unsigned char *dest, CONST unsigned char *source,
		      unsigned int length)
/*  This routine will perform an eXlusive OR operation on two blocks of data
    (the source and destination) and will write the result into the destination
    block.
    The destination block must be pointed to by  dest  .
    The source block must be pointed to by  source  .
    The length of the blocks (in bytes) must be given by  length  .
    The routine returns nothing.
*/
{
    while (length-- > 0) *dest++ ^= *source++;
}   /*  End Function xor_copy  */
