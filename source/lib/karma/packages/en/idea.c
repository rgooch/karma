/*LINTLIBRARY*/
/*  idea.c

    This code provides support for the IDEA cipher.

    Copyright (C) 1994,1995  Richard Gooch

    Based on code obtained from Colin Plumb  (colin@nyx10.cs.du.edu)

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


    The IDEA(tm) block cipher is covered by a patent held by ETH and a
    Swiss company called Ascom-Tech AG.  The Swiss patent number is
    PCT/CH91/00117.  International patents are pending. IDEA(tm) is a
    trademark of Ascom-Tech AG.  There is no license fee required for
    noncommercial use.  Commercial users may obtain licensing details
    from Dieter Profos, Ascom Tech AG, Solothurn Lab, Postfach 151, 4502
    Solothurn, Switzerland, Tel +41 65 242885, Fax +41 65 235761.

    The IDEA block cipher uses a 64-bit block size, and a 128-bit key 
    size.  It breaks the 64-bit cipher block into four 16-bit words
    because all of the primitive inner operations are done with 16-bit 
    arithmetic.  It likewise breaks the 128-bit cipher key into eight 
    16-bit words.

    For further information on the IDEA cipher, see these papers:
    1) Xuejia Lai, "Detailed Description and a Software Implementation of 
       the IPES Cipher", Institute for Signal and Information
       Processing, ETH-Zentrum, Zurich, Switzerland, 1991
    2) Xuejia Lai, James L. Massey, Sean Murphy, "Markov Ciphers and 
       Differential Cryptanalysis", Advances in Cryptology- EUROCRYPT'91
*/

/*  This file contains all routines needed for encryption of data using the
  IDEA cipher.


    Written by      Richard Gooch   9-APR-1994

    Updated by      Richard Gooch   13-APR-1994

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   26-AUG-1994: Moved  typedef  of
  idea_status  class to header.

    Updated by      Richard Gooch   4-SEP-1994: Rewrote CFB code to do IDEA
  encryption once per IDEA block size: speeds up encryption 8 times.

    Updated by      Richard Gooch   21-NOV-1994: Tidied up  VERIFY_STATUS
  macro.

    Updated by      Richard Gooch   25-NOV-1994: Stripped support for channel
  encryption and renamed to  en_  package.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/en/idea.c

    Updated by      Richard Gooch   27-FEB-1995: Changed to MACHINE_BIG_ENDIAN
  macro.

    Last updated by Richard Gooch   5-MAY-1995: Placate SGI compiler.


*/
#include <stdio.h>
#include <ctype.h>
#include <karma.h>
#include <karma_en.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_p.h>
#include <os.h>


/*  Try to speed things up: probably should tie this into the machine
    architecture at a later stage.
*/
#define IDEA32


#if __STDC__ == 1
#  define MAGIC_NUMBER 743934209U
#else
#  define MAGIC_NUMBER (unsigned int) 743934209
#endif

#define VERIFY_STATUS(st) if (st == NULL) \
{(void) fprintf (stderr, "NULL IDEA status passed\n"); \
 a_prog_bug (function_name); } \
