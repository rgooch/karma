/*LINTLIBRARY*/
/*  main.c

    This code provides astronomical projection support.

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

    This file provides astronomical projection support.


    Written by      Richard Gooch   3-MAY-1996

    Updated by      Richard Gooch   12-JUN-1996

    Updated by      Richard Gooch   14-JUN-1996: Moved into library as <wcs>
  package.

    Updated by      Richard Gooch   15-JUN-1996: Created <wcs_astro_destroy>
  and fixed bug in <transform_ra_dec_sin> when converting (A,D) to (X,Y).

    Updated by      Richard Gooch   21-JUN-1996: Created <wcs_astro_format_all>

    Last updated by Richard Gooch   28-JUN-1996: Fixed bug in
  <wcs_astro_transform> when either RA or DEC values not supplied.


*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <karma.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>


/*  Projection types  */
#define PROJ_INIT    -1    /*  Initialised           */
#define PROJ_SIN     0     /*  Orthographic          */
#define PROJ_TAN     1     /*  Gnomonic              */
#define PROJ_NCP     2     /*  North Celestial Pole  */
#define PROJ_FLT     3     /*  Flat                  */
#define PROJ_UNKNOWN 4     /*  Unknown               */

/*  Direction types  */
#define DIR_ADtoXY  0       /*  alpha,delta -> x,y          */
#define DIR_XDtoAY  1       /*  x,delta     -> alpha,y      */
#define DIR_AYtoXD  2       /*  alpha,y     -> x,delta      */
#define DIR_XYtoAD  3       /*  x,y         -> alpha,delta  */

/*  Velocity types  */
#define VELOCITY_UNKNOWN 0  /*  Unknown                     */
#define VELOCITY_VELO    1  /*  m/s                         */
#define VELOCITY_FELO    2  /*  m/s                         */
#define VELOCITY_FREQ    3  /*  Hz                          */

/*  Equinox types  */
#define EQUINOX_J2000  0   /*  Julian 2000           */
#define EQUINOX_B1950  1   /*  Bessellian 1950       */

#define MAGIC_NUMBER (unsigned int) 2076765342

#define VERIFY_AP(ap) if (ap == NULL) \
{(void) fprintf (stderr, "NULL astro context passed\n"); \
 a_prog_bug (function_name); } \
