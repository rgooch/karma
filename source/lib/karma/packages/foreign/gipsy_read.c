/*LINTLIBRARY*/
/*  gipsy_read.c

    This code provides a GIPSY read facility.

    Copyright (C) 1996  Richard Gooch

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

    This file contains the various utility routines for reading data files in
  Miriad Image format.


    Written by      Richard Gooch   21-JUN-1996

    Updated by      Richard Gooch   23-JUN-1996

    Last updated by Richard Gooch   24-JUN-1996: Only display debugging
  information when "FOREIGN_GIPSY_DEBUG" environment variable exists. Fixed a
  few bugs.


*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_dsrw.h>
#include <karma_pio.h>
#include <karma_dmp.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_r.h>
#include <os.h>


#define FC_INF  0x01
#define FC_SWAP 0x02

#define FITS_CARD_WIDTH 80
#define FITS_CARD_LENGTH 36
#define FITS_EQUALS_POSITION 8
#define KEY_LEN 21
#define MAXDIM  20
/*  Stuff taken from GIPSY gdsdescr.h  */
#define REC_SIZ 200
#ifndef FLT_MAX
#  define FLT_MAX 3.40282347e+38F
#endif

#ifdef Kword32u
typedef struct
{
    Kword32u key_ind;
    Kword32u length;
    Kword32u readpos;
    Kword32u level;
    Kword32u next_key;
    Kword32u next_ext;
    Kword32u last_ext;
    Kword32u curr_ext;
    char type;
    char name[KEY_LEN];
} _keyhead, *keyhead;

typedef struct
{
    Kword32u key_ind;
    Kword32u next_ext;
} _exthead, *exthead;

#define SKH     (sizeof(_keyhead))
#define SEH     (sizeof(_exthead))
#define SL      (sizeof(Kword32u))
#define KEY_AL  (SL+(SL*SKH-SKH)%SL)
#define EXT_AL  (SL+(SL*SEH-SEH)%SL)
#define KEY_DL  (REC_SIZ-SKH-KEY_AL )
#define EXT_DL  (REC_SIZ-SEH-EXT_AL )

typedef struct
{
    _keyhead h;
    char alignment[KEY_AL];
    char data[KEY_DL];
} _keyrec, *keyrec;

typedef struct
{
    _exthead h;
    char alignment[EXT_AL];
    char data[EXT_DL];
} _extrec, *extrec;

typedef union
{
    _keyrec key;
    _extrec ext;
} _record, *record;

typedef struct
{
    Kword32u version;
    Kword32u subversion;
    Kword32u one;
    Kword32u osdep;
    double ax_origin[MAXDIM];
    Kword32u ax_size[MAXDIM];
    Kword32u ax_factor[MAXDIM+1];
    Kword32u naxis;
    Kword32u nitems;
    Kword32u reserved2;
    Kword32u reserved3;
    Kword32u rec_start;
    Kword32u n_alloc;
    Kword32u maxrec;
    Kword32u n_buck;
    Kword32u spare_fint[8];
    Kword32u free;
    Kword32u hash_tab[1];
} _header, *header;

typedef union
{
    _header h;
    _record r[1];
} _dscfile, *dscfile;

#endif

/*  Structure declarations  */
#ifdef Kword32u
struct header_type
{
    Kword32u version;
    Kword32u subversion;
    flag swap;
    Kword32u naxis;
    double ax_origin[MAXDIM];
    Kword32u ax_size[MAXDIM];
    Kword32u ax_factor[MAXDIM + 1];
    Kword32u nitems;
    Kword32u rec_start;
    Kword32u n_alloc;
    Kword32u maxrec;
    Kword32u n_buck;
    Kword32u free;
    Kword32u hash_tab[1];
};
#endif


/*  Private data follows  */
#ifdef MACHINE_BIG_ENDIAN
static unsigned char inf_bytes[4] = {0x7f, 0x80, 0, 0};
#endif
#ifdef MACHINE_LITTLE_ENDIAN
static unsigned char inf_bytes[4] = {0, 0, 0x80, 0x7f};
#endif
static flag verbose = FALSE;


