
/*
    Rosegarden-4 v0.1
    A sequencer and musical notation editor.

    This program is Copyright 2000-2001
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

#include "trackscanvas.h" // needed for TracksCanvas::ToolType

#include "rosedebug.h"


class RosegardenGUIDoc;
class NotationView;
class TrackPart;
class TracksEditor;

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
    /** Constructor for the main view */
    RosegardenGUIView(QWidget *parent = 0, const char *name=0);
    /** Destructor for the main view */
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

    /** contains the implementation for printing functionality */
    void print(QPrinter *pPrinter);

    // the following aren't slots because they're called from
    // RosegardenGUIApp

    /**
     * track eraser tool is selected
     */
    void eraseSelected();
    
    /**
     * track draw tool is selected
     */
    void drawSelected();
    
    /**
     * track move tool is selected
     */
    void moveSelected();

    /**
     * track resize tool is selected
     */
    void resizeSelected();
    
    void setPointerPosition(const int &position);
    
public slots:
    void editTrackNotation(Rosegarden::Track*);
    void editTrackNotationSmall(Rosegarden::Track*);

signals:
    void setTool(TracksCanvas::ToolType);
    void setPositionPointer(int);

protected:
    NotationView* m_notationView;

};

#endif // ROSEGARDENGUIVIEW_H