if (ap->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid astro context object\n"); \
 a_prog_bug (function_name); }


/*  Structure declarations follow  */

struct sky_coord_type
{
    char dim_name[STRING_LENGTH];
    double reference;        /*  Degrees  */
    double sin_ref;
    double cos_ref;
};

struct velocity_coord_type
{
    char dim_name[STRING_LENGTH];
    unsigned int type;
};

struct extra_type
{
    char *ctype;
    double refval;
    struct extra_type *next;
};

struct astro_projection_type
{
    /*  Common information  */
    unsigned int magic_number;
    double sin_rotation;
    double cos_rotation;
    int projection;
    unsigned int equinox;
    /*  Right ascension axis  */
    struct sky_coord_type ra;
    /*  Declination axis  */
    struct sky_coord_type dec;
    /*  Velocity axis  */
    struct velocity_coord_type vel;
    /*  Extra information  */
    struct extra_type *first_extra;
    /*  Buffer for unspecified RA or DEC co-ordinates  */
    double *radec_buffer;
    unsigned int radec_buf_len;  /*  Number of doubles  */
};

/*  Note that the header information taken from a Karma data structure is
    assumed to be in FITS format. The readers for foreign data (e.g. Miriad)
    perform on-the-fly conversions to FITS, so what is left in the Karma data
    structure appears to have come from FITS.
    Below are some units used in FITS:
        Right Ascension         Degrees
	Declination             Degrees
	Velocity                m/s (meters per second)
	Temperature             Kelvin
	LL-tangent plane E-W    Degrees
	MM-tangent plane N-S    Degrees
*/

/*  Private functions  */
STATIC_FUNCTION (flag process_axis,
		 (KwcsAstro ap,
		  CONST packet_desc *pack_desc, CONST char *packet,
		  CONST char *axis_name, unsigned int index) );
STATIC_FUNCTION (void add_extra,
		 (KwcsAstro ap, CONST char *ctype, double refval) );
STATIC_FUNCTION (flag fill_radec_buffer,
		 (KwcsAstro ap, CONST char *dim_name, unsigned int num_coords,
		  unsigned int num_restr, CONST char **restr_names,
		  CONST double *restr_values) );
STATIC_FUNCTION (CONST char* find_unspecified,
		 (KwcsAstro ap, CONST char *dim_name, double *value,
		  unsigned int num_restr, CONST char **restr_names,
		  CONST double *restr_values) );
STATIC_FUNCTION (void coords_to_radecvel,
		 (KwcsAstro ap,
		  CONST char *name0, double *coords0, flag to_lin0,
		  CONST char *name1, double *coords1, flag to_lin1,
		  CONST char *name2, double *coords2, flag to_lin2,
		  unsigned int num_restr, CONST char **restr_names,
		  CONST double *restr_values,
		  double **ra, flag *ra_to_linear,
		  double **dec, flag *dec_to_linear,
		  double **vel, flag *vel_to_linear) );
STATIC_FUNCTION (void transform_ra_dec_sin,
		 (KwcsAstro ap, unsigned int num_coords,
		  double *ra, double *dec, unsigned int direction) );
STATIC_FUNCTION (void transform_ra_dec_tan,
		 (KwcsAstro ap, unsigned int num_coords,
		  double *ra, double *dec, unsigned int direction) );
STATIC_FUNCTION (void transform_ra_dec_ncp,
		 (KwcsAstro ap, unsigned int num_coords,
		  double *ra, double *dec, unsigned int direction) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KwcsAstro wcs_astro_setup (CONST packet_desc *pack_desc, CONST char *packet)
/*  [SUMMARY] Setup for Astronomical Sky Co-ordinate projections.
    <pack_desc> The packet descriptor containing the FITS-style keywords.
    <packet> The packet data containing the FITS-stype keyword data.
    [RETURNS] A KwcsAstro object on success, else NULL.
*/
{
    KwcsAstro new;
    int equinox_val;
    unsigned int count;
    char *key_string_value;
    char txt[STRING_LENGTH];
    double value[2];
    static char function_name[] = "wcs_astro_setup";

    if ( (pack_desc == NULL) || (packet == NULL) )
    {
	(void) fputs ("NULL pointer(s) passed\n", stderr);
	a_prog_bug (function_name);
    }
    /*  Theoretically, one should use the "NAXIS" keyword to determine how many
	axes there are to check, but since perhaps not everyone writes this
	keyword, I just keep searching until there are no more "CTYPEn"
	keywords. If "CTYPEn" does not exist, assume "CTYPEm" (where m > n)
	does not exist.
	Note that this whole scheme depends on the Karma dimension names being
	the same as those in the FITS header  */
    if ( (key_string_value = ds_get_unique_named_string (pack_desc, packet,
							 "CTYPE1") ) == NULL )
    {
	return (NULL);
    }
    if ( ( new = (KwcsAstro) m_alloc (sizeof *new) ) == NULL )
    {
	m_error_notify (function_name, "KwcsAstro object");
	return (NULL);
    }
    /*  Initialise  */
    new->projection = PROJ_INIT;
    new->sin_rotation = TOOBIG;
    new->cos_rotation = TOOBIG;
    new->equinox = EQUINOX_J2000;
    equinox_val = 2000;
    new->vel.type = VELOCITY_UNKNOWN;
    new->first_extra = NULL;
    new->radec_buffer = NULL;
    new->radec_buf_len = 0;
    if ( ds_get_unique_named_value (pack_desc, packet, "EQUINOX", NULL,value) )
    {
	/*  EQUINOX: preferred FITS keyword  */
	equinox_val = value[0];
    }
    else
    {
	/*  Not EQUINOX  */
	if ( ds_get_unique_named_value (pack_desc, packet, "EPOCH", NULL,
					value) )
	{
	    /*  EPOCH: old FITS keyword  */
	    equinox_val = value[0];
	}
    }
    if (equinox_val == 2000) new->equinox = EQUINOX_J2000;
    else if (equinox_val == 1950) new->equinox = EQUINOX_B1950;
    else
    {
	(void) fprintf (stderr, "Unknown equinox: %d, assuming J2000\n",
			equinox_val);
	new->equinox = EQUINOX_J2000;
    }
    /*  Start searching for and processing RA and DEC information  */
    count = 1;
    while (key_string_value != NULL)
    {
	if ( !process_axis (new, pack_desc, packet, key_string_value, count) )
	{
	    m_free (key_string_value);
	    m_free ( (char *) new );
	    return (NULL);
	}
	m_free (key_string_value);
	++count;
	(void) sprintf (txt, "CTYPE%u", count);
	key_string_value = ds_get_unique_named_string (pack_desc, packet, txt);
    }
    if (new->projection == PROJ_INIT) new->projection = PROJ_UNKNOWN;
    if ( (new->projection == PROJ_UNKNOWN) &&
	 (new->vel.type == VELOCITY_UNKNOWN) )
    {
	m_free ( (char *) new );
	return (NULL);
    }
    if (new->sin_rotation >= TOOBIG)
    {
	new->sin_rotation = 0.0;
	new->cos_rotation = 1.0;
    }
    new->magic_number = MAGIC_NUMBER;
    return (new);
}   /*  End Function wcs_astro_setup  */

/*PUBLIC_FUNCTION*/
void wcs_astro_destroy (KwcsAstro ap)
/*  [SUMMARY] Destroy KwcsAstro object.
    <ap> The KwcsAstro object.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "wcs_astro_destroy";

    VERIFY_AP (ap);
    m_clear ( (char *) ap, sizeof *ap );
    m_free ( (char *) ap );
}   /*  End Function wcs_astro_destroy  */

/*EXPERIMENTAL_FUNCTION*/
flag wcs_astro_test_radec (KwcsAstro ap)
/*  [SUMMARY] Test if a KwcsAstro object has RA and DEC defined.
    <ap> The KwcsAstro object. This may be NULL.
    [RETURNS] TRUE if the KwcsAstro object has RA and DEC defined, else FALSE.
*/
{
    static char function_name[] = "wcs_astro_test_radec";

    if (ap == NULL) return (FALSE);
    VERIFY_AP (ap);
    if (ap->projection == PROJ_UNKNOWN) return (FALSE);
    return (TRUE);
}   /*  End Function wcs_astro_test_radec  */

/*EXPERIMENTAL_FUNCTION*/
flag wcs_astro_test_velocity (KwcsAstro ap)
/*  [SUMMARY] Test if a KwcsAstro object has velocity or frequency defined.
    <ap> The KwcsAstro object. This may be NULL.
    [RETURNS] TRUE if the KwcsAstro object has velocity or frequency defined,
    else FALSE.
*/
{
    static char function_name[] = "wcs_astro_test_velocity";

    if (ap == NULL) return (FALSE);
    VERIFY_AP (ap);
    if (ap->vel.type == VELOCITY_UNKNOWN) return (FALSE);
    return (TRUE);
}   /*  End Function wcs_astro_test_velocity  */

/*PUBLIC_FUNCTION*/
void wcs_astro_transform (KwcsAstro ap, unsigned int num_coords,
			  double *ra, flag ra_to_linear,
			  double *dec, flag dec_to_linear,
			  double *vel, flag vel_to_linear,
			  unsigned int num_restr, CONST char **restr_names,
			  CONST double *restr_values)
/*  [SUMMARY] Transform between linear and projected co-ordinates.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    <ra_to_linear> If TRUE the right ascension values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <dec> A pointer to the declination values. These will be modified.
    <dec_to_linear> If TRUE the declination values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <vel> A pointer to the velocity values. These will be modified.
    <vel_to_linear> If TRUE the velocity values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <num_restr> The number of restrictions.
    <restr_names> The array of restriction names.
    <restr_values> The restriction values.
    [RETURNS] Nothing.
*/
{
    unsigned int direction;
    static char function_name[] = "wcs_astro_transform";

    VERIFY_AP (ap);
    if (vel != NULL)
    {
	/*  Transform velocity  */
    }
    if ( (ra == NULL) && (dec == NULL) ) return;
    /*  Transform RA and DEC  */
    /*  At this point at least one of ra and dec are not NULL  */
    if (ra == NULL)
    {
	/*  No RA array: construct a fake array filled with the reference
	    value in the "extra" section  */
	if ( !fill_radec_buffer (ap, ap->ra.dim_name, num_coords,
				 num_restr, restr_names, restr_values) )
	{
	    (void) fprintf (stderr, "%s: RA information missing\n",
			    function_name);
	    return;
	}
	ra = ap->radec_buffer;
	ra_to_linear = FALSE;
    }
    if (dec == NULL)
    {
	/*  No DEC array: construct a fake array filled with the reference
	    value in the "extra" section  */
	if ( !fill_radec_buffer (ap, ap->dec.dim_name, num_coords,
				 num_restr, restr_names, restr_values) )
	{
	    (void) fprintf (stderr, "%s: DEC information missing\n",
			    function_name);
	    return;
	}
	dec = ap->radec_buffer;
	dec_to_linear = FALSE;
    }
    /*  Convert flags to direction. Must wait until RA and DEC arrays have been
	checked  */
    if (!ra_to_linear && !dec_to_linear) direction = DIR_XYtoAD;
    else if (!ra_to_linear && dec_to_linear)
    {
	direction = DIR_XDtoAY;
	if (ap->cos_rotation == 0.0)
	{
	    (void) fprintf (stderr, "%s: bad rotation\n", function_name);
	    return;
	}
    }
    else if (ra_to_linear && !dec_to_linear)
    {
	if (ap->cos_rotation == 0.0)
	{
	    (void) fprintf (stderr, "%s: bad rotation\n", function_name);
	    return;
	}
	direction = DIR_AYtoXD;
    }
    else direction = DIR_ADtoXY;
    /*(void) fprintf (stderr, "f: %d %d d: %u  RAin: %e  DECin: %.10e  ",
		    ra_to_linear, dec_to_linear, direction, ra[0], dec[0]);*/
    switch (ap->projection)
    {
      case PROJ_SIN:
	transform_ra_dec_sin (ap, num_coords, ra, dec, direction);
	break;
      case PROJ_TAN:
	transform_ra_dec_tan (ap, num_coords, ra, dec, direction);
	break;
      case PROJ_NCP:
	transform_ra_dec_ncp (ap, num_coords, ra, dec, direction);
	break;
      case PROJ_FLT:
	break;
      default:
	break;
    }
    /*(void) fprintf (stderr, "RAout: %e\n", ra[0]);*/
}   /*  End Function wcs_astro_transform  */

/*PUBLIC_FUNCTION*/
void wcs_astro_transform3 (KwcsAstro ap, unsigned int num_coords,
			   CONST char *name0, double *coords0, flag to_lin0,
			   CONST char *name1, double *coords1, flag to_lin1,
			   CONST char *name2, double *coords2, flag to_lin2,
			   unsigned int num_restr, CONST char **restr_names,
			   CONST double *restr_values)
/*  [SUMMARY] Transform between linear and projected co-ordinates.
    [PURPOSE] This routine will convert up to three arrays of co-ordinates
    using an astronomical projection system. The co-ordinate arrays are named.
    If a co-ordinate array is not defined the co-ordinates are unchanged.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <name0> The name of co-ordinate array 0.
    <coords0> Co-ordinate array 0.
    <to_lin0> If TRUE the co-ordinate 0 values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <name1> The name of co-ordinate array 1.
    <coords1> Co-ordinate array 1.
    <to_lin1> If TRUE the co-ordinate 1 values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <name2> The name of co-ordinate array 2.
    <coords2> Co-ordinate array 2.
    <to_lin2> If TRUE the co-ordinate 2 values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <num_restr> The number of restrictions.
    <restr_names> The array of restriction names.
    <restr_values> The restriction values.
    [RETURNS] Nothing.
*/
{
    flag ra_to_linear;
    flag dec_to_linear;
    flag vel_to_linear;
    double *ra;
    double *dec;
    double *vel;
    static char function_name[] = "wcs_astro_transform3";

    VERIFY_AP (ap);
    coords_to_radecvel (ap,
			name0, coords0, to_lin0,
			name1, coords1, to_lin1,
			name2, coords2, to_lin2,
			num_restr, restr_names, restr_values,
			&ra, &ra_to_linear,
			&dec, &dec_to_linear,
			&vel, &vel_to_linear);
    wcs_astro_transform (ap, num_coords, ra, ra_to_linear,
			 dec, dec_to_linear, vel, vel_to_linear,
			 num_restr, restr_names, restr_values);
}   /*  End Function wcs_astro_transform3  */

/*PUBLIC_FUNCTION*/
void wcs_astro_format_ra (char *string, double ra)
/*  [SUMMARY] Format a Right Ascension value into a string.
    <string> The string to write to. Sufficient storage must be available.
    <ra> The Right Ascension value.
    [RETURNS] Nothing.
*/
{
    double hours, minutes, seconds;

    /*  Convert from degrees to hours  */
    ra /= 15.0;
    /*  Extract hours, minutes and seconds  */
    hours = floor (ra);
    ra -= hours;
    ra *= 60.0;
    minutes = floor (ra);
    ra -= minutes;
    ra *= 60.0;
    seconds = ra;
    (void) sprintf (string, "%.2dh %.2dm %5.2fs",
		    (int) hours, (int) minutes, seconds);
}   /*  End Function wcs_astro_format_ra  */

/*PUBLIC_FUNCTION*/
void wcs_astro_format_dec (char *string, double dec)
/*  [SUMMARY] Format a Declination value into a string.
    <string> The string to write to. Sufficient storage must be available.
    <dec> The Declination value.
    [RETURNS] Nothing.
*/
{
    double degrees, minutes, seconds;

    if (dec < 0.0)
    {
	degrees = ceil (dec);
	dec = fabs (dec);
    }
    else degrees = floor (dec);
    dec -= fabs (degrees);
    dec *= 60.0;
    minutes = floor (dec);
    dec -= minutes;
    dec *= 60.0;
    seconds = dec;
    (void) sprintf (string, "%.2dd %.2dm %5.2fs",
		    (int) degrees, (int) minutes, seconds);
}   /*  End Function wcs_astro_format_dec  */

/*PUBLIC_FUNCTION*/
void wcs_astro_format_vel (KwcsAstro ap, char *string, double vel)
/*  [SUMMARY] Format a velocity value into a string.
    <ap> The KwcsAstro object.
    <string> The string to write to. Sufficient storage must be available.
    <vel> The velocity value.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "wcs_astro_format_vel";

    VERIFY_AP (ap);
    switch (ap->vel.type)
    {
      case VELOCITY_VELO:
      case VELOCITY_FELO:
	sprintf (string, "Vel: %.2f km/s", vel * 1e-3);
	break;
      case VELOCITY_FREQ:
	sprintf (string, "Freq: %.3f MHz", vel * 1e-6);
	break;
      case VELOCITY_UNKNOWN:
	sprintf (string, "v/f: %e", vel);
	break;
    }
}   /*  End Function wcs_astro_format_vel  */

/*PUBLIC_FUNCTION*/
void wcs_astro_format (KwcsAstro ap, CONST char *dim_name,
		       char string[STRING_LENGTH], double value)
/*  [SUMMARY] Format a value.
    <ap> The KwcsAstro object.
    <dim_name> The name of the dimension.
    <string> The string to write to.
    <value> The value.
    [RETURNS] Nothing.
*/
{
    char stokes_char;
    char txt[STRING_LENGTH];
    static char function_name[] = "wcs_astro_format";

    VERIFY_AP (ap);
    if (strcmp (dim_name, ap->ra.dim_name) == 0)
    {
	wcs_astro_format_ra (txt, value);
	(void) sprintf (string, "Ra %s", txt);
	return;
    }
    if (strcmp (dim_name, ap->dec.dim_name) == 0)
    {
	wcs_astro_format_dec (txt, value);
	(void) sprintf (string, "Dec %s", txt);
	return;
    }
    if (strcmp (dim_name, ap->vel.dim_name) == 0)
    {
	wcs_astro_format_vel (ap, string, value);
	return;
    }
    if (strcmp (dim_name, "STOKES") == 0)
    {
	switch ( (int) value )
	{
	  case 1:
	    stokes_char = 'I';
	    break;
	  case 2:
	    stokes_char = 'Q';
	    break;
	  case 3:
	    stokes_char = 'U';
	    break;
	  case 4:
	    stokes_char = 'V';
	    break;
	  default:
	    stokes_char = '?';
	    break;
	}
	sprintf (string, "Stokes %c", stokes_char);
	return;
    }
    (void) sprintf (string, "%e %s", value, dim_name);
}   /*  End Function wcs_astro_format  */

/*PUBLIC_FUNCTION*/
void wcs_astro_format_extra (KwcsAstro ap, char string[STRING_LENGTH])
/*  [SUMMARY] Format extra information.
    <ap> The KwcsAstro object.
    <string> The string to write to.
    [RETURNS] Nothing.
*/
{
    struct extra_type *curr;
    char txt[STRING_LENGTH];
    static char function_name[] = "wcs_astro_format_extra";

    VERIFY_AP (ap);
    string[0] = '\0';
    for (curr = ap->first_extra; curr != NULL; curr = curr->next)
    {
	wcs_astro_format (ap, curr->ctype, txt, curr->refval);
	(void) strcat (string, txt);
	if (curr->next != NULL) (void) strcat (string, "  ");
    }
}   /*  End Function wcs_astro_format_extra  */

/*PUBLIC_FUNCTION*/
void wcs_format_all (KwcsAstro ap, char coord_string[STRING_LENGTH],
		     CONST char *name0, double coord0,
		     CONST char *name1, double coord1,
		     CONST char *name2, double coord2,
		     unsigned int num_restr, CONST char **restr_names,
		     CONST double *restr_values,
		     char other_string[STRING_LENGTH])
/*  [SUMMARY] Transform and format all available information.
    [PURPOSE] This routine will transform and format all available information.
    The data is transformed from linear to non-linear values.
    <ap> The KwcsAstro object.
    <coord_string> Formatted co-ordinate information is written here.
    <name0> The name of co-ordinate 0.
    <coord0> Co-ordinate 0.
    <name1> The name of co-ordinate 1. If this is NULL, it is ignored.
    <coord1> Co-ordinate 1.
    <name2> The name of co-ordinate 2. If this is NULL, it is ignored.
    <coord2> Co-ordinate 2.
    <num_restr> The number of restrictions.
    <restr_names> The array of restriction names.
    <restr_values> The restriction values.
    <other_string> Formatted extra information is written here.
    [RETURNS] Nothing.
*/
{
    flag ra_to_linear;
    flag dec_to_linear;
    flag vel_to_linear;
    unsigned int count;
    double radec_value;
    double *ra;
    double *dec;
    double *vel;
    CONST char *unspec_name = NULL;
    struct extra_type *curr;
    char txt[STRING_LENGTH];
    static char function_name[] = "wcs_format_all";

    VERIFY_AP (ap);
    *coord_string = '\0';
    *other_string = '\0';
    coords_to_radecvel (ap,
			name0, &coord0, FALSE,
			name1, &coord1, FALSE,
			name2, &coord2, FALSE,
			num_restr, restr_names, restr_values,
			&ra, &ra_to_linear,
			&dec, &dec_to_linear,
			&vel, &vel_to_linear);
    if ( (ra == NULL) && (dec != NULL) )
    {
	/*  Have to get unspecified DEC value  */
	if ( ( unspec_name =
	       find_unspecified (ap, ap->ra.dim_name, &radec_value,
				 num_restr, restr_names, restr_values) )
	     == NULL )
	{
	    (void) fprintf (stderr, "%s: missing DEC information\n",
			    function_name);
	    return;
	}
	ra = &radec_value;
    }
    else if ( (ra != NULL) && (dec == NULL) )
    {
	/*  Have to get unspecified RA value  */
	if ( ( unspec_name =
	       find_unspecified (ap, ap->dec.dim_name, &radec_value,
				 num_restr, restr_names, restr_values) )
	     == NULL )
	{
	    (void) fprintf (stderr, "%s: missing RA information\n",
			    function_name);
	    return;
	}
	dec = &radec_value;
    }
    wcs_astro_transform (ap, 1, ra, FALSE, dec, FALSE, vel, FALSE,
			 num_restr, restr_names, restr_values);
    wcs_astro_format (ap, name0, coord_string, coord0);
    if (name1 != NULL)
    {
	wcs_astro_format (ap, name1, txt, coord1);
	(void) strcat (coord_string, "  ");
	(void) strcat (coord_string, txt);
    }
    if (name2 != NULL)
    {
	wcs_astro_format (ap, name2, txt, coord2);
	(void) strcat (coord_string, "  ");
	(void) strcat (coord_string, txt);
    }
    /*  Add any restriction information  */
    for (count = 0; count < num_restr; ++count)
    {
	if (unspec_name == restr_names[count])
	{
	    wcs_astro_format (ap, restr_names[count], txt, radec_value);
	}
	else wcs_astro_format (ap, restr_names[count], txt,
			       restr_values[count]);
	(void) strcat (coord_string, "  ");
	(void) strcat (coord_string, txt);
    }
    for (curr = ap->first_extra; curr != NULL; curr = curr->next)
    {
	if (unspec_name == curr->ctype)
	{
	    wcs_astro_format (ap, curr->ctype, txt, radec_value);
	}
	else wcs_astro_format (ap, curr->ctype, txt, curr->refval);
	(void) strcat (other_string, txt);
	if (curr->next != NULL) (void) strcat (other_string, "  ");
    }
}   /*  End Function wcs_format_all  */


/*  Private functions follow  */

static flag process_axis (KwcsAstro ap,
			  CONST packet_desc *pack_desc, CONST char *packet,
			  CONST char *axis_name, unsigned int index)
/*  [SUMMARY] Process a single axis.
    <ap> The KwcsAstro object.
    <axis_name> The name of the axis.
    <index> The FITS index of the axis.
    <pack_desc> The packet descriptor containing the FITS-style keywords.
    <packet> The packet data containing the FITS-stype keyword data.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int str_len;
    int proj;
    unsigned int item_index;
    double deg_to_rad = PION180;
    struct sky_coord_type *sky = NULL;
    char *encls_desc;
    char txt[STRING_LENGTH];
    double reference[2];
    double value[2];

    (void) sprintf (txt, "CRVAL%u", index);
    if ( !ds_get_unique_named_value (pack_desc, packet, txt, NULL, reference) )
    {
	return (FALSE);
    }
    if (strncmp (axis_name, "RA-", 3) == 0)
    {
	/*  Right ascension  */
	sky = &ap->ra;
    }
    else if (strncmp (axis_name, "DEC-", 4) == 0)
    {
	/*  Declination  */
	sky = &ap->dec;
    }
    if (sky != NULL)
    {
	/*  Right ascension or declination  */
	sky->reference = reference[0];
	sky->cos_ref = cos (sky->reference * deg_to_rad);
	sky->sin_ref = sin (sky->reference * deg_to_rad);
	/*  Determine the projection system  */
	str_len = strlen (axis_name);
	if (strcmp (axis_name + str_len - 4, "-SIN") == 0) proj = PROJ_SIN;
	else if (strcmp (axis_name + str_len -4, "-TAN") == 0) proj = PROJ_TAN;
	else if (strcmp (axis_name + str_len -4, "-NCP") == 0) proj = PROJ_NCP;
	else if (strcmp (axis_name + str_len -4, "-FLT") == 0) proj = PROJ_FLT;
	else proj = PROJ_UNKNOWN;
	if (ap->projection == PROJ_INIT)
	{
	    ap->projection = proj;
	}
	else if (ap->projection != proj)
	{
	    (void) fputs ("Projections do not match\n", stderr);
	    return (FALSE);
	}
	(void) sprintf (txt, "CROTA%u", index);
	if ( ds_get_unique_named_value (pack_desc, packet, txt, NULL, value) &&
	     (value[0] != 0.0) )
	{
	    if ( (ap->sin_rotation < TOOBIG) &&
		 (sin (deg_to_rad * value[0]) != ap->sin_rotation) )
	    {
		fprintf (stderr,
			 "Rotation multiply and inconsistently defined\n");
		return (FALSE);
	    }
	    ap->sin_rotation = sin (deg_to_rad * value[0]);
	    ap->cos_rotation = cos (deg_to_rad * value[0]);
	}
	(void) strcpy (sky->dim_name, axis_name);
    }
    else if (strncmp (axis_name, "FREQ", 4) == 0)
    {
	/*  Frequency  */
	ap->vel.type = VELOCITY_FREQ;
	(void) strcpy (ap->vel.dim_name, axis_name);
    }
    else if (strncmp (axis_name, "VELO", 4) == 0)
    {
	/*  Velocity  */
	ap->vel.type = VELOCITY_VELO;
	(void) strcpy (ap->vel.dim_name, axis_name);
    }
    else if (strncmp (axis_name, "FELO", 4) == 0)
    {
	/*  Velocity  */
	ap->vel.type = VELOCITY_FELO;
	(void) strcpy (ap->vel.dim_name, axis_name);
    }
    /*  Check if this axis is a dimension  */
    /*  Search for specified name first  */
    switch ( ds_f_name_in_packet (pack_desc, axis_name, &encls_desc,
				  &item_index) )
    {
      case IDENT_NOT_FOUND:
	add_extra (ap, axis_name, reference[0]);
	break;
      default:
	break;
    }
    return (TRUE);
}   /*  End Function process_axis  */

