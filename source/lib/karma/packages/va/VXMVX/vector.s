//  vector.s

//  This code provides some optimised vector arithmetic operations for a VX.

//  Copyright (C) 1992,1993,1994  Richard Gooch

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


	.file	"vector.c"
	.text
	.align	8
_L00TEXT: 
	.text;	.align	8
//--------------| va_scale_float |-----------------------: frame was never referenced.
//Stack allocation: Autos:0

_asm__va_scale_float::
//	r16:	float *out
//	r17:	int out_stride
//	r18:	float *inp
//	r19:	int inp_stride
//	r20:	int length
//	f16:	float scale
//	f17:	float offset

	subs		r16, r17, r16	// Prepare for autoincrement
	subs		r18, r19, r18	// Prepare for autoincrement
	d.r2pt.ss	f16, f0, f0	// Store  scale  in KR
	nop

// This code runs slower (but is easier to write!)

//  The following setup and loop processes 13 elements: branch if not enough
	d.r2p1.ss	f0, f0, f0	;	adds	-13, r20, r20
	d.r2p1.ss	f0, f0, f0	;	bc.t	.va_s_f.2
					// Restore only if not enough
	d.r2p1.ss	f0, f0, f0	;	adds	13, r20, r20
					// Load first input
	d.r2p1.ss	f0, f0, f0	;	fld.l	r19(r18)++, f31
	// Scale			// Load f20:f25
	d.fmul.ss	f31, f16, f31	;	fld.l	r19(r18)++, f20
	// Offset			
	d.fadd.ss	f31, f17, f31	;	fld.l	r19(r18)++, f21
	d.r2p1.ss	f0, f0, f0	;	fld.l	r19(r18)++, f22
	d.r2p1.ss	f0, f0, f0	;	fld.l	r19(r18)++, f23
	d.r2p1.ss	f0, f0, f0	;	fld.l	r19(r18)++, f24
	d.r2p1.ss	f0, f0, f0	;	fld.l	r19(r18)++, f25
					// Load decrement counter
	d.r2p1.ss	f0, f0, f0	;	mov	-6, r30
	d.r2p1.ss	f0, f0, f0	;	bla	r30, r20, .va_s_f.1
	d.r2p1.ss	f0, f0, f0	;	nop
.va_s_f.1:
// slow loop
	d.r2p1.ss	f17, f20, f0	;	fst.l	f31, r17(r16)++
	d.r2p1.ss	f17, f21, f0	;	fld.l	r19(r18)++, f20
	d.r2p1.ss	f17, f22, f0	;	fld.l	r19(r18)++, f21
	d.r2p1.ss	f17, f23, f0	;	fld.l	r19(r18)++, f22
	d.r2p1.ss	f17, f24, f0	;	fld.l	r19(r18)++, f23
	d.r2p1.ss	f17, f25, f0	;	fld.l	r19(r18)++, f24

	d.r2p1.ss	f17, f0, f26	;	fld.l	r19(r18)++, f25
	d.r2p1.ss	f17, f0, f27	;	fst.l	f26, r17(r16)++
	d.r2p1.ss	f17, f0, f28	;	fst.l	f27, r17(r16)++
	d.r2p1.ss	f17, f0, f29	;	fst.l	f28, r17(r16)++
	d.r2p1.ss	f17, f0, f30	;	fst.l	f29, r17(r16)++
	d.r2p1.ss	f17, f0, f31	;	bla 	r30, r20, .va_s_f.1
	d.r2p1.ss	f0, f0, f0	;	fst.l	f30, r17(r16)++
// end loop

	d.r2p1.ss	f17, f20, f0	;	fst.l	f31, r17(r16)++
	d.r2p1.ss	f17, f21, f0	;	nop
	d.r2p1.ss	f17, f22, f0	;	nop
	d.r2p1.ss	f17, f23, f0	;	nop
	d.r2p1.ss	f17, f24, f0	;	nop
	d.r2p1.ss	f17, f25, f0	;	nop

	d.r2p1.ss	f17, f0, f26	;	nop
	d.r2p1.ss	f17, f0, f27	;	fst.l	f26, r17(r16)++
	d.r2p1.ss	f17, f0, f28	;	fst.l	f27, r17(r16)++
	d.r2p1.ss	f17, f0, f29	;	fst.l	f28, r17(r16)++
	d.r2p1.ss	f17, f0, f30	;	fst.l	f29, r17(r16)++
	d.r2p1.ss	f17, f0, f31	;	fst.l	f30, r17(r16)++
	d.r2p1.ss	f0, f0, f0	;	fst.l	f31, r17(r16)++

.va_s_f.2:
//  r20 contains number of elements still to process
	d.r2p1.ss	f0, f0, f0	;	adds	-1, r20, r20
	d.r2p1.ss	f0, f0, f0	;	bc	.va_s_f.3
.va_s_f.4:
	d.r2p1.ss	f0, f0, f0	;	fld.l	r19(r18)++, f31
	// Scale
	d.fmul.ss	f31, f16, f31	;	adds	-1, r20, r20
	// Offset			
	d.fadd.ss	f31, f17, f31	;	bnc.t	.va_s_f.4
	d.r2p1.ss	f0, f0, f0	;	fst.l	f31, r17(r16)++

.va_s_f.3:
// Exit dual instruction mode
	r2p1.ss		f0, f0, f0	;	nop
	r2p1.ss		f0, f0, f0	;	nop
	bri	r1
	nop	


//	load f22:f23
//	load f28:f31

	d.r2p1.ss	f17, f22, f0
	d.r2p1.ss	f17, f23, f0

	d.r2p1.ss	f17, f28, f0
	d.r2p1.ss	f17, f29, f0		; // load f24:f27
	d.r2p1.ss	f17, f30, f0
	d.r2p1.ss	f17, f31, f0

	d.r2p1.ss	f17, f24, f28
	d.r2p1.ss	f17, f25, f29	; // load f20:f23
	d.r2p1.ss	f17, f26, f30
	d.r2p1.ss	f17, f27, f31

// loop:
	d.r2p1.ss	f17, f20, f24	; // store f28:f31
	d.r2p1.ss	f17, f21, f25	; // load f28:f31
	d.r2p1.ss	f17, f22, f26
	d.r2p1.ss	f17, f23, f27

	d.r2p1.ss	f17, f28, f20	; // store f24:f27
	d.r2p1.ss	f17, f29, f21	; // load f24:f27
	d.r2p1.ss	f17, f30, f22
	d.r2p1.ss	f17, f31, f23

	d.r2p1.ss	f17, f24, f28	; // store f20:f23
	d.r2p1.ss	f17, f25, f29	; // load f20:f23
	d.r2p1.ss	f17, f26, f30
	d.r2p1.ss	f17, f27, f31
// End loop

	d.r2p1.ss	f17, f20, f24	; // store f28:f31
	d.r2p1.ss	f17, f21, f25
	d.r2p1.ss	f17, f22, f26
	d.r2p1.ss	f17, f23, f27

	d.r2p1.ss	f0, f0, f20		; // store f24:f27
	d.r2p1.ss	f0, f0, f21
	d.r2p1.ss	f0, f0, f22
	d.r2p1.ss	f0, f0, f23

	d.r2p1.ss	f0, f0, f28		; // store f20:f23
	d.r2p1.ss	f0, f0, f29

//	store f28:f29


// Something required at the end of each file
	.oVhc2.3  =: 0
