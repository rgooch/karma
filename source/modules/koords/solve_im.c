/* solve_im.c Get plate solution for image from input lists of RA,DEC & x,y */
/* to compile just this module:
 unix> gcc -c solve_im.c -I$KARMAINCLUDEPATH -Wall
 */

#include <stdio.h>
#include <strings.h>
#include <math.h>
#include <karma.h>
#include "koords.h"
#include "solve_im.h"

#define		EPSILON	1.e-10
/*#define DIAG*/

/* External functions */
/* File: radecio.c */
/* EXTERN_FUNCTION ( void put_radec, (double ra, double dec) ); */
/* File: mat.c */

/* Private functions */
/* STATIC_FUNCTION ( void put_imgdes, (struct imgdes *p) );*/
STATIC_FUNCTION (unsigned int trans_def,
		 (double *x, double *xpr,
		  double scale, double angle, int invert,
		  double mat[2][2], double *offset) );
STATIC_FUNCTION ( void lintran,
		  (double mat[2][2], double offset[], double xin[], double xout[]) );
STATIC_FUNCTION ( unsigned int radec_to_xy,
		  (double racen, double decen, double ra, double dec,
		   double *x, double *y) );
STATIC_FUNCTION ( unsigned int xy_to_radec,
		  (double racen, double decen, double x, double y,
		   double *ra, double *dec) );

/* Public functions */
/*-----------------------begin function compute_coords-------------------------*/
unsigned int compute_coords (double *crval1, double *crpix1, double *cdelt1,
                             double *crval2, double *crpix2, double *cdelt2,
                             double *crota, CONST char *projection,
                             unsigned int num_pairs,
                             CONST double *ra, CONST double *dec,
                             CONST double *x, CONST double *y,
                             unsigned int naxis1, unsigned int naxis2)
/*  [SUMMARY] Compute co-ordinate system.
    <crval1> The reference point along FITS axis 1 is written here.
    <crpix1> The reference pixel position along FITS axis 1 is written here.
    <cdelt1> The increment along FITS axis 1 is written here.
    <crval2> The reference point along FITS axis 2 is written here.
    <crpix2> The reference pixel position along FITS axis 2 is written here.
    <cdelt2> The increment along FITS axis 2 is written here.
    <crota> The rotation angle is written here.
    <projection> The projection system to use. If this is NULL the default
    "ARC" (rectangular) projection is used.
    <num_pairs> The number of co-ordinate pairs.
    <ra> The reference RA values.
    <dec> The reference DEC values.
    <x> The target x pixel values.
    <y> The target y pixel values.
    <naxis1> The length of the target images FITS axis 1.
    <naxis2> The length of the target images FITS axis 2.
    [RETURNS] The number of coefficients written on success, else 0.

 [HISTORY]
 Org: solve_im(), part of POSIT, by Knox Long at STScI (ksl@stsci.edu).
 Mod: Fix up some aspects of the output, so it's a bit more human-readable.
      1996/06/21 VJM
 Bug: Program doesn't trap the condition of too many input stars in plate soln.
      This results in 'segmentation fault' at runtime. Fixed 1992/06/29 VJM.
 Bug: This routine originally declared q as a pointer which did
	not cause an error on Sun 3/60's, but wouldn't run on IPC's.
	This is because no space is allocated to q, and it must be
	declared as a structure variable. Fixed 10-16-92.  Doug Mink & 
	S. Gordon
 Mod: Convert to interface with KOORDS, which allows graphical input of RA, DEC
      and x, y. 96/10/14 VJM
 Bug: Add 1 to CRPIXn values 96/11/13 REG,VJM

*/