static void add_extra (KwcsAstro ap, CONST char *ctype, double refval)
/*  [SUMMARY] Add extra information.
    <ap> The KwcsAstro object.
    <ctype> The value of the "CTYPEn" keyword.
    <refval> The value of the "CRVALn" keyword.
    [RETURNS] Nothing.
*/
{
    struct extra_type *new, *curr;
    static char function_name[] = "wcs_astro__add_extra";

    if ( ( new = (struct extra_type *) m_alloc (sizeof *new) ) == NULL )
    {
	m_abort (function_name, "extra entry");
    }
    if ( ( new->ctype = st_dup (ctype) ) == NULL )
    {
	m_abort (function_name, "ctype name");
    }
    new->refval = refval;
    new->next = NULL;
    /*  Append to list  */
    if (ap->first_extra == NULL)
    {
	ap->first_extra = new;
	return;
    }
    for (curr = ap->first_extra; curr->next != NULL; curr = curr->next);
    curr->next = new;
}   /*  End Function add_extra  */

static flag fill_radec_buffer (KwcsAstro ap, CONST char *dim_name,
			       unsigned int num_coords,
			       unsigned int num_restr,CONST char **restr_names,
			       CONST double *restr_values)
/*  [SUMMARY] Fill the RA/DEC buffer with RA or DEC values.
    <ap> The KwcsAstro object.
    <dim_name> The name of the dimension to fill for.
    <num_coords> The number of values to place into the buffer.
    <num_restr> The number of restrictions.
    <restr_names> The array of restriction names.
    <restr_values> The restriction values.
    [RETURNS] TRUE if data was available to fill, else FALSE.
*/
{
    unsigned int count;
    double value;
    static char function_name[] = "wcs_astro__fill_radec_buffer";

    /*  First make sure buffer is large enough  */
    if (num_coords > ap->radec_buf_len)
    {
	if (ap->radec_buffer != NULL) m_free ( (char *) ap->radec_buffer );
	if ( ( ap->radec_buffer = (double *)
	       m_alloc (num_coords * sizeof *ap->radec_buffer) ) == NULL )
	{
	    m_abort (function_name, "RA/DEC buffer");
	}
	ap->radec_buf_len = num_coords;
    }
    /*  Search for it  */
    if (find_unspecified (ap, dim_name, &value, num_restr, restr_names,
			  restr_values) == NULL) return (FALSE);
    for (count = 0; count < num_coords; ++count)
	ap->radec_buffer[count] = value;
    return (TRUE);
}   /*  End Function fill_radec_buffer  */

