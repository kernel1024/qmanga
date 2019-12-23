/*
 Copyright (C) 2004, 2005, 2007
 Daniel M. Duley <daniel.duley@xxxxxxxxxxx>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
 Portions of this software are were originally based on ImageMagick's
 algorithms. ImageMagick is copyrighted under the following conditions:

Copyright (C) 2003 ImageMagick Studio, a non-profit organization dedicated to
making software imaging solutions freely available.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files ("ImageMagick"), to deal
in ImageMagick without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of ImageMagick, and to permit persons to whom the ImageMagick is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of ImageMagick.

The software is provided "as is", without warranty of any kind, express or
implied, including but not limited to the warranties of merchantability,
fitness for a particular purpose and noninfringement. In no event shall
ImageMagick Studio be liable for any claim, damages or other liability,
whether in an action of contract, tort or otherwise, arising from, out of or
in connection with ImageMagick or the use or other dealings in ImageMagick.

Except as contained in this notice, the name of the ImageMagick Studio shall
not be used in advertising or otherwise to promote the sale, use or other
dealings in ImageMagick without prior written authorization from the
ImageMagick Studio.
*/

#include <QImage>
#include <QApplication>
#include <cmath>
#include <vector>
#include <array>
#include "scalefilter.h"
/**
 * This is a port of the ImageMagick scaling functions from resize.c.
 *
 * The most signficant change is ImageMagick uses a function pointer for the
 * filter type. This is called usually a couple times in a loop for each
 * horizontal and vertical coordinate. I changed this into a switch statement
 * that does each type with inline functions. More code but faster.
 */

