/*  karma_foreign.h

    Header for  foreign_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the foreign_ routines in the Karma library.


    Written by      Richard Gooch   15-APR-1995

    Last updated by Richard Gooch   12-OCT-1996

*/

#if !defined(KARMA_IARRAY_DEF_H) || defined(MAKEDEPEND)
#  include <karma_iarray_def.h>
#endif

#if !defined(KARMA_WCS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_wcs_def.h>
#endif

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#if !defined(KARMA_CH_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ch_def.h>
#endif

#ifndef KARMA_FOREIGN_H
#define KARMA_FOREIGN_H

typedef struct miriad_data_context_type * KMiriadDataContext;


#define FA_PPM_WRITE_END               0


#define FA_PPM_READ_END                0


#define FA_FITS_READ_HEADER_END        0

#define FA_FITS_READ_DATA_END          0
#define FA_FITS_READ_DATA_NUM_BLANKS   1


#define FA_FITS_GENERATE_HEADER_END    0


#define FA_FITS_WRITE_END              0

#define FA_FITS_WRITE_DATA_END         0


#define FA_MIRIAD_READ_HEADER_END      0

#define FA_MIRIAD_READ_DATA_END        0
#define FA_MIRIAD_READ_DATA_NUM_BLANKS 1
#define FA_MIRIAD_READ_DATA_NUM_MASKED 2

#define FA_MIRIAD_READ_END             0
#define FA_MIRIAD_READ_NUM_BLANKS      1


#define FA_MIRIAD_WRITE_END            0


#define FA_GIPSY_READ_HEADER_END       0

#define FA_GIPSY_READ_DATA_END         0
#define FA_GIPSY_READ_DATA_NUM_BLANKS  1

#define FA_GIPSY_READ_END              0
#define FA_GIPSY_READ_NUM_BLANKS       1


#define FA_GIPSY_WRITE_END             0


#define FA_GIPSY_WRITE_HEADER_END      0


#define FA_GIPSY_WRITE_DATA_END        0


#define FA_GUESS_READ_END              0
#define FA_GUESS_READ_FITS_TO_FLOAT    1


#define FA_SUNRAS_READ_END             0


#define FA_SUNRAS_WRITE_END            0
#define FA_SUNRAS_WRITE_NO_IMAGE       1


#define FOREIGN_FILE_FORMAT_KARMA   0
#define FOREIGN_FILE_FORMAT_UNKNOWN 1
#define FOREIGN_FILE_FORMAT_PPM     2
#define FOREIGN_FILE_FORMAT_FITS    3
#define FOREIGN_FILE_FORMAT_SUNRAS  4
#define FOREIGN_FILE_FORMAT_MIRIAD  5
#define FOREIGN_FILE_FORMAT_GIPSY   6


/*  File:   ppm_write.c   */
EXTERN_FUNCTION (flag foreign_ppm_write,
		 (Channel channel, multi_array *multi_desc,flag binary, ...) );
EXTERN_FUNCTION (flag foreign_ppm_write_pseudo,
		 (Channel channel, flag binary,
		  CONST char *image, unsigned int type,
		  uaddr *hoffsets, uaddr *voffsets,
		  unsigned int width, unsigned int height,
		  CONST unsigned short *cmap_reds,
		  CONST unsigned short *cmap_greens,
		  CONST unsigned short *cmap_blues,
		  unsigned int cmap_size, unsigned int cmap_stride,
		  double i_min, double i_max) );
EXTERN_FUNCTION (flag foreign_ppm_write_rgb,
		 (Channel channel, flag binary,
		  CONST unsigned char *image_red,
		  CONST unsigned char *image_green,
		  CONST unsigned char *image_blue,
		  uaddr *hoffsets, uaddr *voffsets,
		  unsigned int width, unsigned int height,
		  CONST unsigned short *cmap_red,
		  CONST unsigned short *cmap_green,
		  CONST unsigned short *cmap_blue,
		  unsigned int cmap_stride) );

/*  File:  ppm_read.c   */
EXTERN_FUNCTION (multi_array *foreign_ppm_read, (Channel channel, ...) );

/*  File:  fits_read.c   */
EXTERN_FUNCTION (multi_array *foreign_fits_read_header,
		 (Channel channel, flag data_alloc, flag convert_int_to_float,
		  flag sanitise, ...) );
EXTERN_FUNCTION (flag foreign_fits_read_data,
		 (Channel channel, multi_array *multi_desc, char *data,
		  uaddr num_values, ...) );

/*  File:  fits_write.c  */
EXTERN_FUNCTION (flag foreign_fits_write,
		 (Channel channel, multi_array *multi_desc, ...) );
EXTERN_FUNCTION (flag foreign_fits_write_iarray,
		 (Channel channel, iarray array, ...) );
EXTERN_FUNCTION (flag foreign_fits_write_data,
		 (Channel channel, multi_array *multi_desc,
		  CONST packet_desc *header_pack_desc,
		  CONST char *header_packet,
		  char *data, uaddr num_values, ...) );
