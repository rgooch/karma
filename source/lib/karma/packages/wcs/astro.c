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

    Updated by      Richard Gooch   28-JUN-1996: Fixed bug in
  <wcs_astro_transform> when either RA or DEC values not supplied.

    Updated by      Richard Gooch   5-AUG-1996: Fixed bug in
  <wcs_astro_format_dec> when dealing with {-1 < dec < 0}.

    Updated by      Richard Gooch   29-AUG-1996: Created
  <wcs_astro_get_preferred_units> routine.

    Updated by      Richard Gooch   25-SEP-1996: Changed to implement the new
  FITS-style co-ordinate handling, where dimension co-ordinates range from 0
  to length - 1.

    Updated by      Richard Gooch   28-SEP-1996: Supported offset angle axes.
  Fixed name for <wcs_astro_format_all> so that "astro" is a part of the name.

    Updated by      Richard Gooch   6-OCT-1996: Added support for Cartesian
  (*-CAR) projection.

    Updated by      Richard Gooch   7-OCT-1996: Added support for Digital Sky
  Survey plate solutions (i.e. x,y -> RA,DEC).

    Updated by      Richard Gooch   7-OCT-1996: Added support for Digital Sky
  Survey inverse plate solutions (i.e. RA,DEC -> x,y).

    Last updated by Richard Gooch   15-OCT-1996: Added support for Rectangular
  (*-ARC) projection.


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
#define PROJ_INIT    -1    /*  Initialised                                */
#define PROJ_UNKNOWN 0     /*  Unknown                                    */
#define PROJ_SIN     1     /*  Orthographic                               */
#define PROJ_TAN     2     /*  Gnomonic                                   */
#define PROJ_NCP     3     /*  North Celestial Pole                       */
#define PROJ_FLT     4     /*  Flat                                       */
#define PROJ_ARC     5     /*  Rectangular                                */
#define PROJ_CAR     6     /*  Cartesian (seen in Miriad for DSS images)  */
#define PROJ_DSS     7     /*  Digital Sky Survey plate solutions         */

/*  Direction types  */
#define DIR_ADtoXY  0       /*  alpha,delta -> x,y          */
#define DIR_XDtoAY  1       /*  x,delta     -> alpha,y      */
#define DIR_AYtoXD  2       /*  alpha,y     -> x,delta      */
#define DIR_XYtoAD  3       /*  x,y         -> alpha,delta  */

/*  Velocity types  */
#define VELOCITY_UNKNOWN 0  /*  Unknown                           */
#define VELOCITY_VELO    1  /*  m/s (radio doppler convention)    */
#define VELOCITY_FELO    2  /*  m/s (optical doppler convention)  */
#define VELOCITY_FREQ    3  /*  Hz                                */
#define VELOCITY_RAWVEL  4  /*  m/s  rest frequency not known     */

/*  Equinox types  */
#define EQUINOX_J2000  0   /*  Julian 2000           */
#define EQUINOX_B1950  1   /*  Bessellian 1950       */

/*  DSS stuff  */
#define NUM_PLATE_COEFFS 20

#define MAGIC_NUMBER (unsigned int) 2076765342

#define VERIFY_AP(ap) if (ap == NULL) \
{fprintf (stderr, "NULL astro context passed\n"); \
 a_prog_bug (function_name); } \
if (ap->magic_number != MAGIC_NUMBER) \
{fprintf (stderr, "Invalid astro context object\n"); \
 a_prog_bug (function_name); }


/*  Structure declarations follow  */

struct sky_coord_type
{
    char dim_name[STRING_LENGTH];
    double reference;        /*  Degrees  (CRVALn) in degrees               */
    double ref_pos;          /*  Pixel position of renference (CRPIXn - 1)  */
    double increment;        /*  Increment between pixels (CDELTn)          */
    double sin_ref;
    double cos_ref;
};

struct velocity_coord_type
{
    char dim_name[STRING_LENGTH];
    unsigned int type;
    double reference;        /*  Reference pixel (CRVALn)                   */
    double ref_pos;          /*  Pixel position of renference (CRPIXn - 1)  */
    double increment;        /*  Increment between pixels (CDELTn)          */
    double restfreq;         /*  Rest frequency (Hz)                        */
};

struct linear_coord_type
{
    char dim_name[STRING_LENGTH];
    double reference;        /*  Reference pixel (CRVALn)                   */
    double ref_pos;          /*  Pixel position of renference (CRPIXn - 1)  */
    double increment;        /*  Increment between pixels (CDELTn)          */
    struct linear_coord_type *next;
};

struct extra_type
{
    char *ctype;
    double refval;
    struct extra_type *next;
};

struct dss_plate_type  /*  Digital Sky Survey plate solution  */
{
    double corner_pixel_x1;            /*  Pixels                            */
    double corner_pixel_y1;            /*  Pixels                            */
    double scale;                      /*  arcsec/mm                         */
    double pixel_size_x;               /*  Microns                           */
    double pixel_size_y;               /*  Microns                           */
    double centre_ra;                  /*  Degrees                           */
    double centre_dec;                 /*  Degrees                           */
    double centre_off_x;               /*  Microns                           */
    double centre_off_y;               /*  Microns                           */
    unsigned int num_amdx;
    double amdx[NUM_PLATE_COEFFS];     /*  Plate solution Xi co-efficients   */
    unsigned int num_amdy;
    double amdy[NUM_PLATE_COEFFS];     /*  Plate solution Eta co-efficients  */
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
    struct dss_plate_type *dss;
    /*  Velocity axis  */
    struct velocity_coord_type vel;
    /*  Linear co-ordinate axes  */
    struct linear_coord_type *linear_axes;
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
STATIC_FUNCTION (KwcsAstro create_new, () );
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
STATIC_FUNCTION (void transform_ra_dec_flat,
		 (KwcsAstro ap, unsigned int num_coords,
		  double *ra, double *dec, unsigned int direction) );
STATIC_FUNCTION (void transform_ra_dec_arc,
		 (KwcsAstro ap, unsigned int num_coords,
		  double *ra, double *dec, unsigned int direction) );
STATIC_FUNCTION (void transform_ra_dec_car,
		 (KwcsAstro ap, unsigned int num_coords,
		  double *ra, double *dec, unsigned int direction) );
STATIC_FUNCTION (void transform_vel,
		 (KwcsAstro ap, unsigned int num_coords, double *vel,
		  flag to_linear) );
STATIC_FUNCTION (void transform_linear,
		 (KwcsAstro ap, unsigned int num_coords,
		  CONST char *name, double *coords, flag to_pix) );
STATIC_FUNCTION (flag scan_dss_header,
		 (KwcsAstro ap, CONST packet_desc *pack_desc,
		  CONST char *packet) );
STATIC_FUNCTION (void transform_ra_dec_dss,
		 (KwcsAstro ap, unsigned int num_coords,
		  double *ra, double *dec, unsigned int direction) );
STATIC_FUNCTION (void dss_xy2radec, (KwcsAstro ap, unsigned int num_coords,
				     double *ra, double *dec) );
STATIC_FUNCTION (void dss_radec2xy, (KwcsAstro ap, unsigned int num_coords,
				     double *ra, double *dec) );


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
	fputs ("NULL pointer(s) passed\n", stderr);
	a_prog_bug (function_name);
    }
    /*  Theoretically, one should use the "NAXIS" keyword to determine how many
	axes there are to check, but since perhaps not everyone writes this
	keyword, I just keep searching until there are no more "CTYPEn"
	keywords. If "CTYPEn" does not exist, assume "CTYPEm" (where m > n)
	does not exist.
	Note that this whole scheme depends on the Karma dimension names being
	the same as those in the FITS header. For Digital Sky Survey data,
	which doesn't use "CTYPEn" and friends, this assumption is irrelevant
	*/
    if ( (key_string_value = ds_get_unique_named_string (pack_desc, packet,
							 "CTYPE1") ) == NULL )
    {
	/*  It's not a conventional header: perhaps it's DSS data?  */
	if ( !ds_get_unique_named_value (pack_desc, packet, "PLTSCALE", NULL,
					 value) ) return (NULL);
    }
    if ( ( new = create_new () ) == NULL ) return (NULL);
    equinox_val = 2000;
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
	fprintf (stderr, "Unknown equinox: %d, assuming J2000\n", equinox_val);
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
	sprintf (txt, "CTYPE%u", count);
	key_string_value = ds_get_unique_named_string (pack_desc, packet, txt);
    }
    if (new->projection == PROJ_INIT)
    {
	/*  No conventional RA-DEC projection found yet: try DSS plate soln. */
	if ( !scan_dss_header (new, pack_desc, packet) )
	{
	    m_free ( (char *) new );
	    return (NULL);
	}
    }
    if (new->projection == PROJ_INIT) new->projection = PROJ_UNKNOWN;
    if ( (new->projection == PROJ_UNKNOWN) &&
	 (new->vel.type == VELOCITY_UNKNOWN) && (new->linear_axes == NULL) )
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
    struct linear_coord_type *curr, *next;
    static char function_name[] = "wcs_astro_destroy";

    VERIFY_AP (ap);
    for (curr = ap->linear_axes; curr != NULL; curr = next)
    {
	next = curr->next;
	m_free ( (char *) curr );
    }
    m_clear ( (char *) ap, sizeof *ap );
    m_free ( (char *) ap );
}   /*  End Function wcs_astro_destroy  */