static CONST char *find_unspecified (KwcsAstro ap, CONST char *dim_name,
				     double *value, unsigned int num_restr,
				     CONST char **restr_names,
				     CONST double *restr_values)
/*  [SUMMARY] Find an unspecified value.
    <ap> The KwcsAstro object.
    <dim_name> The name of the dimension to fill for.
    <value> The unspecified value is written here.
    <num_restr> The number of restrictions.
    <restr_names> The array of restriction names.
    <restr_values> The restriction values.
    [RETURNS] A pointer to the found name on success, else NULL.
*/
{
    unsigned int count;
    struct extra_type *curr;
    /*static char function_name[] = "wcs_astro__find_unspecified";*/

    /*  Search for it in extra information  */
    for (curr = ap->first_extra; curr != NULL; curr = curr->next)
    {
	if (strcmp (curr->ctype, dim_name) == 0)
	{
	    *value = curr->refval;
	    return (curr->ctype);
	}
    }
    /*  Search for it in restrictions  */
    for (count = 0; count < num_restr; ++count)
    {
	if (strcmp (restr_names[count], dim_name) == 0)
	{
	    *value = restr_values[count];
	    return (restr_names[count]);
	}
    }
    return (NULL);
}   /*  End Function find_unspecified  */

static void coords_to_radecvel (KwcsAstro ap,
				CONST char *name0,double *coords0,flag to_lin0,
				CONST char *name1,double *coords1,flag to_lin1,
				CONST char *name2,double *coords2,flag to_lin2,
				unsigned int num_restr,
				CONST char **restr_names,
				CONST double *restr_values,
				double **ra, flag *ra_to_linear,
				double **dec, flag *dec_to_linear,
				double **vel, flag *vel_to_linear)