EXTERN_FUNCTION (flag foreign_fits_generate_header,
		 (packet_desc **header_pack_desc, char **header_packet,
		  CONST multi_array *multi_desc, ...) );

/*  File:  miriad_read.c  */
EXTERN_FUNCTION (flag foreign_miriad_test, (CONST char *dirname) );
EXTERN_FUNCTION (multi_array *foreign_miriad_read_header,
		 (Channel channel, flag data_alloc, flag sanitise, ...) );
EXTERN_FUNCTION (multi_array *foreign_miriad_read,
		 (CONST char *dirname, flag sanitise, ...) );
EXTERN_FUNCTION (KMiriadDataContext foreign_miriad_create_data_context,
		 (CONST char *dirname) );
EXTERN_FUNCTION (flag foreign_miriad_read_data,
		 (KMiriadDataContext context, multi_array *multi_desc,
		  char *data, uaddr num_values, ...) );
EXTERN_FUNCTION (void foreign_miriad_close_data_context,
		 (KMiriadDataContext context) );
EXTERN_FUNCTION (flag foreign_miriad_read_history,
		 (CONST char *dirname, multi_array *multi_desc) );
EXTERN_FUNCTION (double foreign_miriad_get_units_scale,
		 (CONST char *keyword) );

/*  File:  miriad_write.c  */
EXTERN_FUNCTION (flag foreign_miriad_write,
		 (CONST char *dirname, multi_array *multi_desc, ...) );
EXTERN_FUNCTION (flag foreign_miriad_write_iarray,
		 (CONST char *dirname, iarray array, ...) );

/*  File:  gipsy_read.c  */
EXTERN_FUNCTION (flag foreign_gipsy_test, (CONST char *filename) );
EXTERN_FUNCTION (multi_array *foreign_gipsy_read_header,
		 (Channel channel, flag data_alloc, flag sanitise, ...) );
EXTERN_FUNCTION (flag foreign_gipsy_read_data,
		 (Channel channel, multi_array *multi_desc,
		  char *data, uaddr num_values, ...) );
EXTERN_FUNCTION (multi_array *foreign_gipsy_read,
		 (CONST char *filename, flag sanitise, ...) );

/*  File:  gipsy_write.c  */
EXTERN_FUNCTION (flag foreign_gipsy_write,
		 (CONST char *basename, multi_array *multi_desc, ...) );
EXTERN_FUNCTION (flag foreign_gipsy_write_iarray,
		 (CONST char *basename, iarray array, ...) );
EXTERN_FUNCTION (flag foreign_gipsy_write_header,
		 (Channel channel, CONST packet_desc *header_pack_desc,
		  CONST char *header_packet, ...) );
EXTERN_FUNCTION (flag foreign_gipsy_write_data,
		 (Channel channel, CONST multi_array *multi_desc,
		  CONST packet_desc *header_pack_desc,
		  CONST char *header_packet,
		  char *data, uaddr num_values, ...) );

/*  File:  sunras_read.c   */
EXTERN_FUNCTION (multi_array *foreign_sunras_read, (Channel channel, ...) );

/*  File:  sunras_write.c   */
EXTERN_FUNCTION (flag foreign_sunras_write,
		 (Channel channel, multi_array *multi_desc, ...) );
EXTERN_FUNCTION (flag foreign_sunras_write_pseudo,
		 (Channel channel, CONST char *image, unsigned int type,
		  uaddr *hoffsets, uaddr *voffsets,
		  unsigned int width, unsigned int height,
		  CONST unsigned short *cmap_reds,
		  CONST unsigned short *cmap_greens,
		  CONST unsigned short *cmap_blues,
		  unsigned int cmap_size, unsigned int cmap_stride,
		  double i_min, double i_max) );
EXTERN_FUNCTION (flag foreign_sunras_write_rgb,
		 (Channel channel, CONST unsigned char *image_red,
		  CONST unsigned char *image_green,
		  CONST unsigned char *image_blue,
		  uaddr *hoffsets, uaddr *voffsets,
		  unsigned int width, unsigned int height,
		  CONST unsigned short *cmap_red,
		  CONST unsigned short *cmap_green,
		  CONST unsigned short *cmap_blue, unsigned int cmap_stride) );


/*  File:  misc.c  */
EXTERN_FUNCTION (unsigned int foreign_guess_format_from_filename,
		 (CONST char *filename) );

/*  File:  guess_read.c  */
EXTERN_FUNCTION (multi_array *foreign_guess_and_read,
		 (CONST char *filename, unsigned int mmap_option,
		  flag writeable, unsigned int *ftype, ...) );
EXTERN_FUNCTION (flag foreign_read_and_setup,
		 (CONST char *filename, unsigned int mmap_option,
		  flag writeable, unsigned int *ftype, flag inform,
		  unsigned int num_dim,
		  unsigned int preferred_type, flag force_type,
		  iarray *array, double *min, double *max,
		  flag discard_zero_range, KwcsAstro *ap) );


#endif /*  KARMA_FOREIGN_H  */