{

#define MAXSTARS 30
#define MAXARRAY 900 /* This has to be >= dim(d)*(dim(d)-1) */
                     /* vjm 1992/06/29 */

struct imgdes    q;           /* the image description: centre, scale, rotn */
struct map       d[MAXSTARS]; /* the RA/DEC/X/Y data */
struct xxx       e[MAXARRAY]; /* Structure definining the interrelation between
				 points in ra,dec and image space */

int    i,n,m,mdata,ndata;
double s,c,xx,yy,dx,dy,refra,refdec,dis;
double xcenpr[2],xcen[2],scale,angle,mat[2][2],offset[2];
double lenradec,lenim;
double diff,diffinv,var,varinv,racen,decen,xin[2],xout[2];
double ds,dc,dsinv,dcinv;
int    invert;
char   proj[STRING_LENGTH];



sprintf ( proj, "ARC" );
if ( projection != NULL ) sprintf(proj,"%s",projection);

if ( num_pairs < 3 ) {
  fprintf(stderr,"Insufficient data for a solution.\nExiting...");
  return(0);
}
ndata = num_pairs;
if ( num_pairs >= MAXSTARS ) {
  ndata = MAXSTARS;
  fprintf(stderr, "Input data truncated after %d stars\n",MAXSTARS);
}

q.x = (double) naxis1 * 0.5;
q.y = (double) naxis2 * 0.5;

/* initialise the other coefficients */
q.scale=0.0;
q.invert=0;
q.angle=0.0;
q.ra=0.0;
q.dec=0.0;

/*put_imgdes(&q);*/
/* copy data into the POSIT data structure */
for (i=0; i < ndata; i++) {
	d[i].xim = x[i];
	d[i].yim = y[i];
	d[i].ra  = ra[i];
	d[i].dec = dec[i];
}


/*
   Calculate distances and angles between the points; 
   Do diffs from each point to all the others.
   The total no. of these is given by mdata, =ndata*(ndata-1)
*/

mdata=0;
for ( n=0; n < (ndata-1); n++){
  for ( m=n+1; m < ndata; m++){
    /*Calculate distances in ra,dec*/
    dis =  sin(PION180*d[m].dec)*sin(PION180*d[n].dec) +
      cos(PION180*d[m].dec)*cos(PION180*d[n].dec)*cos(PION180*(d[m].ra-d[n].ra));
    dis = acos(dis)/PION180;
    e[mdata].drd = dis;

    /* calculate angles in ra,dec */
    c  = sin(PION180*d[m].dec)-cos(PION180*dis)*sin(PION180*d[n].dec);
    c /= (sin(PION180*e[mdata].drd)*cos(PION180*d[n].dec));
    s  = sin(PION180*(d[m].ra-d[n].ra))*cos(PION180*d[m].dec)/sin(PION180*dis);
    e[mdata].ard = atan2(s,c)/PION180 + 90.0;
    if ( e[mdata].ard < 0.0 ) e[mdata].ard += 360.0;

    /* calculate distances in the image plane */
    dx           = d[m].xim - d[n].xim;
    dy           = d[m].yim - d[n].yim;
    e[mdata].dim = sqrt( dx*dx + dy*dy );
    
    /* Calculate angles in the image plane */
    e[mdata].aim = atan2(dy,dx) / PION180;
    if ( e[mdata].aim < 0.0 ) e[mdata].aim += 360.0;
		
#ifdef DIAG
 fprintf(stderr,"LENGTHS,ANGLES %d %d %d %f %f %f %f\n",
	 mdata,n,m,
	 e[mdata].drd,e[mdata].ard,e[mdata].dim,e[mdata].aim);
  fprintf(stderr," %12.8e\n",
	  (e[mdata].aim - e[mdata].ard));
  fprintf(stderr," %12.8e %12.8e\n",
	  sin((PION180*(e[mdata].aim - e[mdata].ard))),
	  cos((PION180*(e[mdata].aim - e[mdata].ard))));
#endif

    mdata++;
  }
}

/* determine the scale. */
lenradec=lenim=0.0;
for ( m=0; m < mdata; m++ ) {
  lenradec += e[m].drd;
  lenim    += e[m].dim;
}
q.scale=lenradec/lenim;
#ifdef DIAG
 fprintf(stderr,"The calculated image scale is %20.15e arcsec/pix\n",
	 3600.*q.scale);
#endif

/* determine rotation angle & whether the image is inverted */
diff=diffinv=ds=dc=dsinv=dcinv=0;
for ( m=0; m < mdata; m++ ) {
  xx     = e[m].aim - e[m].ard;
  ds    += sin(PION180*xx);
  dc    += cos(PION180*xx);
  xx     = e[m].aim + e[m].ard;
  dsinv += sin(PION180*xx);
  dcinv += cos(PION180*xx);
}
diff    = atan2(ds,dc)/PION180;
diffinv = atan2(dsinv,dcinv)/PION180;
#ifdef DIAG
 fprintf(stderr,"The average angular diff and diffinv is %20.15e %20.15e\n",
	 diff,diffinv);
#endif

var=varinv=0;
for ( m=0; m < mdata; m++ ) {
  xx       = e[m].aim - e[m].ard - diff;
  xx       = atan2(sin(PION180*xx),cos(PION180*xx));  /*now xx runs from +-pi*/
  var     += xx*xx;
  xx       = e[m].aim + e[m].ard - diffinv;
  xx       = atan2(sin(PION180*xx),cos(PION180*xx));  /*now xx runs from +-pi*/
  varinv  += xx*xx;
}
#ifdef DIAG
 fprintf(stderr,"The variance for var and vainv is %20.15e %20.15e\n",
	 var,varinv);
#endif

if(varinv<var) {
  q.invert=(-1); 
}
else {
  q.invert=1;
}

/* fix up sense of position angle */
if(q.invert==1) q.angle=diff;
else q.angle=diffinv+180;

#ifdef DIAG
 fprintf(stderr,"The calculated rotation angle is %20.15e deg.\n",q.angle);
#endif

/*
At this point it should be possible to rotate the image so that everything is
correct except the image shift. The following procedure estimates the shift.
	a. Estimate an image center based on the data points.
	b. Convert all the datapoints to NUEL with this center.
	c. Convert all the image data points to NUEL.  Because
           we do not accurately know the center of the image, the two sets of
           points will be offset w.r.t one another. 
	d. Calculate the mean offset.
	e. Invert the mean offset to an ra,dec
This is the "true" image center */

racen=decen=0.0;
for ( n=0; n < ndata; n++ ){
  racen += (d[n].ra+360);
  decen += d[n].dec;
}
racen /= ndata; racen -= 360.;
decen /= ndata;

/*
#ifdef DIAG
 printf("INITIAL ESTIMATE of RACEN DECEN %f %f\n",racen,decen);
#endif
*/

for ( n=0; n < ndata; n++){
  radec_to_xy(racen,decen,d[n].ra,d[n].dec,&d[n].xrdn,&d[n].yrdn);
}

xcenpr[0]=q.x; xcenpr[1]=q.y;
xcen[0]=xcen[1]=0;
scale=q.scale;
angle=(-(q.angle));
invert=q.invert;
if ( invert == -1 ) angle = q.angle+180;  

(void) trans_def(xcen,xcenpr,scale,angle,invert,mat,offset);

for ( n=0; n < ndata; n++ ) {
  xin[0]=d[n].xim; xin[1]=d[n].yim;
  (void) lintran(mat,offset,xin,xout);
  d[n].ximn=xout[0]; d[n].yimn=xout[1];
}

/* this next bit gets mean of x, y */
xx=yy=0;
for( n=0; n < ndata; n++ ) {
#ifdef DIAG
  printf("NUEL xy--rd %f %f  im %f %f\n",d[n].xrdn,d[n].yrdn,
		  d[n].ximn,d[n].yimn);
#endif
  xx += d[n].ximn-d[n].xrdn;
  yy += d[n].yimn-d[n].yrdn;
}
xx /= ndata;
yy /= ndata;

xy_to_radec(racen,decen,-xx,-yy,&refra,&refdec);

q.ra=refra; q.dec=refdec;

/* Now display the calculated image orientation*/
fprintf(stderr,"Here is the calculated image description\n");
/*put_imgdes(&q);*/
/* Now calculate pixel positions using the derived image orientation*/
/*
fprintf(stderr,"Results of comparing actual and predicted positions\n");
fprintf(stderr,
 "%3s %10s %7s %8s   %11s   %11s   %9s\n",
 "No.","   Name   ","  RA  ","  DEC  ","  Actual  ","Predicted","  Error  \n");
for ( n=0; n < ndata; n++ ) {
	radec_to_pix(&q,d[n].ra,d[n].dec,&x,&y);
}
*/

/* instead of this I must output crpix{1,2} crval{1,2}, cdelt{1,2} crota

Assumed projection in POSIT is the 'RECTANGULAR' or 'ARC' projection
 L = distance*sin(position_angle)
 M = distance*cos(position_angle)
 where cos(distance) = sin(dec)*sin(deccen) + cos(dec)*cos(deccen)*cos(ra-racen)
 To go the other way, from L,M to ra, dec:
 distance = sqrt (L^2 + M^2)
 sin(dec) = M*cos(deccen)/(distance/sin(distance)) + sin(deccen)*cos(distance)
 sin(ra-racen) = (sin(distance)/distance) * (L/cos(dec))

*/
/*  Stupid FITS (fortran) convention starts numbering CRPIX's from 1  */
*crpix1 = q.x + 1.0;
*crval1 = q.ra;
/*  Normally, CDELT1 is negative (RA decreases left to right (+x)), so we need
    to invert the sign of the invert value  */
*cdelt1 = -q.scale*( (double) q.invert); 
*crval2 = q.dec;
*crpix2 = q.y + 1.0;
*cdelt2 = q.scale;
*crota  = -q.angle;  /*  ???  */

return(7);
}

