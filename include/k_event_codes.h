/*
    Definition of event codes.

    Copyright (C) 1993-1996  Richard Gooch

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

#ifndef K_EVENT_CODES_H
#define K_EVENT_CODES_H

#define K_CANVAS_EVENT_LEFT_MOUSE_CLICK (unsigned int) 0
#define K_CANVAS_EVENT_LEFT_MOUSE_DRAG (unsigned int) 1
#define K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK (unsigned int) 2
#define K_CANVAS_EVENT_MIDDLE_MOUSE_DRAG (unsigned int) 3
#define K_CANVAS_EVENT_RIGHT_MOUSE_CLICK (unsigned int) 4
#define K_CANVAS_EVENT_RIGHT_MOUSE_DRAG (unsigned int) 5
#define K_CANVAS_EVENT_POINTER_MOVE (unsigned int) 6
#define K_CANVAS_EVENT_LEFT_MOUSE_RELEASE (unsigned int) 7
#define K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE (unsigned int) 8
#define K_CANVAS_EVENT_RIGHT_MOUSE_RELEASE (unsigned int) 9
#define K_CANVAS_EVENT_PLAIN_KEY_PRESS (unsigned int) 10

#define K_CANVAS_EVENT_UNDEFINED (unsigned int) 29999

#define K_CANVAS_EVENT_USER_SPACE (unsigned int) 30000

#endif  /*  K_EVENT_CODES_H  */