if ( (*st).magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid IDEA status object\n"); \
 a_prog_bug (function_name); }

#ifdef MACHINE_BIG_ENDIAN
#  define HIGHFIRST
#endif

/*  Declarations of private functions follow  */


/*  Code ripped off from  usuals.h  from PGP  */
/*****************************************************************************/
typedef unsigned char boolean;	/* values are TRUE or FALSE */
typedef unsigned char byte;	/* values are 0-255 */
typedef byte *byteptr;	/* pointer to byte */
typedef char *string;	/* pointer to ASCII character string */
typedef unsigned short word16;	/* values are 0-65535 */
#ifdef __alpha
typedef unsigned int word32;	/* values are 0-4294967295 */
#else
typedef unsigned long word32;	/* values are 0-4294967295 */
#endif

#ifndef min	/* if min macro not already defined */
#define min(a,b) (((a)<(b)) ? (a) : (b) )
#define max(a,b) (((a)>(b)) ? (a) : (b) )
#endif	/* if min macro not already defined */
/*****************************************************************************/

/*  Code ripped off from  idea.h  from PGP  */
/*****************************************************************************/
#define IDEAKEYSIZE 16
#define IDEABLOCKSIZE 8
/*****************************************************************************/

/*  Code ripped off from  idea.c  from PGP  */
/*****************************************************************************/
#define ROUNDS	8		/* Don't change this value, should be 8 */
#define KEYLEN	(6*ROUNDS+4)	/* length of key schedule */

typedef word16 IDEAkey[KEYLEN];

#ifdef IDEA32	/* Use >16-bit temporaries */
#define low16(x) ((x) & 0xFFFF)
typedef unsigned int uint16;	/* at LEAST 16 bits, maybe more */
#else
#define low16(x) (x)	/* this is only ever applied to uint16's */
typedef word16 uint16;
#endif

static void encrypt_key_idea (/* word16 *userkey, word16 *Z */);
#ifdef dummy
static void decrypt_key_idea (/* IDEAkey Z, IDEAkey DK */);
#endif
static void cipher_idea (/* word16 in[4], word16 out[4], CONST IDEAkey Z */);

/*
 *	Multiplication, modulo (2**16)+1
 * Note that this code is structured like this on the assumption that
 * untaken branches are cheaper than taken branches, and the compiler
 * doesn't schedule branches.
 */
#ifdef SMALL_CACHE
CONST static uint16 mul(register uint16 a, register uint16 b)
{
    register word32 p;

    p = (word32)a * b;
    if (p)
    {
	b = low16(p);
	a = p>>16;
	return (b - a) + (b < a);
    }
    else
    {
	return 1-a-b;
    }
} /* mul */
#endif /* SMALL_CACHE */

#ifdef dummy
/*
 *	Compute multiplicative inverse of x, modulo (2**16)+1,
 *	using Euclid's GCD algorithm.  It is unrolled twice to
 *	avoid swapping the meaning of the registers each iteration,
 *	and some subtracts of t have been changed to adds.
 */
CONST static uint16 inv(uint16 x)     
{
    uint16 t0, t1;
    uint16 q, y;

    if (x <= 1)
    return x;			/* 0 and 1 are self-inverse */
    t1 = 0x10001L / x;		/* Since x >= 2, this fits into 16 bits */
    y = 0x10001L % x;
    if (y == 1)
    return low16(1-t1);
    t0 = 1;
    do
    {	q = x / y;
	x = x % y;
	t0 += q * t1;
	if (x == 1)
	return t0;
	q = y / x;
	y = y % x;
	t1 += q * t0;
    } while (y != 1);
    return low16(1-t1);
} /* inv */
#endif

/*	Compute IDEA encryption subkeys Z */
static void encrypt_key_idea(word16 *userkey, word16 *Z)
{
    int i,j;

    /*
     * shifts
     */
    for (j=0; j<8; j++)
    Z[j] = *userkey++;

    for (i=0; j<KEYLEN; j++)
    {	i++;
	Z[i+7] = Z[i & 7] << 9 | Z[(i+1) & 7] >> 7;
	Z += i & 8;
	i &= 7;
    }
} /* encrypt_key_idea */

#ifdef dummy
/*	Compute IDEA decryption subkeys DK from encryption subkeys Z */
/* Note: these buffers *may* overlap! */
static void decrypt_key_idea(IDEAkey Z, IDEAkey DK)
{
    int j;
    uint16 t1, t2, t3;
    IDEAkey T;
    word16 *p = T + KEYLEN;

    t1 = inv(*Z++);
    t2 = -*Z++;
    t3 = -*Z++;
    *--p = inv(*Z++);
    *--p = t3;
    *--p = t2;
    *--p = t1;

    for (j = 1; j < ROUNDS; j++)
    {
	t1 = *Z++;
	*--p = *Z++;
	*--p = t1;

	t1 = inv(*Z++);
	t2 = -*Z++;
	t3 = -*Z++;
	*--p = inv(*Z++);
	*--p = t2;
	*--p = t3;
	*--p = t1;
    }
    t1 = *Z++;
    *--p = *Z++;
    *--p = t1;

    t1 = inv(*Z++);
    t2 = -*Z++;
    t3 = -*Z++;
    *--p = inv(*Z++);
    *--p = t3;
    *--p = t2;
    *--p = t1;
    /* Copy and destroy temp copy */
    for (j = 0, p = T; j < KEYLEN; j++)
    {
	*DK++ = *p;
	*p++ = 0;
    }
} /* decrypt_key_idea */
#endif

/*
 * MUL(x,y) computes x = x*y, modulo 0x10001.  Requires two temps, 
 * t16 and t32.  x must me a side-effect-free lvalue.  y may be 
 * anything, but unlike x, must be strictly 16 bits even if low16() 
 * is #defined.
 * All of these are equivalent - see which is faster on your machine
 */
#ifdef SMALL_CACHE
#define MUL(x,y) (x = mul(low16(x),y))
#else
#ifdef AVOID_JUMPS
#define MUL(x,y) (x = low16(x-1), t16 = low16((y)-1), \
                t32 = (word32)x*t16 + x + t16, x = low16(t32), \
                t16 = t32>>16, x = (x-t16) + (x<t16) + 1)
