
/*----------------------------------------------------------------------*/
/* This code provides an image display widget for Xt. */
/**/
/*
 Name		         Class		   RepType         Default Value
 ----		         -----		   -------         -------------
 pseudoColourCanvas      WorldCanvas         Pointer         NULL
 directColourCanvas      WorldCanvas         Pointer         NULL
 trueColourCanvas        WorldCanvas         Pointer         NULL
 pseudoColourLeftCanvas  WorldCanvas         Pointer         NULL
 pseudoColourRightCanvas WorldCanvas         Pointer         NULL
 directColourLeftCanvas  WorldCanvas         Pointer         NULL
 directColourRightCanvas WorldCanvas         Pointer         NULL
 trueColourLeftCanvas    WorldCanvas         Pointer         NULL
 trueColourRightCanvas   WorldCanvas         Pointer         NULL
 visibleCanvas           WorldCanvas         Pointer         NULL
 imageName               ImageName           String          "fred"
 enableAnimation         EnableAnimation     Bool            False
 showAnimateButton       ShowAnimateButton   Bool            True
 showQuitButton          ShowQuitButton      Bool            True
 cmapSize                CmapSize            Int             200
 fullscreen              Fullscreen          Bool            False
 verbose                 Verbose             Bool            False
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
#define XkwNimageName "imageName"
#define XkwNenableAnimation "enableAnimation"
#define XkwNshowAnimateButton "showAnimateButton"
#define XkwNshowQuitButton "showQuitButton"
#define XkwNcmapSize "cmapSize"
#define XkwNfullscreen "fullscreen"
#define XkwNverbose "verbose"

#define XkwCWorldCanvas "WorldCanvas"
#define XkwCImageName "ImageName"
#define XkwCEnableAnimation "EnableAnimation"
#define XkwCShowAnimateButton "ShowAnimateButton"
#define XkwCShowQuitButton "ShowQuitButton"
#define XkwCCmapSize "CmapSize"
#define XkwCFullscreen "Fullscreen"
#define XkwCVerbose "Verbose"

#endif
