//  kwin.s

//  This code provides some optimised KPixCanvas routines for a VX display.

//  Copyright (C) 1994  Richard Gooch

//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.

//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.

//  You should have received a copy of the GNU Library General Public
//  License along with this library; if not, write to the Free
//  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

//  Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
//  The postal address is:
//    Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.


	.file	"asm_kwin.c"
	.text
	.align	8
_L00TEXT: 
	.text;	.align	8

//Stack allocation: Autos:0

_asm__kwin_fill_rectangle::
//void asm__kwin_fill_rectangle (flag depth, unsigned char *origin,
//				int width, int height,
//				unsigned int pixel_value, int pixel_stride,
//				int line_stride)

// Register Parameters
#define depth		r16
#define origin		r17
#define width		r18
#define height		r19
#define pixel_value	r20
#define pixel_stride	r21
#define line_stride	r22

// Temporaries
#define b_ptr		r23
#define x		r24
#define red_pixel	r25
#define green_pixel	r26
#define blue_pixel	r27

	adds	-1, height, height		// --height
	bte	3, depth, .true_colour		// Branch IF (depth == 3)
.y_loop8:
	adds	-1, width, x			// x = width - 1
	adds	r0, origin, b_ptr		// b_ptr = origin
.x_loop8:
	adds	-1, x, x			// CC clear IF x >= 1; --x
	st.b	pixel_value, 0(b_ptr)		// *b_ptr = pixel_value
	bnc.t	.x_loop8			// Branch if CC clear
	adds	b_ptr, pixel_stride, b_ptr	// b_ptr += pixel_stride
	adds	-1, height, height	   // CC clear IF height >= 1; --height
	bnc.t	.y_loop8				// Branch if CC clear
	adds	line_stride, origin, origin	// origin += line_stride
	bri	r1				// return
	nop

.true_colour:
	adds	r0, pixel_value, red_pixel	// red_pixel = pixel_value
	shra	8, pixel_value, green_pixel   // green_pixel = pixel_value >> 8
	shra	16, pixel_value, blue_pixel   // blue_pixel = pixel_value >> 16
.y_loop24:
	adds	-1, width, x			// x = width - 1
	adds	r0, origin, b_ptr		// b_ptr = origin
.x_loop24:
	adds	-1, x, x			// CC clear IF x >= 1; --x
	st.b	blue_pixel, 0(b_ptr)		// b_ptr[0] = blue_pixel
	st.b	green_pixel, 1(b_ptr)		// b_ptr[1] = green_pixel
	st.b	red_pixel, 2(b_ptr)		// b_ptr[2] = red_pixel
	bnc.t	.x_loop24			// Branch if CC clear
	adds	b_ptr, pixel_stride, b_ptr	// b_ptr += pixel_stride
	adds	-1, height, height	   // CC clear IF height >= 1; --height
	bnc.t	.y_loop24				// Branch if CC clear
	adds	line_stride, origin, origin	// origin += line_stride
	bri	r1				// return
	nop

	.oVhc2.3  =: 0