/*--------------------------end function solve_image--------------------------*/
/*-------------------------------begin function-------------------------------*/
/*void put_imgdes(struct imgdes *p)*/
     /*struct imgdes *p;*/
/*
{
double ra,dec;

ra  = p->ra;
dec = p->dec;

fprintf(stderr,"Image description:\n Ra,Dec:    %f %f deg.\n",ra,dec);
fprintf(stderr,"Ra, Dec in hms:"); put_radec(ra,dec);
fprintf(stderr,"\nxcen,ycen:    %f %f\n",p->x,p->y);
fprintf(stderr,"scale:          %f arcsec/pix\n",3600.*(p->scale));
fprintf(stderr,"rotation angle: %f degrees\n",   p->angle);
fprintf(stderr,"inversion:      %d (-1=inverted 1=not inverted)\n",   p->invert);
fprintf(stderr,"The rotation angle is the angle between north and the y axis.\n\
+90 degrees would mean that north is along the -x axis.\n");
 
}
*/
/*--------------------------------end function--------------------------------*/
/*-------------------------------begin function-------------------------------*/
unsigned int trans_def (double *x, double *xpr,
			double scale, double angle, int invert,
			double mat[2][2], double *offset)
{
double m[2][2],rot[2][2],q[2];
 
#ifdef DIAG
        printf("trans_def: x %f %f, xpr %f %f\n",x[0],x[1],xpr[0],xpr[1]);
        printf("trans_def: scale %f, angle %f, invert %d\n",scale,angle,invert);
#endif
 
m[0][0]=scale;
m[1][1]=invert*scale;
m[0][1]=m[1][0]=0.0;
 
rot[0][0]=rot[1][1]=cos(PION180*angle);
rot[1][0]=sin(PION180*angle);
rot[0][1]=(-rot[1][0]);
#ifdef DIAG
 fprintf(stderr,"m %f   %f\nm %f        %f\n",
        m[0][0],m[0][1],m[1][0],m[1][1]);
 fprintf(stderr,"rot %f %f\nrot %f      %f\n",
        rot[0][0],rot[0][1],rot[1][0],rot[1][1]);
#endif
 
mat_mult( (double *) mat, (double *) rot, (double *) m,2,2,2);
mat_mult(q, (double *) mat,xpr,2,2,1);
#ifdef DIAG
 fprintf(stderr,"q %f %f\n",q[0],q[1]);
#endif
mat_sub(offset,x,q,2,1);
 
#ifdef DIAG
 fprintf(stderr,"mat %f %f\n",
        mat[0][0],mat[0][1]);
 fprintf(stderr,"mat %f ",
        mat[1][0]);
 fprintf(stderr,"%f\n",
        mat[1][1]);
 fprintf(stderr,"offset %f\noffset %f\n",offset[0],offset[1]);
#endif
return(0);
}
/*-------------------------------begin function-------------------------------*/
void lintran(double mat[2][2], double offset[], double xin[], double xout[])
{
mat_mult(xout, (double *) mat,xin,2,2,1);
mat_add(xout,xout,offset,2,1);
}
/*--------------------------------end function--------------------------------*/
/*-------------------------------radecxy.c--------------------------------*/

