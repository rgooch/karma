/*LINTLIBRARY*/
/*  get.c

    This code finds various specific forms of data from the Karma data
    structure and returns Intelligent Arrays.

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

/*  This file contains all routines needed for the simple processing of
  n-dimensional data structures.


    Written by      Richard Gooch   25-DEC-1994

    Updated by      Richard Gooch   25-DEC-1994

    Updated by      Richard Gooch   15-APR-1995: Added message when trying for
  TrueColour.

    Last updated by Richard Gooch   1-APR-1996: Moved remaing functions to new
  documentation style.


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_ds.h>
#include <karma_a.h>


#define VERIFY_IARRAY(array) if (array == NULL) \
{(void) fprintf (stderr, "NULL iarray passed\n"); \
 a_prog_bug (function_name); }
#ifdef dummy
if ( (*array).magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid iarray\n"); \
 a_prog_bug (function_name); }
#endif


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag iarray_get_image_from_multi (multi_array *multi_desc, iarray *pseudo,
				  iarray *red, iarray *green, iarray *blue,
				  unsigned int *cmap_index)
/*  [SUMMARY] Get an image from a Karma data structure.
    [PURPOSE] This routine will find an image embedded in a Karma data
    structure. The image may be single-channel (PseudoColour) or it may be a
    TrueColour image (red, green and blue components).
    <multi_desc> The Karma data structure.
    <pseudo> If a single-channel image is found, the corresponding Intelligent
    Array is written here. If no single-channel image is found, NULL is written
    here.
    <red> If a TrueColour image is found, the red component Intelligent Array
    is written here. If no TrueColour image is found, NULL is written here.
    <green> If a TrueColour image is found, the green component Intelligent
    Array is written here. If no TrueColour image is found, NULL is written
    here.
    <blue> If a TrueColour image is found, the blue component Intelligent Array
    is written here. If no TrueColour image is found, NULL is written here.
    <cmap_index> If the image found is a single-channel image and the data
    structure has an associated RGBcolourmap, the index to the colourmap
    structure is written here. If no colourmap is found, the value written here
    is set to the number of general data structures in multi_desc.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    unsigned int dummy;
    static char function_name[] = "iarray_get_image_from_multi";

    *pseudo = NULL;
    *red = NULL;
    *green = NULL;
    *blue = NULL;
    *cmap_index = (*multi_desc).num_arrays;
    if ( (*multi_desc).num_arrays > 1 )
    {
	if ( ( *pseudo = iarray_get_from_multi_array (multi_desc, "Frame", 2,
						      (CONST char **) NULL,
						      NULL) )
	    == NULL )
	{
	    (void) fprintf (stderr,
			    "Error getting Intelligent Array: Frame\n");
	    return (FALSE);
	}
	/*  Check for colourmap  */
	switch ( dummy = ds_f_array_name (multi_desc, "RGBcolourmap",
					  (char **) NULL, cmap_index) )
	{
	  case IDENT_NOT_FOUND:
	    *cmap_index = (*multi_desc).num_arrays;
	    break;
	  case IDENT_GEN_STRUCT:
	    /*  Got it!  */
	    break;
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr,
			    "Multiple RGBcolourmap structures found\n");
	    iarray_dealloc (*pseudo);
	    *cmap_index = (*multi_desc).num_arrays;
	    return (FALSE);