/*PUBLIC_FUNCTION*/
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

/*PUBLIC_FUNCTION*/
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
	transform_vel (ap, num_coords, vel, vel_to_linear);
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
	    fprintf (stderr, "%s: RA information missing\n", function_name);
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
	    fprintf (stderr, "%s: DEC information missing\n", function_name);
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
	    fprintf (stderr, "%s: bad rotation\n", function_name);
	    return;
	}
    }
    else if (ra_to_linear && !dec_to_linear)
    {
	if (ap->cos_rotation == 0.0)
	{
	    fprintf (stderr, "%s: bad rotation\n", function_name);
	    return;
	}
	direction = DIR_AYtoXD;
    }
    else direction = DIR_ADtoXY;
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
	transform_ra_dec_flat (ap, num_coords, ra, dec, direction);
	break;
      case PROJ_ARC:
	transform_ra_dec_arc (ap, num_coords, ra, dec, direction);
	break;
      case PROJ_CAR:
	transform_ra_dec_car (ap, num_coords, ra, dec, direction);
	break;
      case PROJ_DSS:
	transform_ra_dec_dss (ap, num_coords, ra, dec, direction);
	break;
      default:
	fprintf (stderr, "Unknown projection: %u\n", ap->projection);
	break;
    }
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
    transform_linear (ap, num_coords, name0, coords0, to_lin1);
    transform_linear (ap, num_coords, name1, coords1, to_lin1);
    transform_linear (ap, num_coords, name2, coords2, to_lin1);
}   /*  End Function wcs_astro_transform3  */

/*PUBLIC_FUNCTION*/
void wcs_astro_format_ra (char *string, double ra)
/*  [SUMMARY] Format a Right Ascension value into a string.
    <string> The string to write to. Sufficient storage must be available.
    <ra> The Right Ascension value in degrees.
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
    sprintf (string, "%.2dh %.2dm %5.2fs",
		    (int) hours, (int) minutes, seconds);
}   /*  End Function wcs_astro_format_ra  */

/*PUBLIC_FUNCTION*/
void wcs_astro_format_dec (char *string, double dec)
/*  [SUMMARY] Format a Declination value into a string.
    <string> The string to write to. Sufficient storage must be available.
    <dec> The Declination value in degrees.
    [RETURNS] Nothing.
*/
{
    flag negative = FALSE;
    double degrees, minutes, seconds;

    if (dec < 0.0)
    {
	dec = fabs (dec);
	negative = TRUE;
    }
    degrees = floor (dec);
    dec -= fabs (degrees);
    dec *= 60.0;
    minutes = floor (dec);
    dec -= minutes;
    dec *= 60.0;
    seconds = dec;
    if (negative) sprintf (string, "-%.2dd %.2dm %5.2fs",
				  (int) degrees, (int) minutes, seconds);
    else sprintf (string, "%.2dd %.2dm %5.2fs",
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
      case VELOCITY_RAWVEL:
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
/*  [SUMMARY] Format a value. No transformation is applied.
    <ap> The KwcsAstro object.
    <dim_name> The name of the dimension.
    <string> The string to write to.
    <value> The value.
    [RETURNS] Nothing.
*/
{
    int str_len;
    char stokes_char;
    char txt[STRING_LENGTH];
    static char function_name[] = "wcs_astro_format";

    VERIFY_AP (ap);
    str_len = strlen (dim_name);
    if (strcmp (dim_name, ap->ra.dim_name) == 0)
    {
	wcs_astro_format_ra (txt, value);
	sprintf (string, "Ra %s", txt);
	return;
    }
    if (strcmp (dim_name, ap->dec.dim_name) == 0)
    {
	wcs_astro_format_dec (txt, value);
	sprintf (string, "Dec %s", txt);
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
    else if ( (strcmp (dim_name, "ANGLE") == 0) ||
	      (strcmp (dim_name + str_len - 5, "(deg)") == 0) )
    {
	/*  Offset angle  */
	wcs_astro_format_dec (txt, value);
	sprintf (string, "Offset %s", txt);
	return;
    }
    /*  Don't know how to deal with this one  */
    sprintf (string, "%e %s", value, dim_name);
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
	strcat (string, txt);
	if (curr->next != NULL) strcat (string, "  ");
    }
}   /*  End Function wcs_astro_format_extra  */

/*PUBLIC_FUNCTION*/
void wcs_astro_format_all (KwcsAstro ap, char coord_string[STRING_LENGTH],
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
    double radec_value, value;
    double *ra;
    double *dec;
    double *vel;
    CONST char *unspec_name = NULL;
    struct extra_type *curr;
    char txt[STRING_LENGTH];
    static char function_name[] = "wcs_astro_format_all";

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
	    fprintf (stderr, "%s: missing DEC information\n", function_name);
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
	    fprintf (stderr, "%s: missing RA information\n", function_name);
	    return;
	}
	dec = &radec_value;
    }
    wcs_astro_transform (ap, 1, ra, FALSE, dec, FALSE, vel, FALSE,
			 num_restr, restr_names, restr_values);
    transform_linear (ap, 1, name0, &coord0, FALSE);
    wcs_astro_format (ap, name0, coord_string, coord0);
    if (name1 != NULL)
    {
	transform_linear (ap, 1, name1, &coord1, FALSE);
	wcs_astro_format (ap, name1, txt, coord1);
	strcat (coord_string, "  ");
	strcat (coord_string, txt);
    }
    if (name2 != NULL)
    {
	transform_linear (ap, 1, name2, &coord2, FALSE);
	wcs_astro_format (ap, name2, txt, coord2);
	strcat (coord_string, "  ");
	strcat (coord_string, txt);
    }
    /*  Add any restriction information  */
    for (count = 0; count < num_restr; ++count)
    {
	if (unspec_name == restr_names[count])
	{
	    value = radec_value;
	}
	else
	{
	    value = restr_values[count];
	    if (strcmp (restr_names[count], ap->vel.dim_name) == 0)
	    {
		transform_vel (ap, 1, &value, FALSE);
	    }
	}
	wcs_astro_format (ap, restr_names[count], txt, value);
	strcat (coord_string, "  ");
	strcat (coord_string, txt);
    }
    for (curr = ap->first_extra; curr != NULL; curr = curr->next)
    {
	if (unspec_name == curr->ctype)
	{
	    wcs_astro_format (ap, curr->ctype, txt, radec_value);
	}
	else wcs_astro_format (ap, curr->ctype, txt, curr->refval);
	strcat (other_string, txt);
	if (curr->next != NULL) strcat (other_string, "  ");
    }
}   /*  End Function wcs_astro_format_all  */

