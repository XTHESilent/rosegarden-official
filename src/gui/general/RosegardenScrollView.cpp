/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
  Rosegarden
  A MIDI and audio sequencer and musical notation editor.
  Copyright 2000-2015 the Rosegarden development team.
 
  Other copyrights also apply to some parts of this work.  Please
  see the AUTHORS file and individual file headers for details.
 
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License as
  published by the Free Software Foundation; either version 2 of the
  License, or (at your option) any later version.  See the file
  COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING "[RosegardenScrollView]"

#include "RosegardenScrollView.h"

#include "misc/Debug.h"

//#include <QApplication>
#include <QCursor>
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QRect>
#include <QScrollBar>
#include <QSizePolicy>
#include <QTimer>
#include <QWheelEvent>
#include <QWidget>

#include <algorithm>  // std::swap()


namespace Rosegarden
{

// Smooth scroll checks
//

const double RosegardenScrollView::DefaultMinDeltaScroll = 1.2;
const int RosegardenScrollView::DefaultSmoothScrollTimeInterval = 10;
//const int RosegardenScrollView::AutoscrollMargin = 16;
const int RosegardenScrollView::InitialScrollTime = 30;
const int RosegardenScrollView::InitialScrollShortcut = 5;
const int RosegardenScrollView::MaxScrollDelta = 100;      // max a.scroll speed
const double RosegardenScrollView::ScrollShortcutValue = 1.04;// shortcuteration rate

RosegardenScrollView::RosegardenScrollView(QWidget *parent)
    : QAbstractScrollArea(parent),

