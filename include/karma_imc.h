/*  karma_imc.h

    Header for  imc_  package.

    Copyright (C) 1993,1994,1995  Richard Gooch

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
  needed to interface to the imc_ routines in the Karma library.


    Written by      Richard Gooch   18-SEP-1993

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#ifndef KARMA_IMC_H
#define KARMA_IMC_H


/*  File:   image_conv.c   */
EXTERN_FUNCTION (flag imc_24to8, (unsigned int image_size,
				  unsigned char *image_reds,
				  unsigned char *image_greens,
				  unsigned char *image_blues, int stride24,
				  unsigned char *out_image, int stride8,
				  unsigned int max_colours, unsigned int speed,
				  packet_desc **pack_desc, char **packet) );


#endif /*  KARMA_IMC_H  */