/*EXPERIMENTAL_FUNCTION*/
flag wcs_astro_get_preferred_units (char new_units[STRING_LENGTH],
				    char format_str[STRING_LENGTH],
				    double *scale, CONST char *old_units)
/*  [SUMMARY] Get preferred units for data.
    <new_units> The new units string is written here.
    <format_str> The format string is written here.
    <scale> The scale value to convert to the new units will be written here.
    <old_units> The old units string.
    [RETURNS] TRUE if preferred units were found, else FALSE.
*/
{
    if (strncmp (old_units, "M/S", 3) == 0)
    {
	strcpy (new_units, "km/s");
	strcpy (format_str, "%.2f");
	*scale = 1e-3;
    }
    else if (strncmp (old_units, "KM/S", 4) == 0)
    {
	strcpy (new_units, "km/s");
	strcpy (format_str, "%.1f");
	*scale = 1.0;
    }
    else if (strncmp (old_units, "JY/BEAM", 7) == 0)
    {
	strcpy (new_units, "mJy/Beam");
	strcpy (format_str, "%.1f");
	*scale = 1e+3;
    }
    else if (strncmp (old_units, "FREQ", 4) == 0)
    {
	strcpy (new_units, "MHz");
	strcpy (format_str, "%.3f");
	*scale = 1e-6;
    }
    else if (strcmp (old_units, "HZ") == 0)
    {
	strcpy (new_units, "MHz");
	strcpy (format_str, "%.3f");
	*scale = 1e-6;
    }
    else if (strncmp (old_units, "FELO", 4) == 0)
    {
	strcpy (new_units, "km/s");
	strcpy (format_str, "%.2f");
	*scale = 1e-3;
    }
    else if (strncmp (old_units, "VELO", 4) == 0)
    {
	strcpy (new_units, "km/s");
	strcpy (format_str, "%.2f");
	*scale = 1e-3;
    }
    else return (FALSE);
    return (TRUE);
}   /*  End Function wcs_astro_get_preferred_units  */


/*  Private functions follow  */