/*  [SUMMARY] Convert named co-ordinates to RA, DEC and VEL.
    <ap> The KwcsAstro object.
    <name0> The name of co-ordinate array 0.
    <coords0> Co-ordinate array 0.
    <to_lin0> If TRUE the co-ordinate 0 values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <name1> The name of co-ordinate array 1.
    <coords1> Co-ordinate array 1.
    <to_lin1> If TRUE the co-ordinate 1 values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <name2> The name of co-ordinate array 2.
    <coords2> Co-ordinate array 2.
    <to_lin2> If TRUE the co-ordinate 2 values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <num_restr> The number of restrictions.
    <restr_names> The array of restriction names.
    <restr_values> The restriction values.
    <ra> The pointer to the RA co-ordinate array is written here.
    <ra_to_linear> The to_linear flag is written here.
    <dec> The pointer to the DEC co-ordinate array is written here.
    <dec_to_linear> The to_linear flag is written here.
    <vel> The pointer to the VEL co-ordinate array is written here.
    <vel_to_linear> The to_linear flag is written here.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    flag to_lin[3];
    CONST char *names[3];
    double *coords[3];
    static char function_name[] = "wcs_astro__coords_to_radecvel";

    VERIFY_AP (ap);
    names[0] = name0;
    names[1] = name1;
    names[2] = name2;
    to_lin[0] = to_lin0;
    to_lin[1] = to_lin1;
    to_lin[2] = to_lin2;
    coords[0] = coords0;
    coords[1] = coords1;
    coords[2] = coords2;
    *ra = NULL;
    *dec = NULL;
    *vel = NULL;
    for (count = 0; count < 3; ++count)
    {
	if (names[count] == NULL) continue;
	if (strcmp (names[count], ap->ra.dim_name) == 0)
	{
	    *ra = coords[count];
	    *ra_to_linear = to_lin[count];
	}
	else if (strcmp (names[count], ap->dec.dim_name) == 0)
	{
	    *dec = coords[count];
	    *dec_to_linear = to_lin[count];
	}
	else if (strcmp (names[count], ap->vel.dim_name) == 0)
	{
	    *vel = coords[count];
	    *vel_to_linear = to_lin[count];
	}
    }
}   /*  End Function coords_to_radecvel  */