#else /* !AVOID_JUMPS (default) */
#define MUL(x,y) \
        (x = ((t32 = (word32)(x = low16(x))*(t16 = low16(y))) != 0) ? \
	 x = low16(t32), \
	 t16 = (uint16)(t32>>16), \
	 (x-t16)+(x<t16) \
	 : \
	 (uint16)(1-x-t16))
#endif
#endif

/*	IDEA encryption/decryption algorithm */
/* Note that in and out can be the same buffer */ 
static void cipher_idea(word16 in[4], word16 out[4], register CONST IDEAkey Z)
{
    register uint16 x1, x2, x3, x4, s2, s3;
#ifndef SMALL_CACHE
    register uint16 t16;
    register word32 t32;
#endif

    int r = ROUNDS;

    x1 = *in++;  x2 = *in++;
    x3 = *in++;  x4 = *in;
    do
    {
	MUL(x1,*Z++);
	x2 += *Z++;
	x3 += *Z++;
	MUL(x4, *Z++);

	s3 = x3;
	x3 ^= x1;
	MUL(x3, *Z++);
	s2 = x2;
	x2 ^= x4;
	x2 += x3;
	MUL(x2, *Z++);
	x3 += x2;

	x1 ^= x2;
	x4 ^= x3;

	x2 ^= s3;
	x3 ^= s2;
    } while (--r);
    MUL(x1, *Z++);
    *out++ = x1;
    *out++ = x3 + *Z++;
    *out++ = x2 + *Z++;
    MUL(x4, *Z);
    *out = x4;
} /* cipher_idea */

/*	Run a 64-bit block thru IDEA in ECB (Electronic Code Book) mode,
	using the supplied key schedule.
*/
static void idea_ecb(word16 *inbuf, word16 *outbuf, IDEAkey Z)
{
	/* Assume each pair of bytes comprising a word is ordered MSB-first. */
#ifndef HIGHFIRST	/* If this is a least-significant-byte-first CPU */
	word16 x;

	/* Invert the byte order for each 16-bit word for internal use. */
	x = inbuf[0]; outbuf[0] = x >> 8 | x << 8;
	x = inbuf[1]; outbuf[1] = x >> 8 | x << 8;
	x = inbuf[2]; outbuf[2] = x >> 8 | x << 8;
	x = inbuf[3]; outbuf[3] = x >> 8 | x << 8;
	cipher_idea(outbuf, outbuf, Z);
	x = outbuf[0]; outbuf[0] = x >> 8 | x << 8;
	x = outbuf[1]; outbuf[1] = x >> 8 | x << 8;
	x = outbuf[2]; outbuf[2] = x >> 8 | x << 8;
	x = outbuf[3]; outbuf[3] = x >> 8 | x << 8;
#else	/* HIGHFIRST */
	/* Byte order for internal and external representations is the same. */
	cipher_idea(inbuf, outbuf, Z);
#endif	/* HIGHFIRST */
} /* idea_ecb */
/*****************************************************************************/


/*  Internal definition of IDEA status object structure type  */
struct idea_cipher_status_type  /*  Must come after the IDEA code  */
{
    unsigned int magic_number;
    flag decrypt;
    unsigned int pos;
    IDEAkey Z;
    char cfb_buffer[EN_IDEA_BLOCK_SIZE];  /*  0: First In  n-1: First out  */
};


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
idea_status en_idea_init (char key[EN_IDEA_KEY_SIZE], flag decrypt,
			  char init_vector[EN_IDEA_BLOCK_SIZE], flag clear)
