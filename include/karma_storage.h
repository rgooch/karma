/*  karma_storage.h

    Header for  storage_  package.

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
  needed to interface to the storage_ routines in the Karma library.


    Written by      Richard Gooch   8-APR-1993

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_IARRAY_H) || defined(MAKEDEPEND)
#  include <karma_iarray.h>
#endif

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#ifndef KARMA_STORAGE_H
#define KARMA_STORAGE_H

#define SM_MAX_PATHNAME_LENGTH (unsigned int) 256

#define STORAGE_ERR_VALUE_NOT_FOUND 0
#define STORAGE_ERR_DUPLICATE_NAME 1
#define STORAGE_ERR_DIMENSION_NOT_VALUE 2
#define STORAGE_ERR_UNDEFINED 1023

#ifndef DATASTORE_DEFINED
#define DATASTORE_DEFINED
typedef void * DataStore;
typedef void * DataSectionInternal;
typedef void * DataVectorInternal;
#endif

typedef struct              /*  Change any field data and you're dead        */
{
    iarray iarray;          /* Intelligent Array                             */
#ifdef not_implemented
    double **coordinates;   /* Array of co-ordinate arrays for each dimension*/
#endif
#ifdef not_implemented
    unsigned int num_other_keys; /* Number of other keys in DataStore        */
    char *other_key_names;  /*  Names of other keys                          */
    double *other_key_values; /* Array of values of other keys               */
#endif
    DataSectionInternal priv;
} *DataSection;

typedef struct              /*  Change any field data and you're dead        */
{
    char *data;             /*  Pointer to start of section data             */
    unsigned int *offsets;  /*  Array of offsets for each value              */
    unsigned int type;      /*  Type of the data values                      */
    unsigned int length;    /*  Length of the vector                         */
    DataVectorInternal priv;
} *DataVector;


/*  File:   storage.c   */
EXTERN_FUNCTION (DataStore storage_open, (CONST char *pathname, flag read_only,
					  flag reshape) );
EXTERN_FUNCTION (DataStore storage_create, (CONST char *pathname,
					    packet_desc *top_pack_desc) );
EXTERN_FUNCTION (flag storage_close, (DataStore datastore) );
EXTERN_FUNCTION (CONST char *storage_get_one_value,
		 (DataStore datastore, CONST char *value_name,
		  unsigned int *type, unsigned int num_keys,
		  CONST char **key_names, double *key_values,
		  unsigned int *errcode) );
EXTERN_FUNCTION (flag storage_put_one_value,
		 (DataStore datastore, CONST char *value_name,
		  unsigned int type, char *value, unsigned int num_keys,
		  CONST char **key_names, double *key_values,
		  unsigned int *errcode) );
EXTERN_FUNCTION (DataSection storage_get_one_section,
		 (DataStore datastore, CONST char *value_name,
		  flag auto_update,
		  unsigned int num_axis_keys, CONST char **axis_key_names,
		  unsigned long *axis_key_lengths,
		  double **axis_key_coordinates,
		  unsigned int num_loc_keys, CONST char **loc_key_names,
		  double *loc_key_values, unsigned int *errcode) );
EXTERN_FUNCTION (DataSection storage_define_iterator,
		 (DataStore datastore, CONST char *value_name,
		  flag auto_update,
		  unsigned int num_axis_keys, CONST char **axis_key_names,
		  unsigned long *axis_key_lengths,
		  double **axis_key_coordinates, unsigned int num_ordered_keys,
		  CONST char **ordered_key_names,
		  unsigned long *ordered_key_lengths,
		  double **ordered_key_coordinates, unsigned int *errcode) );
EXTERN_FUNCTION (flag storage_goto_next_section, (DataSection section) );
EXTERN_FUNCTION (DataSection storage_create_section,
		 (DataStore datastore, CONST char *value_name,
		  unsigned int num_axis_keys, CONST char **axis_key_names,
		  unsigned long *axis_key_lengths,
		  double **axis_key_coordinates,
		  unsigned int num_loc_keys, CONST char **loc_key_names,
		  double *loc_key_values, unsigned int *errcode) );
EXTERN_FUNCTION (void storage_free_section, (DataSection section) );
EXTERN_FUNCTION (void storage_flush_section, (DataSection section) );
EXTERN_FUNCTION (unsigned int storage_get_keywords,
		 (DataStore datastore, char ***keyword_names,
		  unsigned int **keyword_types) );
EXTERN_FUNCTION (CONST char *storage_get_keyword_value,
		 (DataStore datastore, CONST char *name,
		  unsigned int *keyword_type) );
EXTERN_FUNCTION (flag storage_change_keyword_value,
		 (DataStore datastore, CONST char *name, char *value,
		  unsigned int type) );
EXTERN_FUNCTION (DataVector storage_get_first_vector,
		 (DataStore datastore, CONST char *value_name,
		  flag auto_update,
		  unsigned int *errcode) );
EXTERN_FUNCTION (flag storage_goto_next_vector, (DataVector vector) );


#endif /*  KARMA_STORAGE_H  */
