
#ifndef TWODPOSP__H
#define TWODPOSP__H

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <Xkw/Twodpos.h>

typedef struct _TwodposPart
{
  Pixel foreground;
  double minimum_x; 
  double maximum_x;
  double value_x;
  double minimum_y; 
  double maximum_y;
  double value_y;
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
