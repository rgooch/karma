/*  karma_kwin_hooks.h

    Header for  kwin_  package. This file ONLY contains the hook functions

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

    This include file contains all the definitions for the psw_ package in the
    Karma library.


    Written by      Richard Gooch   19-APR-1996: Moved from karma_kwin.h

    Last updated by Richard Gooch   19-APR-1996

*/

#ifndef KARMA_KWIN_HOOKS_H
#define KARMA_KWIN_HOOKS_H


#define KWIN_FUNC_DRAW_PC_IMAGE     10000
#define KWIN_FUNC_DRAW_RGB_IMAGE    10001
#define KWIN_FUNC_DRAW_CACHED_IMAGE 10002
#define KWIN_FUNC_FREE_CACHE_DATA   10003
#define KWIN_FUNC_DRAW_LINE         10004
#define KWIN_FUNC_DRAW_ARC          10005
#define KWIN_FUNC_DRAW_POLYGON      10006
#define KWIN_FUNC_DRAW_STRING       10007
#define KWIN_FUNC_DRAW_RECTANGLE    10008
#define KWIN_FUNC_DRAW_LINES        10009
#define KWIN_FUNC_DRAW_ARCS         10010
#define KWIN_FUNC_DRAW_SEGMENTS     10011
#define KWIN_FUNC_GET_COLOUR        10012
#define KWIN_FUNC_LOAD_FONT         10013
#define KWIN_FUNC_GET_STRING_SIZE   10014
#define KWIN_FUNC_SET_FONT          10015
#define KWIN_FUNC_QUERY_COLOURMAP   10016
#define KWIN_FUNC_RESIZE            10017
#define KWIN_FUNC_DRAW_POINTS       10018
#define KWIN_FUNC_SET_LINEWIDTH     10019


#endif /*  KARMA_KWIN_HOOKS_H  */