static void transform_ra_dec_sin (KwcsAstro ap, unsigned int num_coords,
				  double *ra, double *dec,
				  unsigned int direction)
/*  [SUMMARY] Orthographic projection of co-ordinates.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <dec> A pointer to the declination values. These will be modified.
    <direction> The direction of the transform.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double deg_to_rad = PION180;
    double rad_to_deg = 180.0 / PI;
    double l, m, x, y, tmp;
    double delta, sin_delta, cos_delta;
    double alpha, sin_alpha, cos_alpha;
    double diff_alpha, sin_diff_alpha, cos_diff_alpha;
    double one = 1.0;

    for (count = 0; count < num_coords; ++count)
    {
	if (direction == DIR_ADtoXY)
	{
	    diff_alpha = (ra[count] - ap->ra.reference) * deg_to_rad;
	    delta = dec[count] * deg_to_rad;
	    cos_delta = cos (delta);
	    l = cos_delta * sin (diff_alpha);
	    m = sin (delta) * ap->dec.cos_ref -
		cos_delta * ap->dec.sin_ref * cos (diff_alpha);
	    /*  Rotate  */
	    x = l * ap->cos_rotation + m * ap->sin_rotation;
	    y = m * ap->cos_rotation - l * ap->sin_rotation;
	    /*  Convert back to degrees and add to reference  */
	    ra[count] = ap->ra.reference + x * rad_to_deg;
	    dec[count] = ap->dec.reference + y * rad_to_deg;
	}
	else if (direction == DIR_XDtoAY)
	{
	    double b, c, s;

	    alpha = ap->cos_rotation;
	    b = ap->dec.sin_ref * ap->sin_rotation;
	    delta = deg_to_rad * dec[count];
	    x = deg_to_rad * (ra[count] - ap->ra.reference);
	    cos_delta = cos (delta);
	    sin_delta = sin (delta);
	    s = x - sin_delta * ap->dec.cos_ref * ap->sin_rotation;
	    tmp = cos_delta * sqrt (alpha * alpha + b * b);
	    diff_alpha = atan (b / alpha) + asin (s / tmp);
	    cos_alpha = cos (diff_alpha);
	    sin_alpha = sin (diff_alpha);
	    alpha = sin_delta * ap->dec.cos_ref * ap->cos_rotation;
	    b = cos_delta * ap->dec.sin_ref * cos_alpha * ap->cos_rotation;
	    c = cos_delta * sin_alpha * ap->sin_rotation;
	    ra[count] = ap->ra.reference + rad_to_deg * diff_alpha;
	    dec[count] = ap->dec.reference + rad_to_deg * (alpha - b - c);
	}
	else if (direction == DIR_AYtoXD)
	{
	    double b, c;

	    diff_alpha = deg_to_rad * (ra[count] - ap->ra.reference);
	    cos_diff_alpha = cos (diff_alpha);
	    sin_diff_alpha = sin (diff_alpha);
	    y = deg_to_rad * (dec[count] - ap->dec.reference);
	    alpha = ap->dec.cos_ref * ap->cos_rotation;
	    b = ap->dec.sin_ref * cos_diff_alpha * ap->cos_rotation +
		sin_diff_alpha * ap->sin_rotation;
	    delta = atan (b / alpha) + asin ( y / sqrt (alpha *alpha + b *b) );
	    cos_delta = cos (delta);
	    sin_delta = sin (delta);
	    alpha = cos_delta * sin_diff_alpha * ap->cos_rotation;
	    b = sin_delta * ap->dec.cos_ref * ap->sin_rotation;
	    c = cos_delta * ap->dec.sin_ref * cos_diff_alpha *ap->sin_rotation;
	    ra[count] = ap->ra.reference + rad_to_deg * (alpha + b - c);
	    dec[count] = rad_to_deg * delta;
	}
	else if (direction == DIR_XYtoAD)
	{
	    /*  Subtract reference and convert to radians  */
	    x = (ra[count] - ap->ra.reference) * deg_to_rad;
	    y = (dec[count] - ap->dec.reference) * deg_to_rad;
	    /*  Rotate  */
	    l = x * ap->cos_rotation - y * ap->sin_rotation;
	    m = y * ap->cos_rotation + x * ap->sin_rotation;
	    tmp = sqrt (one - l * l - m * m);
	    diff_alpha = atan ( l / (ap->dec.cos_ref * tmp - m *
				     ap->dec.sin_ref) );
	    /*  Convert back to degrees and add to reference  */
	    ra[count] = ap->ra.reference + diff_alpha * rad_to_deg;
	    dec[count] = rad_to_deg * asin (m * ap->dec.cos_ref +
					    ap->dec.sin_ref * tmp);
	}
    }
}   /*  End Function transform_ra_dec_sin  */

