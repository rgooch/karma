/*LINTLIBRARY*/
/*  ds_globals.c

    This code provides global variables for the Karma data structure.

    Copyright (C) 1992,1993  Richard Gooch

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

    This file contains the definitions for various global variables which are
    used by the ds_ routines in the Karma library.


    Written by      Richard Gooch   13-DEC-1992

    Last updated by Richard Gooch   28-MAY-1993


*/

#include <karma_ds_def.h>


/*  Define host type sizes  */
char host_type_sizes[NUMTYPES] =
{
    0,                                  /*  None size                        */
    sizeof (float),                     /*  Float size                       */
    sizeof (double),                    /*  Double size                      */
    sizeof (char),                      /*  Char size                        */
    sizeof (int),                       /*  Int size                         */
    sizeof (short),                     /*  Short size                       */
    sizeof (char *) + sizeof (int),     /*  (unpadded) array pointer size    */
    sizeof (char *),                    /*  List pointer size                */
    0,                                  /*  Multi Array size                 */
    2 * sizeof (float),                 /*  Float Complex size               */
    2 * sizeof (double),                /*  Double Complex size              */
    2 * sizeof (char),                  /*  Byte Complex size                */
    2 * sizeof (int),                   /*  Int Complex size                 */
    2 * sizeof (short),                 /*  Short Complex size               */
    sizeof (long),                      /*  Long size                        */
    2 * sizeof (long),                  /*  Long Complex size                */
    sizeof (unsigned char),             /*  Unsigned Char size               */
    sizeof (unsigned int),              /*  Unsigned Int size                */
    sizeof (unsigned short),            /*  Unsigned Short size              */
    sizeof (unsigned long),             /*  Unsigned Long size               */
    2 * sizeof (unsigned char),         /*  Unsigned Byte Complex size       */
    2 * sizeof (unsigned int),          /*  Unsigned Int Complex size        */
    2 * sizeof (unsigned short),        /*  Unsigned Short Complex size      */
    2 * sizeof (unsigned long),         /*  Unsigned Long Complex size       */
    sizeof (char *) + sizeof (int),     /*  Array pointer size               */
    sizeof (char *),                    /*  Variable string pointer size     */
    sizeof (FString)                    /*  Fixed string header size         */
};

char *data_type_names[NUMTYPES] =
{
    "undefined",
    "float",
    "double",
    "byte",
    "integer",
    "short",
    "OBSOLETE",
    "listp",
    "multi_array",
    "complex",
    "dcomplex",
    "bcomplex",
    "icomplex",
    "scomplex",
    "long",
    "lcomplex",
    "ubyte",
    "uinteger",
    "ushort",
    "ulong",
    "ubcomplex",
    "uicomplex",
    "uscomplex",
    "ulcomplex",
    "arrayp",
    "vstring",
    "fstring"
};
