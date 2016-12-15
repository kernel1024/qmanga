/*========================================================================
//
// ImageOutputDev.cc
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
// Copyright (C) 2005, 2007, 2011 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006 Rainer Keller <class321@gmx.de>
// Copyright (C) 2008 Timothy Lee <timothy.lee@siriushk.com>
// Copyright (C) 2008 Vasile Gaburici <gaburici@cs.umd.edu>
// Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2009 William Bader <williambader@hotmail.com>
// Copyright (C) 2010 Jakob Voss <jakob.voss@gbv.de>
// Copyright (C) 2012, 2013 Adrian Johnson <ajohnson@redneon.com>
// Copyright (C) 2013 Thomas Fischer <fischer@unix-ag.uni-kl.de>
// Copyright (C) 2013 Hib Eris <hib@hiberis.nl>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================
*/

/*
 * Modified to capture image stream size and positions inside PDF file,
 * image and page counting, checking streams to be in supported image formats.
 */

#ifdef WITH_POPPLER

#include "zpdfimageoutdev.h"

#include <poppler/poppler-config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <math.h>
#include <poppler/goo/gmem.h>
#include <poppler/Error.h>
#include <poppler/GfxState.h>
#include <poppler/Object.h>
#include <poppler/Stream.h>
#include <QImageWriter>
#include "zpdfimageoutdev.h"

ZPDFImageOutputDev::ZPDFImageOutputDev()
{
    ok = gTrue;
    pages.clear();
    imageCounting = true;
}

void ZPDFImageOutputDev::startPage(int, GfxState *, XRef *)
{
}

GBool ZPDFImageOutputDev::tilingPatternFill(GfxState *, Gfx *, Catalog *, Object *,
                                            double *, int , int , Dict *, double *,
                                            double *, int , int , int , int ,
                                            double , double )
{
    return gTrue;
}

void ZPDFImageOutputDev::writeImage(GfxState *state, Object *ref, Stream *str,
                                    int width, int height,
                                    GfxImageColorMap *colorMap, GBool inlineImg) {
    Q_UNUSED(state)
    Q_UNUSED(ref)
    Q_UNUSED(width)
    Q_UNUSED(height)
    Q_UNUSED(colorMap)

    if (inlineImg) return;

    Goffset embedSize = str->getBaseStream()->getLength();
    Goffset embedPos = str->getBaseStream()->getStart();

    if (embedSize<=0 || embedPos <=0) return;

    if (str->getKind() == strDCT)
        pages << ZPDFImg(embedPos,embedSize,"JPG"); // jpg

    else if (str->getKind() == strJPX &&
             QImageWriter::supportedImageFormats().contains("JP2"))
        pages << ZPDFImg(embedPos,embedSize,"JP2"); // jpeg2000
}

void ZPDFImageOutputDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
                                       int width, int height, GBool,
                                       GBool, GBool inlineImg) {
    writeImage(state, ref, str, width, height, NULL, inlineImg);
}

void ZPDFImageOutputDev::drawImage(GfxState *state, Object *ref, Stream *str,
                                   int width, int height,
                                   GfxImageColorMap *colorMap,
                                   GBool, int *, GBool inlineImg) {
    writeImage(state, ref, str, width, height, colorMap, inlineImg);
}

void ZPDFImageOutputDev::drawMaskedImage(
        GfxState *state, Object *ref, Stream *str,
        int width, int height, GfxImageColorMap *colorMap, GBool,
        Stream *, int, int, GBool, GBool) {
    writeImage(state, ref, str, width, height, colorMap, gFalse);
}

void ZPDFImageOutputDev::drawSoftMaskedImage(
        GfxState *state, Object *ref, Stream *str,
        int width, int height, GfxImageColorMap *colorMap, GBool,
        Stream *, int, int,
        GfxImageColorMap *, GBool) {
    writeImage(state, ref, str, width, height, colorMap, gFalse);
}

ZPDFImg::ZPDFImg()
{
    pos = 0;
    size = 0;
    format = QString();
}

ZPDFImg::ZPDFImg(const ZPDFImg &other)
{
    pos = other.pos;
    size = other.size;
    format = other.format;
}

ZPDFImg::ZPDFImg(Goffset a_pos, Goffset a_size, QString a_format)
{
    pos = a_pos;
    size = a_size;
    format = a_format;
}

ZPDFImg &ZPDFImg::operator=(const ZPDFImg &other)
{
    pos = other.pos;
    size = other.size;
    format = other.format;
    return *this;
}

bool ZPDFImg::operator==(const ZPDFImg &ref) const
{
    return (pos==ref.pos && size==ref.size && format==ref.format);
}

bool ZPDFImg::operator!=(const ZPDFImg &ref) const
{
    return (pos!=ref.pos || size!=ref.size || format!=ref.format);
}
#endif