static KwcsAstro create_new ()
/*  [SUMMARY] Create new KwcsAstro object.
    [RETURNS] A KwcsAstro object on success, else NULL.
*/
{
    KwcsAstro new;
    static char function_name[] = "__wcs_astro_create_new";

    if ( ( new = (KwcsAstro) m_alloc (sizeof *new) ) == NULL )
    {
	m_error_notify (function_name, "KwcsAstro object");
	return (NULL);
    }
    /*  Initialise  */
    new->projection = PROJ_INIT;
    new->dss = NULL;
    new->sin_rotation = TOOBIG;
    new->cos_rotation = TOOBIG;
    new->equinox = EQUINOX_J2000;
    new->vel.type = VELOCITY_UNKNOWN;
    new->linear_axes = NULL;
    new->first_extra = NULL;
    new->radec_buffer = NULL;
    new->radec_buf_len = 0;
    return (new);
}   /*  End Function create_new  */

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
    struct velocity_coord_type *vel = NULL;
    struct linear_coord_type *linear = NULL;
    char *encls_desc;
    char txt[STRING_LENGTH];
    double crval[2], crpix[2], cdelt[2];
    double value[2];
    static char function_name[] = "__wcs_astro_process_axis";

    str_len = strlen (axis_name);
    sprintf (txt, "CRVAL%u", index);
    if ( !ds_get_unique_named_value (pack_desc, packet, txt, NULL, crval) )
    {
	return (TRUE);
    }
    sprintf (txt, "CRPIX%u", index);
    if ( !ds_get_unique_named_value (pack_desc, packet, txt, NULL, crpix) )
    {
	return (TRUE);
    }
    sprintf (txt, "CDELT%u", index);
    if ( !ds_get_unique_named_value (pack_desc, packet, txt, NULL, cdelt) )
    {
	return (TRUE);
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
    else if (strncmp (axis_name, "FREQ", 4) == 0)
    {
	/*  Frequency  */
	vel = &ap->vel;
	ap->vel.type = VELOCITY_FREQ;
    }
    else if (strncmp (axis_name, "VELO", 4) == 0)
    {
	/*  Velocity  */
	vel = &ap->vel;
	ap->vel.type = VELOCITY_VELO;
    }
    else if (strncmp (axis_name, "FELO", 4) == 0)
    {
	/*  Velocity  */
	vel = &ap->vel;
	ap->vel.type = VELOCITY_FELO;
    }
    else if ( (strcmp (axis_name, "ANGLE") == 0) ||
	      (strcmp (axis_name + str_len - 5, "(deg)") == 0) )
    {
	/*  Offset angle  */
	if ( ( linear = (struct linear_coord_type *)
	       m_alloc (sizeof *linear) ) == NULL )
	{
	    m_abort (function_name, "linear axis");
	}
    }
    if (sky != NULL)
    {
	/*  Right ascension or declination  */
	sky->reference = crval[0];
	sky->ref_pos = crpix[0] - 1.0;
	sky->increment = cdelt[0];
	sky->cos_ref = cos (sky->reference * deg_to_rad);
	sky->sin_ref = sin (sky->reference * deg_to_rad);
	/*  Determine the projection system  */
	if (strcmp (axis_name + str_len - 4, "-SIN") == 0) proj = PROJ_SIN;
	else if (strcmp (axis_name + str_len -4, "-TAN") == 0) proj = PROJ_TAN;
	else if (strcmp (axis_name + str_len -4, "-NCP") == 0) proj = PROJ_NCP;
	else if (strcmp (axis_name + str_len -4, "-FLT") == 0) proj = PROJ_FLT;
	else if (strcmp (axis_name + str_len -4, "-ARC") == 0) proj = PROJ_ARC;
	else if (strcmp (axis_name + str_len -4, "-CAR") == 0) proj = PROJ_CAR;
	else proj = PROJ_UNKNOWN;
	if (ap->projection == PROJ_INIT)
	{
	    ap->projection = proj;
	}
	else if (ap->projection != proj)
	{
	    fputs ("Projections do not match\n", stderr);
	    return (FALSE);
	}
	sprintf (txt, "CROTA%u", index);
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
	strcpy (sky->dim_name, axis_name);
    }
    if (vel != NULL)
    {
	vel->reference = crval[0];
	vel->ref_pos = crpix[0] - 1.0;
	vel->increment = cdelt[0];
	strcpy (ap->vel.dim_name, axis_name);
	if ( !ds_get_unique_named_value (pack_desc, packet, "RESTFREQ", NULL,
					 value) )
	{
	    vel->restfreq = TOOBIG;
	    if (ap->vel.type == VELOCITY_VELO) ap->vel.type = VELOCITY_RAWVEL;
	}
	else vel->restfreq = value[0];
    }
    if (linear != NULL)
    {
	linear->reference = crval[0];
	linear->ref_pos = crpix[0] - 1.0;
	linear->increment = cdelt[0];
	strcpy (linear->dim_name, axis_name);
	linear->next = ap->linear_axes;
	ap->linear_axes = linear;
    }
    /*  Check if this axis is a dimension  */
    /*  Search for specified name first  */
    switch ( ds_f_name_in_packet (pack_desc, axis_name, &encls_desc,
				  &item_index) )
    {
      case IDENT_NOT_FOUND:
	add_extra (ap, axis_name, crval[0]);
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

    if (direction == DIR_ADtoXY)
    {
	/*  Convert from RA,DEC to x,y  */
	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert degrees to offset radians  */
	    diff_alpha = (ra[count] - ap->ra.reference) * deg_to_rad;
	    delta = dec[count] * deg_to_rad;
	    cos_delta = cos (delta);
	    l = cos_delta * sin (diff_alpha);
	    m = sin (delta) * ap->dec.cos_ref -
		cos_delta * ap->dec.sin_ref * cos (diff_alpha);
	    /*  Rotate  */
	    x = l * ap->cos_rotation + m * ap->sin_rotation;
	    y = m * ap->cos_rotation - l * ap->sin_rotation;
	    /*  Convert to pixel positions  */
	    ra[count] = ap->ra.ref_pos + x * rad_to_deg / ap->ra.increment;
	    dec[count] = ap->dec.ref_pos + y * rad_to_deg / ap->dec.increment;
	}
    }
    else if (direction == DIR_XDtoAY)
    {
	/*  Convert from x,DEC to RA,y  */
	double b, c, s;

	for (count = 0; count < num_coords; ++count)
	{
	    alpha = ap->cos_rotation;
	    x = (ra[count] - ap->ra.ref_pos) * ap->ra.increment * deg_to_rad;
	    b = ap->dec.sin_ref * ap->sin_rotation;
	    delta = deg_to_rad * dec[count];
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
	    dec[count] = ap->dec.ref_pos + rad_to_deg * (alpha - b - c) /
		ap->dec.increment;
	}
    }
    else if (direction == DIR_AYtoXD)
    {
	/*  Convert from RA,y to x,DEC  */
	double b, c;

	for (count = 0; count < num_coords; ++count)
	{
    	    diff_alpha = deg_to_rad * (ra[count] - ap->ra.reference);
	    cos_diff_alpha = cos (diff_alpha);
	    sin_diff_alpha = sin (diff_alpha);
	    y = deg_to_rad * (dec[count] - ap->dec.ref_pos) *ap->dec.increment;
	    alpha = ap->dec.cos_ref * ap->cos_rotation;
	    b = ap->dec.sin_ref * cos_diff_alpha * ap->cos_rotation +
		sin_diff_alpha * ap->sin_rotation;
	    delta = atan (b / alpha) + asin ( y / sqrt (alpha *alpha + b *b) );
	    cos_delta = cos (delta);
	    sin_delta = sin (delta);
	    alpha = cos_delta * sin_diff_alpha * ap->cos_rotation;
	    b = sin_delta * ap->dec.cos_ref * ap->sin_rotation;
	    c = cos_delta * ap->dec.sin_ref * cos_diff_alpha *ap->sin_rotation;
	    ra[count] = ap->ra.ref_pos + rad_to_deg * (alpha + b - c) /
		ap->ra.increment;
	    dec[count] = rad_to_deg * delta;
	}
    }
    else if (direction == DIR_XYtoAD)
    {
	/*  Convert from x,y to RA,DEC  */
	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert pixels to offset radians  */
	    x = (ra[count] - ap->ra.ref_pos) * ap->ra.increment * deg_to_rad;
	    y = (dec[count] - ap->dec.ref_pos) * ap->dec.increment *deg_to_rad;
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

    if (direction == DIR_ADtoXY)
    {
	/*  Convert from RA,DEC to x,y  */
	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert degrees to offset radians  */
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
	    /*  Convert to pixel positions  */
	    ra[count] = ap->ra.ref_pos + x * rad_to_deg / ap->ra.increment;
	    dec[count] = ap->dec.ref_pos + y * rad_to_deg / ap->dec.increment;
	}
    }
    else if (direction == DIR_XDtoAY)
    {
	/*  Convert from x,DEC to RA,y  */
	double b, c, s, tan_delta;

	for (count = 0; count < num_coords; ++count)
	{
	    x = (ra[count] - ap->ra.ref_pos) * ap->ra.increment * deg_to_rad;
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
	    dec[count] = ap->dec.ref_pos +
		rad_to_deg * (alpha - b - c) / (s + tmp) / ap->dec.increment;
	}
    }
    else if (direction == DIR_AYtoXD)
    {
	/*  Convert from RA,y to x,DEC  */
	double b, c, s, tan_delta;

	for (count = 0; count < num_coords; ++count)
	{
	    diff_alpha = deg_to_rad * (ra[count] - ap->ra.reference);
	    cos_diff_alpha = cos (diff_alpha);
	    sin_diff_alpha = sin (diff_alpha);
	    y = deg_to_rad * (dec[count] - ap->dec.ref_pos) *ap->dec.increment;
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
	    ra[count] = ap->ra.ref_pos +
		rad_to_deg * (alpha + b - c) / (s + tmp) / ap->ra.increment;
	    dec[count] = rad_to_deg * atan (tan_delta);
	}
    }
    else if (direction == DIR_XYtoAD)
    {
	/*  Convert from x,y to RA,DEC  */
	double s;

	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert pixels to offset radians  */
	    x = (ra[count] - ap->ra.ref_pos) * ap->ra.increment * deg_to_rad;
	    y = (dec[count] - ap->dec.ref_pos) * ap->dec.increment *deg_to_rad;
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

    if (direction == DIR_ADtoXY)
    {
	/*  Convert from RA,DEC to x,y  */
	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert degrees to offset radians  */
	    diff_alpha = (ra[count] - ap->ra.reference) * deg_to_rad;
	    delta = dec[count] * deg_to_rad;
	    cos_delta = cos (delta);
	    l = cos_delta * sin (diff_alpha);
	    m = ( ap->dec.cos_ref - cos_delta * cos (diff_alpha) ) /
		ap->dec.sin_ref;
	    /*  Rotate  */
	    x = l * ap->cos_rotation + m * ap->sin_rotation;
	    y = m * ap->cos_rotation - l * ap->sin_rotation;
	    /*  Convert to pixel positions  */
	    ra[count] = ap->ra.ref_pos + x * rad_to_deg / ap->ra.increment;
	    dec[count] = ap->dec.ref_pos + y * rad_to_deg / ap->dec.increment;
	}
    }
    else if (direction == DIR_XDtoAY)
    {
	/*  Convert from x,DEC to RA,y  */
	double b, c, s;

	for (count = 0; count < num_coords; ++count)
	{
	    x = (ra[count] - ap->ra.ref_pos) * ap->ra.increment * deg_to_rad;
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
	    dec[count] = ap->dec.ref_pos + rad_to_deg * (alpha - b - c) /
		ap->dec.increment;
	}
    }
    else if (direction == DIR_AYtoXD)
    {
	/*  Convert from RA,y to x,DEC  */
	double b, c, s, q;

	for (count = 0; count < num_coords; ++count)
	{
	    diff_alpha = deg_to_rad * (ra[count] - ap->ra.reference);
	    y = deg_to_rad * (dec[count] - ap->dec.ref_pos) *ap->dec.increment;
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
	    ra[count] = ap->ra.ref_pos + rad_to_deg * (alpha + b - c) /
		ap->ra.increment;
	    dec[count] = rad_to_deg * delta;
	}
    }
    else if (direction == DIR_XYtoAD)
    {
	/*  Convert from x,y to RA,DEC  */
	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert pixels to offset radians  */
	    x = (ra[count] - ap->ra.ref_pos) * ap->ra.increment * deg_to_rad;
	    y = (dec[count] - ap->dec.ref_pos) * ap->dec.increment *deg_to_rad;
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

static void transform_ra_dec_flat (KwcsAstro ap, unsigned int num_coords,
				   double *ra, double *dec,
				   unsigned int direction)
/*  [SUMMARY] Flat projection of co-ordinates.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    <dec> A pointer to the declination values. These will be modified.
    <direction> The direction of the transform.
    [RETURNS] Nothing.
*/
{
    unsigned int count;

    if (direction == DIR_ADtoXY)
    {
	/*  Convert from RA,DEC to x,y  */
	for (count = 0; count < num_coords; ++count)
	{
	    ra[count] = ap->ra.ref_pos + (ra[count] - ap->ra.reference) /
		ap->ra.increment;
	    dec[count] = ap->dec.ref_pos + (dec[count] - ap->dec.reference) /
		ap->dec.increment;
	}
    }
    else if (direction == DIR_XDtoAY)
    {
	/*  Convert from x,DEC to RA,y  */
	for (count = 0; count < num_coords; ++count)
	{
	    ra[count] = ap->ra.reference + (ra[count] - ap->ra.ref_pos) *
		ap->ra.increment;
	    dec[count] = ap->dec.ref_pos + (dec[count] - ap->dec.reference) /
		ap->dec.increment;
	}
    }
    else if (direction == DIR_AYtoXD)
    {
	/*  Convert from RA,y to x,DEC  */
	for (count = 0; count < num_coords; ++count)
	{
	    ra[count] = ap->ra.ref_pos + (ra[count] - ap->ra.reference) /
		ap->ra.increment;
	    dec[count] = ap->dec.reference + (dec[count] - ap->dec.ref_pos) *
		ap->dec.increment;
	}
    }
    else if (direction == DIR_XYtoAD)
    {
	/*  Convert from x,y to RA,DEC  */
	for (count = 0; count < num_coords; ++count)
	{
	    ra[count] = ap->ra.reference + (ra[count] - ap->ra.ref_pos) *
		ap->ra.increment;
	    dec[count] = ap->dec.reference + (dec[count] - ap->dec.ref_pos) *
		ap->dec.increment;
	}
    }
}   /*  End Function transform_ra_dec_flat  */

static void transform_ra_dec_arc (KwcsAstro ap, unsigned int num_coords,
				  double *ra, double *dec,
				  unsigned int direction)
/*  [SUMMARY] Rectangular projection of co-ordinates.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    <dec> A pointer to the declination values. These will be modified.
    <direction> The direction of the transform.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double deg_to_rad = PION180;
    double rad_to_deg = 180.0 / PI;
    double l, m, x, y;
    double delta, sin_delta, cos_delta;
    double diff_alpha, sin_diff_alpha, cos_diff_alpha;
    double one = 1.0;

    if (direction == DIR_ADtoXY)
    {
	/*  Convert from RA,DEC to x,y  */
	double a, s, t;

	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert degrees to offset radians  */
	    diff_alpha = (ra[count] - ap->ra.reference) * deg_to_rad;
	    delta = dec[count] * deg_to_rad;
	    cos_diff_alpha = cos (diff_alpha);
	    sin_diff_alpha = sin (diff_alpha);
	    cos_delta = cos (delta);
	    sin_delta = sin (delta);
	    s = sin_delta * ap->dec.sin_ref +
		cos_delta * ap->dec.cos_ref * cos_diff_alpha;
	    if (s > one) s = one;
	    else if (s < -one) s = -one;
	    t = acos (s);
	    if (t == 0.0) a = one;
	    else a = t / sin (t);
	    l = a * cos_delta * sin_diff_alpha;
	    m = a * (sin_delta * ap->dec.cos_ref -
		     cos_delta * ap->dec.sin_ref * cos_diff_alpha);
	    /*  Rotate  */
	    x = l * ap->cos_rotation + m * ap->sin_rotation;
	    y = m * ap->cos_rotation - l * ap->sin_rotation;
	    /*  Convert to pixel positions  */
	    ra[count] = ap->ra.ref_pos + x * rad_to_deg / ap->ra.increment;
	    dec[count] = ap->dec.ref_pos + y * rad_to_deg / ap->dec.increment;
	}
    }
    else if (direction == DIR_XDtoAY)
    {
	/*  Convert from x,DEC to RA,y  */
	double a, b, f, t, yold;

	if (ap->cos_rotation == 0.0) return;
	for (count = 0; count < num_coords; ++count)
	{
	    x = (ra[count] - ap->ra.ref_pos) * ap->ra.increment * deg_to_rad;
	    delta = deg_to_rad * dec[count];
	    cos_delta = cos (delta);
	    sin_delta = sin (delta);
	    a = x * ap->sin_rotation * ap->dec.cos_ref;
	    b = ap->cos_rotation * ap->dec.cos_ref;
	    y = 0.0;
	    do
	    {
		yold = y;
		t = sqrt (x * x + y * y);
		if (t == 0.0) f = one;
		else f = sin (t) / t;
		y = (sin_delta - ap->dec.sin_ref * cos (t) - a * f) / b / f;
	    }
	    while (fabs (y - yold) > 1e-10);
	    t = sqrt (x * x + y * y);
	    if (t == 0.0) f = one;
	    else f = sin (t) / t;
	    diff_alpha = asin (f * (x * ap->cos_rotation -
				    y * ap->sin_rotation) / cos_delta);
	    ra[count] = ap->ra.reference + rad_to_deg * diff_alpha;
	    dec[count] = ap->dec.ref_pos + rad_to_deg * y / ap->dec.increment;
	}
    }
    else if (direction == DIR_AYtoXD)
    {
	/*  Convert from RA,y to x,DEC  */
	double a, b, c, s, t, xold;

	if (ap->cos_rotation == 0.0) return;
	for (count = 0; count < num_coords; ++count)
	{
    	    diff_alpha = deg_to_rad * (ra[count] - ap->ra.reference);
	    cos_diff_alpha = cos (diff_alpha);
	    sin_diff_alpha = sin (diff_alpha);
	    y = deg_to_rad * (dec[count] - ap->dec.ref_pos) *ap->dec.increment;
	    a = atan (ap->dec.sin_ref / ap->dec.cos_ref / cos_diff_alpha);
	    b = sqrt (one - ap->dec.cos_ref * ap->dec.cos_ref *
		      sin_diff_alpha * sin_diff_alpha);
	    x = 0.0;
	    do
	    {
		xold = x;
		t = sqrt (x * x + y * y);
		s = cos (t) / b;
		if (s > one) s = one;
		else if (s < -one) s = one;
		c = acos (s);
		if (y < 0.0) delta = a - c;
		else delta = a + c;
		x = ( y * ap->sin_rotation +
		      cos (delta) * sin_diff_alpha * t / sin (t) ) /
		    ap->cos_rotation;
	    }
	    while (fabs (x - xold) > 1e-10);
	    ra[count] = ap->ra.ref_pos + rad_to_deg * x / ap->ra.increment;
	    dec[count] = rad_to_deg * delta;
	}
    }
    else if (direction == DIR_XYtoAD)
    {
	/*  Convert from x,y to RA,DEC  */
	double a, t;

	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert pixels to offset radians  */
	    x = (ra[count] - ap->ra.ref_pos) * ap->ra.increment * deg_to_rad;
	    y = (dec[count] - ap->dec.ref_pos) * ap->dec.increment *deg_to_rad;
	    /*  Rotate  */
	    l = x * ap->cos_rotation - y * ap->sin_rotation;
	    m = y * ap->cos_rotation + x * ap->sin_rotation;
	    t = sqrt (l * l + m * m);
	    if (t == 0.0) a = one;
	    else a = sin (t) / t;
	    delta = asin ( a *m * ap->dec.cos_ref + ap->dec.sin_ref *cos (t) );
	    /*  Convert back to degrees and add to reference  */
	    ra[count] = ap->ra.reference + asin ( a * l / cos (delta) ) *
		rad_to_deg;
	    dec[count] = rad_to_deg * delta;
	}
    }
}   /*  End Function transform_ra_dec_arc  */

