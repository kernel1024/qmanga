#ifndef QXTTOOLTIP_P_H
/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/

#define QXTTOOLTIP_P_H

#include <QPointer>
#include <QWidget>
#include <QHash>
#include "qxttooltip.h"

QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

typedef QPointer<QWidget> WidgetPtr;
typedef QPair<WidgetPtr, QRect> WidgetArea;

class QxtToolTipPrivate : public QWidget
{
    Q_OBJECT

public:
    QxtToolTipPrivate();
    ~QxtToolTipPrivate();

    static QxtToolTipPrivate* instance();
    void show(const QPoint& pos, QWidget* tooltip, QWidget* parent = nullptr,
              const QRect& rect = QRect(), const bool allowMouseEnter = false);
    void setToolTip(QWidget* tooltip);
    bool eventFilter(QObject* parent, QEvent* event);
    void hideLater();
    QPoint calculatePos(int scr, const QPoint& eventPos) const;
    QHash<WidgetPtr, WidgetArea> tooltips;
    QVBoxLayout* vbox;

protected:
    void enterEvent(QEvent* event);
    void leaveEvent(QEvent* event);
    void paintEvent(QPaintEvent* event);

private:
    static QxtToolTipPrivate* self;
    QWidget* currentParent;
    QRect currentRect;
    bool ignoreEnterEvent;
    bool allowCloseOnLeave;
};

#endif // QXTTOOLTIP_P_H
