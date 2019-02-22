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
#include <QHash>
#include <QDebug>

static const Qt::WindowFlags FLAGS = Qt::ToolTip;

QxtToolTipPrivate* QxtToolTipPrivate::self = nullptr;

QxtToolTipPrivate* QxtToolTipPrivate::instance()
{
    if (!self)
        self = new QxtToolTipPrivate();
    return self;
}

QxtToolTipPrivate::QxtToolTipPrivate() : QWidget(nullptr, FLAGS)
{
    currentParent = nullptr;
    ignoreEnterEvent = false;
    allowCloseOnLeave = false;
    setWindowFlags(FLAGS);
    vbox = new QVBoxLayout(this);
    setPalette(QToolTip::palette());
    setWindowOpacity(style()->styleHint(QStyle::SH_ToolTipLabel_Opacity, nullptr, this) / 255.0);
    layout()->setMargin(style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth, nullptr, this));
    QApplication::instance()->installEventFilter(this);
}

QxtToolTipPrivate::~QxtToolTipPrivate()
{
    QApplication::instance()->removeEventFilter(this); // not really necessary but rather for completeness :)
    self = nullptr;
}

void QxtToolTipPrivate::show(const QPoint& pos, QWidget* tooltip, QWidget* parent, const QRect& rect, const bool allowMouseEnter)
{
    //    Q_ASSERT(tooltip && parent);
    if (!isVisible())
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
        currentParent = parent;
        currentRect = rect;
        ignoreEnterEvent = allowMouseEnter;
        allowCloseOnLeave = false;
        move(calculatePos(screen, pos));
        QWidget::show();
    }
}

void QxtToolTipPrivate::setToolTip(QWidget* tooltip)
{
    for (int i = 0; i < vbox->count(); ++i)
    {
        QLayoutItem* item = layout()->takeAt(i);
        if (item->widget())
            item->widget()->hide();
    }
    vbox->addWidget(tooltip);
    tooltip->show();
}

void QxtToolTipPrivate::enterEvent(QEvent* event)
{
    Q_UNUSED(event);
    if (ignoreEnterEvent)
        allowCloseOnLeave = true;
    else
        hideLater();
}

void QxtToolTipPrivate::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (!ignoreEnterEvent || allowCloseOnLeave)
        hideLater();
}

void QxtToolTipPrivate::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QStylePainter painter(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    painter.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
}