static void transform_ra_dec_car (KwcsAstro ap, unsigned int num_coords,
				   double *ra, double *dec,
				   unsigned int direction)
/*  [SUMMARY] Cartesian projection of co-ordinates.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    <dec> A pointer to the declination values. These will be modified.
    <direction> The direction of the transform.
    [NOTE] This projection is used by Miriad to approximate the plate solutions
    of DSS (Digital Sky Survey) images. It is better to use the plate solutions
    directly.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double l, m, x, y;
    double diff_alpha, diff_delta;

    if (direction == DIR_ADtoXY)
    {
	/*  Convert from RA,DEC to x,y  */
	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert degrees to offset degrees  */
	    diff_alpha = ra[count] - ap->ra.reference;
	    diff_delta = dec[count] - ap->dec.reference;
	    l = diff_alpha * ap->dec.cos_ref;
	    m = diff_delta;
	    /*  Rotate  */
	    x = l * ap->cos_rotation + m * ap->sin_rotation;
	    y = m * ap->cos_rotation - l * ap->sin_rotation;
	    /*  Convert to pixel positions  */
	    ra[count] = ap->ra.ref_pos + x / ap->ra.increment;
	    dec[count] = ap->dec.ref_pos + y / ap->dec.increment;
	}
    }
    else if (direction == DIR_XDtoAY)
    {
	/*  Convert from x,DEC to RA,y  */
	fprintf (stderr,
		 "x,DEC to RA,y conversion for Cartesian projection not supported yet\n");
	return;
    }
    else if (direction == DIR_AYtoXD)
    {
	/*  Convert from RA,y to x,DEC  */
	fprintf (stderr,
		 "RA,y to x,DEC conversion for Cartesian projection not supported yet\n");
	return;
    }
    else if (direction == DIR_XYtoAD)
    {
	/*  Convert from x,y to RA,DEC  */
	for (count = 0; count < num_coords; ++count)
	{
	    /*  Convert pixels to offset degrees  */
	    x = (ra[count] - ap->ra.ref_pos) * ap->ra.increment;
	    y = (dec[count] - ap->dec.ref_pos) * ap->dec.increment;
	    /*  Rotate  */
	    l = x * ap->cos_rotation - y * ap->sin_rotation;
	    m = y * ap->cos_rotation + x * ap->sin_rotation;
	    /*  Transform and add to reference  */
	    ra[count] = ap->ra.reference + l / ap->dec.cos_ref;
	    dec[count] = ap->dec.reference + m;
	}
    }
}   /*  End Function transform_ra_dec_car  */