/* Programs to move between ra's and dec's and a local xy coordinate
system.  For these programs the local xy system is oriented so
that +x is to the west and +y is to the north.  Thus the local
coordinate system has "north up and east to the left".  This will be
known as the NUEL system. The NUEL system has units of degrees.

All units for these programs are in fact in degrees!!!!

NB: the assumed projection here is the rectangular, or 'ARC' projection.
See AIPS memo 27, reissued 16 Jan 93 by E.Griesen, for complete mathematical
description.

radec_to_xy(racen,decen,ra,dec,x,y) :  Convert from an ra and dec to
		x,y in the NUEL system.
	racen,decen	ra and dec of the field center mapped to 0,0.
	ra,dec		ra and dec of the star to project.
	*x,*y		resulting x,y projection.

xy_to_radec(racen,decen,x,y,ra,dec) :  Convert from a x,y in the NUEL
		system to an ra,dec (in degrees).
	racen,decen	ra and dec of the field center mapped to 0,0.
	x,y		x,y whose corresponding ra and dec is desired.
	*ra,*dec	resulting ra and dec.

Coded by ksl 1/8/88.
*/

unsigned int radec_to_xy(double racen, double decen, double ra, double dec,
		 double *x, double *y)
{
double distance,angle,c,s;

#ifdef DIAG
 fprintf(stderr,"inputs: %f %f %f %f\n",racen,decen,ra,dec);
#endif
/*Convert everythin to radian*/
	racen *= PION180;
	decen *= PION180;
	ra    *= PION180;
	dec   *= PION180;

/*Calculate the distance from the center to the star using the law of cosines*/
	distance=sin(decen)*sin(dec) + cos(decen)*cos(dec)*cos(ra-racen);
	distance=acos(distance);
#ifdef DIAG
 fprintf(stderr,"distance %f\n",distance/PION180);
#endif
/* Check that the distance is not negligible*/
	if(distance<EPSILON) {(*x)=(*y)=0.0; return(0);}

/* Calculate the angle defined by the north pole, the center of
the field and the star using the law of sines.*/
	c=(sin(dec) - cos(distance)*sin(decen)) / (sin(distance)*cos(decen));
	s=(sin(ra-racen) * cos(dec)) / sin(distance);
	angle=atan2(s,c);  /* This is the angle E of N of the star */
#ifdef DIAG
 fprintf(stderr,"angle %f\n",angle/PION180);
#endif
	angle+=PI_ON_2;			/* To get the angle w.r.t. the X axis*/
/* Having determined the angle and the distance, calculate x and y */
	distance *= 1.0/PION180;
	(*x)=(distance*cos(angle)); 
	(*y)=(distance*sin(angle));
#ifdef DIAG
 fprintf(stderr,"xy: %f %f\n",*x,*y);
#endif
	return(0);
}
/*--------------------------------end function--------------------------------*/
/*------------------------------begin function--------------------------------*/

