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

#ifndef RG_ROSEGARDENSCROLLVIEW_H
#define RG_ROSEGARDENSCROLLVIEW_H

#include <QPoint>
#include <QAbstractScrollArea>
#include <QDateTime>
#include <QTimer>


class QWidget;
class QWheelEvent;
class QScrollBar;
class QResizeEvent;
class QPaintEvent;
class QMouseEvent;


namespace Rosegarden
{


/// QAbstractScrollArea with auto-scroll and bottom ruler.
/**
 * A QAbstractScrollArea with more elaborate auto-scrolling capabilities
 * and the ability to have a vertically "fixed" widget (ruler) at its
 * bottom, just above the horizontal scrollbar.
 *
 * Some Q3ScrollView compatibility is provided to ease the transition
 * from Q3ScrollView to QAbstractScrollArea.
 *
 * CompositionView derives from this class.
 */
class RosegardenScrollView : public QAbstractScrollArea
{
    Q_OBJECT

public:

    RosegardenScrollView(QWidget *parent);

    /// Connect the bottom ruler.
    /**
     * Sets the ruler widget which will be between the scrollable part of
     * the view and the horizontal scrollbar.
     *
     * This is called by TrackEditor::init() to connect a StandardRuler
     * instance.
     */
    void setBottomFixedWidget(QWidget *);

    // Q3ScrollView functions that are missing from QAbstractScrollArea
    // (Most of these can be moved to protected as only CompositionView
    // uses them.)

    // Q3ScrollView viewport position and size functions.

    /// X coordinate of the contents that are at the left edge of the viewport.
    int contentsX();
    /// Y coordinate of the contents that are at the top edge of the viewport.
    int contentsY();
    /// Scrolls the content so that the point (x, y) is in the top-left corner.
    void setContentsPos(int x, int y);
    /// Visible portion of the contents.  The viewport width.
    int visibleWidth();
    /// Visible portion of the contents.  The viewport height.
    int visibleHeight();

    // Q3ScrollView contents size and update functions.

    /// Width of the contents area.
    int contentsWidth();
    /// Height of the contents area.
    int contentsHeight();

    /// Sets the size of the contents area and updates the viewport accordingly.
    void resizeContents(int width, int height);
    /// Calls update() on a rectangle defined by x, y, w, h, translated appropriately.
    void updateContents(int x, int y, int width, int height);
    void updateContents(const QRect &);
    void updateContents();


    //QPoint viewportToContents(QPoint &);

    // From Q3ScrollView
    //void setDragAutoScroll(bool);

    /// Map a point with the inverse world matrix
    //QPoint inverseMapPoint(const QPoint& p) { return inverseWorldMatrix().map(p); }

    //void setSmoothScroll(bool s)  { m_smoothScroll = s; }
    bool isTimeForSmoothScroll();

    /// Follow Mode
    /**
     * Derivers from SegmentTool override SegmentTool::handleMouseMove() and
     * return an OR-ed combination of these to indicate the auto-scroll
     * direction.
     *
     * See MatrixTool::FollowMode, NotationTool::FollowMode, and
     * ControlTool::FollowMode.
     *
     * Would this make more sense in SegmentTool?
     */
    enum FollowMode {
        NoFollow = 0x0,
        FollowHorizontal = 0x1,
        FollowVertical = 0x2
    };

    /// See enum FollowMode.
    void setFollowMode(int followMode)  { m_followMode = followMode; }

    //int getDeltaScroll() const  { return m_minDeltaScroll; }

    bool isAutoScrolling() const  { return m_autoScrolling; }

    /**
     * Scroll horizontally to make the given position visible,
     * paging so as to get some visibility of the next screenful
     * (for playback etc)
     */
    void scrollHoriz(int hpos);

    /**
     * Scroll horizontally to make the given position somewhat
     * nearer to visible, scrolling by only "a small distance"
     * at a time
     */
    void scrollHorizSmallSteps(int hpos);

    /**
     * Scroll vertically to make the given position somewhat
     * nearer to visible, scrolling by only "a small distance"
     * at a time
     */
    void scrollVertSmallSteps(int vpos);

public slots:
    /**
     * Scroll vertically so as to place the given position
     * somewhere near the top of the viewport.
     */
    //void slotScrollVertToTop(int vpos);

    /**
     * Set the x and y scrollbars to a particular position
     */
    //void slotSetScrollPos(const QPoint &);

    /// See enum FollowMode for valid mask values.
    void startAutoScroll(int followMode);
    void stopAutoScroll();
    /// Handler for m_autoScrollTimer.
    /**
     * Also called by TrackEditor::handleAutoScroll().
     */
    void doAutoScroll();

signals:
    //void bottomWidgetHeightChanged(int newHeight);

    void zoomIn();
    void zoomOut();

protected:

    void startAutoScroll();

    /// Viewport resize.
    virtual void resizeEvent(QResizeEvent *);

    virtual void paintEvent(QPaintEvent *);

    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);

    virtual void wheelEvent(QWheelEvent *);

    // Q3ScrollView compatibility functions.

    virtual void viewportPaintEvent(QPaintEvent *);

    virtual void contentsMousePressEvent(QMouseEvent *);
    virtual void contentsMouseReleaseEvent(QMouseEvent *);
    virtual void contentsMouseMoveEvent(QMouseEvent *);
    virtual void contentsMouseDoubleClickEvent(QMouseEvent *);

private:

    // ??? These are called exactly once.  They can be inlined into
    //     their callers.
    void viewportMousePressEvent(QMouseEvent *);
    void viewportMouseReleaseEvent(QMouseEvent *);
    void viewportMouseMoveEvent(QMouseEvent *);
    void viewportMouseDoubleClickEvent(QMouseEvent *);

    QPoint viewportToContents(const QPoint &);

    //void setHBarGeometry(QScrollBar &hbar, int x, int y, int w, int h);
    QScrollBar *getMainHorizontalScrollBar()  { return horizontalScrollBar(); }

    QWidget *m_bottomWidget;
    /// Make sure the bottom ruler stays in the proper place.
    void updateBottomWidgetGeometry();

    bool m_smoothScroll;  // always true
    int m_smoothScrollTimeInterval;
    float m_minDeltaScroll;
    QTime m_scrollTimer;
    QTime m_scrollShortcuterationTimer;

    /// Calls doAutoScroll().
    QTimer m_autoScrollTimer;
    int m_autoScrollTime;
    int m_autoScrollShortcut;
    QPoint m_previousP;
    int m_autoScrollXMargin;
    int m_autoScrollYMargin;
    enum ScrollDirection { None, Top, Bottom, Left, Right };
    ScrollDirection m_currentScrollDirection;
    /// See enum FollowMode for valid mask values.
    int m_followMode;
    bool m_autoScrolling;

    static const int DefaultSmoothScrollTimeInterval;
    static const double DefaultMinDeltaScroll;

    //static const int AutoscrollMargin;
    static const int InitialScrollTime;
    static const int InitialScrollShortcut;
    static const int MaxScrollDelta;
    static const double ScrollShortcutValue;

    int m_contentsWidth;
    int m_contentsHeight;

    /// Adjust the scrollbars' max and page step.
    void updateScrollBars();
};


}

#endif
