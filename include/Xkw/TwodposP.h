
#ifndef TWODPOSP__H
#define TWODPOSP__H

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <Xkw/Twodpos.h>
#include <karma.h>

typedef struct _TwodposPart
{
  Pixel foreground;
  float minimum_x; 
  float maximum_x;
  float value_x;
  float minimum_y; 
  float maximum_y;
  float value_y;
  /*  Private section  */
  double scale_x;
  double scale_y;
  GC gc;
  GC eraseGC;
  XtCallbackList valueChangeCallback;
} TwodposPart, *TwodposPartPtr;

typedef struct _TwodposRec
{
  CorePart core;
  TwodposPart twodpos;
} TwodposRec, *TwodposPtr;

typedef struct _TwodposClassPart
{
  int empty;
} TwodposClassPart;

typedef struct _TwodposClassRec
{
  CoreClassPart core_class;	
  TwodposClassPart twodpos_class;
} TwodposClassRec, *TwodposClassPtr;

extern TwodposClassRec twodposClassRec;

#endif