      m_bottomWidget(0),
      m_smoothScroll(true),
      m_smoothScrollTimeInterval(DefaultSmoothScrollTimeInterval),
      m_minDeltaScroll(DefaultMinDeltaScroll),
      m_autoScrollTime(InitialScrollTime),
      m_autoScrollShortcut(InitialScrollShortcut),
      m_autoScrollXMargin(0),
      m_autoScrollYMargin(0),
      m_currentScrollDirection(None),
      m_followMode(NoFollow),
      m_autoScrolling(false),
      m_contentsWidth(0),
      m_contentsHeight(0)
{
    // From Q3ScrollView, not in Qt4's QAbstractScrollArea.
    //setDragAutoScroll(true);  //&&& could not find replacement

    // Turn off the frame which causes positioning issues.
    // The rest of the code assumes there is no frame.
    setFrameStyle(QFrame::NoFrame);

    connect( &m_autoScrollTimer, SIGNAL( timeout() ),
             this, SLOT( doAutoScroll() ) );
}

int RosegardenScrollView::contentsX()
{
    return horizontalScrollBar()->value();
}

int RosegardenScrollView::contentsY()
{
    return verticalScrollBar()->value();
}

void RosegardenScrollView::setContentsPos(int posX, int posY)
{
    horizontalScrollBar()->setValue(posX);
    verticalScrollBar()->setValue(posY);
}

int RosegardenScrollView::visibleWidth()
{
    return viewport()->width();
}

int RosegardenScrollView::visibleHeight()
{
    return viewport()->height();
}

int RosegardenScrollView::contentsWidth()
{
    return m_contentsWidth;
}

int RosegardenScrollView::contentsHeight()
{
    return m_contentsHeight;
}

void RosegardenScrollView::resizeContents(int w, int h)  // Code lifted from Q3ScrollView
{
    // Hold on to the old values.
    int ow = m_contentsWidth;
    int oh = m_contentsHeight;

    // We need to set these before we do the swaps, otherwise we may be
    // storing the wrong (post-swap) values.
    m_contentsWidth = w;
    m_contentsHeight = h;

//    d->scrollbar_timer.start(0, true); // This was necessary until I fixed the resizeEvent connection

//### CJ - Don't think this is necessary - slightly confused as we're resizing the content, not the widget
//    if (d->children.isEmpty() && d->policy == Default)
//        setResizePolicy(Manual);

    // Make sure "w" is the larger one.
    // ??? This would be easier to read using w1 and w2 for the
    //     smaller and the larger.
    if (ow > w) {
        std::swap(w, ow);
    }

    // Refresh area ow..w
    if (ow < visibleWidth() && w >= 0) {
        if (ow < 0)
            ow = 0;
        if (w > visibleWidth())
            w = visibleWidth();
        viewport()->update(contentsX()+ow, 0, w-ow, visibleHeight());
    }

    // Make sure "h" is the larger one.
    if (oh > h) {
        std::swap(h, oh);
    }

    // Refresh area oh..h
    if (oh < visibleHeight() && h >= 0) {
        if (oh < 0)
            oh = 0;
        if (h > visibleHeight())
            h = visibleHeight();
        viewport()->update(0, contentsY()+oh, visibleWidth(), h-oh);
    }

    // Since the contents size has changed, make sure the
    // scrollbars are updated.
    updateScrollBars();
}

void RosegardenScrollView::updateContents(int x, int y, int w, int h)  // Code lifted from Q3ScrollView
{
    if (!isVisible() || !updatesEnabled())
        return;

    //RG_DEBUG << "updateContents()";

    // Translate contents coords to viewport coords.
    x -= contentsX();
    y -= contentsY();

    // Cut off any portion left of the left edge.
    if (x < 0) {
        w += x;
        x = 0;
    }
    // Cut off any portion above the top edge.
    if (y < 0) {
        h += y;
        y = 0;
    }

    if (w < 0 || h < 0)
        return;

    // If x or y are beyond the viewport, bail.
    if (x > visibleWidth() || y > visibleHeight())
        return;

    // No need to update more than can be seen.
    if (w > visibleWidth())
        w = visibleWidth();
    if (h > visibleHeight())
        h = visibleHeight();

    //### CJ - I don't think we used a clipped_viewport on Q3ScrollView
    //if (d->clipped_viewport) {
    //// Translate clipper() to viewport()
    //x -= d->clipped_viewport->x();
    //y -= d->clipped_viewport->y();
    //}

    viewport()->update(x, y, w, h);
}

void RosegardenScrollView::updateContents(const QRect &r)
{
    updateContents(r.x(), r.y(), r.width(), r.height());
}

void RosegardenScrollView::updateContents()
{
    updateContents(contentsX(), contentsY(), visibleWidth(), visibleHeight());
}

void RosegardenScrollView::updateScrollBars()
{
    horizontalScrollBar()->setMaximum(
        std::max(contentsWidth()-visibleWidth(), 0));
    horizontalScrollBar()->setPageStep(visibleWidth());
    //horizontalScrollBar()->setLineStep(visibleWidth() / 10);

    verticalScrollBar()->setMaximum(
        std::max(contentsHeight()-visibleHeight(), 0));
    verticalScrollBar()->setPageStep(visibleHeight());
    //verticalScrollBar()->setLineStep(visibleHeight() / 10);
}

void RosegardenScrollView::paintEvent(QPaintEvent *event)
{
    viewportPaintEvent(event);
}

QPoint RosegardenScrollView::viewportToContents(const QPoint &vp)
{
    return QPoint(vp.x() + contentsX(),
                  vp.y() + contentsY());
}

void RosegardenScrollView::viewportPaintEvent(QPaintEvent * /* event */)
{
}

void RosegardenScrollView::mousePressEvent(QMouseEvent *event)
{
    viewportMousePressEvent(event);
}

void RosegardenScrollView::viewportMousePressEvent(QMouseEvent *e)
{
    QMouseEvent ce(e->type(), viewportToContents(e->pos()),
                   e->globalPos(), e->button(), e->buttons(), e->modifiers());
    contentsMousePressEvent(&ce);
    if (!ce.isAccepted())
        e->ignore();
}

void RosegardenScrollView::contentsMousePressEvent(QMouseEvent * /* event */)
{
}

void RosegardenScrollView::mouseReleaseEvent(QMouseEvent *event)
{
    viewportMouseReleaseEvent(event);
}

void RosegardenScrollView::viewportMouseReleaseEvent(QMouseEvent *e)
{
    QMouseEvent ce(e->type(), viewportToContents(e->pos()),
                   e->globalPos(), e->button(), e->buttons(), e->modifiers());
    contentsMouseReleaseEvent(&ce);
    if (!ce.isAccepted())
        e->ignore();
}

void RosegardenScrollView::contentsMouseReleaseEvent(QMouseEvent * /* event */)
{
}

void RosegardenScrollView::mouseMoveEvent(QMouseEvent *event)
{
    viewportMouseMoveEvent(event);
}

void RosegardenScrollView::viewportMouseMoveEvent(QMouseEvent *e)
{
    QMouseEvent ce(e->type(), viewportToContents(e->pos()),
                   e->globalPos(), e->button(), e->buttons(), e->modifiers());
    contentsMouseMoveEvent(&ce);
    if (!ce.isAccepted())
        e->ignore();
}

void RosegardenScrollView::contentsMouseMoveEvent(QMouseEvent * /* event */)
{
}

void RosegardenScrollView::mouseDoubleClickEvent(QMouseEvent *event)
{
    viewportMouseDoubleClickEvent(event);
}

void RosegardenScrollView::viewportMouseDoubleClickEvent(QMouseEvent *e)
{
    QMouseEvent ce(e->type(), viewportToContents(e->pos()),
                   e->globalPos(), e->button(), e->buttons(), e->modifiers());
    contentsMouseDoubleClickEvent(&ce);
    if (!ce.isAccepted())
        e->ignore();
}

void RosegardenScrollView::contentsMouseDoubleClickEvent(QMouseEvent * /* event */)
{
}

#if 0
// Q3ScrollView
void RosegardenScrollView::setDragAutoScroll(bool)
{
}
#endif

void RosegardenScrollView::setBottomFixedWidget(QWidget *w)
{
    m_bottomWidget = w;
    if (m_bottomWidget) {
        m_bottomWidget->setParent(this);
        m_bottomWidget->setSizePolicy(QSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed));
        // ??? Why do we have to add 1 to get enough room?
        //     - Are the viewport's limits being ignored?  Is someone
        //       overdrawing the bottom by 1?
        //     - Is the bottom widget expanding by 1?  No.  The hint is 25,
        //       we size it to 25, and it stays at 25.
        //     - Inclusive vs. exclusive math?  Don't think so.
        setViewportMargins( 0, 0, 0, m_bottomWidget->sizeHint().height() + 1 );
    }
}

