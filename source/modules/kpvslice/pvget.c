/*  pvget.c

    Slice file for  kpvslice  (X11 Position-Velocity slice tool for Karma).

    Copyright (C) 1996  Vincent McIntyre

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

    Vincent McIntyre may be reached by email at  vjm@davinci.sci.uow.edu.au
    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*
    This Karma module will enable interactive selection of a Position-Velocity
    slice through a data cube.
    This module runs on an X11 server.


    Written by      Vincent McIntyre 12-JUN-1996

    Updated by      Vincent McIntyre 14-JUN-1996

    Updated by      Vincent McIntyre 17-JUN-1996: Separated computation of
  locus and slice extraction.

    Last updated by Richard Gooch    17-JUN-1996: Cleaned code to pass strict
  compilers and completed change to ordinary C arrays for locii.


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_iarray.h>
#include <karma_m.h>

/*-----------------------------------------------------------------------------*/
static float weight (float x, float y, float cx, float cy) {
 /* calculates weight to give when summing contributions from neighbours.
    In this version I make the weight inversely proportional to the distance
    from the projected centre of the current slice pixel. The pixel this point
    lands on will get highest weight, but the near neighbours will get some
    weight too.
    
  */

  float dx,dy,r,w;

  dx = x-cx; dy = y-cy;
  r = sqrt((dx*dx+dy*dy));
  if (r>0.0) {
    w = 1.0/r;
  }else{
    w = 1.0;
  }
  return(w);
} /* end weight() */
/*-----------------------------------------------------------------------------*/
static flag check_bounds (int *x1,  int *y1,  int *x2,  int *y2,  int xpix,
		         int ypix, int *blx, int *bly, int *trx, int *try)
{
  /* checks bounds & if endpoints are outside the area of cube's front face,
     computes overlapping section of slice & returns new endpoints */

  int ix1,iy1,ix2,iy2,swapped,v[5],w[5];
  float f,x,y;

  swapped=0;
  ix1 = *x1; iy1 = *y1;  ix2 = *x2; iy2 = *y2;
/*(void) fprintf(stderr, "initialpoints; (%d,%d) (%d,%d)\n",ix1,iy1,ix2,iy2);*/

  /* do everything assuming first point is left of second, swap back later */
  if (ix2 < ix1) {
    ix1 = *x2; ix2 = *x1; iy1 = *y2; iy2 = *y1; swapped=1;
  } else {
    if ( ix1 == ix2 && iy1 > iy2 ) {
      iy1 = *y2; iy2 = *y1; swapped=2;
    }
  }
/*  if (swapped != 0) {
(void) fprintf(stderr, "swappedpoints; (%d,%d) (%d,%d)\n",ix1,iy1,ix2,iy2);
  }*/

  *blx = ix1; *bly = iy1; *trx = ix2; *try = iy2;

  /* handle no-overlap cases: the extended slice line may overlap eventually,
   but the piece within the endpoints does not. So stop here. */
  if ( (ix1 < 0 && ix2 < 0) || (ix1 >= xpix && ix2 >= xpix) ) {
    (void) fprintf(stderr, "PVGET: Illegal x-range\n"); return(FALSE);
  }
  if ( (iy1 < 0 && iy2 < 0) || (iy1 >= ypix && iy2 >= ypix) ) {
    (void) fprintf(stderr, "PVGET: Illegal y-range\n"); return(FALSE);
  }

  /* single pixel case */
  if ( ix1 == ix2 && iy1 == iy2 ) {
    (void) fprintf(stderr, "PVGET: Line segment too short\n"); return(FALSE);
  }

  /* handle the case of a vertical slice */
  if ( ix1 == ix2 ) {

      if (ix1 < 0 || ix1 >= xpix) {
	(void) fprintf(stderr, "PVGET: Illegal x-range\n"); return(FALSE);
      }else{
	*blx = ix1; *trx = ix2;
	/* now check if either of the y coords are good */
	if (iy1 < 0 || iy1 >= ypix) { *bly = 0;}
	if (iy2 < 0 || iy2 >= ypix) { *try = ypix-1;}
      }
  }else{
    /* handle the case of a horizontal slice */
    if ( iy1 == iy2 ) {

      if (iy1 < 0 || iy1 >= ypix) {
	(void) fprintf(stderr, "PVGET: Illegal y-range\n"); return(FALSE);
      }else{    
	*bly = iy1; *try = iy2;
	/* now check if either of the x coords are good */
	if (ix1 < 0 || ix1 >= xpix) { *blx = 0;}
	if (ix2 < 0 || ix2 >= xpix) { *trx = xpix-1;}
      }
    }else{
      /* calculate intercepts with x=0, y=0, x=xpix-1, y=ypix-1
	 store x-intercepts in v[] and y-intercepts in w[]       */
      f = ( (float)(iy2) - (float)(iy1) ) / ( (float)(ix2) - (float)(ix1) );
      
      v[1]=0; w[2]=0; v[3]=xpix-1; w[4]=ypix-1;
      x=0.0;    y=(float)(iy1) + (x-(float)(ix1))*f;          w[1]=(int)(y+0.5);
      y=0.0;    x=(float)(ix1) + (y-(float)(iy1))/f;          v[2]=(int)(x+0.5);
      x=(float)(xpix-1); y=(float)(iy1) + (x-(float)(ix1))*f; w[3]=(int)(y+0.5);
      y=(float)(ypix-1); x=(float)(ix1) + (y-(float)(iy1))/f; v[4]=(int)(x+0.5);
/*      (void) fprintf(stderr,"v= %5d %5d %5d %5d\n",v[1],v[2],v[3],v[4]);
      (void) fprintf(stderr,"w= %5d %5d %5d %5d\n",w[1],w[2],w[3],w[4]);*/

      /*  sample case (v[2]>0 && v[4]<xpix && w[3]<ypix)

               |          |
               |          |
      ---------============----------------
	       =          =    B
	       =          =  .
	       =          =.
	       =         .=
      ---------========.===-----------------
               |     .    |
               |    A     |
               |          |
      */

      /* things are different depending on if slope is +ve or -ve */
      /* For either, there are six cases of where the intercepts of the
	 slice line (extended to infinity in both directions) lie w.r.t. the
	 four lines noted above. See diagram. */
      if ( f >= 0.0 ) {
	if (v[2] < 0 ) {
	  if ( w[1] < ypix ) {
	    *blx = 0; *bly = w[1];
	    if ( w[3] < ypix ) {
	      *trx = xpix-1; *try = w[3];
	    }else{
	      *trx = v[4]; *try = ypix-1;
	    }
	  }else{
	    (void) fprintf(stderr, "PVGET: Out of bounds\n"); return(FALSE);
	  }
	}else{
	  if ( v[2] < xpix ) {
	    *bly = 0; *blx = v[2];
	    if ( v[4] < xpix ) {
	      *try = ypix-1; *trx = v[4];
	    }else{
	      *try = w[3]; *trx = xpix-1;
	    }
	  }else{
	    (void) fprintf(stderr, "PVGET: Out of bounds\n"); return(FALSE);
	  }
	}
	/* end +ve slope cases */
      }else{
	if ( v[4] < 0 ) {
	  if ( w[1] > 0 ) {
	    *blx = 0; *bly = w[1];
	    if ( w[3] > 0 ) {
	      *trx = xpix-1; *try = w[3];
	    }else{
	      *trx = v[2]; *try = 0;
	    }
	  }else{
	    (void) fprintf(stderr, "PVGET: Out of bounds\n"); return(FALSE);
	  }
	}else{
	  if ( v[4] < xpix ) {
	    *blx = v[4]; *bly = ypix-1;
	    if ( v[2] < xpix ) {
	      *trx = v[2]; *try = 0;
	    }else{
	      *trx = xpix-1; *try = w[3];
	    }
	  }else{
	    (void) fprintf(stderr, "PVGET: Out of bounds\n"); return(FALSE);
	  }
	}
      } /* end -ve slope cases */
    } /* end finte-slope cases */
  } /* end normal (nonvertical) cases */

  /* now check to see if either endpoint NEEDS to be replaced:
     if they are legal return the input value */
  if ( (ix1 >= 0 && ix1 < xpix) && (iy1 >= 0 && iy1 < ypix) ) {
    *blx = ix1; *bly = iy1;
  }
  if ( (ix2 >= 0 && ix2 < xpix) && (iy2 >= 0 && iy2 < ypix) ) {
    *trx = ix2; *try = iy2;
  }

  /* swap ends back if necessary */
  if (swapped > 0) {
    ix1 = *blx; *blx = *trx; *trx = ix1;
    iy1 = *bly; *bly = *try; *try = iy1;
  }

  return(TRUE);
  
} /* end check_bounds */
/*-----------------------------------------------------------------------------*/
unsigned int get_slice (iarray cube, int x1, int y1, int x2, int y2,
			double **x_locus, double **y_locus)
  /* determine a locus of pixels in a slice line, given two endpoints */
  /* NB the coordinates are returned via pointers to two dynamically allocated
     arrays, written to *x_locus and *y_locus; these must be externally freed.
     The values have to be reals rather than integers, so that antialiasing can
     be done.
     Double-precision is used because this is the `least common denominator
     for C.
   */
{

  int    xpix, ypix, zpix, blx, bly, trx, try;
  int    i, slicepixels;

  float  dx, dy, theta, ctheta, stheta, slicelength;
  double *xloc, *yloc;

  zpix = iarray_dim_length (cube, 0);
  ypix = iarray_dim_length (cube, 1);
  xpix = iarray_dim_length (cube, 2);
  if ( !(xpix*ypix*zpix > 0) ) {
    (void) fprintf (stderr, "GETSLICE: problem with input cube dimensions\n");
  }

  if ( !check_bounds(&x1,&y1,&x2,&y2,xpix,ypix,&blx,&bly,&trx,&try) )
      return (0);

  /* now calculate slice length */
  dx    = (float) (trx-blx+1);
  dy    = (float) (try-bly+1);
  if (blx==trx) { 
    theta = PI/2.0;       ctheta=0.0;          stheta=1.0;
    if ( bly>try ) {theta = 3.0*PI/2.0;        stheta=-1.0; }
  }else{
    theta = atan2(dy,dx); ctheta = cos(theta); stheta = sin(theta);
  }

  slicelength = sqrt(dx*dx+dy*dy);
  slicepixels = (int)(slicelength)+1;

/* allocate enough memory. This has to be deallocated OUTSIDE, once
     the array has been passed to pvslice, and so is finished with */
  xloc = (double *) m_alloc (slicepixels * sizeof *xloc);
  yloc = (double *) m_alloc (slicepixels * sizeof *yloc);

#ifdef TESTSUITE
  (void) fprintf (stderr, " x1: %d  y1: %d  x2: %d  y2: %d Theta: %6.2f\n", x1,y1,x2,y2,theta*180.0/PI);
  (void) fprintf (stderr, "blx: %d bly: %d trx: %d try: %d Theta: %6.2f\n", x1,y1,x2,y2,theta*180.0/PI);
  (void) fprintf (stderr, "dx: %8.3f dy: %8.3f sl: %10.3f sp: %d\n",dx,dy,slicelength,slicepixels);
  (void) fprintf (stderr, "xpix: %3d  ypix: %3d zpix: %3d\n",xpix,ypix,zpix);
#endif

  /* loop over length of slice:
     NB: c-convention array sizing --- index always < size */
  for (i=0; i<slicepixels; i++) {
    /* these are for a straight-line locus; we may want to do other shapes */
    xloc[i] = (double)(i) * ctheta + (double)(blx);
    yloc[i] = (double)(i) * stheta + (double)(bly);
  }

  *x_locus = xloc;
  *y_locus = yloc;
  return(slicepixels);
} /* end get_slice() */
/*-----------------------------------------------------------------------------*/
iarray pvslice(iarray cube, unsigned int num_points, double *x_locus,
		double *y_locus)
{

  /* From a list of pixels to extract, returns a "position-velocity" array from
     a datacube.
     The cube is assumed to have axes in x,y,v order.
     The output array has the same spatial scale as input cube, ie
     the scale is assumed to be the same for both spatial axes,
     and velocity pixels are given the same width as the channel spacing.
     Output is a weighted sum over all pixels nearby the point where the
     slice locus passes, to reduce aliasing effects.


     There are four cases to consider for the neighbour pixels, depending
     where within a pixel the hit occurs:

             |-----------|-----------|     |-----------|-----------|
             |           |           |	   |           |           |
        2.0  -     3     |     2     | 	   |     3     |     2     |
             |           |           |	   |       xy  |           |
             |-----------|-----------|	   |-----------|-----------|
             |           | xy        |	   |           |           |
        1.0  -     4     |     1     |	   |     4     |     1     |
             |           |           |	   |           |           |
             |-----|-----|-----|-----|	   |-----------|-----------|
                  1.0         2.0

             |-----------|-----------|     |-----------|-----------|
             |           |           |	   |           |           |
             |     3     |     2     | 	   |     3     |     2     |
             |           | xy        |	   |           |           |
             |-----------|-----------|	   |-----------|-----------|
             |           |           |	   |       xy  |           |
             |     4     |     1     |	   |     4     |     1     |
             |           |           |	   |           |           |
             |-----------|-----------|	   |-----------|-----------|

     All these can be handled by int(x+/-0.5), int(y+/-0.5)
     Pixel coordinates are associated with centres of pixels.
     Some pixels don't have four neighbours e.g. at the edges of the channel
     maps. If one of the 4 is missing, I 'mask' it with a zero weight.
  */
  iarray pvmap;
#define MAXNB 5

  int    xpix, ypix, zpix, lpix, blx, bly, trx, try;
  int    i, j, z, xp, yp, xn, yn;
  iarray nb;

  float  xc, yc, xx, yy, f, sw;
  iarray wt;

  /* NB for iarrays, and Karma genrally; 0th is most significant dimension */
  zpix = iarray_dim_length (cube, 0);
  ypix = iarray_dim_length (cube, 1);
  xpix = iarray_dim_length (cube, 2);
  if ( !(xpix*ypix*zpix > 0) ) {
    (void) fprintf (stderr, "PVGET: problem with input cube dimensions\n");
  }

  lpix = num_points;

#ifdef TESTSUITE
  (void) fprintf (stderr,"locus is %d pixels long\n",lpix);
#endif

  pvmap  = iarray_create_2D ( zpix,  lpix, K_FLOAT);
  wt     = iarray_create_2D ( MAXNB, lpix, K_FLOAT);
  nb     = iarray_create_3D ( 2, MAXNB, lpix, K_INT);

  /* find the neighbour pixels. ---------------------------------------------- */
  for (i=0; i<lpix; i++) {

    xc = x_locus[i];          yc = y_locus[i];
    xp = (int)( xc+0.5 );         yp = (int)( yc+0.5 );
    /* here I only do the 4 nearest pixels but the code could be altered to do
       a summation over e.g. a gaussian beam */
    I3(nb,0,0,i) = xp;            I3(nb,1,0,i) = yp; /* pixel we landed on */
    I3(nb,0,1,i) = (int)(xc+0.5); I3(nb,1,1,i) = (int)(yc-0.5); /* neigbours */
    I3(nb,0,2,i) = (int)(xc+0.5); I3(nb,1,2,i) = (int)(yc+0.5);
    I3(nb,0,3,i) = (int)(xc-0.5); I3(nb,1,3,i) = (int)(yc+0.5);
    I3(nb,0,4,i) = (int)(xc-0.5); I3(nb,1,4,i) = (int)(yc-0.5);

    /* calculate the weight for each neighbour */
    for (j=1; j<MAXNB; j++) {
      xp = I3(nb, 0,j,i); yp = I3(nb, 1,j,i);
      xx = (float)(xp);   yy = (float)(yp);
      F2(wt, j,i) = 0.0;
      if (xp >= 0 && xp<xpix && yp>=0 && yp<ypix) {
        F2 (wt, j,i) = weight(xx,yy,xc,yc);
      }
    } /* end neighbours loop */
  } /* end slice loop */

  /* apply neighbour list & weights to each x,y plane in turn  --------------- */
  for (z=0; z<zpix; z++) {
    for (i=0; i<lpix; i++) {
      f = 0.0; sw = 0.0;
      xp = I3(nb, 0,0,i);
      yp = I3(nb, 1,0,i);

      for (j=1; j<MAXNB; j++) {
        xn = I3(nb, 0,j,i);
        yn = I3(nb, 1,j,i);
	/* The first test protects the second; it is possible for xn and yn
	   to go out of range (neighbours of a pixel at edge of cube) */
        if ( F2(wt, j,i) >0.0 && F3(cube, z,yn,xn)!=TOOBIG ) {
          sw = sw + F2(wt, j,i);
          f  = f  + F3(cube, z,yn,xn) * F2(wt, j,i);
        }
      } /* end neighbours loop */

      if ( sw>0.0 ) {
        F2(pvmap, z,i) = f/sw;
      } else {
	if ( xp >= 0 && xp<xpix && yp>=0 && yp<ypix ) {
	  F2(pvmap, z,i) = F3(cube, z,yp,xp);
	}else {
	  F2(pvmap, z,i) = 0.0;
	}
      }
    } /* end slice loop */
  } /* end channels loop */

  return(pvmap);
} /* end pvslice() */
/*-----------------------------------------------------------------------------*/
#ifdef TESTSUITE
void main(argc, argv)
int   argc;
char *argv[];
{
#define MAXNB 5

  iarray cube, pvmap;
  double *x_locus, *y_locus;
  char  *cubefile, *pvmapfile;
  int   x1, y1, x2, y2;

  int xpix,ypix,zpix,lpix;

  if (argc==7 ) {
    cubefile  = argv[1];
    pvmapfile = argv[2];
    x1        = atoi (argv[3]);
    y1        = atoi (argv[4]);
    x2        = atoi (argv[5]);
    y2        = atoi (argv[6]);
  }else{
    printf("Usage: <cube> <o/p slice> <x1> <y1> <x2> <y2>\n");
    exit (1);
  }    
  /* Read in from disk a file called "input" which contains an iarray */
  cube = iarray_read_nD (cubefile, FALSE, NULL, 3, NULL, NULL, K_CH_MAP_LOCAL);

  /* need something to check the inputs */

  zpix = iarray_dim_length (cube, 0);
  ypix = iarray_dim_length (cube, 1);
  xpix = iarray_dim_length (cube, 2);
  if ( !(xpix*ypix*zpix > 0) ) {
    (void) fprintf (stderr, "PVGET: problem with input cube dimensions\n");
    exit(1);
  }

  /*extract the slice */
  lpix = get_slice(cube,x1,y1,x2,y2,&x_locus,&y_locus);
  (void) fprintf (stderr, "got the slice locus, %d points\n",lpix);

  pvmap = pvslice(cube,lpix,x_locus,y_locus);
  (void) m_free( (char *) x_locus);
  (void) m_free( (char *) y_locus);

  (void) fprintf (stderr, "slice extracted, writing...\n");

  /* Write pvmap to disk */
  iarray_write (pvmap, pvmapfile);

  exit(0);
}
#endif
