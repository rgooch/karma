/*  karma_ch_def.h

    Header for  ch_  package. This file ONLY contains the object definitions

    Copyright (C) 1994  Richard Gooch

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

    This include file contains all the definitions for the ch_ routines in the
    Karma library.


    Written by      Richard Gooch   21-MAY-1994

    Last updated by Richard Gooch   25-NOV-1994

*/

#ifndef KARMA_CH_DEF_H
#define KARMA_CH_DEF_H


typedef struct channel_type * Channel;
typedef struct _converter_struct * ChConverter;
typedef struct channel_tap_type * ChannelTap;


/*  Control values for memory mapping disc files  */
#define K_CH_MAP_NEVER        (unsigned int) 0  /*  Never map                */
#define K_CH_MAP_LARGE_LOCAL  (unsigned int) 1  /*  Map if local FS and > 1MB*/
#define K_CH_MAP_LOCAL        (unsigned int) 2  /*  Map if local filesystem  */
#define K_CH_MAP_LARGE        (unsigned int) 3  /*  Map if file over 1 MByte */
#define K_CH_MAP_IF_AVAILABLE (unsigned int) 4  /*  Map if OS supports it    */
#define K_CH_MAP_ALWAYS       (unsigned int) 5  /*  Always map               */


#endif /*  KARMA_CH_DEF_H  */
