/* mat.c
 Simple matrix manipulations.
 Written by Knox Long, STScI (ksl@stsci.edu) 1988.
 Updated by Vince McIntyre 15-OCT-1996: clean up to conform to programming style
                                        of karma modules.
 [PURPOSE]
 Simple double precision matrix arithmetic routines for 2-d matrices 
	mat_mult(c,a,b,rowsa,colsa,colsb): 	C = A X B. 
		Note that C should refer to the same physical storage space as
		A or B so one should be careful when dealing with operations
		like  A = B X A.  This is not a problem with any of the
		routines below.
	mat_add(c,a,b,row,col):			C = A + B
	mat_sub(c,a,b,row,col):			C = A - B
	mat_sca(c,scalar,row,col):		C *= scalar
	mat_copy(c,a,row,col)			A---->C

*/
#include <stdio.h>
/* #define		DIAG	0*/

/*---------------------------------------------------------------------------*/
void mat_mult(double *c, double *a, double *b, int rowsa, int colsa, int colsb)
{
    int i,j,k,l;

    for (i = 0; i < rowsa; i++)
    {
	for (k = 0; k < colsb; k++)
	{
	    l = i * colsb + k;
	    c[l] = 0.0;
	    for (j = 0; j < colsa; j++)
	    {
		c[l] += a[i * colsa + j] * b[j * colsb + k];
	    }
	}
    }
}

/*---------------------------------------------------------------------------*/
void mat_add (double *c, double *a, double *b, int row, int col)
{
int i,imax;
imax=row,col;
for(i=0;i<imax;i++) c[i]=a[i]+b[i];
}

/*---------------------------------------------------------------------------*/
void mat_sub (double *c, double *a, double *b, int row, int col)
{
int i,imax;
imax=row,col;
for(i=0;i<imax;i++) c[i]=a[i]-b[i];
}
/*---------------------------------------------------------------------------*/

/*
mat_sca(c,scalar,row,col)
double c[],scalar;
int row,col;
*/
void mat_sca(double c[], double scalar, int row, int col)
{
int i,imax;
imax=row,col;
for(i=0;i<imax;i++) c[i]*=scalar;
}

/*---------------------------------------------------------------------------*/
/*
mat_copy(c,a,row,col)
double c[],a[];
int row,col;
*/
void mat_copy(double c[], double a[], int row, int col)
{
int i,imax;
imax=row,col;
for(i=0;i<imax;i++) c[i]=a[i];
}