static void transform_vel (KwcsAstro ap, unsigned int num_coords,
			   double *vel, flag to_linear)
/*  [SUMMARY] Velocity/frequency transformation.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <vel> A pointer to the velocity/frequency values. These will be modified.
    <to_linear> If TRUE the values are transformed to linear
    co-ordinates, else they are transformed to non-linear co-ordinates.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double n;
    double c = 299792458.0;

    switch (ap->vel.type)
    {
      case VELOCITY_UNKNOWN:
	break;
      case VELOCITY_FELO:
	for (count = 0; count < num_coords; ++count)
	{
	    if (to_linear)
	    {
		/*  TODO: this should really be a non-linear transformation  */
		vel[count] = ap->vel.ref_pos + (vel[count] - ap->vel.reference)
		    / ap->vel.increment;
	    }
	    else
	    {
		n = vel[count] - ap->vel.ref_pos;
		vel[count] = 1.0 / ( ( n / (ap->vel.reference + ap->vel.increment + c) ) + (1.0 - n) / (ap->vel.reference + c) ) - c;
	    }
	}
	break;
      case VELOCITY_VELO:
      case VELOCITY_FREQ:
      case VELOCITY_RAWVEL:
	for (count = 0; count < num_coords; ++count)
	{
	    if (to_linear)
	    {
		vel[count] = ap->vel.ref_pos + (vel[count] - ap->vel.reference)
		    / ap->vel.increment;
	    }
	    else
	    {
		vel[count] = ap->vel.reference + (vel[count] - ap->vel.ref_pos)
		    * ap->vel.increment;
	    }
	}
	break;
      default:
	break;
    }
}   /*  End Function transform_vel  */

static void transform_linear (KwcsAstro ap, unsigned int num_coords,
			      CONST char *name, double *coords, flag to_pix)
/*  [SUMMARY] Linear transformation.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <name> The axis name. This may be NULL.
    <coords> A pointer to the co-ordinate values. These will be modified.
    <to_pix> If TRUE the values are transformed to pixel
    co-ordinates, else they are transformed to world co-ordinates.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    struct linear_coord_type *curr;
    struct linear_coord_type *found = NULL;

    if (name == NULL) return;
    /*  First search for matching linear co-ordinate axis  */
    for (curr = ap->linear_axes; (curr != NULL) && (found == NULL);
	 curr = curr->next)
    {
	if (strcmp (curr->dim_name, name) == 0) found = curr;
    }
    if (found == NULL) return;
    for (count = 0; count < num_coords; ++count)
    {
	if (to_pix)
	{
	    coords[count] = found->ref_pos + (coords[count] - found->reference)
		/ found->increment;
	}
	else
	{
	    coords[count] = found->reference + (coords[count] - found->ref_pos)
		* found->increment;
	}
    }
}   /*  End Function transform_linear  */

