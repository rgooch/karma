/*  ImageDisplayP.h

    Public header for  ImageDisplay  widget class.

    Copyright (C) 1994-1996  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*

    This include file contains the public class declarations for the
  ImageDisplay widget, an image display (application) widget for Xt.


    Written by      Richard Gooch   18-DEC-1994

    Last updated by Richard Gooch   21-OCT-1996

*/

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