/*
	    break;
*/
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value: %u from: ds_f_array_name\n",
			    dummy);
	    a_prog_bug (function_name);
	    break;
	}
	return (TRUE);
    }
    if ( ( *pseudo = iarray_get_from_multi_array (multi_desc, NULL, 2,
						  (CONST char **) NULL, NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "%s: trying TrueColour...\n", function_name);
	if ( ( *red =
	      iarray_get_from_multi_array (multi_desc, NULL, 2,
					   (CONST char **) NULL,
					   "Red Intensity") ) == NULL )
	{
	    return (FALSE);
	}
	if ( ( *green =
	      iarray_get_from_multi_array (multi_desc, NULL, 2,
					   (CONST char **) NULL,
					   "Green Intensity") ) == NULL )
	{
	    (void) fprintf (stderr, "Error getting green array\n");
	    iarray_dealloc (*red);
	    return (FALSE);
	}
	if ( (**red).arr_desc != (**green).arr_desc )
	{
	    (void) fprintf (stderr,
			    "Green array descriptor different than red\n");
	    iarray_dealloc (*red);
	    iarray_dealloc (*green);
	    return (FALSE);
	}
	if ( ( *blue =
	      iarray_get_from_multi_array (multi_desc, NULL, 2,
					   (CONST char **) NULL,
					   "Blue Intensity") ) == NULL )
	{
	    (void) fprintf (stderr, "Error getting blue array\n");
	    iarray_dealloc (*red);
	    iarray_dealloc (*green);
	    return (FALSE);
	}
	if ( (**red).arr_desc != (**blue).arr_desc )
	{
	    (void) fprintf (stderr,
			    "Blue array descriptor different than red\n");
	    iarray_dealloc (*red);
	    iarray_dealloc (*green);
	    iarray_dealloc (*blue);
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function iarray_get_image_from_multi  */

/*PUBLIC_FUNCTION*/
flag iarray_get_movie_from_multi (multi_array *multi_desc, iarray *pseudo,
				  iarray *red, iarray *green, iarray *blue,
				  unsigned int *cmap_index)
/*  [SUMMARY] Get a movie from a Karma data structure.
    [PURPOSE] This routine will find a movie embedded in a Karma data
    structure. The movie may be single-channel (PseudoColour) or it may be a
    TrueColour movie (red, green and blue components).
    <multi_desc> The Karma data structure.
    <pseudo> If a single-channel movie is found, the corresponding Intelligent
    Array is written here. If no single-channel movie is found, NULL is written
    here.
    <red> If a TrueColour movie is found, the red component Intelligent Array
    is written here. If no TrueColour movie is found, NULL is written here.
    <green> If a TrueColour movie is found, the green component Intelligent
    Array is written here. If no TrueColour movie is found, NULL is written
    here.
    <blue> If a TrueColour movie is found, the blue component Intelligent Array
    is written here. If no TrueColour movie is found, NULL is written here.
    <cmap_index> If the movie found is a single-channel movie and the data
    structure has an associated RGBcolourmap, the index to the colourmap
    structure is written here. If no colourmap is found, the value written here
    is set to the number of general data structures in multi_desc.
    [RETURNS] TRUE on succes, else FALSE.
*/
{
    unsigned int dummy;
    static char function_name[] = "iarray_get_movie_from_multi";

    *pseudo = NULL;
    *red = NULL;
    *green = NULL;
    *blue = NULL;
    *cmap_index = (*multi_desc).num_arrays;
    if ( (*multi_desc).num_arrays > 1 )
    {
	if ( ( *pseudo = iarray_get_from_multi_array (multi_desc, "Movie", 3,
						      (CONST char **) NULL,
						      NULL) )
	    == NULL )
	{
	    (void) fprintf (stderr,
			    "Error getting Intelligent Array: Frame\n");
	    return (FALSE);
	}
	/*  Check for colourmap  */
	switch ( dummy = ds_f_array_name (multi_desc, "RGBcolourmap",
					  (char **) NULL, cmap_index) )
	{
	  case IDENT_NOT_FOUND:
	    *cmap_index = (*multi_desc).num_arrays;
	    break;
	  case IDENT_GEN_STRUCT:
	    /*  Got it!  */
	    break;
	  case IDENT_MULTIPLE:
	    (void) fprintf (stderr,
			    "Multiple RGBcolourmap structures found\n");
	    iarray_dealloc (*pseudo);
	    *cmap_index = (*multi_desc).num_arrays;
	    return (FALSE);
/*
	    break;
*/
	  default:
	    (void) fprintf (stderr,
			    "Illegal return value: %u from: ds_f_array_name\n",
			    dummy);
	    a_prog_bug (function_name);
	    break;
	}
	return (TRUE);
    }
    if ( ( *pseudo = iarray_get_from_multi_array (multi_desc, NULL, 3,
						  (CONST char **) NULL, NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "%s: trying TrueColour...\n", function_name);
	if ( ( *red =
	      iarray_get_from_multi_array (multi_desc, NULL, 3,
					   (CONST char **) NULL,
					   "Red Intensity") ) == NULL )
	{
	    return (FALSE);
	}
	if ( ( *green =
	      iarray_get_from_multi_array (multi_desc, NULL, 3,
					   NULL,
					   "Green Intensity") ) == NULL )
	{
	    (void) fprintf (stderr, "Error getting green array\n");
	    iarray_dealloc (*red);
	    return (FALSE);
	}
	if ( (**red).arr_desc != (**green).arr_desc )
	{
	    (void) fprintf (stderr,
			    "Green array descriptor different than red\n");
	    iarray_dealloc (*red);
	    iarray_dealloc (*green);
	    return (FALSE);
	}
	if ( ( *blue =
	      iarray_get_from_multi_array (multi_desc, NULL, 3, NULL,
					   "Blue Intensity") ) == NULL )
	{
	    (void) fprintf (stderr, "Error getting blue array\n");
	    iarray_dealloc (*red);
	    iarray_dealloc (*green);
	    return (FALSE);
	}
	if ( (**red).arr_desc != (**blue).arr_desc )
	{
	    (void) fprintf (stderr,
			    "Blue array descriptor different than red\n");
	    iarray_dealloc (*red);
	    iarray_dealloc (*green);
	    iarray_dealloc (*blue);
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function iarray_get_movie_from_multi  */