/*  This routine will initialise the IDEA cipher in Cipher Feed Back mode for
    a stream of data. The stream is uni-directional.
    The 16 byte IDEA key must be pointed to by  key  .
    If the stream is to be decrypted, the value of  decrypt  must be TRUE.
    The 8 byte initialisation vector must be pointed to by  init_vector  .
    If the value of  clear  is TRUE the key and initialisation vector will be
    cleared after use (highly recommended if they will not be needed again).
    The routine returns an IDEA cipher status which may be sebsequently used by
    en_idea_cfb  on success, else it returns NULL.
*/
{
    int count;
    unsigned long data;
    word16 tmp[EN_IDEA_KEY_SIZE / 2];
    idea_status status;
    static char function_name[] = "en_idea_init";

    FLAG_VERIFY (decrypt);
    FLAG_VERIFY (clear);
    if ( (key == NULL) || (init_vector == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( ( status = (idea_status) m_alloc (sizeof *status) ) == NULL )
    {
	m_error_notify (function_name, "IDEA status object");
	return (NULL);
    }
    (*status).magic_number = MAGIC_NUMBER;
    (*status).decrypt = decrypt;
    (*status).pos = 0;
    for (count = 0; count < EN_IDEA_KEY_SIZE / 2; ++count)
    {
	if (p_read_buf16 (key + count * 2, &data) != TRUE)
	{
	    a_prog_bug (function_name);
	}
	tmp[count] = data;
    }
    encrypt_key_idea (tmp, (*status).Z);
    for (count = 0; count < EN_IDEA_KEY_SIZE / 2; ++count) tmp[count] = 0;
    m_copy ( (*status).cfb_buffer, init_vector, EN_IDEA_BLOCK_SIZE );
    if (clear)
    {
	m_clear (key, EN_IDEA_KEY_SIZE);
	m_clear (init_vector, EN_IDEA_BLOCK_SIZE);
    }
    return (status);
}   /*  End Function en_idea_init  */

/*PUBLIC_FUNCTION*/
void en_idea_cfb (idea_status status, char *buffer, unsigned int length)
/*  This routine will encrypt (or decrypt) a sequence of bytes using the IDEA
    cipher in Cipher Feed Back mode.
    The IDEA cipher status must be given by  status  .
    The data to encrypt must be pointed to by  buffer  .
    The number of bytes to encrypt (or decrypt) must be given by  length  .
    The routine returns nothing.
*/
{
    char new_ch;
    static char function_name[] = "en_idea_cfb";

    VERIFY_STATUS (status);
    if (buffer == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (length < 1) return;
    /*  Encrypt/ decrypt one byte at a time using CFB  */
    for (; length > 0; --length, ++buffer, ++(*status).pos)
    {
	if ( (*status).pos >= EN_IDEA_BLOCK_SIZE )
	{
	    /*  Encrypt CFB buffer  */
	    idea_ecb ( (word16 *) (*status).cfb_buffer, 
		      (word16 *) (*status).cfb_buffer, (*status).Z );
	    (*status).pos = 0;
	}
	if ( (*status).decrypt )
	{
	    new_ch = *buffer ^ (*status).cfb_buffer[(*status).pos];
	    (*status).cfb_buffer[(*status).pos] = *buffer;
	    *buffer = new_ch;
	}
	else
	{
	    new_ch = *buffer ^ (*status).cfb_buffer[(*status).pos];
	    *buffer = new_ch;
	    (*status).cfb_buffer[(*status).pos] = new_ch;
	}
    }
}   /*  End Function en_idea_cfb  */

/*PUBLIC_FUNCTION*/
void en_idea_close (idea_status status)
/*  This routine will clean up an IDEA cipher status, removing sensitive key
    information.
    The IDEA cipher status must be given by  status  .
    The routine returns nothing.
*/
{
    static char function_name[] = "en_idea_close";

    VERIFY_STATUS (status);
    m_clear ( (char *) status, sizeof *status );
    m_free ( (char *) status );
}   /*  End Function en_idea_close  */
