/* #define		DIAG	0*/
#define		YES	1
#define		NO	0

struct imgdes {
	double ra,dec;	/*ra,dec of a pixel position x,y*/
	double x,y;	/*fiducial pixel position (usually image center)*/
	double angle;	/*angle between column direction and north.
			Angle	 From	    To
			  0 ---- [1,1]  -->[1,10]
			 90 ---- [10,1] -->[1,1]
			180 ---- [1,10] --> [1,1]
			270 ---- [1,1]  --> [10,1]
			Note: Image described here in IRAF [Col,Row] notation.
			*/
	double scale;	/*degrees/pixel */
	int invert;	/* +-1
			If the image is displayed with 
				[1,1] 		at the lower left, 
				[Colmax,1] 	at the lower right,
				[Colmax,Rowmax]	at the upper right
				[1,Rowmax] 	at the upper left
			then
				+1---->east is 90 deg counterclockwise from
					north
				-1---->east is 90 deg        clockwise from
					north
			*/
	};


/* Stucture containing the data */
struct map {
	double ra,dec;		/*Ra,dec of stars used for orienting image*/
	double xrdn,yrdn;	/*Position of star in NUEL system         */
	double xim,yim;		/*Position of star in image               */
	double ximn,yimn;	/*Projection of xim,yim in NUEL system    */
	char	name[20];	/*Name of the star*/
	};

/* Structure definining the interrelation between points in ra,dec and image
space.  */
struct xxx {
	double drd;		/*dist. between two points in ra,dec.*/
	double ard;		/*angle betwwen two points in ra,dec */
	double dim;		/*dist. between two points in the image.*/
	double aim;		/*angle between the line defined by 
					two points and the x direction 
					in the image plane.*/
	};