static flag scan_dss_header (KwcsAstro ap, CONST packet_desc *pack_desc,
			     CONST char *packet)
/*  [SUMMARY] Attempt to scan DSS header information.
    <ap> The KwcsAstro object.
    <pack_desc> The packet descriptor containing the FITS-style keywords.
    <packet> The packet data containing the FITS-stype keyword data.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag scan;
    double pltdecsn;
    struct dss_plate_type *new;
    char *ptr;
    char txt[STRING_LENGTH];
    double cnpix1[2], cnpix2[2], pltscale[2], xpixelsz[2], ypixelsz[2];
    double pltrah[2], pltram[2], pltras[2], pltdecd[2], pltdecm[2], pltdecs[2];
    double ppo3[2], ppo6[2];
    double value[2];
    static char function_name[] = "scan_dss_header";

    if ( !ds_get_unique_named_value (pack_desc, packet, "CNPIX1", NULL,
				     cnpix1) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "CNPIX2", NULL,
				     cnpix2) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "PLTSCALE", NULL,
				     pltscale) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "XPIXELSZ", NULL,
				     xpixelsz) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "YPIXELSZ", NULL,
				     ypixelsz) ) return (TRUE);
    /*  Some fool decided it was better to put the plate centre RA and DEC
	values into seven (!) separate header keywords. Unbelievable!  */
    if ( !ds_get_unique_named_value (pack_desc, packet, "PLTRAH", NULL,
				     pltrah) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "PLTRAM", NULL,
				     pltram) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "PLTRAS", NULL,
				     pltras) ) return (TRUE);
    /*  And to hammer home the stupidity, a string is used to give the sign  */
    if ( ( ptr = ds_get_unique_named_string (pack_desc, packet, "PLTDECSN") )
	 == NULL ) return (TRUE);
    pltdecsn = (*ptr == '-') ? -1.0 : 1.0;
    m_free (ptr);
    if ( !ds_get_unique_named_value (pack_desc, packet, "PLTDECD", NULL,
				     pltdecd) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "PLTDECM", NULL,
				     pltdecm) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "PLTDECS", NULL,
				     pltdecs) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "PPO3", NULL,
				     ppo3) ) return (TRUE);
    if ( !ds_get_unique_named_value (pack_desc, packet, "PPO6", NULL,
				     ppo6) ) return (TRUE);
    /*  Now have all the "essentials" (except for some number of Xi and Eta
	co-efficients  */
    if ( ( new = (struct dss_plate_type *) m_alloc (sizeof *new) ) == NULL )
    {
	m_error_notify (function_name, "plate solution structure");
	return (FALSE);
    }
    new->corner_pixel_x1 = cnpix1[0];
    new->corner_pixel_y1 = cnpix2[0];
    new->scale = pltscale[0];
    new->pixel_size_x = xpixelsz[0];
    new->pixel_size_y = ypixelsz[0];
    /*  Now have to convert those seven lovely RA,DEC values into one RA and
	one DEC value. I like having to do all this work  */
    new->centre_ra = (pltrah[0] + pltram[0] / 60.0 + pltras[0] / 3600.0) *15.0;
    new->centre_dec = pltdecd[0] + pltdecm[0] / 60.0 + pltdecs[0] / 3600.0;
    new->centre_dec *= pltdecsn;
    new->centre_off_x = ppo3[0];
    new->centre_off_y = ppo6[0];
    /*  Now get the Xi co-efficients  */
    for (scan = TRUE, new->num_amdx = 0; scan; )
    {
	sprintf (txt, "AMDX%u", new->num_amdx + 1);
	if ( ds_get_unique_named_value (pack_desc, packet, txt, NULL, value) )
	{
	    new->amdx[new->num_amdx++] = value[0];
	}
	else scan = FALSE;
    }
    /*  Now get the Eta co-efficients  */
    for (scan = TRUE, new->num_amdy = 0; scan; )
    {
	sprintf (txt, "AMDY%u", new->num_amdy + 1);
	if ( ds_get_unique_named_value (pack_desc, packet, txt, NULL, value) )
	{
	    new->amdy[new->num_amdy++] = value[0];
	}
	else scan = FALSE;
    }
    ap->dss = new;
    ap->projection = PROJ_DSS;
    strcpy (ap->ra.dim_name, "Axis 1");
    strcpy (ap->dec.dim_name, "Axis 0");
    return (TRUE);
}   /*  End Function scan_dss_header  */

static void transform_ra_dec_dss (KwcsAstro ap, unsigned int num_coords,
				   double *ra, double *dec,
				   unsigned int direction)
/*  [SUMMARY] DSS projection of co-ordinates.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    <dec> A pointer to the declination values. These will be modified.
    <direction> The direction of the transform.
    [RETURNS] Nothing.
*/
{
    if (direction == DIR_ADtoXY)
    {
	/*  Convert from RA,DEC to x,y  */
	dss_radec2xy (ap, num_coords, ra, dec);
    }
    else if (direction == DIR_XDtoAY)
    {
	/*  Convert from x,DEC to RA,y  */
	fprintf (stderr,
		 "x,DEC to RA,y conversion for DSS projection not supported yet\n");
	return;
    }
    else if (direction == DIR_AYtoXD)
    {
	/*  Convert from RA,y to x,DEC  */
	fprintf (stderr,
		 "RA,y to x,DEC conversion for DSS projection not supported yet\n");
	return;
    }
    else if (direction == DIR_XYtoAD)
    {
	/*  Convert from x,y to RA,DEC  */
	dss_xy2radec (ap, num_coords, ra, dec);
    }
}   /*  End Function transform_ra_dec_dss  */

static void dss_xy2radec (KwcsAstro ap, unsigned int num_coords,
			  double *ra, double *dec)
/*  [SUMMARY] Convert x,y (pixels) on a DSS plate to RA,DEC.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    <dec> A pointer to the declination values. These will be modified.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double x, y, xx, yy, xy, xxPyy;
    double xi, eta;
    double arcsec2rad = PION180 / 3600.0;
    double cos_dec, tan_dec, f;
    double *a, *b;

    a = ap->dss->amdx - 1;
    b = ap->dss->amdy - 1;
    cos_dec = cos (ap->dss->centre_dec * PION180);
    tan_dec = tan (ap->dss->centre_dec * PION180);
    for (count = 0; count < num_coords; ++count)
    {
	/*  Convert pixel positions to offset from plate centre in mm  */
	x = 1e-3 * (ap->dss->centre_off_x -
		    (ra[count] + ap->dss->corner_pixel_x1) *
		    ap->dss->pixel_size_x);
	/*  Y is flipped!  */
	y = -1e-3 * (ap->dss->centre_off_y -
		     (dec[count] + ap->dss->corner_pixel_y1) *
		     ap->dss->pixel_size_y);
	xx = x * x;
	yy = y * y;
	xy = x * y;
	xxPyy = xx + yy;
	/*  Note how co-efficients start from 1, not 0. This makes it
	    easier to relate back to the equations  */
	xi = a[1] * x + a[2] * y + a[3] + a[4] * xx + a[5] * xy + a[6] * yy +
	    a[7] * xxPyy + a[8] * x * xx + a[9] * x * xy + a[10] * x * yy +
	    a[11] * y * yy + a[12] * x * xxPyy + a[13] * x * xxPyy * xxPyy;
	eta = b[1] * y + b[2] * x + b[3] + b[4] * yy + b[5] * xy +
	    b[6] * xx + b[7] * xxPyy + b[8] * y * yy + b[9] * x * yy +
	    b[10] * x * xy + b[11] * x * xx + b[12] * y * xxPyy +
	    b[13] * y * xxPyy * xxPyy;
	xi *= arcsec2rad;
	eta *= arcsec2rad;
	/*  Compute RA,DEC in J2000  */
	f = 1.0 - eta * tan_dec;
	ra[count] = atan ( (xi / cos_dec) / f ) / PION180 +
	    ap->dss->centre_ra;
	dec[count] = atan ( ( (eta + tan_dec) *
			      cos ( (ra[count] - ap->dss->centre_ra) *
				    PION180 ) ) / f ) / PION180;
    }
}   /*  End Function dss_xy2radec  */

