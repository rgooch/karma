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

    Last updated by Vincent McIntyre 12-AUG-1996; minor tidyup; dump obsolete
  functions and optimise multithreading. New antialias routine.

    Updated by      Richard Gooch    26-SEP-1996: Changed to implement the new
  FITS-style co-ordinate handling, where dimension co-ordinates range from 0
  to length - 1.

    Updated by      Richard Gooch    12-OCT-1996: Removed unthreaded code (was
  disabled anyway).

    Last updated by Richard Gooch    28-OCT-1996: Changed from <abs> to <fabs>.


             **********  THIS IS A WORK IN PROGRESS  **********

*/
/* #define CMDLINE */          /* define to have cmd-line control, otherwise
                                  the program is an x-windows program */
/* #define TESTSUITE */        /* diagnostic messages */
/* #define GAUSSWT */          /* a new addition; summing over gaussian beam */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>       /* for unlink() */
#include <karma.h>
#include <karma_foreign.h>
#include <karma_iarray.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_m.h>
#include <karma_mt.h>

/* have to make a structure to pass to each thread, as I need >4 params and
   mt_launch_job only allows up to four. */
struct nbjobpars
{
    int ypix;
    int xpix;
    int slpix;
    int yp;
    int xp;
    int MAXNB;
};
struct chjobpars
{
    int    lpix;
    int    zpix;
    int    ypix;
    int    xpix;
    int    nch;
    int    MAXNB;
    iarray nb;
    iarray wt;
};

