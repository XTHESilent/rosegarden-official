// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2002
        Guillaume Laurent   <glaurent@telegraph-road.org>,
        Chris Cannam        <cannam@all-day-breakfast.com>,
        Richard Bown        <bownie@bownie.com>

    The moral right of the authors to claim authorship of this work
    has been asserted.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef ROSEGARDENGUIVIEW_H
#define ROSEGARDENGUIVIEW_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

// include files for Qt
#include <qvbox.h>
#include <list>
#include "segmentcanvas.h" // needed for SegmentCanvas::ToolType

#include "rosedebug.h"

namespace Rosegarden { class Composition; class MappedEvent; class RulerScale; }

class QScrollView;
class RosegardenGUIDoc;
class TrackEditor;
class KPrinter;
class BarButtons;
class TrackButtons;

/**
 * The RosegardenGUIView class provides the view widget for the
 * RosegardenGUIApp instance.  The View instance inherits QWidget as a
 * base class and represents the view object of a KTMainWindow. As
 * RosegardenGUIView is part of the docuement-view model, it needs a
 * reference to the document object connected with it by the
 * RosegardenGUIApp class to manipulate and display the document
 * structure provided by the RosegardenGUIDoc class.
 * 	
 * @author Source Framework Automatically Generated by KDevelop, (c) The KDevelop Team.
 * @version KDevelop version 0.4 code generation
 */
class RosegardenGUIView : public QVBox
{
    Q_OBJECT
public:

    /**
     * Constructor for the main view
     */
    RosegardenGUIView(QWidget *parent = 0, const char *name=0);

    /**
     * Destructor for the main view
     */
    ~RosegardenGUIView();

    /**
     * returns a pointer to the document connected to the view
     * instance. Mind that this method requires a RosegardenGUIApp
     * instance as a parent widget to get to the window document
     * pointer by calling the RosegardenGUIApp::getDocument() method.
     *
     * @see RosegardenGUIApp#getDocument
     */
    RosegardenGUIDoc* getDocument() const;

    /**
     * contains the implementation for printing functionality
     */
    void print(KPrinter *pPrinter, Rosegarden::Composition*);

    // the following aren't slots because they're called from
    // RosegardenGUIApp

    /**
     * segment selector mode
     */
    void pointerSelected();
    
    /**
     * segment eraser tool is selected
     */
    void eraseSelected();
    
    /**
     * segment draw tool is selected
     */
    void drawSelected();
    
    /**
     * segment move tool is selected
     */
    void moveSelected();

    /**
     * segment resize tool is selected
     */
    void resizeSelected();

    /**
     * Edit all the segment at once
     *
     * Show all the tracks in a single Notation window
     */
    void editAllTracks(Rosegarden::Composition*);

    /**
     * Set the time pointer position during playback
     */
    void setPointerPosition(const Rosegarden::timeT &position);

    /**
     * These two are just-passing-through methods called from
     * the GUI when it has key presses that the SegmentCanvas
     * or anything else downstairsis interested in.
     *
     */
    void setShift(const bool &value);
    void setControl(const bool &value);

    /**
     * A newly recorded Segment has to be added to the view
     *
     */
    void createSegmentItem(Rosegarden::Segment* segment);

    /*
     * Destroy a SegmentItem
     */
    void destroySegmentItem(Rosegarden::Segment* segment);

    /**
     * Show a Segment as it records - remove the SegmentItem
     * when no longer needed
     */
    void showRecordingSegmentItem(Rosegarden::Segment* segment);
    void destroyRecordingSegmentItem();

    /**
     * set the viewable loop on the clients and SegmentCanvas
     */      
    void setLoopMarker(Rosegarden::timeT startLoop, Rosegarden::timeT endLoop);

    /**
     * Show output levels
     */
    void showVisuals(const Rosegarden::MappedEvent *mE);
    
public slots:
    void editSegmentNotation(Rosegarden::Segment*);
    void editSegmentMatrix(Rosegarden::Segment*);
    void editSegmentAudio(Rosegarden::Segment*);

    /**
     * Scroll the TrackEditor horizontally to the specified
     * position
     *
     * @see TrackEditor#scrollHorizTo(int)
     */
    void scrollTrackEditorHoriz(int hpos);

    // Called by signal from Track selection routine to highlight
    // all available Segments on a Track
    //
    void selectTrackSegments(int);

signals:
    // This signals is emitted upwards towards the top level
    // objects and sets the position pointer (this emit
    // sets off a chain of events that comes back down
    // through setCanvasPositionPointer() to actually
    // move the pointer line
    //
    void setGUIPositionPointer(Rosegarden::timeT);
    void setGUIPlayPosition(Rosegarden::timeT);
    void setGUILoop(Rosegarden::timeT, Rosegarden::timeT);

    void activateTool(SegmentCanvas::ToolType);

protected:
    //--------------- Data members ---------------------------------

    Rosegarden::RulerScale   *m_rulerScale;
    QScrollView              *m_trackEditorScrollView;
    TrackEditor              *m_trackEditor;
    BarButtons               *m_barButtons;
    TrackButtons             *m_trackButtons;

};

#endif // ROSEGARDENGUIVIEW_H