void RosegardenScrollView::startAutoScroll()
{
    if ( !m_autoScrollTimer.isActive() ) {
        m_autoScrollTime = InitialScrollTime;
        m_autoScrollShortcut = InitialScrollShortcut;
        m_autoScrollTimer.start( m_autoScrollTime );
    }

    QPoint autoScrollStartPoint = viewport()->mapFromGlobal( QCursor::pos() );
    m_autoScrollYMargin = autoScrollStartPoint.y() / 10;
    m_autoScrollXMargin = autoScrollStartPoint.x() / 10;

    m_autoScrolling = true;
}

void RosegardenScrollView::startAutoScroll(int followMode)
{
    setFollowMode(followMode);
    startAutoScroll();
}

void RosegardenScrollView::stopAutoScroll()
{
    m_autoScrollTimer.stop();
    m_minDeltaScroll = DefaultMinDeltaScroll;
    m_currentScrollDirection = None;

    m_autoScrolling = false;
}

void RosegardenScrollView::doAutoScroll()
{
    QPoint p = viewport()->mapFromGlobal( QCursor::pos() );
    QPoint dp = p - m_previousP;
    m_previousP = p;

    m_autoScrollTimer.start( m_autoScrollTime );
    ScrollDirection scrollDirection = None;

    int dx = 0, dy = 0;
    if (m_followMode & FollowVertical) {
        if ( p.y() < m_autoScrollYMargin ) {
            dy = -(int(m_minDeltaScroll));
            scrollDirection = Top;
        } else if ( p.y() > visibleHeight() - m_autoScrollYMargin ) {
            dy = + (int(m_minDeltaScroll));
            scrollDirection = Bottom;
        }
    }
    bool startDecelerating = false;
    if (m_followMode & FollowHorizontal) {

        //        RG_DEBUG << "p.x() : " << p.x() << " - visibleWidth : " << visibleWidth() << " - autoScrollXMargin : " << m_autoScrollXMargin << endl;

        if ( p.x() < m_autoScrollXMargin ) {
            if ( dp.x() > 0 ) {
                startDecelerating = true;
                m_minDeltaScroll /= ScrollShortcutValue;
            }
            dx = -(int(m_minDeltaScroll));
            scrollDirection = Left;
        } else if ( p.x() > visibleWidth() - m_autoScrollXMargin ) {
            if ( dp.x() < 0 ) {
                startDecelerating = true;
                m_minDeltaScroll /= ScrollShortcutValue;
            }
            dx = + (int(m_minDeltaScroll));
            scrollDirection = Right;
        }
    }

    //     RG_DEBUG << "dx: " << dx << ", dy: " << dy << endl;

    if ( (dx || dy) &&
         ((scrollDirection == m_currentScrollDirection) || (m_currentScrollDirection == None)) ) {

//        scroll(dx, dy);
        horizontalScrollBar()->setValue( horizontalScrollBar()->value() + dx );
        verticalScrollBar()->setValue( verticalScrollBar()->value() + dy );

        if ( startDecelerating )
            m_minDeltaScroll /= ScrollShortcutValue;
        else
            m_minDeltaScroll *= ScrollShortcutValue;
        if (m_minDeltaScroll > MaxScrollDelta )
            m_minDeltaScroll = MaxScrollDelta;
        m_currentScrollDirection = scrollDirection;

    } else {
        // Don't automatically stopAutoScroll() here, the mouse button
        // is presumably still pressed.
        m_minDeltaScroll = DefaultMinDeltaScroll;
        m_currentScrollDirection = None;
    }
}