static void dss_radec2xy (KwcsAstro ap, unsigned int num_coords,
			  double *ra, double *dec)
/*  [SUMMARY] Convert RA,DEC to x,y (pixels) on a DSS plate.
    <ap> The KwcsAstro object.
    <num_coords> The number of co-ordinates to transform.
    <ra> A pointer to the right ascension values. These will be modified.
    <dec> A pointer to the declination values. These will be modified.
    [NOTE] This routine must compute the inverse of the plate solution, which
    requires iteration, so this may be a little slow.
    [RETURNS] Nothing.
*/
{
    int iter_count;
    int max_iter = 50;
    int total_iters = 0;
    unsigned int count;
    double tolerance, z, f, cos_dec, sin_dec, cos_dec0, sin_dec0;
    double cos_dra, sin_dra;
    double xi0, eta0;
    double x, y, xx, yy, xy, xxPyy, xT2, yT2;
    double Dx = 0.0;  /*  Initialised to keep compiler happy  */
    double Dy = 0.0;  /*  Initialised to keep compiler happy  */
    double xi, eta, Dxi_Dx, Dxi_Dy, Deta_Dx, Deta_Dy, delta_xi, delta_eta;
    double *a, *b;

    a = ap->dss->amdx - 1;
    b = ap->dss->amdy - 1;
    /*  Compute tolerance (10% of pixel size)  */
    tolerance = ap->dss->pixel_size_x;
    if (tolerance > ap->dss->pixel_size_y) tolerance = ap->dss->pixel_size_y;
    tolerance *= 1e-4;
    /*  Cache a few values  */
    z = ap->dss->centre_dec * PION180;
    cos_dec0 = cos (z);
    sin_dec0 = sin (z);
    for (count = 0; count < num_coords; ++count)
    {
	/*  Convert RA, DEC to Xi, Eta co-rdinates  */
	z = dec[count] * PION180;
	cos_dec = cos (z);
	sin_dec = sin (z);
	z = (ra[count] - ap->dss->centre_ra) * PION180;
	cos_dra = cos (z);
	sin_dra = sin (z);
	f = 180.0 * 3600.0 / PI / (sin_dec * sin_dec0 +
				   cos_dec * cos_dec0 * cos_dra);
	xi0 = cos_dec * sin_dra * f;
	eta0 = (sin_dec * cos_dec0 - cos_dec * sin_dec0 * cos_dra) * f;
	/*  Begin iterating. Make a first guess of offset in mm  */
	x = xi0 / ap->dss->scale;
	y = eta0 / ap->dss->scale;
	for (iter_count = 0, Dx = 9.0 * tolerance, Dy = 9.0 * tolerance;
	     (iter_count < max_iter) && (fabs (Dx) > tolerance) &&
		 (fabs (Dy) > tolerance);
	     ++iter_count)
	{
	    /*  Compute plate co-ordinates and their derivatives  */
	    /*  Note how co-efficients start from 1, not 0. This makes it
		easier to relate back to the equations  */
	    xx = x * x;
	    yy = y * y;
	    xy = x * y;
	    xxPyy = xx + yy;
	    xT2 = 2.0 * x;
	    yT2 = 2.0 * y;
	    /*  Compute Xi  */
	    xi = a[1] * x + a[2] * y + a[3] + a[4] * xx + a[5] * xy +
		a[6] * yy + a[7] * xxPyy + a[8] * x * xx + a[9] * x * xy +
		a[10] * x * yy + a[11] * y * yy + a[12] * x * xxPyy +
		a[13] * x * xxPyy * xxPyy;
	    /*  Compute derivative of Xi wrt x  */
	    Dxi_Dx = ( a[1] + a[4] * xT2 + a[5] * y + a[7] * xT2 +
		       a[8] * 3.0 * xx + a[9] * 2.0 * xy + a[10] * yy +
		       a[12] * (3.0 * xx + yy) +
		       a[13] * (5.0 * xx * xx + 6.0 * xx * yy + yy * yy) );
	    /*  Compute derivative of Xi wrt y  */
	    Dxi_Dy = ( a[2] + a[5] * x + a[6] * yT2 + a[7] * yT2 +
		       a[9] * xx + a[10] * x * yT2 + a[11] * 3.0 * yy +
		       a[12] * x * yT2 +
		       a[13] * (4.0 * xx * xy + 4.0 * xy * yy) );
	    /*  Compute Eta  */
	    eta = b[1] * y + b[2] * x + b[3] + b[4] * yy + b[5] * xy +
		b[6] * xx + b[7] * xxPyy + b[8] * y * yy + b[9] * x * yy +
		b[10] * x * xy + b[11] * x * xx + b[12] * y * xxPyy +
		b[13] * y * xxPyy * xxPyy;
	    /*  Compute derivative of Eta wrt x  */
	    Deta_Dx = ( b[2] + b[5] * y + b[6] * xT2 + b[7] * xT2 + b[9] * yy +
			b[10] * xT2 * y + b[11] * 3.0 * xx + b[12] * y * xT2 +
			b[13] * y * (4.0 * x * xx + 4.0 * x * yy) );
	    /*  Compute derivative of Eta wrt y  */
	    Deta_Dy = ( b[1] + b[4] * yT2 + b[5] * x + b[7] * yT2 +
			b[8] * 3.0 * yy + b[9] * x * yT2 + b[10] * xx +
			b[12] * (xx + 3.0 * yy) +
			b[13] * (5.0 * yy * yy + 6.0 * yy * xx + xx * xx) );
	    /*  Compute error terms  */
	    delta_xi = xi0 - xi;
	    delta_eta = eta0 - eta;
	    /*  Compute correction  */
	    z = 1.0 / (Dxi_Dx * Deta_Dy - Dxi_Dy * Deta_Dx);
	    Dx = (delta_xi * Deta_Dy - delta_eta * Dxi_Dy) * z;
	    Dy = (delta_eta * Dxi_Dx - delta_xi * Deta_Dx) * z;
	    /*  Apply correction  */
	    x += Dx;
	    y += Dy;
	}
	total_iters += iter_count;
	/*  Convert offsets in mm to pixels  */
	ra[count] = (ap->dss->centre_off_x - 1e3 * x) / ap->dss->pixel_size_x -
	    ap->dss->corner_pixel_x1;
	dec[count] = (ap->dss->centre_off_y + 1e3 *y) / ap->dss->pixel_size_y -
	    ap->dss->corner_pixel_y1;
    }
}   /*  End Function dss_radec2xy  */
