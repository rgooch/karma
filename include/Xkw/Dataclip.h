
/*----------------------------------------------------------------------
   This widget implements a data clip control

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 iarray                  Iarray           Pointer         NULL
 intensityScaleCallback  Callback         Callback        NULL
 maxDataRegions          MaxDataRegions   int             1
 showIscaleButton        ShowIscaleButton Boolean         False
 regionCallback          Callback         Callback        NULL
 autoValueScale          AutoValueScale   Boolean         True
------------------------------------------------------------------------*/

#ifndef DATACLIP__H
#define DATACLIP__H

#include <karma_iarray.h>

extern WidgetClass dataclipWidgetClass;
typedef struct _DataclipClassRec *DataclipWidgetClass;
typedef struct _DataclipRec *DataclipWidget;

typedef struct
{
    unsigned int num_regions;
    double *minima;
    double *maxima;
} DataclipRegions;

#define XtIsDataclip(w) XtIsSubclass((w), dataclipWidgetClass)

#define XkwNiarray "iarray"
#define XkwNintensityScaleCallback "intensityScaleCallback"
#define XkwNmaxDataRegions "maxDataRegions"
#define XkwNshowIscaleButton "showIscaleButton"
#define XkwNregionCallback "regionCallback"
#define XkwNautoValueScale "autoValueScale"

#define XkwCIarray "Iarray"
#define XkwCMaxDataRegions "MaxDataRegions"
#define XkwCShowIscaleButton "ShowIscaleButton"
#define XkwCAutoValueScale "AutoValueScale"

#endif