bool RosegardenScrollView::isTimeForSmoothScroll()
{
    // static int desktopWidth = QApplication::desktop()->width(),
    //    desktopHeight = QApplication::desktop()->height();

    if (m_smoothScroll) {
        int ta = m_scrollShortcuterationTimer.elapsed();
        int t = m_scrollTimer.elapsed();

        //RG_DEBUG << "t = " << t << ", ta = " << ta << ", int " << m_smoothScrollTimeInterval << ", delta " << m_minDeltaScroll << endl;

        if (t < m_smoothScrollTimeInterval) {

            return false;

        } else {

            if (ta > 300) {
                // reset smoothScrollTimeInterval
                m_smoothScrollTimeInterval = DefaultSmoothScrollTimeInterval;
                m_minDeltaScroll = DefaultMinDeltaScroll;
                m_scrollShortcuterationTimer.restart();
            } else if (ta > 50) {
                //                 m_smoothScrollTimeInterval /= 2;
                m_minDeltaScroll *= 1.08;
                m_scrollShortcuterationTimer.restart();
            }

            m_scrollTimer.restart();
            return true;
        }
    }

    return true;
}

void RosegardenScrollView::scrollHoriz(int hpos)
{
    QScrollBar *hbar = getMainHorizontalScrollBar();
    int currentContentYPos = contentsY();

    /* Lots of performance hitting debug
       RG_DEBUG << "RosegardenScrollView::scrollHoriz: hpos is " << hpos
       << ", contentsX is " << contentsX() << ", visibleWidth is "
       << visibleWidth() << endl;
    */

    if (hpos == 0) {

        // returning to zero
        //         hbar->setValue(0);
        setContentsPos(0, currentContentYPos);  //@@@

        // possible replacement: ??
        //widget()->move( 0, currentContentYPos )


    } else if (hpos > (contentsX() +
                       visibleWidth() * 1.6) ||
               hpos < (contentsX() -
                       visibleWidth() * 0.7)) {

        // miles off one side or the other
        //   hbar->setValue(hpos - int(visibleWidth() * 0.4));
        setContentsPos(hpos - int(visibleWidth() * 0.4), currentContentYPos);

    } else if (hpos > (contentsX() +
                       visibleWidth() * 0.9)) {

        // moving off the right hand side of the view
        //   hbar->setValue(hbar->value() + int(visibleWidth() * 0.6));
        setContentsPos(hbar->value() + int(visibleWidth() * 0.6), currentContentYPos);

    } else if (hpos < (contentsX() +
                       visibleWidth() * 0.1)) {

        // moving off the left
        //   hbar->setValue(hbar->value() - int(visibleWidth() * 0.6));
        setContentsPos(hbar->value() - int(visibleWidth() * 0.6), currentContentYPos);
    }
}

void RosegardenScrollView::scrollHorizSmallSteps(int hpos)
{
    QScrollBar *hbar = getMainHorizontalScrollBar();
    int currentContentYPos = contentsY();

    int diff = 0;

    if (hpos == 0) {

        // returning to zero
        //         hbar->setValue(0);
        setContentsPos(0, currentContentYPos);

    } else if ((diff = int(hpos - (contentsX() +
                                   visibleWidth() * 0.90))) > 0) {

        // moving off the right hand side of the view

        int delta = diff / 6;
        int diff10 = std::min(diff, (int)m_minDeltaScroll);
        delta = std::max(delta, diff10);

        // hbar->setValue(hbar->value() + delta);
        setContentsPos(hbar->value() + delta, currentContentYPos);

    } else if ((diff = int(hpos - (contentsX() +
                                   visibleWidth() * 0.10))) < 0) {
        // moving off the left

        int delta = -diff / 6;
        int diff10 = std::min( -diff, (int)m_minDeltaScroll);
        delta = std::max(delta, diff10);

        // hbar->setValue(hbar->value() - delta);
        setContentsPos(hbar->value() - delta, currentContentYPos);

    }
}

