
/*----------------------------------------------------------------------
   This widget implements a data clip control

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 iarray                  Iarray           Pointer         NULL
 maxDataRegions          MaxDataRegions   int             1
 regionCallback          Callback         Callback        NULL
 showBlankControl        ShowBlankControl Bool            False
 fixedOutputType         FixedOutputType  Cardinal        NONE
 verbose                 Verbose          Bool            False
------------------------------------------------------------------------*/

#ifndef DATACLIP__H
#define DATACLIP__H

#include <karma_iarray.h>

extern WidgetClass dataclipWidgetClass;
typedef struct _DataclipClassRec *DataclipWidgetClass;
typedef struct _DataclipRec *DataclipWidget;

typedef struct
{
    flag blank_data_outside_regions;
    unsigned int num_regions;
    double *minima;
    double *maxima;
} DataclipRegions;

#define XtIsDataclip(w) XtIsSubclass((w), dataclipWidgetClass)

#define XkwNiarray "iarray"
#define XkwNmaxDataRegions "maxDataRegions"
#define XkwNregionCallback "regionCallback"
#define XkwNshowBlankControl "showBlankControl"
#define XkwNfixedOutputType "fixedOutputType"
#define XkwNverbose "verbose"

#define XkwCIarray "Iarray"
#define XkwCMaxDataRegions "MaxDataRegions"
#define XkwCShowBlankControl "ShowBlankControl"
#define XkwCFixedOutputType "FixedOutputType"
#define XkwCVerbose "Verbose"

#endif
