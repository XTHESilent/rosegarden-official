
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.

    This program is Copyright 2000-2006
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <richard.bown@ferventsoftware.com>

    The moral rights of Guillaume Laurent, Chris Cannam, and Richard
    Bown to claim authorship of this work have been asserted.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_RAWNOTERULER_H_
#define _RG_RAWNOTERULER_H_

#include "base/Segment.h"
#include <qsize.h>
#include <qwidget.h>
#include <utility>
#include <vector>
#include "base/Event.h"


class QPaintEvent;
class QPainter;


namespace Rosegarden
{

class RulerScale;
class DefaultVelocityColour;


/**
 * RawNoteRuler is a ruler that shows in a vaguely matrix-like fashion
 * when notes start and end, for use with a notation view that can't
 * otherwise show this relatively precise unquantized information.
 * It has no editing function (yet?)
 */

class RawNoteRuler : public QWidget
{
    Q_OBJECT

public:
    RawNoteRuler(RulerScale *rulerScale,
		 Segment *segment,
		 double xorigin = 0.0,
		 int height = 0,
		 QWidget* parent = 0,
		 const char *name = 0);

    ~RawNoteRuler();

    void setCurrentSegment(Segment *segment) {
	m_segment = segment;
    }

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    void setMinimumWidth(int width) { m_width = width; }

public slots:
    void slotScrollHoriz(int x);

protected:
    virtual void paintEvent(QPaintEvent*);

private:
    double m_xorigin;
    int  m_height;
    int  m_currentXOffset;
    int  m_width;

    Segment *m_segment;
    RulerScale *m_rulerScale;

    struct EventTreeNode
    {
	typedef std::vector<EventTreeNode *> NodeList;
	
	EventTreeNode(Segment::iterator n) : node(n) { }
	~EventTreeNode() {
	    for (NodeList::iterator i = children.begin();
		 i != children.end(); ++i) {
		delete *i;
	    }
	}

	int getDepth();
	int getChildrenAboveOrBelow(bool below = false, int pitch = -1);
	
	Segment::iterator node;
	NodeList children;
    };

    std::pair<timeT, timeT> getExtents(Segment::iterator);
    Segment::iterator addChildren(Segment *, Segment::iterator, timeT, EventTreeNode *);
    void dumpSubtree(EventTreeNode *, int);
    void dumpForest(std::vector<EventTreeNode *> *);
    void buildForest(Segment *, Segment::iterator, Segment::iterator);

    void drawNode(QPainter &, DefaultVelocityColour &, EventTreeNode *,
		  double height, double yorigin);

    // needs to be class with dtor &c and containing above methods
    EventTreeNode::NodeList m_forest;
};


}

#endif