void RosegardenScrollView::scrollVertSmallSteps(int vpos)
{
    QScrollBar *vbar = verticalScrollBar();

    //    RG_DEBUG << "RosegardenScrollView::scrollVertSmallSteps - Start: vpos is " << vpos << ", contentsY is " << contentsY() << ", visibleHeight is " << visibleHeight() << endl;

    // As a special case (or hack), ignore any request made before we've
    // actually been rendered and sized
    if (visibleHeight() <= 1)
        return ;

    int diff = 0;

    if (vpos == 0) {

        // returning to zero
        vbar->setValue(0);

    } else if ((diff = int(vpos - (contentsY() +
                                   visibleHeight() * 0.90))) > 0) {

        // moving off up

        int delta = diff / 6;
        int diff10 = std::min(diff, (int)m_minDeltaScroll);
        delta = std::max(delta, diff10);

        vbar->setValue(vbar->value() + diff);

    } else if ((diff = int(vpos - (contentsY() +
                                   visibleHeight() * 0.10))) < 0) {

        // moving off down

        int delta = -diff / 6;
        int diff10 = std::min( -diff, (int)m_minDeltaScroll);
        delta = std::max(delta, diff10);

        vbar->setValue(vbar->value() - delta);

    }
}

#if 0
void RosegardenScrollView::slotScrollVertToTop(int vpos)
{
    QScrollBar* vbar = verticalScrollBar();
    if (vpos < visibleHeight() / 3)
        vbar->setValue(0);
    else
        vbar->setValue(vpos - visibleHeight() / 5);
}

void RosegardenScrollView::slotSetScrollPos(const QPoint &pos)
{
    horizontalScrollBar()->setValue(pos.x());
    verticalScrollBar()->setValue(pos.y());
}
#endif

void RosegardenScrollView::resizeEvent(QResizeEvent *e)
{
    RG_DEBUG << "resizeEvent()";

    QAbstractScrollArea::resizeEvent(e);

    // Since the viewport size has changed, we need to update
    // the scrollbars.
    updateScrollBars();

    // Make sure the bottom ruler is where it needs to be.
    updateBottomWidgetGeometry();
}

#if 0
void RosegardenScrollView::setHBarGeometry(QScrollBar &/* hbar */, int /* x */, int /* y */, int /* w */, int /* h */)
{
    RG_DEBUG << "setHBarGeometry()";
    // Not available in QAbstractScrollArea - Q3ScrollView::setHBarGeometry(hbar, x, y, w, h);
//    hbar.setGeometry( x,y, w,h );
    updateBottomWidgetGeometry();
}
#endif

void RosegardenScrollView::updateBottomWidgetGeometry()
{
    RG_DEBUG << "updateBottomWidgetGeometry()";

    if (!m_bottomWidget)
        return;

    int bottomWidgetHeight = m_bottomWidget->sizeHint().height();
    // Since there's no margin (see the call to setFrameStyle() in
    // the ctor), we can assume the viewport coords match up with
    // the parent coords.  No need to transform.
    QRect viewportRect = viewport()->rect();

    // Move the bottom widget to below the viewport.
    m_bottomWidget->setGeometry(
            viewportRect.left(),
            viewportRect.bottom() + 1,  // +1 to be just under
            viewportRect.width(),
            bottomWidgetHeight);  // See the call to setViewportMargins().
}

void RosegardenScrollView::wheelEvent(QWheelEvent *e)
{
    if (e->modifiers() & Qt::CTRL) {
        if (e->delta() > 0)
            emit zoomIn();
        else if (e->delta() < 0)
            emit zoomOut();

        return;
    }
    
    // For some reason that I don't understand, vertical scrolling
    // with the wheel in the main window is extremely slow.  The
    // factor of 10 applied here is to compensate for that.  No doubt
    // on someone else's machine it will turn out to have been fine
    // before this factor was applied, and unusable now...

    QWheelEvent w(e->pos(),
                  e->globalPos(),
                  e->delta() * 10,
                  e->buttons(),
                  e->modifiers(),
                  e->orientation());
    QAbstractScrollArea::wheelEvent(&w);
}

}
#include "RosegardenScrollView.moc"