namespace BlitzScaleFilter{

constexpr double MagickEpsilon = 1.0e-6;
constexpr double MagickPI = 3.14159265358979323846264338327950288419716939937510;

using ContributionInfo = struct{
    double weight;
    unsigned int pixel;
};

constexpr int checkEventsFreq = 25;

bool horizontalFilter(const QImage *srcImg, QImage *destImg,
                      double x_factor, double blur,
                      std::vector<ContributionInfo> contribution,
                      Blitz::ScaleFilterType filter, int page,
                      const int *currentPage);
bool verticalFilter(const QImage *srcImg, QImage *destImg,
                    double y_factor, double blur,
                    std::vector<ContributionInfo> contribution,
                    Blitz::ScaleFilterType filter, int page,
                    const int *currentPage);

// These arrays were moved from their respective functions because they
// are inline
static const std::array<double,9> J1Pone = {
    0.581199354001606143928050809e+21,
    -0.6672106568924916298020941484e+20,
    0.2316433580634002297931815435e+19,
    -0.3588817569910106050743641413e+17,
    0.2908795263834775409737601689e+15,
    -0.1322983480332126453125473247e+13,
    0.3413234182301700539091292655e+10,
    -0.4695753530642995859767162166e+7,
    0.270112271089232341485679099e+4
};

static const std::array<double,9> J1Qone = {
    0.11623987080032122878585294e+22,
    0.1185770712190320999837113348e+20,
    0.6092061398917521746105196863e+17,
    0.2081661221307607351240184229e+15,
    0.5243710262167649715406728642e+12,
    0.1013863514358673989967045588e+10,
    0.1501793594998585505921097578e+7,
    0.1606931573481487801970916749e+4,
    0.1e+1
};

static const std::array<double,6> P1Pone = {
    0.352246649133679798341724373e+5,
    0.62758845247161281269005675e+5,
    0.313539631109159574238669888e+5,
    0.49854832060594338434500455e+4,
    0.2111529182853962382105718e+3,
    0.12571716929145341558495e+1
};

static const std::array<double,6> P1Qone = {
    0.352246649133679798068390431e+5,
    0.626943469593560511888833731e+5,
    0.312404063819041039923015703e+5,
    0.4930396490181088979386097e+4,
    0.2030775189134759322293574e+3,
    0.1e+1
};

static const std::array<double,6> Q1Pone = {
    0.3511751914303552822533318e+3,
    0.7210391804904475039280863e+3,
    0.4259873011654442389886993e+3,
    0.831898957673850827325226e+2,
    0.45681716295512267064405e+1,
    0.3532840052740123642735e-1
};

static const std::array<double,6> Q1Qone = {
    0.74917374171809127714519505e+4,
    0.154141773392650970499848051e+5,
    0.91522317015169922705904727e+4,
    0.18111867005523513506724158e+4,
    0.1038187585462133728776636e+3,
    0.1e+1
};

static const std::array<double,Blitz::SincFilter+1> filterSupport = {
    /*Undefined*/ 0.0,
    /*Point*/ 0.0,
    /*Box*/ 0.5,
    /*Triangle*/ 1.0,
    /*Hermite*/ 1.0,
    /*Hanning*/ 1.0,
    /*Hamming*/ 1.0,
    /*Blackman*/ 1.0,
    /*Gaussian*/ 1.25,
    /*Quadratic*/ 1.5,
    /*Cubic*/ 2.0,
    /*Catrom*/ 2.0,
    /*Mitchell*/ 2.0,
    /*Lanczos*/ 3.0,
    /*BlackmanBessel*/ 3.2383,
    /*BlackmanSinc*/ 4.0
};

inline double J1(double x){
    double p;
    double q;
    auto ip = J1Pone.crbegin();
    auto iq = J1Qone.crbegin();
    p=*ip; q=*iq;
    for(; ip!=J1Pone.crend(); ++ip, ++iq){
        p=p*x*x+*ip;
        q=q*x*x+*iq;
    }
    return(p/q);
}

inline double P1(double x){
    double p;
    double q;
    auto ip = P1Pone.crbegin();
    auto iq = P1Qone.crbegin();
    p=*ip; q=*iq;
    for(; ip!=P1Pone.crend(); ++ip, ++iq){
        p=p*(8.0/x)*(8.0/x)+*ip;
        q=q*(8.0/x)*(8.0/x)+*iq;
    }
    return(p/q);
}

inline double Q1(double x){
    double p;
    double q;
    auto ip = Q1Pone.crbegin();
    auto iq = Q1Qone.crbegin();
    p=*ip; q=*iq;
    for(; ip!=Q1Pone.crend(); ++ip, ++iq){
        p=p*(8.0/x)*(8.0/x)+*ip;
        q=q*(8.0/x)*(8.0/x)+*iq;
    }
    return(p/q);
}

inline double BesselOrderOne(double x){
    double p;
    double q;
    if(x == 0.0)
        return(0.0);
    p = x;
    if(x < 0.0)
        x = (-x);
    if(x < 8.0)
        return(p*J1(x));
    q = std::sqrt(2.0/(MagickPI*x))*
        (P1(x)*(1.0/std::sqrt(2.0)*(std::sin(x)-std::cos(x))) -
         8.0/x*Q1(x)*(-1.0/std::sqrt(2.0)*(std::sin(x)+std::cos(x))));
    if (p < 0.0)
        q=(-q);
    return(q);
}

inline double Bessel(const double x, const double /*support*/){
    if(x == 0.0)
        return(MagickPI/4.0);
    return(BesselOrderOne(MagickPI*x)/(2.0*x));
}

inline double Sinc(const double x, const double /*support*/){
    if(x == 0.0)
        return(1.0);
    return(std::sin(MagickPI*x)/(MagickPI*x));
}

inline double Blackman(const double x, const double /*support*/){
    return(0.42+0.5*std::cos(MagickPI*x)+0.08*std::cos(2*MagickPI*x));
}

inline double BlackmanBessel(const double x,const double support){
    return(Blackman(x/support,support)*Bessel(x,support));
}

inline double BlackmanSinc(const double x, const double support){
    return(Blackman(x/support,support)*Sinc(x,support));
}

inline double Box(const double x, const double /*support*/){
    if(x < -0.5)
        return(0.0);
    if(x < 0.5)
        return(1.0);
    return(0.0);
}

inline double Catrom(const double x, const double /*support*/){
    if(x < -2.0)
        return(0.0);
    if(x < -1.0)
        return(0.5*(4.0+x*(8.0+x*(5.0+x))));
    if(x < 0.0)
        return(0.5*(2.0+x*x*(-5.0-3.0*x)));
    if(x < 1.0)
        return(0.5*(2.0+x*x*(-5.0+3.0*x)));
    if(x < 2.0)
        return(0.5*(4.0+x*(-8.0+x*(5.0-x))));
    return(0.0);
}

inline double Cubic(const double x, const double /*support*/){
    if(x < -2.0)
        return(0.0);
    if(x < -1.0)
        return((2.0+x)*(2.0+x)*(2.0+x)/6.0);
    if(x < 0.0)
        return((4.0+x*x*(-6.0-3.0*x))/6.0);
    if(x < 1.0)
        return((4.0+x*x*(-6.0+3.0*x))/6.0);
    if(x < 2.0)
        return((2.0-x)*(2.0-x)*(2.0-x)/6.0);
    return(0.0);
}

inline double Gaussian(const double x, const double /*support*/){
    return(std::exp((-2.0*x*x))*std::sqrt(2.0/MagickPI));
}

inline double Hanning(const double x, const double /*support*/){
    return(0.5+0.5*std::cos(MagickPI*x));
}

inline double Hamming(const double x, const double /*support*/){
    return(0.54+0.46*std::cos(MagickPI*x));
}

inline double Hermite(const double x, const double /*support*/){
    if(x < -1.0)
        return(0.0);
    if(x < 0.0)
        return((2.0*(-x)-3.0)*(-x)*(-x)+1.0);
    if(x < 1.0)
        return((2.0*x-3.0)*x*x+1.0);
    return(0.0);
}

inline double Lanczos(const double x, const double support){
    if(x < -3.0)
        return(0.0);
    if(x < 0.0)
        return(Sinc(-x,support)*Sinc(-x/3.0,support));
    if(x < 3.0)
        return(Sinc(x,support)*Sinc(x/3.0,support));
    return(0.0);
}

inline double Mitchell(const double x, const double /*support*/){
    static constexpr double B = (1.0/3.0);
    static constexpr double C = (1.0/3.0);
    static constexpr double P0 = (( 6.0- 2.0*B )/6.0);
    static constexpr double P2 = ((-18.0+12.0*B+ 6.0*C)/6.0);
    static constexpr double P3 = (( 12.0- 9.0*B- 6.0*C)/6.0);
    static constexpr double Q0 = (( 8.0*B+24.0*C)/6.0);
    static constexpr double Q1 = (( -12.0*B-48.0*C)/6.0);
    static constexpr double Q2 = (( 6.0*B+30.0*C)/6.0);
    static constexpr double Q3 = (( - 1.0*B- 6.0*C)/6.0);

    if(x < -2.0)
        return(0.0);
    if(x < -1.0)
        return(Q0-x*(Q1-x*(Q2-x*Q3)));
    if(x < 0.0)
        return(P0+x*x*(P2-x*P3));
    if(x < 1.0)
        return(P0+x*x*(P2+x*P3));
    if(x < 2.0)
        return(Q0+x*(Q1+x*(Q2+x*Q3)));
    return(0.0);
}

inline double Quadratic(const double x, const double /*support*/){
    if(x < -1.5)
        return(0.0);
    if(x < -0.5)
        return(0.5*(x+1.5)*(x+1.5));
    if(x < 0.5)
        return(0.75-x*x);
    if(x < 1.5)
        return(0.5*(x-1.5)*(x-1.5));
    return(0.0);
}

inline double Triangle(const double x, const double /*support*/){
    if(x < -1.0)
        return(0.0);
    if(x < 0.0)
        return(1.0+x);
    if(x < 1.0)
        return(1.0-x);
    return(0.0);
}
}

