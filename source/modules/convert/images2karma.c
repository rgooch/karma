/*  images2karma.c

    Source file for  images2karma  (Images to Karma movie conversion module).

    Copyright (C) 1996  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This Karma module will read in many images of varying formats and convert
    them to a Karma data file.


    Written by      Richard Gooch   3-DEC-1996

    Last updated by Richard Gooch   3-DEC-1996


*/
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_im.h>
#include <karma_m.h>


/*  Private functions  */
STATIC_FUNCTION (multi_array *read_image, (CONST char *file) );


/*  Public functions follow  */

int main (int argc, char **argv)
{
    unsigned int num_frames, count;
    unsigned long array_bytes;
    char *movie_arr;
    multi_array *image, *first_image, *movie;
    array_desc *first_arr_desc, *arr_desc;
    dim_desc *zdim;
    static char usage_string[] = "Usage:\timages2karma infiles [...] outfile";
    static char function_name[] = "main";

    if (argc < 4)
    {
	fprintf (stderr, "%s\n", usage_string);
	exit (RV_MISSING_PARAM);
    }
    num_frames = argc - 2;
    /*  Read first image  */
    if ( ( first_image = read_image (argv[1]) ) == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    first_arr_desc = (array_desc *) first_image->headers[0]->element_desc[0];
    /*  Construct movie descriptor  */
    if ( ( movie = ds_alloc_multi (first_image->num_arrays) ) == NULL )
    {
	m_abort (function_name, "movie");
    }
    /*  Copy descriptors  */
    for (count = 0; count < first_image->num_arrays; ++count)
    {
	if ( ( movie->headers[count] =
	       ds_copy_desc_until (first_image->headers[count], NULL) )
	     == NULL )
	{
	    m_abort (function_name, "movie");
	}
    }
    arr_desc = (array_desc *) movie->headers[0]->element_desc[0];
    /*  Create and prepend extra dimension  */
    if ( ( zdim = ds_alloc_dim_desc ("Frame Number", num_frames,
				     0.0, num_frames - 1, TRUE) ) == NULL )
    {
	m_abort (function_name, "movie");
    }
    if ( !ds_prepend_dim_desc (arr_desc, zdim) )
	m_abort (function_name, "movie");
    /*  Allocate data and copy attachments from first image  */
    for (count = 0; count < first_image->num_arrays; ++count)
    {
	if ( ( movie->data[count] =
	       ds_alloc_data (movie->headers[count], FALSE, TRUE) ) == NULL )
	{
	    m_abort (function_name, "movie");
	}
	if ( !ds_copy_data (first_image->headers[count],
			    first_image->data[count], movie->headers[count],
			    movie->data[count]) )
	{
	    m_abort (function_name, "movie");
	}
    }
    array_bytes = ds_get_array_size (first_arr_desc);
    array_bytes *= ds_get_packet_size (first_arr_desc->packet);
    movie_arr = *(char **) movie->data[0];
    /*  Copy the image data from the first image  */
    m_copy (movie_arr, *(char **) first_image->data[0], array_bytes);
    /*  Read and copy remaining images  */
    for (count = 2; count < argc - 1; ++count)
    {
	if ( ( image = read_image (argv[count]) ) == NULL )
	{
	    exit (RV_UNDEF_ERROR);
	}
	arr_desc = (array_desc *) image->headers[0]->element_desc[0];
	if ( !ds_compare_array_desc (first_arr_desc, arr_desc, TRUE) )
	{
	    fprintf (stderr, "Different image in file: \"%s\"\n", argv[count]);
	    exit (RV_BAD_DATA);
	}
	m_copy (movie_arr + array_bytes * (count - 1),
		*(char **) image->data[0], array_bytes);
	ds_dealloc_multi (image);
    }
    fprintf (stderr, "Writing...");
    if ( dsxfr_put_multi (argv[argc - 1], movie) ) putc ('\n', stderr);
    return (RV_OK);
}   /*  End Function main  */


/*  Private functions follow  */

static multi_array *read_image (CONST char *file)
/*  [SUMMARY] Read an image.
    <file> The name of the image file to read.
    [RETURNS] A multi_array pointer on success, else NULL.
*/
{
    multi_array *multi_desc;
    array_desc *arr_desc;

    fprintf (stderr, "Reading: \"%s\"...", file);
    if ( ( multi_desc = foreign_guess_and_read (file, K_CH_MAP_IF_AVAILABLE,
						FALSE, NULL,
						FA_GUESS_READ_END) ) == NULL )
    {
	return (NULL);
    }
    if (multi_desc->headers[0]->element_types[0] != K_ARRAY)
    {
	fprintf (stderr, "\tno default image\n");
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    arr_desc = (array_desc *) multi_desc->headers[0]->element_desc[0];
    if (arr_desc->num_dimensions != 2)
    {
	fprintf (stderr, "\tdefault array is: %u dimensional\n",
		 arr_desc->num_dimensions);
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    if ( !ds_packet_all_data (arr_desc->packet) )
    {
	fprintf (stderr, "\theirarchical image");
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    /*  I suppose it's passed the tests  */
    fprintf (stderr, "\tOK\n");
    return (multi_desc);
}   /*  End Function read_image  */
