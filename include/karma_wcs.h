/*  karma_wcs.h

    Header for  wcs_  package.

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
  needed to interface to the wcs_ routines in the Karma library.


    Written by      Richard Gooch   29-MAY-1996

    Last updated by Richard Gooch   27-JUN-1996

*/

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#if !defined(KARMA_WCS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_wcs_def.h>
#endif

#ifndef KARMA_WCS_H
#define KARMA_WCS_H


/*  Generic routines  */
EXTERN_FUNCTION (KwcsAstro wcs_astro_setup,
		 (CONST packet_desc *pack_desc, CONST char *packet) );
EXTERN_FUNCTION (void wcs_astro_destroy, (KwcsAstro ap) );
EXTERN_FUNCTION (flag wcs_astro_test_radec, (KwcsAstro ap) );
EXTERN_FUNCTION (flag wcs_astro_test_velocity, (KwcsAstro ap) );
EXTERN_FUNCTION (void wcs_astro_transform,
		 (KwcsAstro ap, unsigned int num_coords,
		  double *ra, flag ra_to_linear,
		  double *dec, flag dec_to_linear,
		  double *vel, flag vel_to_linear,
		  unsigned int num_restr, CONST char **restr_names,
		  CONST double *restr_values) );
EXTERN_FUNCTION (void wcs_astro_transform3,
		 (KwcsAstro ap, unsigned int num_coords,
		  CONST char *name0, double *coords0, flag to_lin0,
		  CONST char *name1, double *coords1, flag to_lin1,
		  CONST char *name2, double *coords2, flag to_lin2,
		  unsigned int num_restr, CONST char **restr_names,
		  CONST double *restr_values) );
EXTERN_FUNCTION (void wcs_astro_format_ra, (char *string, double ra) );
EXTERN_FUNCTION (void wcs_astro_format_dec, (char *string, double dec) );
EXTERN_FUNCTION (void wcs_astro_format_vel,
		 (KwcsAstro ap, char *string, double vel) );
EXTERN_FUNCTION (void wcs_astro_format,
		 (KwcsAstro ap, CONST char *dim_name,
		  char string[STRING_LENGTH], double value) );
EXTERN_FUNCTION (void wcs_astro_format_extra,
		 (KwcsAstro ap, char string[STRING_LENGTH]) );
EXTERN_FUNCTION (void wcs_format_all,
		 (KwcsAstro ap, char coord_string[STRING_LENGTH],
		  CONST char *name0, double coord0,
		  CONST char *name1, double coord1,
		  CONST char *name2, double coord2,
		  unsigned int num_restr, CONST char **restr_names,
		  CONST double *restr_values,
		  char other_string[STRING_LENGTH]) );


#endif /*  KARMA_WCS_H  */