using namespace BlitzScaleFilter;


//
// Horizontal and vertical filters
//

bool BlitzScaleFilter::horizontalFilter(const QImage *srcImg,
                                        QImage *destImg,
                                        double x_factor, double blur,
                                        std::vector<ContributionInfo> contribution,
                                        Blitz::ScaleFilterType filter,
                                        int page, const int *currentPage)
{
    const double halfPixel = 0.5;
    double fSupport = filterSupport.at(filter);
    const QRgb *srcData = reinterpret_cast<const QRgb *>(srcImg->constBits());
    QRgb *destData = reinterpret_cast<QRgb *>(destImg->bits());
    int sw = srcImg->width();
    int dw = destImg->width();
    QRgb pixel;

    double scale = blur*qMax(1.0/x_factor, 1.0);
    double support = scale*fSupport;
    if(support <= halfPixel){
        support = double(halfPixel+MagickEpsilon);
        scale = 1.0;
    }
    scale = 1.0/scale;

    for(int x=0; x < destImg->width(); ++x){
        double center = (x+halfPixel)/x_factor;
        unsigned int start = static_cast<uint>(qRound(qMax(center-support+halfPixel, 0.0)));
        unsigned int stop = static_cast<uint>(qRound(qMin(center+support+halfPixel, static_cast<double>(srcImg->width()))));
        double density=0.0;
        unsigned int n;

        for(n=0; n < (stop-start); ++n){
            contribution[n].pixel = start+n;
            switch(filter){
                case Blitz::TriangleFilter:
                    contribution[n].weight =
                            Triangle(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::HermiteFilter:
                    contribution[n].weight =
                            Hermite(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::HanningFilter:
                    contribution[n].weight =
                            Hanning(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::HammingFilter:
                    contribution[n].weight =
                            Hamming(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::BlackmanFilter:
                    contribution[n].weight =
                            Blackman(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::GaussianFilter:
                    contribution[n].weight =
                            Gaussian(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::QuadraticFilter:
                    contribution[n].weight =
                            Quadratic(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::CubicFilter:
                    contribution[n].weight =
                            Cubic(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::CatromFilter:
                    contribution[n].weight =
                            Catrom(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::MitchellFilter:
                    contribution[n].weight =
                            Mitchell(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::LanczosFilter:
                    contribution[n].weight =
                            Lanczos(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::BesselFilter:
                    contribution[n].weight =
                            BlackmanBessel(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::SincFilter:
                    contribution[n].weight =
                            BlackmanSinc(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::UndefinedFilter:
                case Blitz::PointFilter:
                case Blitz::BoxFilter:
                default:
                    contribution[n].weight =
                            Box(scale*(start+n-center+halfPixel),fSupport);
                    break;
            }
            density += contribution[n].weight;
        }

        if((density != 0.0) && (density != 1.0)){
            // Normalize
            density = 1.0/density;
            for(unsigned int i=0; i < n; ++i)
                contribution[i].weight *= density;
        }

        for(int y=0; y < destImg->height(); ++y){
            double r = 0.0;
            double g = 0.0;
            double b = 0.0;
            double a = 0.0;
            for(unsigned int i=0; i < n; ++i){
                pixel = *(srcData+(y*sw)+contribution[i].pixel);
                r += qRed(pixel)*contribution[i].weight;
                g += qGreen(pixel)*contribution[i].weight;
                b += qBlue(pixel)*contribution[i].weight;
                a += qAlpha(pixel)*contribution[i].weight;
            }
            r = r < 0 ? 0 : r > 255 ? 255 : r + 0.5;
            g = g < 0 ? 0 : g > 255 ? 255 : g + 0.5;
            b = b < 0 ? 0 : b > 255 ? 255 : b + 0.5;
            a = a < 0 ? 0 : a > 255 ? 255 : a + 0.5;
            *(destData+(y*dw)+x) = qRgba(static_cast<int>(r),
                                         static_cast<int>(g),
                                         static_cast<int>(b),
                                         static_cast<int>(a));
        }
        if (x%checkEventsFreq == 0) {
            QApplication::processEvents();
            if (currentPage!=nullptr && *currentPage!=page)
                return(false);
        }

    }
    return(true);
}

bool BlitzScaleFilter::verticalFilter(const QImage *srcImg,
                                      QImage *destImg,
                                      double y_factor, double blur,
                                      std::vector<ContributionInfo> contribution,
                                      Blitz::ScaleFilterType filter, int page,
                                      const int *currentPage)
{
    const double halfPixel = 0.5;
    double fSupport = filterSupport.at(filter);
    const QRgb *srcData = reinterpret_cast<const QRgb *>(srcImg->constBits());
    QRgb *destData = reinterpret_cast<QRgb *>(destImg->bits());
    int sw = srcImg->width();
    int dw = destImg->width();
    QRgb pixel;

    double scale = blur*qMax(1.0/y_factor, 1.0);
    double support = scale*fSupport;
    if(support <= halfPixel){
        support = double(halfPixel+MagickEpsilon);
        scale = 1.0;
    }
    scale = 1.0/scale;

    for(int y=0; y < destImg->height(); ++y){
        double center = (y+halfPixel)/y_factor;
        unsigned int start = static_cast<uint>(qRound(qMax(center-support+halfPixel, 0.0)));
        unsigned int stop = static_cast<uint>(qRound(qMin(center+support+halfPixel, static_cast<double>(srcImg->height()))));
        double density=0.0;
        unsigned int n;

        for(n=0; n < (stop-start); ++n){
            contribution[n].pixel = start+n;
            switch(filter){
                case Blitz::TriangleFilter:
                    contribution[n].weight =
                            Triangle(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::HermiteFilter:
                    contribution[n].weight =
                            Hermite(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::HanningFilter:
                    contribution[n].weight =
                            Hanning(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::HammingFilter:
                    contribution[n].weight =
                            Hamming(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::BlackmanFilter:
                    contribution[n].weight =
                            Blackman(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::GaussianFilter:
                    contribution[n].weight =
                            Gaussian(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::QuadraticFilter:
                    contribution[n].weight =
                            Quadratic(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::CubicFilter:
                    contribution[n].weight =
                            Cubic(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::CatromFilter:
                    contribution[n].weight =
                            Catrom(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::MitchellFilter:
                    contribution[n].weight =
                            Mitchell(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::LanczosFilter:
                    contribution[n].weight =
                            Lanczos(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::BesselFilter:
                    contribution[n].weight =
                            BlackmanBessel(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::SincFilter:
                    contribution[n].weight =
                            BlackmanSinc(scale*(start+n-center+halfPixel),fSupport);
                    break;
                case Blitz::UndefinedFilter:
                case Blitz::PointFilter:
                case Blitz::BoxFilter:
                default:
                    contribution[n].weight =
                            Box(scale*(start+n-center+halfPixel),fSupport);
                    break;
            }
            density += contribution[n].weight;
        }

        if((density != 0.0) && (density != 1.0)){
            // Normalize
            density = 1.0/density;
            for(unsigned int i=0; i < n; ++i)
                contribution[i].weight *= density;
        }

        for(int x=0; x < destImg->width(); ++x){
            double r = 0.0;
            double g = 0.0;
            double b = 0.0;
            double a = 0.0;
            for(unsigned int i=0; i < n; ++i){
                pixel = *(srcData+(contribution[i].pixel*static_cast<uint>(sw))+x);
                r += qRed(pixel)*contribution[i].weight;
                g += qGreen(pixel)*contribution[i].weight;
                b += qBlue(pixel)*contribution[i].weight;
                a += qAlpha(pixel)*contribution[i].weight;
            }
            r = r < 0 ? 0 : r > 255 ? 255 : r + 0.5;
            g = g < 0 ? 0 : g > 255 ? 255 : g + 0.5;
            b = b < 0 ? 0 : b > 255 ? 255 : b + 0.5;
            a = a < 0 ? 0 : a > 255 ? 255 : a + 0.5;
            *(destData+(y*dw)+x) = qRgba(static_cast<int>(r),
                                         static_cast<int>(g),
                                         static_cast<int>(b),
                                         static_cast<int>(a));
        }
        if (y%checkEventsFreq == 0) {
            QApplication::processEvents();
            if (currentPage!=nullptr && *currentPage!=page)
                return(false);
        }
    }
    return(true);
}

QImage Blitz::smoothScaleFilter(const QImage &img, int w, int h,
                                double blur, ScaleFilterType filter,
                                Qt::AspectRatioMode aspectRatio,
                                int page, const int *currentPage)
{
    return(smoothScaleFilter(img, QSize(w, h), blur, filter, aspectRatio, page, currentPage));
}

QImage Blitz::smoothScaleFilter(const QImage &img, const QSize &sz,
                                double blur, ScaleFilterType filter,
                                Qt::AspectRatioMode aspectRatio,
                                int page, const int *currentPage)
{
    QSize destSize(img.size());
    destSize.scale(sz, aspectRatio);
    if(img.isNull() || !destSize.isValid())
        return(img);
    int dw = destSize.width();
    int dh = destSize.height();

    QImage imgc = img;
    if(imgc.depth() != 32){
        imgc = imgc.convertToFormat(imgc.hasAlphaChannel() ?
                                      QImage::Format_ARGB32 :
                                      QImage::Format_RGB32);
    }
    else if(imgc.format() == QImage::Format_ARGB32_Premultiplied)
        imgc = imgc.convertToFormat(QImage::Format_ARGB32);

    QImage buffer(destSize, imgc.hasAlphaChannel() ?
                      QImage::Format_ARGB32 : QImage::Format_RGB32);

    //
    // Allocate filter contribution info.
    //
    double x_factor= static_cast<double>(buffer.width()) /
              static_cast<double>(imgc.width());
    double y_factor= static_cast<double>(buffer.height()) /
              static_cast<double>(imgc.height());
    ScaleFilterType m_filter = LanczosFilter;
    if(filter != UndefinedFilter) {
        m_filter = filter;
    } else {
        if((x_factor == 1.0) && (y_factor == 1.0)) {
            m_filter = PointFilter;
        } else {
            m_filter = MitchellFilter;
        }
    }
    double x_support = blur*qMax(1.0/x_factor, 1.0)*filterSupport.at(m_filter);
    double y_support = blur*qMax(1.0/y_factor, 1.0)*filterSupport.at(m_filter);
    double support = qMax(x_support, y_support);
    if(support < filterSupport.at(m_filter))
        support = filterSupport.at(m_filter);

    std::vector<ContributionInfo> contribution;
    contribution.resize(static_cast<uint>(qRound(2.0*qMax(support, 0.5)+3)));

    //
    // Scale
    //
    if((dw*(imgc.height()+dh)) > (dh*(imgc.width()+dw))){
        QImage tmp(dw, imgc.height(), buffer.format());
        bool res = horizontalFilter(&imgc, &tmp, x_factor, blur, contribution, filter, page, currentPage);
        if (res) {
            res = verticalFilter(&tmp, &buffer, y_factor, blur, contribution, filter, page, currentPage);
            if (!res)
                buffer = QImage();
        } else {
            buffer = QImage();
        }
    } else {
        QImage tmp(imgc.width(), dh, buffer.format());
        bool res = verticalFilter(&imgc, &tmp, y_factor, blur, contribution, filter, page, currentPage);
        if (res) {
            res = horizontalFilter(&tmp, &buffer, x_factor, blur, contribution, filter, page, currentPage);
            if (!res)
                buffer = QImage();
        } else {
            buffer = QImage();
        }
    }

    return(buffer);
}
