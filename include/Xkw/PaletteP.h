
#ifndef PALETTEP__H
#define PALETTEP__H

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <karma_kcmap.h>
#include <Xkw/Palette.h>

typedef struct _PalettePart
{
  Pixel foreground;
  float minimum; 
  float maximum;
  float value;
  float scale;
  int division;
  XkwOrientation orientation;
  GC gc;
  GC eraseGC;
  GC cbarGC;
  Kcolourmap dcm;
  XtCallbackList valueChangeCallback;
} PalettePart, *PalettePartPtr;

typedef struct _PaletteRec
{
  CorePart core;
  PalettePart palette;
} PaletteRec, *PalettePtr;

typedef struct _PaletteClassPart
{
  int empty;
} PaletteClassPart;

typedef struct _PaletteClassRec
{
  CoreClassPart core_class;	
  PaletteClassPart palette_class;
} PaletteClassRec, *PaletteClassPtr;

extern PaletteClassRec paletteClassRec;

#endif /* PALETTEP__H */