bool QxtToolTipPrivate::eventFilter(QObject* object, QEvent* event)
{
    switch (event->type())
    {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        {
            // accept only modifiers
            const auto keyEvent = dynamic_cast<QKeyEvent*>(event);
            const int key = keyEvent->key();
            const Qt::KeyboardModifiers mods = keyEvent->modifiers();
            if ((mods & Qt::KeyboardModifierMask) ||
                    (key == Qt::Key_Shift || key == Qt::Key_Control ||
                     key == Qt::Key_Alt || key == Qt::Key_Meta))
                break;
            if (!ignoreEnterEvent)
                hideLater();
            break;
        }
        case QEvent::Leave:
        {
            if (!ignoreEnterEvent)
                hideLater();
            break;
        }
        case QEvent::WindowActivate:
        case QEvent::WindowDeactivate:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Wheel:
            hideLater();
            break;

        case QEvent::MouseMove:
        {
            const QPoint pos = dynamic_cast<QMouseEvent*>(event)->pos();
            if (!currentRect.isNull() && !currentRect.contains(pos))
            {
                hideLater();
            }
            break;
        }

        case QEvent::ToolTip:
        {
            // eat appropriate tooltip events
            auto widget = qobject_cast<QWidget*>(object);
            if (widget!=nullptr && tooltips.contains(widget))
            {
                auto helpEvent = dynamic_cast<QHelpEvent*>(event);
                const QRect area = tooltips.value(widget).second;
                if (helpEvent!=nullptr && (area.isNull() || area.contains(helpEvent->pos())))
                {
                    show(helpEvent->globalPos(), tooltips.value(widget).first, widget, area);
                    return true;
                }
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
    if (isVisible())
        QTimer::singleShot(0, this, &QxtToolTipPrivate::hide);
}

QPoint QxtToolTipPrivate::calculatePos(QScreen *scr, const QPoint& eventPos) const
{
    QRect screen = scr->availableGeometry();

    QPoint p = eventPos;
    p += QPoint(2,
            #ifdef Q_OS_WIN
                24
            #else
                16
            #endif
                );
    QSize s = sizeHint();
    if (p.x() + s.width() > screen.x() + screen.width())
        p.rx() -= 4 + s.width();
    if (p.y() + s.height() > screen.y() + screen.height())
        p.ry() -= 24 + s.height();
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

/*!
    \class QxtToolTip
    \inmodule QxtWidgets
    \brief The QxtToolTip class provides means for showing any arbitrary widget as a tooltip.

    QxtToolTip provides means for showing any arbitrary widget as a tooltip.

    \bold {Note:} The rich text support of QToolTip already makes it possible to
    show heavily customized tooltips with lists, tables, embedded images
    and such. However, for example dynamically created images like
    thumbnails cause problems. Basically the only way is to dump the
    thumbnail to a temporary file to be able to embed it into HTML. This
    is where QxtToolTip steps in. A generated thumbnail may simply be set
    on a QLabel which is then shown as a tooltip. Yet another use case
    is a tooltip with dynamically changing content.

    \image qxttooltip.png "QxtToolTip in action."

    \warning Added tooltip widgets remain in the memory for the lifetime
    of the application or until they are removed/deleted. Do NOT flood your
    application up with lots of complex tooltip widgets or it will end up
    being a resource hog. QToolTip is sufficient for most of the cases!
 */

/*!
    \internal
 */
QxtToolTip::QxtToolTip()
= default;

/*!
    Shows the \a tooltip at \a pos for \a parent at \a rect.

    \sa hide()
*/
void QxtToolTip::show(const QPoint& pos, QWidget* tooltip, QWidget* parent, const QRect& rect, const bool allowMouseEnter)
{
    QxtToolTipPrivate::instance()->show(pos, tooltip, parent, rect, allowMouseEnter);
}

/*!
    Hides the tooltip.

    \sa show()
*/
void QxtToolTip::hide()
{
    QxtToolTipPrivate::instance()->hide();
}

/*!
    Returns the tooltip for \a parent.

    \sa setToolTip()
*/
QWidget* QxtToolTip::toolTip(QWidget* parent)
{
    Q_ASSERT(parent);
    QWidget* tooltip = nullptr;
    if (!QxtToolTipPrivate::instance()->tooltips.contains(parent))
        qWarning() << QString("QxtToolTip::toolTip: Unknown parent");
    else
        tooltip = QxtToolTipPrivate::instance()->tooltips.value(parent).first;
    return tooltip;
}

/*!
    Sets the \a tooltip to be shown for \a parent.
    An optional \a rect may also be passed.

    \sa toolTip()
*/
void QxtToolTip::setToolTip(QWidget* parent, QWidget* tooltip, const QRect& rect)
{
    Q_ASSERT(parent);
    if (tooltip)
    {
        // set
        tooltip->hide();
        QxtToolTipPrivate::instance()->tooltips[parent] = qMakePair(QPointer<QWidget>(tooltip), rect);
    }
    else
    {
        // remove
        if (!QxtToolTipPrivate::instance()->tooltips.contains(parent))
            qWarning() << QString("QxtToolTip::setToolTip: Unknown parent");
        else
            QxtToolTipPrivate::instance()->tooltips.remove(parent);
    }
}

/*!
    Returns the rect on which tooltip is shown for \a parent.

    \sa setToolTipRect()
*/
QRect QxtToolTip::toolTipRect(QWidget* parent)
{
    Q_ASSERT(parent);
    QRect rect;
    if (!QxtToolTipPrivate::instance()->tooltips.contains(parent))
        qWarning() << QString("QxtToolTip::toolTipRect: Unknown parent");
    else
        rect = QxtToolTipPrivate::instance()->tooltips.value(parent).second;
    return rect;
}

/*!
    Sets the \a rect on which tooltip is shown for \a parent.

    \sa toolTipRect()
*/
void QxtToolTip::setToolTipRect(QWidget* parent, const QRect& rect)
{
    Q_ASSERT(parent);
    if (!QxtToolTipPrivate::instance()->tooltips.contains(parent))
        qWarning() << QString("QxtToolTip::setToolTipRect: Unknown parent");
    else
        QxtToolTipPrivate::instance()->tooltips[parent].second = rect;
}

/*!
    Returns the margin of the tooltip.

    \sa setMargin()
*/
int QxtToolTip::margin()
{
    return QxtToolTipPrivate::instance()->layout()->margin();
}

/*!
    Sets the \a margin of the tooltip.

    The default value is QStyle::PM_ToolTipLabelFrameWidth.

    \sa margin()
*/
void QxtToolTip::setMargin(int margin)
{
    QxtToolTipPrivate::instance()->layout()->setMargin(margin);
}

/*!
    Returns the opacity level of the tooltip.

    \sa QWidget::windowOpacity()
*/
qreal QxtToolTip::opacity()
{
    return QxtToolTipPrivate::instance()->windowOpacity();
}

/*!
    Sets the opacity \a level of the tooltip.

    The default value is QStyle::SH_ToolTipLabel_Opacity.

    \sa QWidget::setWindowOpacity()
*/
void QxtToolTip::setOpacity(qreal level)
{
    QxtToolTipPrivate::instance()->setWindowOpacity(level);
}
