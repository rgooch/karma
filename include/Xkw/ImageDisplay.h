
/*----------------------------------------------------------------------*/
/* This code provides an image display widget for Xt. */
/**/
/*
 Name		       Class		   RepType         Default Value
 ----		       -----		   -------         -------------
 pseudoColourCanvas    WorldCanvas         Pointer         NULL
 directColourCanvas    WorldCanvas         Pointer         NULL
 trueColourCanvas      WorldCanvas         Pointer         NULL
 visibleCanvas         WorldCanvas         Pointer         NULL
 imageName             ImageName           String          "fred"
 enableAnimation       EnableAnimation     Boolean         False
 showQuitButton        ShowQuitButton      Boolean         True
 cmapSize              CmapSize            Int             200
*/    
/*----------------------------------------------------------------------*/

#ifndef IMAGEDISPLAY__H
#define IMAGEDISPLAY__H

#include <X11/Xmu/Converters.h>
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
#define XkwNvisibleCanvas "visibleCanvas"
#define XkwNimageName "imageName"
#define XkwNenableAnimation "enableAnimation"
#define XkwNshowQuitButton "showQuitButton"
#define XkwNcmapSize "cmapSize"

#define XkwCWorldCanvas "WorldCanvas"
#define XkwCImageName "ImageName"
#define XkwCEnableAnimation "EnableAnimation"
#define XkwCShowQuitButton "ShowQuitButton"
#define XkwCCmapSize "CmapSize"

#endif
