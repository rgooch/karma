	.file	"compute_line.c"
	.data
	.align	8
_L00DATA: 
	.align	4
_vxmodule_sccsid:	.byte	"@(#)vxmodule.c 1.2 91/08/01 Copyright 1991 Sun MicroSyst"
	.byte	"ems\0"
	.text
	.align	8
_L00TEXT: 
	.text;	.align	8
//--------------| compute_x |-----------------------: frame was never referenced.
//Stack allocation: Autos:0 Regsave:44

_compute_x::
//  int compute_x (data, num_iterations, cy, x_min, x_pixels, x_scale,
//		   four, one, stride, num_colours)
//      This routine will compute a line in the complex plane of the Mandelbrot
//	set
//	The line data must be pointed to by  data  .
//	The number of iterations to perform must be given by  num_iterations  .
//	The y (imaginary) co-ordinate of the line must be given by  cy  .
//	The start co-ordinate of the line must be given by  x_min  .
//	The length of the line in pixels must be given by  x_pixels  .
//	The distance between pixels in the line must be given by  x_scale  .
//	The value of  four  must be 4.0
//	The value of  one  must be 1.0
//	The stride (in bytes) between pixels must be given by  stride  .
//	The number of colours (not including black) must be given by
//	num_colours  .
//	The routine returns the number of floating point operations performed.
//  unsigned char *data;
//  int num_iterations;
//  float cy;
//  float x_min;
//  int x_pixels;
//  float x_scale;
//  float four;
//  float one;
//  int stride;
//  int num_colours;

// Parameters
#define data		r16
#define num_iterations	r17
#define cy		f16
#define x_min		f17
#define x_pixels	r18
#define x_scale		f18
#define four		f19
#define one		f20
#define stride		r19
#define num_colours	r20

// Automatic variables
//    int x_index;
//    int iter_count;
//    int flop_count = 0;
//    float cx;
//    float r, i;
//    float xx, yy, xy2;
#define x_index		r24
#define iter_count	r25
#define flop_count	r26
#define cx		f24
#define r		f25
#define i		f26
#define xx		f27
#define yy		f28
#define xy2		f29

#define temp1		r27
#define ftemp1		f30
#define ftemp2		f31
#define ftemp3		yy
#define ftemp4		xy2

// Start dual instruction mode
  d.fnop
    adds		r0, r0, flop_count	// flop_count = 0;
  d.fnop
    adds		r0, r0, x_index		// x_index = 0;
.loop1:
    d.fnop
      orh		32768, r0, temp1	// (float) x_index
    d.fnop
      ixfr		temp1, ftemp1
    d.fnop
      orh		17200, r0, temp1
    d.fnop
      ixfr		temp1, ftemp2
    d.fnop
      xorh		32768, x_index, temp1
    d.fnop
      ixfr		temp1, ftemp3
    d.fmov.ss		ftemp2, ftemp4
      nop
    d.fsub.dd		ftemp3, ftemp1, ftemp3
      nop
    d.fmov.ds		ftemp3, cx		// cx = (float) x_index;
      nop
    d.fmul.ss		cx, x_scale, cx		// x_scale * (float) x_index;
      nop
    d.fadd.ss		cx, x_min, cx	    // cx=x_min+x_scale*(float)x_index;
      nop
    d.fmov.ss		cx, r			// r = cx;
      nop
    d.fmov.ss		cy, i			// i = cy;
      adds		2, flop_count, flop_count // flop_count += 2;
    d.mm12mpm.ss	cx, one, f0		// Start cx * 1.0
      adds		r0, r0, iter_count	// iter_count = 0;
    d.mm12mpm.ss	f0, f0, f0
      nop
    d.mm12mpm.ss	f0, f0, f0
      nop
    d.mm12ttpm.ss	f0, f0, f0		// T := cx
      nop


      d.mm12mpm.ss	r, r, f0		// Start r * r
	nop
      d.mm12mpm.ss	r, i, f0		// Start r * i
	nop
      d.mm12mpm.ss	i, i, f0		// Start i * i
	nop
      d.mm12tpm.ss	i, i, xx      // xx = r * r; Start xx + cx; Start i * i
	nop
      d.mm12mpm.ss	f0, f0, f0		// Get r * i; Start 2 * r * i
	nop
      d.mr2p1.ss	xx, f0, yy		// Get yy = i * i; Start xx +yy
	nop
      d.m12asm.ss	f0, f0, f0 // Get xx + cx; Get i * i; Start xx - yy +cx
	nop
.loop2:
      d.r2ap1.ss	cy, f0, f0 // Get 2 * r * i;Start 2 * r * i + cy;T := ?
	adds		6, flop_count, flop_count	// flop_count += 6;
      d.m12apm.ss	cx, one, ftemp1		// Get xx + yy; Start cx * 1
	nop
      d.pfle.ss		four, ftemp1, r	// r = xx - yy + cx; CC set IF (xx + yy < 4)
	nop
      d.m12tpm.ss	r, r, i		// Get i = 2 * r * i + cy; Start r * r
	nop
      d.m12tpm.ss	r, i, f0		// Start r * i
	bc.t		.jump1			// IF (xx + yy < 4)
      d.mm12ttpm.ss	i, i, f0		// Start i * i; T := cx
	adds		1, iter_count, iter_count // ++iter_count;
.jump2:
    d.fnop
      st.b		iter_count, 0(data)	// *data = iter_count;
    d.fnop
      adds		1, x_index, x_index	// ++x_index;
    d.fnop
      subs		x_index, x_pixels, r0	// CC set IF (x_index < x_pixels)
    d.fnop
      bc.t		.loop1			// IF (x_index < x_pixels)
    d.fnop
      adds		stride, data, data	// data += stride;
// Exit dual instruction mode
  fnop
    adds			0, flop_count, r16	// return (flop_count);
  fnop
    nop
// Dual instruction mode terminated
  bri			r1			// RETURN
  nop
.jump1:
      d.mm12tpm.ss	i, i, xx      // xx = r * r; Start xx + cx; Start i * i
	subs		iter_count, num_iterations, r0; // CC set IF (iter_count < num_iterations)
      d.mm12mpm.ss	f0, f0, f0		// Get r * i; Start 2 * r * i
	nop
      d.mr2p1.ss	xx, f0, yy		// Get yy = i * i; Start xx +yy
	bc.t		.loop2		// IF (iter_count < num_iterations)
      d.m12asm.ss	f0, f0, f0 // Get xx + cx; Get i * i; Start xx - yy +cx
	adds		3, flop_count, flop_count // flop_count += 3;
    d.fnop
      adds		r0, num_colours, iter_count	// iter_count = num_colours;
    d.fnop
      br		.jump2
    d.fnop
      nop

	.oVhc2.3  =: 0

// NEED TO DO  iter_count = iter_count % num_colours
// SOMEWHERE NEAR HERE
//  for (x_index = 0; x_index < x_pixels; ++x_index, data += stride)
//	for (iter_count = 0; iter_count < num_iterations; ++iter_count)
//	    xx = r * r;
//	    yy = i * i;
//	    xy2 = two * r * i;
//	    flop_count += 6;
//	    if (xx + yy >= four) break;
//	    r = xx - yy + cx;
//	    i = xy2 + cy;
//	    flop_count += 3;