unsigned int xy_to_radec(double racen, double decen, double x, double y,
		 double *ra, double *dec)
{
double distance,angle,c,s;

#ifdef DIAG
 fprintf(stderr,"inputs: %f %f %f %f\n",racen,decen,x,y);
#endif

/* Convert it all to radians */
	racen *= PION180;
	decen *= PION180;
	x     *= PION180;
	y     *= PION180;
	distance=sqrt(x*x+y*y);
	angle=atan2(y,x)-PI_ON_2;  /* Subtraction of 90 deg is to
	 	                      get the angle E of north for the star*/
#ifdef DIAG
 fprintf(stderr,"distance,angle: %f %f\n",distance/PION180,angle/PION180);
#endif

/* Calculate the declination of the star using the law of cosines */
	(*dec)=cos(distance)*sin(decen) + sin(distance)*cos(decen)*cos(angle);
	(*dec)=asin((*dec));

/* Calculate the right ascension of the star using the law of sines
and remove any ambiguities around the poles using the law of cosines. */
	s=sin(angle)*sin(distance) / cos((*dec));
	c=(cos(distance) - sin(decen)*sin((*dec))) / (cos(decen)*cos((*dec)));
	(*ra)=racen+atan2(s,c);

/* Now put it all in degrees */
	(*ra)  *= 1.0/PION180;
	(*dec) *= 1.0/PION180;
#ifdef DIAG
 fprintf(stderr,"rac,dec: %f %f\n",*ra,*dec);
#endif
	return(0);
}