static void transform_ra_dec_tan (KwcsAstro ap, unsigned int num_coords,
				  double *ra, double *dec,
				  unsigned int direction)
/*  [SUMMARY] Gnomonic projection of co-ordinates.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <dec> A pointer to the declination values. These will be modified.
    <direction> The direction of the transform.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double deg_to_rad = PION180;
    double rad_to_deg = 180.0 / PI;
    double l, m, x, y, tmp;
    double delta, sin_delta, cos_delta;
    double alpha;
    double diff_alpha, sin_diff_alpha, cos_diff_alpha;
    double minus_one = -1.0;

    for (count = 0; count < num_coords; ++count)
    {
	if (direction == DIR_ADtoXY)
	{
	    diff_alpha = (ra[count] - ap->ra.reference) * deg_to_rad;
	    cos_diff_alpha = cos (diff_alpha);
	    delta = dec[count] * deg_to_rad;
	    sin_delta = sin (delta);
	    cos_delta = cos (delta);
	    tmp = sin_delta * ap->dec.sin_ref +
		cos_delta * ap->dec.cos_ref * cos_diff_alpha;
	    l = cos_delta * sin (diff_alpha) / tmp;
	    m = (sin_delta * ap->dec.cos_ref -
		 cos_delta * ap->dec.sin_ref * cos_diff_alpha) / tmp;
	    /*  Rotate  */
	    x = l * ap->cos_rotation + m * ap->sin_rotation;
	    y = m * ap->cos_rotation - l * ap->sin_rotation;
	    /*  Convert back to degrees and add to reference  */
	    ra[count] = ap->ra.reference + x * rad_to_deg;
	    dec[count] = ap->dec.reference + y * rad_to_deg;
	}
	else if (direction == DIR_XDtoAY)
	{
	    double b, c, s, tan_delta;

	    x = deg_to_rad * (ra[count] - ap->ra.reference);
	    alpha = ap->cos_rotation;
	    b = x * ap->dec.cos_ref + ap->dec.sin_ref * ap->sin_rotation;
	    c = x * ap->dec.sin_ref - ap->dec.cos_ref * ap->sin_rotation;
	    delta = deg_to_rad * dec[count];
	    cos_delta = cos (delta);
	    sin_delta = sin (delta);
	    tan_delta = sin_delta / cos_delta;
	    diff_alpha = atan (b / alpha) +
		asin ( tan_delta * c / sqrt (alpha * alpha + b * b) );
	    cos_diff_alpha = cos (diff_alpha);
	    sin_diff_alpha = sin (diff_alpha);
	    alpha = sin_delta * ap->dec.cos_ref * ap->cos_rotation;
	    b = cos_delta * ap->dec.sin_ref * cos_diff_alpha *ap->cos_rotation;
	    c = cos_delta * sin_diff_alpha * ap->sin_rotation;
	    s = sin_delta * ap->dec.sin_ref;
	    tmp = cos_delta * ap->dec.cos_ref * cos_diff_alpha;
	    ra[count] = ap->ra.reference + rad_to_deg * diff_alpha;
	    dec[count] = ap->dec.reference +
		rad_to_deg * (alpha - b - c) / (s + tmp);
	}
	else if (direction == DIR_AYtoXD)
	{
	    double b, c, s, tan_delta;

	    diff_alpha = deg_to_rad * (ra[count] - ap->ra.reference);
	    cos_diff_alpha = cos (diff_alpha);
	    sin_diff_alpha = sin (diff_alpha);
	    y = deg_to_rad * (dec[count] - ap->dec.reference);
	    alpha = ap->dec.sin_ref * cos_diff_alpha * ap->cos_rotation;
	    b = sin_diff_alpha * ap->sin_rotation;
	    c = y * ap->dec.cos_ref * cos_diff_alpha;
	    s = ap->dec.cos_ref * ap->cos_rotation;
	    tmp = y * ap->dec.sin_ref;
	    tan_delta = (alpha + b + c) / (s - tmp);
	    alpha = sin_diff_alpha * ap->cos_rotation;
	    b = tan_delta * ap->dec.cos_ref * ap->sin_rotation;
	    c = ap->dec.sin_ref * cos_diff_alpha * ap->sin_rotation;
	    s = tan_delta * ap->dec.sin_ref;
	    tmp = ap->dec.cos_ref * cos_diff_alpha;
	    ra[count] = ap->ra.reference +
		rad_to_deg * (alpha + b - c) / (s + tmp);
	    dec[count] = rad_to_deg * atan (tan_delta);
	}
	else if (direction == DIR_XYtoAD)
	{
	    double s;

	    /*  Subtract reference and convert to radians  */
	    x = (ra[count] - ap->ra.reference) * deg_to_rad;
	    y = (dec[count] - ap->dec.reference) * deg_to_rad;
	    /*  Rotate  */
	    l = x * ap->cos_rotation - y * ap->sin_rotation;
	    m = y * ap->cos_rotation + x * ap->sin_rotation;
	    s = m * ap->dec.cos_ref + ap->dec.sin_ref;
	    tmp = ap->dec.cos_ref - m * ap->dec.sin_ref;
	    diff_alpha = atan (l / tmp);
	    /*  Convert back to degrees and add to reference  */
	    ra[count] = ap->ra.reference + diff_alpha * rad_to_deg;
	    dec[count] = rad_to_deg * atan (cos (diff_alpha) * s / tmp);
	    if (dec[count] * ap->dec.reference < 0.0)
	    {
		if (ra[count] > 180.0) ra[count] -= 180.0;
		else ra[count] += 180.0;
		dec[count] *= minus_one;
	    }
	}
    }
}   /*  End Function transform_ra_dec_tan  */

