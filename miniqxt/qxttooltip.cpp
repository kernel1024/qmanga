#include "qxttooltip.h"
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

#include "qxttooltip_p.h"
#include <QStyleOptionFrame>
#include <QWindow>
#include <QScreen>
#include <QStylePainter>
#include <QApplication>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QToolTip>
#include <QPalette>
#include <QTimer>
#include <QFrame>
#include <QStyle>
#include <QPointer>
#include <QDebug>

static const Qt::WindowFlags FLAGS = Qt::ToolTip;

QxtToolTipPrivate* QxtToolTipPrivate::instance()
{
    static QPointer<QxtToolTipPrivate> self;

    if (self.isNull()) {
        self = new QxtToolTipPrivate();

        connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,self,[](){
            self->deleteLater();
        });

    }
    return self.data();
}

QxtToolTipPrivate::QxtToolTipPrivate(QWidget *parent) : QWidget(parent, FLAGS)
{
    const qreal maxOpacity = 255.0;

    setWindowFlags(FLAGS);
    vbox = new QVBoxLayout(this);
    setPalette(QToolTip::palette());
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, nullptr, this) / maxOpacity);
    int margin = style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this);
    layout()->setContentsMargins(margin, margin, margin, margin);
    QApplication::instance()->installEventFilter(this);
}

QxtToolTipPrivate::~QxtToolTipPrivate()
{
    removeAllWidgets();
    QApplication::instance()->removeEventFilter(this); // not really necessary but rather for completeness :)
}

void QxtToolTipPrivate::show(QPoint pos, QWidget* tooltip, QWidget* parent, QRect rect, bool allowMouseEnter, bool forceReplace)
{
    if (!isVisible() || forceReplace)
    {
        QScreen *screen = nullptr;

        if (!QApplication::screens().isEmpty()) {
            screen = QApplication::screenAt(pos);
        } else {
            if (parent && parent->window() && parent->window()->windowHandle()) {
                screen = parent->window()->windowHandle()->screen();
            }
        }
        if (screen == nullptr)
            screen = QApplication::primaryScreen();

        setParent(nullptr);
        setWindowFlags(FLAGS);
        setToolTip(tooltip);
        currentRect = rect;
        ignoreEnterEvent = allowMouseEnter;
        allowCloseOnLeave = false;
        move(calculatePos(screen, pos));
        QWidget::show();
    } else {
        tooltip->setParent(nullptr);
        tooltip->deleteLater();
    }
}

void QxtToolTipPrivate::setToolTip(QWidget* tooltip)
{
    removeAllWidgets();

    if (tooltip!=nullptr) {
        vbox->addWidget(tooltip);
        tooltip->show();
    }
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void QxtToolTipPrivate::enterEvent(QEnterEvent* event)
#else
void QxtToolTipPrivate::enterEvent(QEvent* event)
#endif
{
    Q_UNUSED(event)

    if (ignoreEnterEvent) {
        allowCloseOnLeave = true;
    } else {
        hideLater();
    }
}

void QxtToolTipPrivate::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)

    if (!ignoreEnterEvent || allowCloseOnLeave)
        hideLater();
}

void QxtToolTipPrivate::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QStylePainter painter(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    painter.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
}

void QxtToolTipPrivate::removeAllWidgets()
{
    while (vbox->count()>0) {
        QLayoutItem* item = vbox->takeAt(0);
        QWidget* widget = item->widget();
        delete item;
        widget->deleteLater();
    }
}

bool QxtToolTipPrivate::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object)

    switch (event->type())
    {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        {
            // accept only modifiers
            auto * const keyEvent = dynamic_cast<QKeyEvent*>(event);
            const int key = keyEvent->key();
            const Qt::KeyboardModifiers mods = keyEvent->modifiers();
            if ((mods & Qt::KeyboardModifierMask) != 0U ||
                    (key == Qt::Key_Shift || key == Qt::Key_Control ||
                     key == Qt::Key_Alt || key == Qt::Key_Meta))
                break;
            if (!ignoreEnterEvent)
                hideLater();
            break;
        }
        case QEvent::Leave:
            if (!ignoreEnterEvent)
                hideLater();
            break;

        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Wheel:
            hideLater();
            break;

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        {
            const QPoint pos = dynamic_cast<QMouseEvent*>(event)->pos();
            if (!currentRect.isNull() && !currentRect.contains(pos))
            {
                hideLater();
            }
            break;
        }

        default:
            break;
    }
    return false;
}

void QxtToolTipPrivate::hideLater()
{
    currentRect = QRect();
    removeAllWidgets();
    if (isVisible())
        QMetaObject::invokeMethod(this, &QxtToolTipPrivate::hide, Qt::QueuedConnection);
}

QPoint QxtToolTipPrivate::calculatePos(QScreen *scr, QPoint eventPos) const
{
    QRect screen = scr->availableGeometry();
    const QPoint posMargin(2,16);
    const int horizontalOffset = 4;
    const int verticalOffset = 24;

    QPoint p = eventPos + posMargin;
    QSize s = sizeHint();
    if (p.x() + s.width() > screen.x() + screen.width())
        p.rx() -= horizontalOffset + s.width();
    if (p.y() + s.height() > screen.y() + screen.height())
        p.ry() -= verticalOffset + s.height();
    if (p.y() < screen.y())
        p.setY(screen.y());
    if (p.x() + s.width() > screen.x() + screen.width())
        p.setX(screen.x() + screen.width() - s.width());
    if (p.x() < screen.x())
        p.setX(screen.x());
    if (p.y() + s.height() > screen.y() + screen.height())
        p.setY(screen.y() + screen.height() - s.height());
    return p;
}

QxtToolTip::QxtToolTip()
= default;

void QxtToolTip::show(QPoint pos, QWidget* tooltip, QWidget* parent, QRect rect, bool allowMouseEnter, bool forceReplace)
{
    QxtToolTipPrivate::instance()->show(pos, tooltip, parent, rect, allowMouseEnter, forceReplace);
}

void QxtToolTip::hide()
{
    QxtToolTipPrivate::instance()->hide();
}

qreal QxtToolTip::opacity()
{
    return QxtToolTipPrivate::instance()->windowOpacity();
}

void QxtToolTip::setOpacity(qreal level)
{
    QxtToolTipPrivate::instance()->setWindowOpacity(level);
}