/*----------------------------------------------------------------------------*/
static float weight (float x, float y, float cx, float cy) {
 /*
 [PURPOSE] Calculate weight to give when summing contributions from neighbours.
 [INPUTS]
  <x>, <y>  current neighbour to calculate weight for.
 <cx>,<cy>  centre point.
 These are assumed to be in PIXEL coordinates.
 [NOTES]
 In this version I make the weight proportional to the product distance
 from the projected centre of the current slice pixel. The pixel this point
 lands on will get highest weight, but the near neighbours will get some
 weight too. This version avoids a division but ensuring positive values
 is costly?
  */

  float dx,dy,r,w;

  dx = 1 - (x-cx); dy = 1 - (y-cy); w = fabs(dx*dy); 
  return(w);
} /* end weight() */
/*----------------------------------------------------------------------------*/
static float oldweight (float x, float y, float cx, float cy) {
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
/*----------------------------------------------------------------------------*/
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
unsigned int get_slice(iarray cube, int x1, int y1, int x2, int y2,
			double **x_locus, double **y_locus) {

  /* [PURPOSE] determine a locus of pixels in a slice line, from two endpoints */
  /* [NOTES] 
     * The coordinates are returned via pointers to two dynamically allocated
     arrays, written to *x_locus and *y_locus; these must be externally freed.
     * The values have to be reals rather than integers, so that
     antialiasing can be done. Double-precision is used because this is the
     `least common denominator for C.
     * To improve antialiasing, calculate pixels that are sqrt(0.5) apart.
     This changes the angular scale but doesn't matter if I write proper headers

     * This function is designed to extract a straight-line slice.
     Once we allow arbitrary extraction loci, the approach needs to change.
     Then, one has to check on every pixel. If a locus vertex is outside the cube
     extent, just blank it.
     This could be used in implementing arbitrary loci, by chaining together line
     segments but would have to trap the case of going off the edge; blank
     the off-edge pixels in the trace, but keep the vertex points.
     [RETRUNS] Number of pixels in slice.
  */

  int    xpix, ypix, zpix, blx, bly, trx, try;
  int    i, j, slicepixels;

  float  dx, dy, theta, ctheta, stheta, slicelength;
  double *xloc, *yloc;

  zpix = iarray_dim_length (cube, 0);
  ypix = iarray_dim_length (cube, 1);
  xpix = iarray_dim_length (cube, 2);
  if ( !(xpix*ypix*zpix > 0) ) {
    (void) fprintf (stderr, "GETSLICE: problem with input cube dimensions\n");
  }

  if ( !check_bounds(&x1,&y1,&x2,&y2,xpix,ypix,&blx,&bly,&trx,&try) ) return (0);
#ifdef CMDLINE
  (void) fprintf(stdout, "endpoints are %d %d %d %d\n",blx,bly,trx,try);
#endif
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

  /* remember x,y_locus are passed in as pointers to pointers; need one deref */
  *x_locus = xloc;
  *y_locus = yloc;
  return(slicepixels);
} /* end get_slice() */
/*-----------------------------------------------------------------------------*/
  void get_n_chan(void *poolinfo,
		  void *callinfo1, void *callinfo2,
		  void *callinfo3, void *callinfo4,
		  void *threadinfo)
  /* [PURPOSE] Execute a number of channel-map slices on the same thread.
     [RETURNS] Nothing.
  */
{
  iarray cube               = (iarray)             callinfo1;
  int    z                  = (int)                callinfo2;
  struct chjobpars *chstuff = (struct chjobpars *) callinfo3;
  iarray pvmap              = (iarray)             callinfo4;

  int    i,j,xp,yp,xn,yn,MAXNB;
  float  sw,f;
  iarray nb,wt;
  int    lpix,xpix,ypix,zpix;
  int    zz, nch;

  lpix =  (*chstuff).lpix;  /* need brackets to make dereference occur first */
  zpix =  (*chstuff).zpix;
  ypix =  (*chstuff).ypix;
  xpix =  (*chstuff).xpix;
  nch  =  (*chstuff).nch;
  wt   =  (*chstuff).wt;
  nb   =  (*chstuff).nb;
  MAXNB=  (*chstuff).MAXNB;

  /* (void) fprintf(stderr,"  l=%d x=%d y=%d z=%d",lpix,xpix,ypix,z);*/
  if ( (z + nch) > zpix ) nch = zpix - z;
  for ( zz = 0; zz < nch; ++zz, ++z) {   /* do nch channels on this thread */
    for (i=0; i<lpix; i++) { /* start slice loop */
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
} /* end get_n_chan */
/*-----------------------------------------------------------------------------*/
iarray pvslice(iarray cube, unsigned int num_points, 
		double *x_locus, double *y_locus)
/*  threaded version */
/*  [PURPOSE] Extract pixels from a cube, along an input locus.
    Result is antialiased.
    [NOTES]
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
    [RETURNS] "position-velocity" iarray, of length <num_points>.
*/
{
    iarray pvmap;
#define EPS 1.0e-8

    iarray nb;
    int    xpix, ypix, zpix, lpix, blx, bly, trx, try;
    int    i, j, k, l, m, z, xp, yp, xn, yn, MAXNB;
    int    zstep;
    float  xc, yc, xx, yy, f, sw, fw;
    iarray wt;

    KThreadPool pool = NULL; /* need this for thread pool */
    struct chjobpars chstuff;
    struct nbjobpars nbstuff;
    unsigned long dim_lengths[2];
    CONST char *dim_names[2];

    if (num_points<2) return(NULL);

    /* set number of `neighbour' pixels to use for anitaliasing */
    MAXNB = 5;

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
    dim_lengths[0] = zpix;
    dim_lengths[1] = lpix;
    dim_names[0] = iarray_dim_name (cube, 0);
    dim_names[1] = "ANGLE";
    pvmap = iarray_create (K_FLOAT, 2, dim_names, dim_lengths,
			   iarray_value_name (cube), NULL);
    wt     = iarray_create_2D ( MAXNB, lpix, K_FLOAT);
    nb     = iarray_create_3D ( 2, MAXNB, lpix, K_INT);

    /* find the neighbour pixels & antialiasing weights.                    */
    /* this is done for one channel only, then list applied to all channels */

    for (i=0; i<lpix; i++) {
	xc = x_locus[i];              yc = y_locus[i];
	xp = (int)( xc+0.5 );         yp = (int)( yc+0.5 );
	/* here I only do the 4 nearest pixels */
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
    /* do this in threaded code, to take advantage of multiple cpus if available */
    pool = mt_get_shared_pool();
    zstep = zpix / mt_num_threads (pool);
    /* these are read-only values, so use only one (module-global) instance of the
       structure */
    chstuff.lpix = lpix;
    chstuff.zpix = zpix;
    chstuff.xpix = xpix;
    chstuff.ypix = ypix;
    chstuff.nch  = zstep;
    chstuff.MAXNB= MAXNB;
    chstuff.nb   = nb;
    chstuff.wt   = wt;
    for ( z = 0; z < zpix; z += zstep) {
	mt_launch_job (pool, get_n_chan, (void *) cube, (void *) z,
		       (void *) &chstuff, (void *) pvmap);
    } /* end channels loop */
    mt_wait_for_all_jobs (pool);

    return(pvmap);
} /* end tpvslice() */
/*-----------------------------------------------------------------------------*/

#ifdef CMDLINE
#define VERSION "2.0"
static char *keywords[] =
{
    "OBJECT",
    "DATE-OBS",
    "EPOCH",
    "BUNIT",
    "TELESCOP",
    "INSTRUME",
    "EQUINOX",
    "BTYPE",
    "BMAJ",
    "BMIN",
    "BPA",
    "RESTFREQ",
    "VELREF",
    "PBFWHM",
    NULL
};


static flag fix_slice_header (iarray cube_arr, iarray image,
			      unsigned int num_points,
			      double slice_centre_ra, double slice_centre_dec,
			      double slice_position_angle)
/*  [SUMMARY] Add relevant info from the cube to the slice image
    [RETURNS] TRUE on success, FALSE if there was a problem.
*/
{
    flag           ok = TRUE;
    char           tmp_str[STRING_LENGTH];
    char           hist_str[STRING_LENGTH];
    unsigned int   i;
    static char    function_name[] = "fix_slice_new";

    /* NB:
       EPOCH is used by FITS to mean EQUINOX, ie the equinox the coords
       are referred to. The epoch of observation is stored in DATE-OBS.
       The revised FITS standard ecourages EQUINOX & deprecates EPOCH, but
       I don't attempt to fix headers that use the old way.

      */
    for (i = 0; keywords[i] != NULL; ++i)
    {
	if ( !iarray_copy_named_element (image, cube_arr, keywords[i],
					 FALSE, FALSE, TRUE) )
	{
	    fprintf (stderr, "%s: Failed to copy header keyword %s\n",
		     function_name, keywords[i]);
	    ok = FALSE;
	}
    }
    /* add history */
    sprintf (hist_str, "PVGET: Version %s\n", VERSION);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_ra (tmp_str, slice_centre_ra);
    sprintf (hist_str, "PVGET: Slice Centre RA %s\n", tmp_str);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_dec (tmp_str, slice_centre_dec);
    sprintf (hist_str, "PVGET: Slice Centre DEC %s\n", tmp_str);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_dec (tmp_str, slice_position_angle);
    sprintf (hist_str, "PVGET: Slice PA %s\n", tmp_str);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    return (ok);
}
/* end function fix_slice_header */
/* --------------------------------------------------------------------------*/
void main(argc, argv)
int   argc;
char *argv[];
{
#define MAXNB 5
  multi_array  *multi_desc;
  unsigned int ftype;
  iarray       cube, pvmap;
  double       fwhm;
  double       *x_locus, *y_locus;
  char         *cubefile, *pvmapfile;
  int          x1, y1, x2, y2;
  double       ra[3],dec[3],ra_off,dec_off,first_coord,last_coord,position_angle;
  dim_desc     *xdim, *ydim;
  Channel      fits_ch;
  KwcsAstro    cube_ap;
  int xpix,ypix,zpix,lpix;

  if (argc==7 || argc==8 ) {
    cubefile  = argv[1];
    pvmapfile = argv[2];
    x1        = atoi (argv[3]);
    y1        = atoi (argv[4]);
    x2        = atoi (argv[5]);
    y2        = atoi (argv[6]);
    fwhm      = 0.0;
    if (argc == 8) {fwhm = strtod (argv[7], (char **) NULL); }
  }else{
    printf("Usage: <cube> <o/p slice> <x1> <y1> <x2> <y2>\n");
    printf("       All inputs must be in PIXEL units.\n");
    exit (1);
  }

 /* Read in any kind of file; FITS, MIRIAD, KARMA */
 if ( ( multi_desc = foreign_guess_and_read (cubefile, K_CH_MAP_LOCAL,
                                             FALSE, &ftype,
                                             FA_GUESS_READ_END) ) == NULL )
    {
      (void) fprintf (stderr, "Error reading file: \"%s\"\n", cubefile);
      exit(1);
    }
    cube_ap = wcs_astro_setup (multi_desc->headers[0], multi_desc->data[0]);
    /* if (wcs_astro_test_radec(cube_ap)) {
     (void)fprintf(stderr," cube has ra, dec ok\n");
    }
    if (wcs_astro_test_velocity(cube_ap)) {
     (void)fprintf(stderr," cube has velocity ok\n");
    }
    */
 /* extract cube from karma general data structure */
 if ( (cube = iarray_get_from_multi_array(multi_desc,NULL,3,NULL,NULL))
      == NULL )
    {
        (void) fprintf (stderr, "Could not find file %s\n",cubefile);
        exit(1);
    }

 /* check the inputs */
 /* don't! check_bounds makes sure the endpoints are legal
 if ( x1 < 0 || x2 < 0 || y1 < 0 || y2 < 0 ) {
    (void) fprintf (stderr, "Negative pixel coordinates not allowed.\n");
    (void) fprintf (stderr, "Please use absolute coordinates.\n");
    exit(1);
 }
 */
 /* extract useful numbers from the cube */
 zpix = iarray_dim_length (cube, 0);
 ypix = iarray_dim_length (cube, 1);
 xpix = iarray_dim_length (cube, 2);
 if ( !(xpix*ypix*zpix > 0) ) {
   (void) fprintf (stderr, "PVGET: problem with input cube dimensions\n");
   exit(1);
 }
 
 /* get the slice locus */
 lpix = get_slice(cube,x1,y1,x2,y2,&x_locus,&y_locus);

 (void) fprintf (stderr, "got the slice locus, %d points\n",lpix);

 if (lpix>0) {
   /* extract slice from cube */
    pvmap = pvslice(cube,lpix,x_locus,y_locus);
    (void) fprintf (stderr, "slice extracted, fixing headers...\n");

    if ( wcs_astro_test_radec(cube_ap) ) {
    /* copy velocity coords from input cube. NB velocity axis is numbered 0 */
      iarray_set_dim_name (pvmap, 0, iarray_dim_name(cube, 0), TRUE);
      iarray_get_world_coords (cube, 0, &first_coord, &last_coord);
      iarray_set_world_coords (pvmap, 0, first_coord, last_coord);

      /* get an approximate estimate of the offset in degrees & the p.a.,
	 using cube linear world coordinates. How do I xform to proper world? */
      /* there's something wrong with getting the RA, DECs */
      xdim   = iarray_get_dim_desc (cube, 2);
      ydim   = iarray_get_dim_desc (cube, 1);
      ra[0]  = ds_get_coordinate (xdim, x_locus[0]);
      dec[0] = ds_get_coordinate (ydim, y_locus[0]);
      ra[1]  = ds_get_coordinate (xdim, x_locus[(lpix-1)]);
      dec[1] = ds_get_coordinate (ydim, y_locus[(lpix-1)]);
      ra[2]  = 0.5 * (  ra[0] +  ra[1] );
      dec[2] = 0.5 * ( dec[0] + dec[1] );
      (void)fprintf(stderr, "loc  %f, %f;  %f, %f\n",
		    x_locus[0],y_locus[0],x_locus[(lpix-1)],y_locus[(lpix-1)]);
      (void)fprintf(stderr, "ra,dec  %f, %f;  %f, %f\n",ra[0],dec[0],ra[1],dec[1]);
      ra_off = (ra[0] - ra[2])*cos( dec[2]*PION180 );
      dec_off= dec[1] - dec[2];
      position_angle = atan2( -ra_off, dec_off )/PION180 - 90.0;
      if (position_angle < 0 ) position_angle += 360.0;
      last_coord  = sqrt( ra_off*ra_off + dec_off*dec_off );
      first_coord = -last_coord;
      
      iarray_set_dim_name (pvmap, 1, "ANGLE", TRUE);
      iarray_set_world_coords(pvmap, 1, first_coord, last_coord);
      /* in dimension descriptor, fix up `reference' field */

      if ( ! fix_slice_header(cube, pvmap, lpix, ra[2], dec[2], position_angle) ) {
	/* do nothing fprintf(stderr, "Some problem with headers\n"); */
      }
    }else{
      (void)fprintf(stderr,"No world coordinate info in cube. O/p will be in pixel coords\n");
    }
    (void) m_free( (char *) x_locus);
    (void) m_free( (char *) y_locus);

    (void) fprintf (stderr, "writing...\n");
    /* Write pvmap to disk, as KARMA */
    /* iarray_write (pvmap, pvmapfile); */
    /* updating headers:  iarray_get_named_value, iarray_put_named_value */
    /* FITS standard axis descriptions and units */
    /* clean up */
    if ( ( fits_ch = ch_open_file(pvmapfile, "w") ) == NULL ) {
      (void) fprintf(stderr, "Error opening output file '%s'\n", pvmapfile);
      (void) ch_close(fits_ch);
      (void) unlink(pvmapfile);
      exit(1);
    }
    (void)fprintf(stderr,"opened file ok\n");
    if ( !foreign_fits_write_iarray(fits_ch, pvmap, FA_FITS_WRITE_END) ) {
      (void) fprintf(stderr,"Failed to write file '%s'\n", pvmapfile );
      (void) unlink(pvmapfile);
      exit(1);
    }
    (void) ch_close(fits_ch);
    (void) fprintf(stderr,"File '%s' sucessfully written\n", pvmapfile );

    iarray_dealloc(pvmap);
    iarray_dealloc(cube);

    exit(0);
 }else{
    iarray_dealloc(cube);
    exit(1);
 }
}
#endif
