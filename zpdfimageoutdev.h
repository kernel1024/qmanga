/*========================================================================
//
// ImageOutputDev.h
//
// Copyright 1998-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2006 Rainer Keller <class321@gmx.de>
// Copyright (C) 2008 Timothy Lee <timothy.lee@siriushk.com>
// Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2010 Jakob Voss <jakob.voss@gbv.de>
// Copyright (C) 2012, 2013 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Thomas Freitag <Thomas.Freitag@alfa.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================
*/

#ifndef ZPDFIMAGEOUTDEV_H
#define ZPDFIMAGEOUTDEV_H

#ifdef WITH_POPPLER

#include <QVector>
#include <poppler/poppler-config.h>
#include <poppler/cpp/poppler-version.h>
#include <stdio.h>
#include <poppler/goo/gtypes.h>
#include <poppler/OutputDev.h>

#if POPPLER_VERSION_MAJOR==0
    #if POPPLER_VERSION_MINOR<70
        #define JPDF_PRE070_API 1
    #endif
    #if POPPLER_VERSION_MINOR<71
        #define JPDF_PRE071_API 1
    #endif
#endif

#ifndef JPDF_PRE071_API
    #define GBool bool
    #define gTrue true
    #define gFalse false
#endif

class ZPDFImg {
public:
    Goffset pos, size;
    QString format;
    ZPDFImg();
    ZPDFImg(const ZPDFImg& other);
    ZPDFImg(Goffset a_pos, Goffset a_size, QString a_format);
    ZPDFImg &operator=(const ZPDFImg& other);
    bool operator==(const ZPDFImg& ref) const;
    bool operator!=(const ZPDFImg& ref) const;
};

class GfxState;

class ZPDFImageOutputDev: public OutputDev {
public:

    bool imageCounting;

    ZPDFImageOutputDev();

    ZPDFImg getPage(int idx) const { return pages.at(idx); }
    int getPageCount() const { return pages.count(); }

    // Check if file was successfully created.
    virtual GBool isOk() { return ok; }

    // Does this device use tilingPatternFill()?  If this returns false,
    // tiling pattern fills will be reduced to a series of other drawing
    // operations.
    virtual GBool useTilingPatternFill() { return gTrue; }

    // Does this device use beginType3Char/endType3Char?  Otherwise,
    // text in Type 3 fonts will be drawn with drawChar/drawString.
    virtual GBool interpretType3Chars() { return gFalse; }

    // Does this device need non-text content?
    virtual GBool needNonText() { return gTrue; }

    // Start a page
    virtual void startPage(int pageNumA, GfxState *state, XRef *xref);

    //---- get info about output device

    // Does this device use upside-down coordinates?
    // (Upside-down means (0,0) is the top left corner of the page.)
    virtual GBool upsideDown() { return gTrue; }

    // Does this device use drawChar() or drawString()?
    virtual GBool useDrawChar() { return gFalse; }

    //----- path painting
#ifdef JPDF_PRE070_API
    virtual GBool tilingPatternFill(GfxState *state, Gfx *gfx, Catalog *cat, Object *str,
                                    double *pmat, int paintType, int tilingType, Dict *resDict,
                                    double *mat, double *bbox,
                                    int x0, int y0, int x1, int y1,
                                    double xStep, double yStep);
#else
    virtual GBool tilingPatternFill(GfxState *state, Gfx *gfx, Catalog *cat, Object *str,
                                    const double *pmat, int paintType, int tilingType, Dict *resDict,
                                    const double *mat, const double *bbox,
                                    int x0, int y0, int x1, int y1,
                                    double xStep, double yStep);
#endif

    //----- image drawing
    virtual void drawImageMask(GfxState *state, Object *ref, Stream *str,
                               int width, int height, GBool invert,
                               GBool interpolate, GBool inlineImg);
    virtual void drawImage(GfxState *state, Object *ref, Stream *str,
                           int width, int height, GfxImageColorMap *colorMap,
                           GBool interpolate, int *maskColors, GBool inlineImg);
    virtual void drawMaskedImage(GfxState *state, Object *ref, Stream *str,
                                 int width, int height,
                                 GfxImageColorMap *colorMap,
                                 GBool interpolate,
                                 Stream *maskStr, int maskWidth, int maskHeight,
                                 GBool maskInvert, GBool maskInterpolate);
    virtual void drawSoftMaskedImage(GfxState *state, Object *ref, Stream *str,
                                     int width, int height,
                                     GfxImageColorMap *colorMap,
                                     GBool interpolate,
                                     Stream *maskStr,
                                     int maskWidth, int maskHeight,
                                     GfxImageColorMap *maskColorMap,
                                     GBool maskInterpolate);

private:
    void writeImage(GfxState *state, Object *ref, Stream *str,
                    int width, int height, GfxImageColorMap *colorMap, GBool inlineImg);

    GBool ok;			// set up ok?

    QVector<ZPDFImg> pages;
};

#endif // WITH_POPPLER

#endif // ZPDFIMAGEOUTDEV_H
