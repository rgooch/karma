
/*----------------------------------------------------------------------*/
/* This code provides an image display widget for Xt. */
/**/
/*
 Name                             Class               RepType     Default Value
 ----                             -----               -------     -------------
 XkwNpseudoColourCanvas           WorldCanvas         Pointer     NULL
 XkwNdirectColourCanvas           WorldCanvas         Pointer     NULL
 XkwNtrueColourCanvas             WorldCanvas         Pointer     NULL
 XkwNpseudoColourLeftCanvas       WorldCanvas         Pointer     NULL
 XkwNpseudoColourRightCanvas      WorldCanvas         Pointer     NULL
 XkwNdirectColourLeftCanvas       WorldCanvas         Pointer     NULL
 XkwNdirectColourRightCanvas      WorldCanvas         Pointer     NULL
 XkwNtrueColourLeftCanvas         WorldCanvas         Pointer     NULL
 XkwNtrueColourRightCanvas        WorldCanvas         Pointer     NULL
 XkwNvisibleCanvas                WorldCanvas         Pointer     NULL
 XkwNmagnifierPseudoColourCanvas  WorldCanvas         Pointer     NULL
 XkwNmagnifierDirectColourCanvas  WorldCanvas         Pointer     NULL
 XkwNmagnifierTrueColourCanvas    WorldCanvas         Pointer     NULL
 XkwNmagnifierVisibleCanvas       WorldCanvas         Pointer     NULL
 XkwNimageName                    ImageName           String      "fred"
 XkwNenableAnimation              EnableAnimation     Bool        False
 XkwNshowAnimateButton            ShowAnimateButton   Bool        True
 XkwNshowQuitButton               ShowQuitButton      Bool        True
 XkwNcmapSize                     CmapSize            Int         200
 XkwNfullscreen                   Fullscreen          Bool        False
 XkwNautoIntensityScale           AutoIntensityScale  Bool        True
 XkwNnumTrackLabels               NumTrackLabels      Cardinal    0
 XkwNverbose                      Verbose             Bool        False
*/    
/*----------------------------------------------------------------------*/

#ifndef IMAGEDISPLAY__H
#define IMAGEDISPLAY__H

#include <X11/Xmu/Converters.h>
#include <karma_canvas.h>
#include <Xkw/Filepopup.h>
#include <Xkw/Dataclip.h>
#include <Xkw/AnimateControl.h>

extern WidgetClass imageDisplayWidgetClass;
typedef struct _ImageDisplayClassRec *ImageDisplayWidgetClass;
typedef struct _ImageDisplayRec *ImageDisplayWidget;

#define XtIsImageDisplay(w) XtIsSubclass((w), imageDisplayWidgetClass)

#define XkwNpseudoColourCanvas "pseudoColourCanvas"
#define XkwNdirectColourCanvas "directColourCanvas"
#define XkwNtrueColourCanvas "trueColourCanvas"
#define XkwNpseudoColourLeftCanvas "pseudoColourLeftCanvas"
#define XkwNpseudoColourRightCanvas "pseudoColourRightCanvas"
#define XkwNdirectColourLeftCanvas "directColourLeftCanvas"
#define XkwNdirectColourRightCanvas "directColourRightCanvas"
#define XkwNtrueColourLeftCanvas "trueColourLeftCanvas"
#define XkwNtrueColourRightCanvas "trueColourRightCanvas"
#define XkwNvisibleCanvas "visibleCanvas"
#define XkwNmagnifierPseudoColourCanvas "magnifierPseudoColourCanvas"
#define XkwNmagnifierDirectColourCanvas "magnifierDirectColourCanvas"
#define XkwNmagnifierTrueColourCanvas "magnifierTrueColourCanvas"
#define XkwNmagnifierVisibleCanvas "magnifierVisibleCanvas"
#define XkwNimageName "imageName"
#define XkwNenableAnimation "enableAnimation"
#define XkwNshowAnimateButton "showAnimateButton"
#define XkwNshowQuitButton "showQuitButton"
#define XkwNcmapSize "cmapSize"
#define XkwNfullscreen "fullscreen"
#define XkwNautoIntensityScale "autoIntensityScale"
#define XkwNnumTrackLabels "numTrackLabels"
#define XkwNverbose "verbose"

#define XkwCWorldCanvas "WorldCanvas"
#define XkwCImageName "ImageName"
#define XkwCEnableAnimation "EnableAnimation"
#define XkwCShowAnimateButton "ShowAnimateButton"
#define XkwCShowQuitButton "ShowQuitButton"
#define XkwCCmapSize "CmapSize"
#define XkwCFullscreen "Fullscreen"
#define XkwCAutoIntensityScale "AutoIntensityScale"
#define XkwCNumTrackLabels "NumTrackLabels"
#define XkwCVerbose "Verbose"

/*----------------------------------------------------------------------
 * Functions
 *----------------------------------------------------------------------*/

EXTERN_FUNCTION (void XkwImageDisplayRefresh, (Widget W, flag clear) );


#endif