/*  Declarations of private functions follow  */
STATIC_FUNCTION (void copy_32bits,
		 (char *dest, CONST char *source, flag swap) );
STATIC_FUNCTION (flag read_and_swap_blocks,
		 (Channel channel, char *buffer, unsigned int num_blocks,
		  unsigned int block_size, flag swap) );
STATIC_FUNCTION (flag drain_to_boundary, (Channel channel,unsigned int size) );
STATIC_FUNCTION (flag read_key_record,
		 (Channel channel, keyrec record, flag swap) );
STATIC_FUNCTION (flag fill_fits_line, (Channel channel) );


/*  Public functions follow  */


/*PUBLIC_FUNCTION*/
flag foreign_gipsy_test (CONST char *filename)
/*  [SUMMARY] Test if a file is part of a GIPSY file set.
    <filename> The name of any file in the GIPSY file set.
    [RETURNS] TRUE if the file is part of a GIPSY file set, else FALSE.
*/
{
    struct stat statbuf;
    char *ptr;
    char fname[STRING_LENGTH];
    char header_name[STRING_LENGTH];
    char image_name[STRING_LENGTH];
    extern char *sys_errlist[];

    (void) strcpy (fname, filename);
    if ( ( ptr = strrchr (fname, '.') ) == NULL ) return (FALSE);
    *ptr = '\0';
    if ( (strcmp (ptr + 1, "descr") != 0) &&
	 (strcmp (ptr + 1, "image") != 0) &&
	 (strcmp (ptr + 1, "gipsy") != 0) ) return (FALSE);
    (void) sprintf (header_name, "%s.descr", fname);
    (void) sprintf (image_name, "%s.image", fname);
    /*  Check for the header file  */
    if (stat (header_name, &statbuf) != 0)
    {
	if (errno == ENOENT) return (FALSE);
	(void) fprintf (stderr, "Error statting file: \"%s\"\t%s\n",
			header_name, sys_errlist[errno]);
	return (FALSE);
    }
    /*  Well, the file exists. Check for the image file  */
    if (stat (image_name, &statbuf) != 0)
    {
	if (errno == ENOENT) return (FALSE);
	(void) fprintf (stderr, "Error statting file: \"%s\"\t%s\n",
			image_name, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function foreign_gipsy_test  */

/*PUBLIC_FUNCTION*/
multi_array *foreign_gipsy_read_header (Channel channel, flag data_alloc,
					flag sanitise, ...)
/*  [SUMMARY] Read a GIPSY file header.
    [PURPOSE] This routine will read the header of a GIPSY file from a channel.
    The data section is NOT read.
    <channel> The channel to read from.
    <data_alloc> If TRUE, the data space is allocated.
    <sanitise> If TRUE, GIPSY axes with length 1 are ignored. This is highly
    recommended.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_GIPSY_READ_HEADER_END. See [<FOREIGN_ATT_GIPSY_READ_HEADER>] for a
    list of defined attributes.
    [RETURNS] A pointer to the multi_array data structure on success, else
    NULL.
*/
{
    va_list argp;
    Channel fits_ch;
    unsigned int att_key;
    unsigned int float_control = 0;
    unsigned int count, fits_size, item_count, fits_lines, type;
    double value[2];
    multi_array *multi_desc;
    packet_desc *pack_desc;
    char *packet;
    char *ptr, *fits_type, *fits_data;
    char buffer[16];
    char txt[STRING_LENGTH];
    char item_name[STRING_LENGTH];
#ifdef Kword32u
    Kword32u osdep;
    Kword32u rec_type;
    struct header_type header;
    Kword32u *uint32_ptr;
    _keyrec key_record;
#endif
    extern flag verbose;
    extern char *sys_errlist[];
    static flag first_time = TRUE;
    static char function_name[] = "foreign_gipsy_read_header";

    va_start (argp, sanitise);
    if (first_time)
    {
	if (r_getenv ("FOREIGN_GIPSY_DEBUG") != NULL) verbose = TRUE;
	first_time = FALSE;
    }
    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (data_alloc);
#if !defined(MACHINE_BIG_ENDIAN) && !defined(MACHINE_LITTLE_ENDIAN)
    a_func_abort (function_name, "machine not big endian or little endian");
    return (NULL);
#endif
#if !defined(HAS_IEEE) || !defined(Kword32u)
    a_func_abort (function_name, "machine is not IEEE and has no 32 bit ints");
    return (NULL);
#endif
#ifdef Kword32u
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) )
	    != FA_GIPSY_READ_HEADER_END )
    {
	switch (att_key)
	{
	  default:
	    (void) fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    /*  Read the first 16 bytes containing the version, subversion, number one
	and osdep values  */
    if (ch_read (channel, buffer, 16) < 16)
    {
	(void) fprintf (stderr, "Error reading 16 bytes\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Work out the byte ordering  */
    uint32_ptr = (Kword32u *) (buffer + 8);
    if (*uint32_ptr == 1) header.swap = FALSE;
    else if (*uint32_ptr == 0x01000000) header.swap = TRUE;
    else
    {
	(void) fprintf (stderr, "%s: unknown byte ordering %u\n",
			function_name, *uint32_ptr);
	return (NULL);
    }
    /*  Get version and subversion and osdep  */
    copy_32bits ( (char *) &header.version, buffer, header.swap );
    copy_32bits ( (char *) &header.subversion, buffer + 4, header.swap );
    copy_32bits ( (char *) &osdep, buffer + 12, header.swap );
    if (header.version != 2)
    {
	(void) fprintf (stderr, "%s: GIPSY version: %u not supported\n",
			function_name, header.version);
	return (NULL);
    }
    if (header.subversion != 2)
    {
	(void) fprintf (stderr, "%s: GIPSY subversion: %u not supported\n",
			function_name, header.subversion);
	return (NULL);
    }
    switch (osdep)
    {
      case 0:
	float_control |= FC_INF;
#  ifdef MACHINE_LITTLE_ENDIAN
	float_control |= FC_SWAP;
#  endif
	break;
      case 1:
	float_control |= FC_INF;
#  ifdef MACHINE_BIG_ENDIAN
	float_control |= FC_SWAP;
#  endif
	break;
      case 5:
#  ifdef MACHINE_LITTLE_ENDIAN
	float_control |= FC_SWAP;
#  endif
	break;
      case 6:
#  ifdef MACHINE_BIG_ENDIAN
	float_control |= FC_SWAP;
#  endif
	break;
      default:
	fprintf (stderr, "Float format: %u not supported\n", osdep);
	return (NULL);
/*
	break;
*/
    }
    if (verbose) fprintf (stderr, "Float type: %u\n", osdep);
    /*  Read axis information  */
    if ( !read_and_swap_blocks (channel, (char *) header.ax_origin,
				MAXDIM, 8, header.swap ) )
    {
	(void) fprintf (stderr, "Error reading axes origins\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    if ( !read_and_swap_blocks (channel, (char *) header.ax_size,
				MAXDIM, 4, header.swap) )
    {
	(void) fprintf (stderr, "Error reading axes sizes\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    if ( !read_and_swap_blocks (channel, (char *) header.ax_factor,
				MAXDIM + 1, 4, header.swap) )
    {
	(void) fprintf (stderr, "Error reading axes factors\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Read number of axes  */
    if ( !read_and_swap_blocks (channel, (char *) &header.naxis, 1, 4,
				header.swap) )
    {
	(void) fprintf (stderr, "Error reading number of axes\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    if (verbose)
    {
	(void) fprintf (stderr, "number of axes: %u\n", header.naxis );
	for (count = 0; count < header.naxis; ++count)
	{
	    (void) fprintf (stderr, "Axis[%u] length: %u\n",
			    count, header.ax_size[count]);
	}
    }
    /*  Read number of items  */
    if ( !read_and_swap_blocks (channel, (char *) &header.nitems, 1, 4,
				header.swap) )
    {
	(void) fprintf (stderr, "Error reading number of items\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    if (verbose) fprintf (stderr, "number of items: %u\n", header.nitems );
    /*  Read past reserved bits  */
    if (ch_drain (channel, 8) < 8)
    {
	(void) fprintf (stderr, "Error reading 8 reserved bytes\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Read index to first data record  */
    if ( !read_and_swap_blocks (channel, (char *) &header.rec_start, 1, 4,
				header.swap) )
    {
	(void) fprintf (stderr, "Error reading rec_start\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    if (verbose) fprintf (stderr, "first data record: %u\n", header.rec_start);
    /*  Read number of records allocated  */
    if ( !read_and_swap_blocks (channel, (char *) &header.n_alloc, 1, 4,
				header.swap) )
    {
	(void) fprintf (stderr, "Error reading n_alloc\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    if (verbose) fprintf (stderr, "number of records allocated: %u\n",
			  header.n_alloc);
    /*  Read current maximum number of records in file  */
    if ( !read_and_swap_blocks (channel, (char *) &header.maxrec, 1, 4,
				header.swap) )
    {
	(void) fprintf (stderr, "Error reading maxrec\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Read size of hash table  */
    if ( !read_and_swap_blocks (channel, (char *) &header.n_buck, 1, 4,
				header.swap) )
    {
	(void) fprintf (stderr, "Error reading n_buck\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Read past spares  */
    if (ch_drain (channel, 8 * 4) < 8 * 4)
    {
	(void) fprintf (stderr, "Error reading 8 reserved fints\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Read free list pointer  */
    if ( !read_and_swap_blocks (channel, (char *) &header.free, 1, 4,
				header.swap) )
    {
	(void) fprintf (stderr, "Error reading free\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Read hash table  */
    if ( !read_and_swap_blocks (channel, (char *) header.hash_tab, 1, 4,
				header.swap) )
    {
	(void) fprintf (stderr, "Error reading hash_tab\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    drain_to_boundary (channel, REC_SIZ);
    /*  Skip to first data record. The header is counted as one record even
	though it takes more than one regular record size  */
    for (count = 2; count < header.rec_start; ++count)
    {
	if (ch_drain (channel, REC_SIZ) < REC_SIZ)
	{
	    (void) fprintf (stderr, "Error draining record\t%s\n",
			    sys_errlist[errno]);
	    return (NULL);
	}
    }
    /*  Create memory channel for FITS header  */
    fits_lines = header.nitems + 7;  /*  Count "extras" as well  */
    if (fits_lines % FITS_CARD_LENGTH != 0)
    {
	fits_lines = (fits_lines / FITS_CARD_LENGTH + 1) * FITS_CARD_LENGTH;
    }
    fits_size = fits_lines * FITS_CARD_WIDTH;
    if ( ( fits_ch = ch_open_memory (NULL, fits_size) ) == NULL )
    {
	(void) fprintf (stderr, "Error opening FITS header channel\n");
	return (NULL);
    }
    if ( !ch_puts (fits_ch, "SIMPLE  = T", FALSE) ||
	 !fill_fits_line (fits_ch) )
    {
	(void) fprintf (stderr, "Error writing to FITS channel\t%s\n",
			sys_errlist[errno]);
	ch_close (fits_ch);
	return (NULL);
    }
    if ( !ch_puts (fits_ch, "BITPIX  = -32", FALSE) ||
	 !fill_fits_line (fits_ch) )
    {
	(void) fprintf (stderr, "Error writing to FITS channel\t%s\n",
			sys_errlist[errno]);
	ch_close (fits_ch);
	return (NULL);
    }
    if ( !ch_puts (fits_ch, "BZERO   = 0.0", FALSE) ||
	 !fill_fits_line (fits_ch) )
    {
	(void) fprintf (stderr, "Error writing to FITS channel\t%s\n",
			sys_errlist[errno]);
	ch_close (fits_ch);
	return (NULL);
    }
    if ( !ch_puts (fits_ch, "BSCALE  = 1.0", FALSE) ||
	 !fill_fits_line (fits_ch) )
    {
	(void) fprintf (stderr, "Error writing to FITS channel\t%s\n",
			sys_errlist[errno]);
	ch_close (fits_ch);
	return (NULL);
    }
    if ( !ch_printf (fits_ch, "KGIPSY  = %u", float_control) ||
	 !fill_fits_line (fits_ch) )
    {
	(void) fprintf (stderr, "Error writing to FITS channel\t%s\n",
			sys_errlist[errno]);
	ch_close (fits_ch);
	return (NULL);
    }
    /*  Have to put "NAXIS" at the top of the FITS header because stupid GIPSY
	might define a "?????n" keyword before the "NAXIS" keyword. Later on
	have to trap the "NAXIS" keyword when generating the FITS header.  */
    if ( !ch_printf (fits_ch, "NAXIS   = %u", header.naxis) ||
	 !fill_fits_line (fits_ch) )
    {
	(void) fprintf (stderr, "Error writing to FITS channel\t%s\n",
			sys_errlist[errno]);
	ch_close (fits_ch);
	return (NULL);
    }
    /*  Start processing descriptors and building a fake FITS header  */
    for ( item_count = 0; count < header.n_alloc;
	  ++count, drain_to_boundary (channel, REC_SIZ) )
    {
	if (verbose) fprintf (stderr, "Reading record: %u...\n", count);
	if ( !read_and_swap_blocks (channel, (char *) &rec_type, 1, 4,
				    header.swap) )
	{
	    (void) fprintf (stderr, "Error reading record type\t%s\n",
			    sys_errlist[errno]);
	    ch_close (fits_ch);
	    return (NULL);
	}
	if (rec_type != 1)
	{
	    if (verbose) fprintf (stderr,
				  "%s: record type: %u not supported\n",
				  function_name, rec_type);
	    continue;
	}
	if ( !read_key_record (channel, &key_record, header.swap) )
	{
	    return (NULL);
	}
	(void) strncpy (item_name, key_record.h.name, KEY_LEN);
	item_name[KEY_LEN] = '\0';
	if (item_count >= header.nitems)
	{
	    (void) fprintf (stderr, "Excess item: \"%s\" ignored\n",item_name);
	    continue;
	}
	if (key_record.h.level != 0)
	{
	    if (verbose)fprintf(stderr,
				"Item: \"%s\" level: %u co-ordinate ignored\n",
				item_name, key_record.h.level);
	    continue;
	}
	if (strncmp (key_record.data, "FITS ", 5) != 0)
	{
	    if (verbose)
	    {
		fprintf(stderr,
			"Item: \"%s\" not recognised\tnext_key: %u  next_ext: %u\n",
			item_name, key_record.h.next_key,
			key_record.h.next_ext);
	    }
	    continue;
	}
	strncpy (txt, key_record.data, key_record.h.length);
	txt[key_record.h.length] = '\0';
	ptr = txt + 4;
	/*  Seach for next non-whitespace  */
	while ( isspace (*ptr) ) ++ptr;
	fits_type = ptr;
	/*  Search for next whitespace  */
	while ( !isspace (*ptr) ) ++ptr;
	/*  Seach for next non-whitespace  */
	while ( isspace (*ptr) ) ++ptr;
	fits_data = ptr;
	/*  Correct floating point data with 'D' as exponent delimiter  */
	if ( (strncmp (fits_type, "DBLE", 4) == 0) ||
	     (strncmp (fits_type, "REAL", 4) == 0) )
	{
	    if ( ( ptr = strchr (fits_data, 'D') ) != NULL ) *ptr = 'E';
	}
	if (strcmp (item_name, "NAXIS") == 0)
	{
	    if (atoi (fits_data) != header.naxis)
	    {
		fprintf(stderr,
			"%s: NAXIS value: %u does not match number of axes: %u\n",
			function_name, atoi (fits_data), header.naxis);
		ch_close (fits_ch);
		return (NULL);
	    }
	    /*  Checks OK: skip writing, else it will be there twice  */
	    continue;
	}
	/*  Write to FITS channel  */
	if ( !ch_printf (fits_ch, "%-8s= %s", item_name, fits_data) ||
	     !fill_fits_line (fits_ch) )
	{
	    (void) fprintf (stderr, "Error writing to FITS channel\t%s\n",
			    sys_errlist[errno]);
	    ch_close (fits_ch);
	    return (NULL);
	}
	if (verbose) fprintf (stderr, "%-8s= %s\n", item_name, fits_data);
    }
    if ( !ch_puts (fits_ch, "END", FALSE) || !fill_fits_line (fits_ch) )
    {
	(void) fprintf (stderr, "Error writing to FITS channel\t%s\n",
			sys_errlist[errno]);
	ch_close (fits_ch);
	return (NULL);
    }
    /*  Use FITS reader to construct Karma data structure  */
    if ( ( multi_desc =
	   foreign_fits_read_header (fits_ch, data_alloc, TRUE, sanitise,
				     FA_FITS_READ_HEADER_END) ) == NULL )
    {
	(void) fprintf (stderr, "%s: error processing generated FITS header\n",
			function_name);
	ch_close (fits_ch);
	return (NULL);
    }
    ch_close (fits_ch);
    /*  Check if "NAXISn" values agree with header  */
    pack_desc = multi_desc->headers[0];
    packet = multi_desc->data[0];
    for (count = 0; count < header.naxis; ++count)
    {
	(void) sprintf (txt, "NAXIS%u", count + 1);
	if ( !ds_get_unique_named_value (pack_desc, packet, txt, &type,value) )
	{
	    (void) fprintf (stderr, "%s keyword not found!\n", txt);
	    a_prog_bug (function_name);
	}
	if (header.ax_size[count] != (unsigned int) value[0])
	{
	    fprintf (stderr,
		     "%s: %s value: %u not equal to axis length: %u\n",
		     function_name,
		     txt, (unsigned int) value[0], header.ax_size[count]);
	    ds_dealloc_multi (multi_desc);
	    return (NULL);
	}
    }
    return (multi_desc);
#endif  /*  Kword32u  */
}   /*  End Function foreign_gipsy_read_header  */

/*PUBLIC_FUNCTION*/
flag foreign_gipsy_read_data (Channel channel, multi_array *multi_desc,
			      char *data, uaddr num_values, ...)
/*  [SUMMARY] Read data in a GIPSY image file.
    [PURPOSE] This routine will read the data of a GIPSY image file from a
    channel.
    <channel> The channel to read from.
    <multi_desc> The Karma data structure to write the data into.
    <data> An alternate data array to write the FITS data into. If this is
    NULL, the routine will write the data into the Karma data structure.
    <num_values> The number of values to write into the data array. This is
    only used when data is not NULL.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_GIPSY_READ_DATA_END. See [<FOREIGN_ATT_GIPSY_READ_DATA>] for a list of
    defined attributes.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    unsigned int att_key;
    unsigned int value_count;
    unsigned int float_control = 0;  /*  Initialised to keep compiler happy  */
    unsigned int type;
    unsigned long toobig_count;
    float f_toobig = TOOBIG;
    float g_blank;
    double value[2];
    char *packet;
    unsigned long *blank_count = NULL;
    packet_desc *pack_desc;
    array_desc *arr_desc;
    float *f_ptr;
    extern char *sys_errlist[];
    static char function_name[] = "foreign_gipsy_read_data";

    va_start (argp, num_values);
    if ( (channel == NULL) || (multi_desc == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp,unsigned int) ) !=
	    FA_GIPSY_READ_DATA_END )
    {
	switch (att_key)
	{
	  case FA_GIPSY_READ_DATA_NUM_BLANKS:
	    blank_count = va_arg (argp, unsigned long *);
	    break;
	  default:
	    (void) fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    pack_desc = multi_desc->headers[0];
    packet = multi_desc->data[0];
    if (pack_desc->element_types[0] != K_ARRAY)
    {
	(void) fprintf (stderr,
			"First element in top level packet must be K_ARRAY\n");
	a_prog_bug (function_name);
    }
    arr_desc = (array_desc *) pack_desc->element_desc[0];
    if (data == NULL)
    {
	data = *(char **) packet;
	if (data == NULL)
	{
	    (void) fprintf (stderr, "No array to write data into!\n");
	    a_prog_bug (function_name);
	}
	num_values = ds_get_array_size (arr_desc);
    }
    if ( ds_get_unique_named_value (pack_desc, packet, "KGIPSY", &type,value) )
    {
	float_control = value[0];
    }
    else
    {
	(void) fprintf (stderr, "KGIPSY unknown!\n");
	a_prog_bug (function_name);
    }
    toobig_count = 0;
    /*  Copy data, possibly swapping bytes  */
    if ( !read_and_swap_blocks (channel, data, num_values, 4,
				float_control & FC_SWAP) )
    {
	(void) fprintf (stderr, "Error reading GIPSY file\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Process and convert blanks  */
    g_blank = (float_control & FC_INF) ? -*(float *) inf_bytes : -FLT_MAX;
    for (value_count = 0, f_ptr = (float *) data; value_count < num_values;
	 ++value_count, ++f_ptr)
    {
	if (*f_ptr == g_blank)
	{
	    *f_ptr = f_toobig;
	    ++toobig_count;
	}
    }
    if (blank_count != NULL) *blank_count = toobig_count;
    return (TRUE);
}   /*  End Function foreign_gipsy_read_data  */

/*PUBLIC_FUNCTION*/
multi_array *foreign_gipsy_read (CONST char *filename, flag sanitise, ...)
/*  [SUMMARY] Read a GIPSY file set.
    <filename> The name of any file in the GIPSY file set.
    <sanitise> If TRUE, Miriad axes with length 1 are ignored. This is highly
    recommended.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. This list must be terminated with
    FA_GIPSY_READ_END. See [<FOREIGN_ATT_GIPSY_READ>] for a list of defined
    attributes.
    [RETURNS] A multi_array descriptor on success, else NULL.
*/
{
    va_list argp;
    Channel channel;
    unsigned int att_key;
    unsigned long blank_count_local;
    unsigned long *blank_count = NULL;
    multi_array *multi_desc;
    char *ptr;
    char fname[STRING_LENGTH];
    char header_name[STRING_LENGTH];
    char image_name[STRING_LENGTH];
    extern char *sys_errlist[];
    static char function_name[] = "foreign_gipsy_read";

    va_start (argp, sanitise);
    /*  Process attributes  */
    while ( ( att_key = va_arg (argp, unsigned int) ) != FA_GIPSY_READ_END )
    {
	switch (att_key)
	{
	  case FA_GIPSY_READ_NUM_BLANKS:
	    blank_count = va_arg (argp, unsigned long *);
	    break;
	  default:
	    (void) fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    if (blank_count == NULL) blank_count = &blank_count_local;
    (void) strcpy (fname, filename);
    if ( ( ptr = strrchr (fname, '.') ) == NULL ) return (NULL);
    *ptr = '\0';
    if ( (strcmp (ptr + 1, "descr") != 0) &&
	 (strcmp (ptr + 1, "image") != 0) &&
	 (strcmp (ptr + 1, "gipsy") != 0) )
    {
	return (NULL);
    }
    (void) sprintf (header_name, "%s.descr", fname);
    (void) sprintf (image_name, "%s.image", fname);
    if ( ( channel = ch_open_file (header_name, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening: \"%s\"\t%s\n",
			header_name, sys_errlist[errno]);
	return (NULL);
    }
    multi_desc = foreign_gipsy_read_header (channel, TRUE, sanitise,
					    FA_GIPSY_READ_HEADER_END);
    (void) ch_close (channel);
    if (multi_desc == NULL) return (NULL);
    if ( ( channel = ch_open_file (image_name, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening: \"%s\"\t%s\n",
			header_name, sys_errlist[errno]);
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    if ( !foreign_gipsy_read_data (channel, multi_desc, NULL, 0,
				   FA_GIPSY_READ_DATA_NUM_BLANKS, blank_count,
				   FA_GIPSY_READ_DATA_END) )
    {
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    return (multi_desc);
}   /*  End Function foreign_gipsy_read  */


/*  Private functions follow  */

static void copy_32bits (char *dest, CONST char *source, flag swap)
/*  [SUMMARY] Copy 32 bits of data with optional byte swapping.
    <dest> The destination buffer to write to.
    <source> The source buffer to read from.
    <swap> If TRUE the bytes are swapped, else they are not swapped.
    [RETURNS] Nothing.
*/
{
    if (swap)
    {
	dest[0] = source[3];
	dest[1] = source[2];
	dest[2] = source[1];
	dest[3] = source[0];
    }
    else
    {
	dest[0] = source[0];
	dest[1] = source[1];
	dest[2] = source[2];
	dest[3] = source[3];
    }
}   /*  End Function copy_32bits  */

static flag read_and_swap_blocks (Channel channel, char *buffer,
				  unsigned int num_blocks,
				  unsigned int block_size, flag swap)
/*  [SUMMARY] This routine will read blocks from a channel.
    <channel> The Channel object.
    <buffer> The buffer to write into.
    <num_blocks> The number of blocks to read.
    <block_size> The number of bytes in a block to read.
    <swap> If TRUE, the bytes will be swapped.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    if (swap)
    {
	if (ch_read_and_swap_blocks (channel, buffer, num_blocks, block_size) <
	    num_blocks * block_size)
	{
	    return (FALSE);
	}
	return (TRUE);
    }
    if (ch_read (channel, buffer, num_blocks * block_size) <
	num_blocks * block_size) return (FALSE);
    return (TRUE);
}   /*  End Function read_and_swap_blocks  */

static flag drain_to_boundary (Channel channel, unsigned int size)
/*  [SUMMARY] Drain a channel up to the nearest boundary.
    <channel> The Channel object to read from.
    <size> The boundary size.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned long read_pos, write_pos;
    extern char *sys_errlist[];

    if ( !ch_tell (channel, &read_pos, &write_pos) )
    {
	(void) fprintf (stderr, "Error getting position\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if (read_pos % size == 0) return (TRUE);
    size = size - read_pos % size;
    if (ch_drain (channel, size) < size)
    {
	(void) fprintf (stderr, "Error draining\t%s\n", sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function drain_to_boundary  */

static flag read_key_record (Channel channel, keyrec record, flag swap)
/*  [SUMMARY] Read in a key record.
    <channel> The Channel object to read from.
    <record> The record data will be written here.
    <swap> If TRUE, data will be byte-swapped.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    extern char *sys_errlist[];
    static char function_name[] = "forien_gipsy_read__read_key_record";

    if ( !read_and_swap_blocks (channel, (char *) &record->h.length, 7, 4,
				swap) )
    {
	(void) fprintf (stderr, "%s: error reading key record header\t%s\n",
			function_name, sys_errlist[errno]);
	return (FALSE);
    }
    if (ch_read (channel, &record->h.type, 1) < 1)
    {
	(void) fprintf (stderr,
			"%s: error reading key record header type\t%s\n",
			function_name, sys_errlist[errno]);
	return (FALSE);
    }
    if (ch_read (channel, record->h.name, KEY_LEN) < KEY_LEN)
    {
	(void) fprintf (stderr,
			"%s: error reading key record header name\t%s\n",
			function_name, sys_errlist[errno]);
	return (FALSE);
    }
    if ( !drain_to_boundary (channel, 4) )
    {
	(void) fprintf (stderr, "%s: error draining\t%s\n",
			function_name, sys_errlist[errno]);
	return (FALSE);
    }
    if (ch_read (channel, record->alignment, KEY_AL) < KEY_AL)
    {
	(void) fprintf (stderr,
			"%s: error reading key record alignment\t%s\n",
			function_name, sys_errlist[errno]);
	return (FALSE);
    }
    if (ch_read (channel, record->data, KEY_DL) < KEY_DL)
    {
	(void) fprintf (stderr,
			"%s: error reading key record data\t%s\n",
			function_name, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function read_key_record  */

static flag fill_fits_line (Channel channel)
/*  [SUMMARY] Fill FITS line.
    <channel> The Channel to write to.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int size;
    unsigned long read_pos, write_pos;

    if ( !ch_tell (channel, &read_pos, &write_pos) )
    {
	(void) fprintf (stderr, "Error getting position\n");
	return (FALSE);
    }
    size = write_pos % FITS_CARD_WIDTH;
    if (size == 0) return (TRUE);
    while (size < FITS_CARD_WIDTH)
    {
	if (ch_write (channel, " ", 1) < 1)
	{
	    (void) fprintf (stderr, "Error filling FITS line\n");
	    return (FALSE);
	}
	++size;
    }
    return (TRUE);
}   /*  End Function fill_fits_line  */