static void transform_ra_dec_ncp (KwcsAstro ap, unsigned int num_coords,
				  double *ra, double *dec,
				  unsigned int direction)
/*  [SUMMARY] North Celestial Pole projection of co-ordinates.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    co-ordinates, else they are transformed to non-linear co-ordinates.
    <dec> A pointer to the declination values. These will be modified.
    <direction> The direction of the transform.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double deg_to_rad = PION180;
    double rad_to_deg = 180.0 / PI;
    double l, m, x, y, tmp;
    double delta, cos_delta;
    double alpha;
    double diff_alpha, sin_diff_alpha, cos_diff_alpha;
    double zero = 0.0;
    double one = 1.0;
    double minus_one = -1.0;

    for (count = 0; count < num_coords; ++count)
    {
	if (direction == DIR_ADtoXY)
	{
	    diff_alpha = (ra[count] - ap->ra.reference) * deg_to_rad;
	    delta = dec[count] * deg_to_rad;
	    cos_delta = cos (delta);
	    l = cos_delta * sin (diff_alpha);
	    m = ( ap->dec.cos_ref - cos_delta * cos (diff_alpha) ) /
		ap->dec.sin_ref;
	    /*  Rotate  */
	    x = l * ap->cos_rotation + m * ap->sin_rotation;
	    y = m * ap->cos_rotation - l * ap->sin_rotation;
	    /*  Convert back to degrees and add to reference  */
	    ra[count] = ap->ra.reference + x * rad_to_deg;
	    dec[count] = ap->dec.reference + y * rad_to_deg;
	}
	else if (direction == DIR_XDtoAY)
	{
	    double b, c, s;

	    x = deg_to_rad * (ra[count] - ap->ra.reference);
	    cos_delta = cos (deg_to_rad * dec[count]);
	    alpha = ap->cos_rotation;
	    b = ap->sin_rotation / ap->dec.sin_ref;
	    s = x * ap->dec.sin_ref - ap->dec.cos_ref * ap->sin_rotation;
	    tmp = cos_delta * ap->dec.sin_ref * sqrt (alpha * alpha + b * b);
	    diff_alpha = atan (b / alpha) + asin (s / tmp);
	    alpha = ap->cos_rotation * ap->dec.cos_ref / ap->dec.sin_ref;
	    b = ap->cos_rotation *cos_delta *cos (diff_alpha) /ap->dec.sin_ref;
	    c = ap->sin_rotation * cos_delta * sin (diff_alpha);
	    ra[count] = ap->ra.reference + rad_to_deg * diff_alpha;
	    dec[count] = ap->dec.reference + rad_to_deg * (alpha - b - c);
	}
	else if (direction == DIR_AYtoXD)
	{
	    double b, c, s, q;

	    y = deg_to_rad * (dec[count] - ap->dec.reference);
	    diff_alpha = deg_to_rad * (ra[count] - ap->ra.reference);
	    cos_diff_alpha = cos (diff_alpha);
	    sin_diff_alpha = sin (diff_alpha);
	    s = ap->dec.cos_ref * ap->cos_rotation - y * ap->dec.sin_ref;
	    tmp = cos_diff_alpha * ap->cos_rotation +
		sin_diff_alpha * ap->sin_rotation * ap->dec.sin_ref;
	    q = s / tmp;
	    if (q > one) q = one;
	    else if (q < minus_one) q = minus_one;
	    delta = acos (q);
	    if (ap->dec.reference > zero) delta = fabs (delta);
	    else delta = -fabs (delta);
	    cos_delta = cos (delta);
	    alpha = cos_delta * sin_diff_alpha * ap->cos_rotation;
	    b = ap->dec.cos_ref * ap->sin_rotation / ap->dec.sin_ref;
	    c = cos_delta * cos_diff_alpha * ap->sin_rotation /ap->dec.sin_ref;
	    ra[count] = ap->ra.reference + rad_to_deg * (alpha + b - c);
	    dec[count] = rad_to_deg * delta;
	}
	else if (direction == DIR_XYtoAD)
	{
	    /*  Subtract reference and convert to radians  */
	    x = (ra[count] - ap->ra.reference) * deg_to_rad;
	    y = (dec[count] - ap->dec.reference) * deg_to_rad;
	    /*  Rotate  */
	    l = x * ap->cos_rotation - y * ap->sin_rotation;
	    m = y * ap->cos_rotation + x * ap->sin_rotation;
	    diff_alpha = atan ( l / (ap->dec.cos_ref - m * ap->dec.sin_ref) );
	    cos_diff_alpha = cos (diff_alpha);
	    delta = acos ( (ap->dec.cos_ref - m * ap->dec.sin_ref) /
			   cos_diff_alpha );
	    if (ap->dec.reference > zero) delta = fabs (delta);
	    else delta = -fabs (delta);
	    /*  Convert back to degrees and add to reference  */
	    ra[count] = ap->ra.reference + diff_alpha * rad_to_deg;
	    dec[count] = rad_to_deg * delta;
	}
    }
}   /*  End Function transform_ra_dec_ncp  */
