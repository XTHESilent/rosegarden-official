// -*- c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2005
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

#include <kmessagebox.h>
#include <klocale.h>
#include <kaction.h>
#include <kstddirs.h>
#include <qapplication.h>
#include <qtimer.h>

#include "BaseProperties.h"
#include "SegmentMatrixHelper.h"
#include "Composition.h"

#include "matrixtool.h"
#include "matrixview.h"
#include "matrixstaff.h"
#include "matrixcommands.h"
#include "notationcommands.h"
#include "velocitycolour.h"
#include "zoomslider.h"

#include "rosestrings.h"
#include "rosedebug.h"

#include "dialogs.h"

using Rosegarden::EventSelection;
using Rosegarden::SnapGrid;
using Rosegarden::Event;
using Rosegarden::Note;
using Rosegarden::timeT;
using std::endl;

// Use this to make sure that Matrix events butt up to the grid
// lines in a pleasing fashion,
//
const double _fiddleFactor = 1.0;

//////////////////////////////////////////////////////////////////////
//                     MatrixToolBox
//////////////////////////////////////////////////////////////////////

MatrixToolBox::MatrixToolBox(MatrixView* parent)
    : EditToolBox(parent),
      m_mParentView(parent)
{
}

EditTool* MatrixToolBox::createTool(const QString& toolName)
{
    MatrixTool* tool = 0;

    QString toolNamelc = toolName.lower();

    if (toolNamelc == MatrixPainter::ToolName)

        tool = new MatrixPainter(m_mParentView);

    else if (toolNamelc == MatrixEraser::ToolName)

        tool = new MatrixEraser(m_mParentView);

    else if (toolNamelc == MatrixSelector::ToolName)

        tool = new MatrixSelector(m_mParentView);

    else if (toolNamelc == MatrixMover::ToolName)

        tool = new MatrixMover(m_mParentView);

    else if (toolNamelc == MatrixResizer::ToolName)

        tool = new MatrixResizer(m_mParentView);

    else {
        KMessageBox::error(0, QString("MatrixToolBox::createTool : unrecognised toolname %1 (%2)")
                           .arg(toolName).arg(toolNamelc));
        return 0;
    }

    m_tools.insert(toolName, tool);

    return tool;
    
}

//////////////////////////////////////////////////////////////////////
//                     MatrixTools
//////////////////////////////////////////////////////////////////////

MatrixTool::MatrixTool(const QString& menuName, MatrixView* parent)
    : EditTool(menuName, parent),
      m_mParentView(parent)
{
}

void
MatrixTool::slotSelectSelected()
{
    m_parentView->actionCollection()->action("select")->activate();
}


void
MatrixTool::slotMoveSelected()
{
    m_parentView->actionCollection()->action("move")->activate();
}

void
MatrixTool::slotEraseSelected()
{
    m_parentView->actionCollection()->action("erase")->activate();
}

void
MatrixTool::slotResizeSelected()
{
    m_parentView->actionCollection()->action("resize")->activate();
}

void 
MatrixTool::slotDrawSelected()
{
    m_parentView->actionCollection()->action("draw")->activate();
}

void
MatrixTool::slotHalfSpeed()
{
    m_parentView->actionCollection()->action("half_speed")->activate();
}

void
MatrixTool::slotDoubleSpeed()
{
    m_parentView->actionCollection()->action("double_speed")->activate();
}



//------------------------------


MatrixPainter::MatrixPainter(MatrixView* parent)
    : MatrixTool("MatrixPainter", parent),
      m_currentElement(0),
      m_currentStaff(0)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QCanvasPixmap pixmap(pixmapDir + "/toolbar/select.xpm");
    QIconSet icon = QIconSet(pixmap);

    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    new KAction(i18n("Switch to Move Tool"), "move", 0, this,
                SLOT(slotMoveSelected()), actionCollection(),
                "move");

    pixmap.load(pixmapDir + "/toolbar/resize.xpm");
    icon = QIconSet(pixmap);
    new KAction(i18n("Switch to Resize Tool"), icon, 0, this,
                SLOT(slotResizeSelected()), actionCollection(),
                "resize");

    new KAction(i18n("Half Speed"), 0, 0, this,
                SLOT(slotHalfSpeed()), actionCollection(),
                "half_speed");

    new KAction(i18n("Double Speed"), 0, 0, this,
                SLOT(slotDoubleSpeed()), actionCollection(),
                "double_speed");

    createMenu("matrixpainter.rc");
}

MatrixPainter::MatrixPainter(QString name, MatrixView* parent)
    : MatrixTool(name, parent),
      m_currentElement(0),
      m_currentStaff(0)
{
}

void MatrixPainter::handleEventRemoved(Rosegarden::Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
	m_currentElement = 0;
    }
}

void MatrixPainter::handleLeftButtonPress(Rosegarden::timeT time,
                                          int pitch,
                                          int staffNo,
                                          QMouseEvent *e,
                                          Rosegarden::ViewElement *element)
{
    MATRIX_DEBUG << "MatrixPainter::handleLeftButtonPress : pitch = "
                         << pitch << ", time : " << time << endl;

    QPoint p = m_mParentView->inverseMapPoint(e->pos());

    // Don't create an overlapping event on the same note on the same channel
    if (dynamic_cast<MatrixElement*>(element))
    {
        MATRIX_DEBUG << "MatrixPainter::handleLeftButtonPress : overlap with an other matrix element\n";
        return;
    }

    // This is needed for the event duration rounding
    SnapGrid grid(m_mParentView->getSnapGrid());

    m_currentStaff = m_mParentView->getStaff(staffNo);
 
    Event *ev = new Event(Note::EventType, time,
			  grid.getSnapTime(double(p.x())));
    ev->set<Rosegarden::Int>(Rosegarden::BaseProperties::PITCH, pitch);
    ev->set<Rosegarden::Int>(Rosegarden::BaseProperties::VELOCITY, 100);

    m_currentElement = new MatrixElement(ev);

    int y = m_currentStaff->getLayoutYForHeight(pitch) -
            m_currentStaff->getElementHeight() / 2;

    m_currentElement->setLayoutY(y);
    m_currentElement->setLayoutX(grid.getRulerScale()->getXForTime(time));
    m_currentElement->setHeight(m_currentStaff->getElementHeight());

    double width = ev->getDuration() * m_currentStaff->getTimeScaleFactor();
    m_currentElement->setWidth(int(width + _fiddleFactor)); // fiddle factor

    m_currentStaff->positionElement(m_currentElement);
    m_mParentView->update();

    // preview
    m_mParentView->playNote(ev);
}

int MatrixPainter::handleMouseMove(Rosegarden::timeT time,
                                   int pitch,
                                   QMouseEvent *)
{
    // sanity check
    if (!m_currentElement) return RosegardenCanvasView::NoFollow;

    MATRIX_DEBUG << "MatrixPainter::handleMouseMove : pitch = "
                         << pitch << ", time : " << time << endl;

    using Rosegarden::BaseProperties::PITCH;

    int initialWidth = m_currentElement->getWidth();

    double width = (time - m_currentElement->getViewAbsoluteTime())
	* m_currentStaff->getTimeScaleFactor();

    // ensure we don't have a zero width preview
    if (width == 0) width = initialWidth;
    else width += _fiddleFactor; // fiddle factor

    m_currentElement->setWidth(int(width));
    
    if (m_currentElement->event()->has(PITCH) &&
	pitch != m_currentElement->event()->get<Rosegarden::Int>(PITCH)) {
	m_currentElement->event()->set<Rosegarden::Int>(PITCH, pitch);
	int y = m_currentStaff->getLayoutYForHeight(pitch) -
	    m_currentStaff->getElementHeight() / 2;
	m_currentElement->setLayoutY(y);
	m_currentStaff->positionElement(m_currentElement);

        // preview
        m_mParentView->playNote(m_currentElement->event());
    }
    m_mParentView->update();

    return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
}

void MatrixPainter::handleMouseRelease(Rosegarden::timeT endTime,
                                       int,
                                       QMouseEvent *)
{
    // This can happen in case of screen/window capture -
    // we only get a mouse release, the window snapshot tool
    // got the mouse down
    if (!m_currentElement) return;

    // Insert element if it has a non null duration,
    // discard it otherwise
    //
    timeT time = m_currentElement->getViewAbsoluteTime();

    if (time > endTime) std::swap(time, endTime);

    if (endTime == time) endTime = time + m_currentElement->getViewDuration();

    Rosegarden::SegmentMatrixHelper helper(m_currentStaff->getSegment());
    MATRIX_DEBUG << "MatrixPainter::handleMouseRelease() : helper.insertNote()\n";

    MatrixInsertionCommand* command = 
	new MatrixInsertionCommand(m_currentStaff->getSegment(),
				   time,
                                   endTime,
				   m_currentElement->event());
    
    m_mParentView->addCommandToHistory(command);

    Event* ev = m_currentElement->event();
    delete m_currentElement;
    delete ev;

    ev = command->getLastInsertedEvent();
    if (ev) m_mParentView->setSingleSelectedEvent(m_currentStaff->getSegment(),
						  ev);

    m_mParentView->update();
    m_currentElement = 0;
}

void MatrixPainter::ready()
{
    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixPainter::stow()
{
    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixPainter::slotMatrixScrolled(int newX, int newY) 
{
    if (!m_currentElement) return;

    QPoint newP1(newX, newY), oldP1(m_parentView->getCanvasView()->contentsX(),
                                    m_parentView->getCanvasView()->contentsY());

    QPoint offset = newP1 - oldP1;

    offset = m_mParentView->inverseMapPoint(offset);

    QPoint p(m_currentElement->getCanvasX() + m_currentElement->getWidth(), m_currentElement->getCanvasY());
    p += offset;
    
    timeT newTime = m_mParentView->getSnapGrid().snapX(p.x());
    int newPitch = m_currentStaff->getHeightAtCanvasCoords(p.x(), p.y());

    handleMouseMove(newTime, newPitch, 0);
}

//------------------------------

MatrixEraser::MatrixEraser(MatrixView* parent)
    : MatrixTool("MatrixEraser", parent),
      m_currentStaff(0)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QCanvasPixmap pixmap(pixmapDir + "/toolbar/select.xpm");
    QIconSet icon = QIconSet(pixmap);

    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Draw Tool"), "pencil", 0, this,
                SLOT(slotDrawSelected()), actionCollection(),
                "draw");

    new KAction(i18n("Switch to Move Tool"), "move", 0, this,
                SLOT(slotMoveSelected()), actionCollection(),
                "move");

    pixmap.load(pixmapDir + "/toolbar/resize.xpm");
    icon = QIconSet(pixmap);
    new KAction(i18n("Switch to Resize Tool"), icon, 0, this,
                SLOT(slotResizeSelected()), actionCollection(),
                "resize");

    new KAction(i18n("Half Speed"), 0, 0, this,
                SLOT(slotHalfSpeed()), actionCollection(),
                "half_speed");

    new KAction(i18n("Double Speed"), 0, 0, this,
                SLOT(slotDoubleSpeed()), actionCollection(),
                "double_speed");

    createMenu("matrixeraser.rc");
}

void MatrixEraser::handleLeftButtonPress(Rosegarden::timeT,
                                         int,
                                         int staffNo,
                                         QMouseEvent*,
                                         Rosegarden::ViewElement* el)
{
    MATRIX_DEBUG << "MatrixEraser::handleLeftButtonPress : el = "
                         << el << endl;

    if (!el) return; // nothing to erase

    m_currentStaff = m_mParentView->getStaff(staffNo);

    MatrixEraseCommand* command =
        new MatrixEraseCommand(m_currentStaff->getSegment(), el->event());

    m_mParentView->addCommandToHistory(command);

    m_mParentView->update();
}

//------------------------------

MatrixSelector::MatrixSelector(MatrixView* view)
    : MatrixTool("MatrixSelector", view),
      m_selectionRect(0),
      m_updateRect(false),
      m_currentStaff(0),
      m_clickedElement(0),
      m_dispatchTool(0),
      m_justSelectedBar(false),
      m_matrixView(view),
      m_selectionToMerge(0)
{
    connect(m_parentView, SIGNAL(usedSelection()),
            this,         SLOT(slotHideSelection()));

    new KAction(i18n("Switch to Draw Tool"), "pencil", 0, this,
                SLOT(slotDrawSelected()), actionCollection(),
                "draw");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    new KAction(i18n("Switch to Move Tool"), "move", 0, this,
                SLOT(slotMoveSelected()), actionCollection(),
                "move");

    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QCanvasPixmap pixmap(pixmapDir + "/toolbar/resize.xpm");
    QIconSet icon = QIconSet(pixmap);

    new KAction(i18n("Switch to Resize Tool"), icon, 0, this,
                SLOT(slotResizeSelected()), actionCollection(),
                "resize");

    new KAction(i18n("Half Speed"), 0, 0, this,
                SLOT(slotHalfSpeed()), actionCollection(),
                "half_speed");

    new KAction(i18n("Double Speed"), 0, 0, this,
                SLOT(slotDoubleSpeed()), actionCollection(),
                "double_speed");

    createMenu("matrixselector.rc");
}

void MatrixSelector::handleEventRemoved(Rosegarden::Event *event)
{
    if (m_dispatchTool) m_dispatchTool->handleEventRemoved(event);
    if (m_clickedElement && m_clickedElement->event() == event) {
	m_clickedElement = 0;
    }
}

void MatrixSelector::slotClickTimeout()
{
    m_justSelectedBar = false;
}

void MatrixSelector::handleLeftButtonPress(Rosegarden::timeT time,
                                           int height,
                                           int staffNo,
                                           QMouseEvent* e,
                                           Rosegarden::ViewElement *element)
{
    MATRIX_DEBUG << "MatrixSelector::handleMousePress" << endl;

    if (m_justSelectedBar) {
	handleMouseTripleClick(time, height, staffNo, e, element);
	m_justSelectedBar = false;
	return;
    }

    QPoint p = m_mParentView->inverseMapPoint(e->pos());

    m_currentStaff = m_mParentView->getStaff(staffNo);

    // Do the merge selection thing
    //
    delete m_selectionToMerge; // you can safely delete 0, you know?
    const Rosegarden::EventSelection *selectionToMerge = 0;
    if (e->state() & Qt::ShiftButton)
        selectionToMerge = m_mParentView->getCurrentSelection();

    m_selectionToMerge =
        (selectionToMerge ? new EventSelection(*selectionToMerge) : 0);

    // Now the rest of the element stuff
    //
    m_clickedElement = dynamic_cast<MatrixElement*>(element);

    if (m_clickedElement)
    {
        int x = int(m_clickedElement->getLayoutX());
        int width = m_clickedElement->getWidth();
        int resizeStart = int(double(width) * 0.85) + x;

        // max size of 10
        if ((x + width ) - resizeStart > 10)
            resizeStart = x + width - 10;

        if (p.x() > resizeStart)
        {
            m_dispatchTool = m_parentView->
                getToolBox()->getTool(MatrixResizer::ToolName);
        }
        else
        {
            m_dispatchTool = m_parentView->
                getToolBox()->getTool(MatrixMover::ToolName);
        }

        m_dispatchTool->handleLeftButtonPress(time, 
                                              height,
                                              staffNo,
                                              e,
                                              element);
        return;
    }
    else
    {
	// Workaround for #930420 Positional error in sweep-selection box
	// boundary
	int zoomValue = (int)m_matrixView->m_hZoomSlider->getCurrentSize();
	MatrixStaff *staff = m_mParentView->getStaff(staffNo);
	int pitch = m_currentStaff->getHeightAtCanvasCoords(p.x(), p.y());
	int pitchCentreHeight = staff->getTotalHeight() -
				pitch * staff->getLineSpacing() - 2; // 2 or ?
	int pitchLineHeight = pitchCentreHeight + staff->getLineSpacing()/2;
	int drawHeight = p.y();
	if (drawHeight <= pitchLineHeight + 1 &&
	    drawHeight >= pitchLineHeight - 1) {
	    if (drawHeight == pitchLineHeight)
		drawHeight += 2;
	    else
		drawHeight += 2 * (drawHeight - pitchLineHeight);
	}
	MATRIX_DEBUG << "#### MatrixSelector::handleLeftButtonPress() : zoom "
		<< zoomValue
		<< " pitch " << pitch
		<< " pitchCentreHeight " << pitchCentreHeight
		<< " pitchLineHeight " << pitchLineHeight
		<< " lineSpacing " << staff->getLineSpacing()
		<< " drawHeight " << drawHeight << endl;
        m_selectionRect->setX(int(p.x()/4)*4); // more workaround for #930420
        m_selectionRect->setY(drawHeight);
        m_selectionRect->setSize(0,0);

        m_selectionRect->show();
        m_updateRect = true;

        // Clear existing selection if we're not merging
        //
        if (!m_selectionToMerge)
        {
            m_mParentView->setCurrentSelection(0, false, true);
            m_mParentView->canvas()->update();
        }
    }

    //m_parentView->setCursorPosition(p.x());
}

void MatrixSelector::handleMidButtonPress(Rosegarden::timeT time,
                                          int height,
                                          int staffNo,
                                          QMouseEvent* e,
                                          Rosegarden::ViewElement *element)
{
    m_clickedElement = 0; // should be used for left-button clicks only

    // Don't allow overlapping elements on the same channel
    if (dynamic_cast<MatrixElement*>(element)) return;

    m_dispatchTool = m_parentView->
        getToolBox()->getTool(MatrixPainter::ToolName);

    m_dispatchTool->handleLeftButtonPress(time, height, staffNo, e, element);
}

// Pop up an event editor - send a signal or something
//
void MatrixSelector::handleMouseDoubleClick(Rosegarden::timeT ,
					    int ,
					    int staffNo,
					    QMouseEvent *ev,
					    Rosegarden::ViewElement *element)
{
/*
    if (m_dispatchTool)
    {
        m_dispatchTool->handleMouseDoubleClick(time, height, staffNo, e, element);
    }
*/
    
    m_clickedElement = dynamic_cast<MatrixElement*>(element);

    MatrixStaff *staff = m_mParentView->getStaff(staffNo);
    if (!staff) return;

    if (m_clickedElement) {

	if (m_clickedElement->event()->isa(Rosegarden::Note::EventType) &&
	    m_clickedElement->event()->has(Rosegarden::BaseProperties::TRIGGER_SEGMENT_ID)) {
	    
	    int id = m_clickedElement->event()->get<Rosegarden::Int>
		(Rosegarden::BaseProperties::TRIGGER_SEGMENT_ID);
	    emit editTriggerSegment(id);
	    return;
	}

	if (ev->state() & ShiftButton) { // advanced edit

	    EventEditDialog dialog(m_mParentView, *m_clickedElement->event(), true);

	    if (dialog.exec() == QDialog::Accepted &&
		dialog.isModified()) {
		
		EventEditCommand *command = new EventEditCommand
		    (staff->getSegment(),
		     m_clickedElement->event(),
		     dialog.getEvent());
		
		m_mParentView->addCommandToHistory(command);
	    }
	} else {

	    SimpleEventEditDialog dialog(m_mParentView, m_mParentView->getDocument(),
					 *m_clickedElement->event(), false);

	    if (dialog.exec() == QDialog::Accepted &&
		dialog.isModified()) {
		
		EventEditCommand *command = new EventEditCommand
		    (staff->getSegment(),
		     m_clickedElement->event(),
		     dialog.getEvent());
		
		m_mParentView->addCommandToHistory(command);
	    }
	}	    

    } /*  
	  
      #988167: Matrix:Multiclick select methods don't work in matrix editor
      Postponing this, as it falls foul of world-matrix transformation
      etiquette and other such niceties

	  else {

	QRect rect = staff->getBarExtents(ev->x(), ev->y());

	m_selectionRect->setX(rect.x() + 2);
	m_selectionRect->setY(rect.y());
	m_selectionRect->setSize(rect.width() - 4, rect.height());

	m_selectionRect->show();
	m_updateRect = false;
	
	m_justSelectedBar = true;
	QTimer::singleShot(QApplication::doubleClickInterval(), this,
			   SLOT(slotClickTimeout()));
    } */
}

void MatrixSelector::handleMouseTripleClick(Rosegarden::timeT t,
					    int height,
					    int staffNo,
					    QMouseEvent *ev,
					    Rosegarden::ViewElement *element)
{
    if (!m_justSelectedBar) return;
    m_justSelectedBar = false;

    MatrixStaff *staff = m_mParentView->getStaff(staffNo);
    if (!staff) return;

    if (m_clickedElement) {

	// should be safe, as we've already set m_justSelectedBar false
	handleLeftButtonPress(t, height, staffNo, ev, element);
	return;

    } else {

	m_selectionRect->setX(staff->getX());
	m_selectionRect->setY(staff->getY());
	m_selectionRect->setSize(int(staff->getTotalWidth()) - 1,
				 staff->getTotalHeight() - 1);

	m_selectionRect->show();
	m_updateRect = false;
    }
}

int MatrixSelector::handleMouseMove(timeT time, int height,
                                    QMouseEvent *e)
{
    QPoint p = m_mParentView->inverseMapPoint(e->pos());

    if (m_dispatchTool)
    {
        return m_dispatchTool->handleMouseMove(time, height, e);
    }

    if (!m_updateRect) return RosegardenCanvasView::NoFollow;

    int w = int(p.x() - m_selectionRect->x());
    int h = int(p.y() - m_selectionRect->y());

    // Qt rectangle dimensions appear to be 1-based
    if (w > 0) ++w; else --w;
    if (h > 0) ++h; else --h;

    // Workaround for #930420 Positional error in sweep-selection box boundary
    int wFix = (w > 0) ? 3 : 0;
    int hFix = (h > 0) ? 3 : 0;
    int xFix = (w < 0) ? 3 : 0;
    m_selectionRect->setSize(w-wFix,h-hFix);
    m_selectionRect->setX(m_selectionRect->x()+xFix);
    setViewCurrentSelection();
    m_selectionRect->setSize(w,h);
    m_selectionRect->setX(m_selectionRect->x()-xFix);
    m_mParentView->canvas()->update();

    return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
}

void MatrixSelector::handleMouseRelease(timeT time, int height, QMouseEvent *e)
{
    MATRIX_DEBUG << "MatrixSelector::handleMouseRelease" << endl;

    if (m_dispatchTool)
    {
        m_dispatchTool->handleMouseRelease(time, height, e);

        // don't delete the tool as it's still part of the toolbox
        m_dispatchTool = 0;

        return;
    }

    m_updateRect = false;

    if (m_clickedElement)
    {
        m_mParentView->setSingleSelectedEvent(m_currentStaff->getSegment(),
                                              m_clickedElement->event(),
					      false, true);
        m_mParentView->canvas()->update();
        m_clickedElement = 0;

    }
    else if (m_selectionRect)
    {
        setViewCurrentSelection();
        m_selectionRect->hide();
        m_mParentView->canvas()->update();
    }

    // Tell anyone who's interested that the selection has changed
    emit gotSelection();
}

void MatrixSelector::ready()
{
    if (m_mParentView)
    {
        m_selectionRect = new QCanvasRectangle(m_mParentView->canvas());
        m_selectionRect->hide();
        m_selectionRect->setPen(Rosegarden::GUIPalette::getColour(Rosegarden::GUIPalette::SelectionRectangle));

        m_mParentView->setCanvasCursor(Qt::arrowCursor);
        //m_mParentView->setPositionTracking(false);
    }

    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));

}

void MatrixSelector::stow()
{
    if (m_selectionRect)
    {
        delete m_selectionRect;
        m_selectionRect = 0;
        m_mParentView->canvas()->update();
    }

    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));

}


void MatrixSelector::slotHideSelection()
{
    if (!m_selectionRect) return;
    m_selectionRect->hide();
    m_selectionRect->setSize(0,0);
    m_mParentView->canvas()->update();
}

void MatrixSelector::slotMatrixScrolled(int newX, int newY)
{
    if (m_updateRect) {
        int offsetX = newX - m_parentView->getCanvasView()->contentsX();
        int offsetY = newY - m_parentView->getCanvasView()->contentsY();

        int w = int(m_selectionRect->width() + offsetX);
        int h = int(m_selectionRect->height() + offsetY);

        // Qt rectangle dimensions appear to be 1-based
        if (w > 0) ++w; else --w;
        if (h > 0) ++h; else --h;

        m_selectionRect->setSize(w,h);
        setViewCurrentSelection();
        m_mParentView->canvas()->update();
    }
}


void MatrixSelector::setViewCurrentSelection()
{
    EventSelection* selection = getSelection();

    if (m_selectionToMerge && selection &&
        m_selectionToMerge->getSegment() == selection->getSegment()) {

        selection->addFromSelection(m_selectionToMerge);
        m_mParentView->setCurrentSelection(selection, true, true);

    } else if (!m_selectionToMerge) {

        m_mParentView->setCurrentSelection(selection, true, true);

    }
    
}

EventSelection* MatrixSelector::getSelection()
{
    if (!m_selectionRect->visible()) return 0;

    Rosegarden::Segment& originalSegment = m_currentStaff->getSegment();
    EventSelection* selection = new EventSelection(originalSegment);

    // get the selections
    //
    QCanvasItemList l = m_selectionRect->collisions(true);

    if (l.count())
    {
        for (QCanvasItemList::Iterator it=l.begin(); it!=l.end(); ++it)
        {
            QCanvasItem *item = *it;
            QCanvasMatrixRectangle *matrixRect = 0;

            if ((matrixRect = dynamic_cast<QCanvasMatrixRectangle*>(item)))
            {
                MatrixElement *mE = &matrixRect->getMatrixElement();
                selection->addEvent(mE->event());
            }
        }
    }

    return (selection->getAddedEvents() > 0) ? selection : 0;
}

//------------------------------

MatrixMover::MatrixMover(MatrixView* parent)
    : MatrixTool("MatrixMover", parent),
      m_currentElement(0),
      m_currentStaff(0),
      m_oldWidth(0),
      m_oldX(0),
      m_oldY(0)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QCanvasPixmap pixmap(pixmapDir + "/toolbar/select.xpm");
    QIconSet icon = QIconSet(pixmap);

    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Draw Tool"), "pencil", 0, this,
                SLOT(slotDrawSelected()), actionCollection(),
                "draw");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    pixmap.load(pixmapDir + "/toolbar/resize.xpm");
    icon = QIconSet(pixmap);
    new KAction(i18n("Switch to Resize Tool"), icon, 0, this,
                SLOT(slotResizeSelected()), actionCollection(),
                "resize");

    new KAction(i18n("Half Speed"), 0, 0, this,
                SLOT(slotHalfSpeed()), actionCollection(),
                "half_speed");

    new KAction(i18n("Double Speed"), 0, 0, this,
                SLOT(slotDoubleSpeed()), actionCollection(),
                "double_speed");

    createMenu("matrixmover.rc");
}

void MatrixMover::handleEventRemoved(Rosegarden::Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
	m_currentElement = 0;
    }
}

void MatrixMover::handleLeftButtonPress(Rosegarden::timeT,
                                        int,
                                        int staffNo,
                                        QMouseEvent* e,
                                        Rosegarden::ViewElement* el)
{
    MATRIX_DEBUG << "MatrixMover::handleLeftButtonPress() : el = "
                         << el << endl;

    if (!el) return; // nothing to erase

    m_currentElement = dynamic_cast<MatrixElement*>(el);
    m_currentStaff = m_mParentView->getStaff(staffNo);

    if (m_currentElement)
    {
        // store these so that we know not to resize if we've not modified
        // the physical blob on the screen.
        //
        m_oldWidth = m_currentElement->getWidth();
        m_oldX = m_currentElement->getLayoutX();
        m_oldY = m_currentElement->getLayoutY();
        
        // Add this element and allow movement
        //
        EventSelection* selection = m_mParentView->getCurrentSelection();

        if (selection) 
        {
            EventSelection *newSelection;
           
            if ((e->state() & Qt::ShiftButton) ||
                selection->contains(m_currentElement->event()))
                newSelection = new EventSelection(*selection);
            else
                newSelection = new EventSelection(m_currentStaff->getSegment());

            newSelection->addEvent(m_currentElement->event());
            m_mParentView->setCurrentSelection(newSelection, true, true);
            m_mParentView->canvas()->update();
        }
        else
        {
            m_mParentView->setSingleSelectedEvent(m_currentStaff->getSegment(),
                                                  m_currentElement->event(),
						  true);
            m_mParentView->canvas()->update();
        }
    }
}

int MatrixMover::handleMouseMove(Rosegarden::timeT newTime,
                                 int newPitch,
                                 QMouseEvent*)
{
    MATRIX_DEBUG << "MatrixMover::handleMouseMove() time = "
                         << newTime << endl;

    if (!m_currentElement || !m_currentStaff) return RosegardenCanvasView::NoFollow;

    /*
    using Rosegarden::BaseProperties::PITCH;
    timeT oldTime = m_currentElement->event()->getAbsoluteTime();
    int oldPitch = m_currentElement->event()->get<Rosegarden::Int>(PITCH);

    timeT diffTime = newTime - oldTime;

    int diffX = int(double(diffTime) * m_currentStaff->getTimeScaleFactor());
    int newY = m_currentStaff->getLayoutYForHeight(newPitch) 
                - m_currentStaff->getElementHeight() / 2;
    int oldY = m_currentStaff->getLayoutYForHeight(oldPitch) 
                - m_currentStaff->getElementHeight() / 2;
    int diffY = newY - oldY;

    cout << "DIFF TIME = " << diffTime << " (x = " << diffX << " )" << endl;
    cout << "DIFF PITCH = " << diffPitch << " (y= " << diffY << " )" << endl;

    if (diffX == 0 && diffY == 0) { // don't generate command or refresh
	m_mParentView->canvas()->update();
	m_currentElement = 0;
	return false;
    }
    */

    using Rosegarden::BaseProperties::PITCH;
    int diffPitch = 0;
    if (m_currentElement->event()->has(PITCH))
    {
        diffPitch = newPitch -
            m_currentElement->event()->get<Rosegarden::Int>(PITCH);
    }

    int diffX = 
        int(double(newTime - m_currentElement->getViewAbsoluteTime()) *
		    m_currentStaff->getTimeScaleFactor());

    // Add this fiddle factor to ensure the notes butt properly
    //
    if (diffX < 0) diffX -= int(_fiddleFactor);
    else if (diffX > 0) diffX += int(_fiddleFactor);

    int diffY = 
        int(((m_currentStaff->getLayoutYForHeight(newPitch) -
		      m_currentStaff->getElementHeight() / 2) -
		     m_currentElement->getLayoutY()));

    EventSelection* selection = m_mParentView->getCurrentSelection();
    Rosegarden::EventSelection::eventcontainer::iterator it =
        selection->getSegmentEvents().begin();

    MatrixElement *element = 0;
    int maxY = m_currentStaff->getCanvasYForHeight(0);

    /*
    MATRIX_DEBUG << "MatrixMover::handleMouseMove - "
                 << "oldX = " << m_oldX
                 << ", diffX = " << diffX << endl;
                 */

    m_currentElement->setLayoutX(m_oldX + diffX);
    
    for (; it != selection->getSegmentEvents().end(); it++)
    {
        element = m_currentStaff->getElement(*it);

        if (element)
        {
            int newDiffX = int(
                double(element->getViewAbsoluteTime() -
				  m_currentElement->getViewAbsoluteTime()) *
			   m_currentStaff->getTimeScaleFactor());

            if (newDiffX < 0) newDiffX -= int(_fiddleFactor);
            else if (newDiffX > 0) newDiffX += int(_fiddleFactor);

            int newX = int(m_currentElement->getLayoutX() + newDiffX);
            int newY = int(element->getLayoutY() + diffY);

            // bounds checking
            if (newX < 0) newX = 0;
            if (newY < 0) newY = 0;
            if (newY > maxY) newY = maxY;

            if (element != m_currentElement) element->setLayoutX(newX);
            element->setLayoutY(newY);

            m_currentStaff->positionElement(element);
        }

        if (diffY && element->event()->has(PITCH))
        {
            //Rosegarden::Event *e = new Event(*(element->event()));
            //int newPitch = e->get<Rosegarden::Int>(PITCH) + diffPitch;

            //m_mParentView->playNote(selection->getSegment(), newPitch);
        }
    }

    m_mParentView->canvas()->update();
    return RosegardenCanvasView::FollowHorizontal | RosegardenCanvasView::FollowVertical;
}

void MatrixMover::handleMouseRelease(Rosegarden::timeT newTime,
                                     int newPitch,
                                     QMouseEvent*)
{
    MATRIX_DEBUG << "MatrixMover::handleMouseRelease() - newPitch = "
                 << newPitch << '\n';

    if (!m_currentElement || !m_currentStaff) return;

    if (newPitch > MatrixVLayout::maxMIDIPitch)
        newPitch = MatrixVLayout::maxMIDIPitch;
    if (newPitch < 0)
        newPitch = 0;

    MATRIX_DEBUG << "MatrixMover::handleMouseRelease() - corrected newPitch = "
                 << newPitch << '\n';

    int y = m_currentStaff->getLayoutYForHeight(newPitch)
        - m_currentStaff->getElementHeight() / 2;

    MATRIX_DEBUG << "MatrixMover::handleMouseRelease() y = " << y << endl;

    // Don't do anything if we've not changed the size of the physical
    // element.
    //
    if (m_oldWidth == m_currentElement->getWidth() &&
        m_oldX == m_currentElement->getLayoutX() &&
        m_oldY == m_currentElement->getLayoutY())
    {
        m_oldWidth = 0;
        m_oldX = 0.0;
        m_oldY = 0.0;
        m_currentElement = 0;
        return;
    }


    using Rosegarden::BaseProperties::PITCH;
    timeT diffTime = newTime - m_currentElement->getViewAbsoluteTime();
    int diffPitch = 0;
    if (m_currentElement->event()->has(PITCH)) {
	diffPitch = newPitch -
	    m_currentElement->event()->get<Rosegarden::Int>(PITCH);
    }

    if (diffTime == 0 && diffPitch == 0) { // don't generate command or refresh
	m_mParentView->canvas()->update();
	m_currentElement = 0;
	return;
    }

    EventSelection *selection = m_mParentView->getCurrentSelection();

    if (selection->getAddedEvents() == 0)
        return;
    else 
    { 
        QString commandLabel = i18n("Move Event");

        if (selection->getAddedEvents() > 1)
            commandLabel = i18n("Move Events");

        KMacroCommand *macro = new KMacroCommand(commandLabel);

        Rosegarden::EventSelection::eventcontainer::iterator it =
            selection->getSegmentEvents().begin();

        EventSelection *newSelection = 
            new EventSelection(m_currentStaff->getSegment());

        for (; it != selection->getSegmentEvents().end(); it++)
        {
            timeT newTime = (*it)->getAbsoluteTime() + diffTime;
            int newPitch = 60;
	    if ((*it)->has(PITCH)) {
		newPitch = (*it)->get<Rosegarden::Int>(PITCH) + diffPitch;
	    }
            
            Rosegarden::Event *newEvent = new Rosegarden::Event(**it, newTime);
            newEvent->set<Rosegarden::Int>
                (Rosegarden::BaseProperties::PITCH,newPitch);

            macro->addCommand(
                    new MatrixModifyCommand(m_currentStaff->getSegment(),
                                            (*it),
                                            newEvent,
                                            true,
					    false));
            newSelection->addEvent(newEvent);
        }

	macro->addCommand(new AdjustMenuNormalizeRestsCommand(*newSelection));

        m_mParentView->setCurrentSelection(0, false, false);
        m_mParentView->addCommandToHistory(macro);
        m_mParentView->setCurrentSelection(newSelection, false, false);

    }

    m_mParentView->canvas()->update();
    m_currentElement = 0;
}

void MatrixMover::ready()
{
    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixMover::stow()
{
    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixMover::slotMatrixScrolled(int newX, int newY) 
{
    if (!m_currentElement) return;

    QPoint newP1(newX, newY), oldP1(m_parentView->getCanvasView()->contentsX(),
                                    m_parentView->getCanvasView()->contentsY());

    QPoint offset = newP1 - oldP1;

    offset = m_mParentView->inverseMapPoint(offset);

    QPoint p(m_currentElement->getCanvasX(), m_currentElement->getCanvasY());
    p += offset;
    
    timeT newTime = m_mParentView->getSnapGrid().snapX(p.x());
    int newPitch = m_currentStaff->getHeightAtCanvasCoords(p.x(), p.y());

    handleMouseMove(newTime, newPitch, 0);
}


//------------------------------
MatrixResizer::MatrixResizer(MatrixView* parent)
    : MatrixTool("MatrixResizer", parent),
      m_currentElement(0),
      m_currentStaff(0)
{
    QString pixmapDir = KGlobal::dirs()->findResource("appdata", "pixmaps/");
    QCanvasPixmap pixmap(pixmapDir + "/toolbar/select.xpm");
    QIconSet icon = QIconSet(pixmap);

    new KAction(i18n("Switch to Select Tool"), icon, 0, this,
                SLOT(slotSelectSelected()), actionCollection(),
                "select");

    new KAction(i18n("Switch to Draw Tool"), "pencil", 0, this,
                SLOT(slotDrawSelected()), actionCollection(),
                "draw");

    new KAction(i18n("Switch to Erase Tool"), "eraser", 0, this,
                SLOT(slotEraseSelected()), actionCollection(),
                "erase");

    new KAction(i18n("Switch to Move Tool"), "move", 0, this,
                SLOT(slotMoveSelected()), actionCollection(),
                "move");

    new KAction(i18n("Half Speed"), 0, 0, this,
                SLOT(slotHalfSpeed()), actionCollection(),
                "half_speed");

    new KAction(i18n("Double Speed"), 0, 0, this,
                SLOT(slotDoubleSpeed()), actionCollection(),
                "double_speed");

    createMenu("matrixresizer.rc");
}

void MatrixResizer::handleEventRemoved(Rosegarden::Event *event)
{
    if (m_currentElement && m_currentElement->event() == event) {
	m_currentElement = 0;
    }
}

void MatrixResizer::handleLeftButtonPress(Rosegarden::timeT,
                                          int,
                                          int staffNo,
                                          QMouseEvent* e,
                                          Rosegarden::ViewElement* el)
{
    MATRIX_DEBUG << "MatrixResizer::handleLeftButtonPress() : el = "
                         << el << endl;

    if (!el) return; // nothing to erase

    m_currentElement = dynamic_cast<MatrixElement*>(el);
    m_currentStaff = m_mParentView->getStaff(staffNo);

    if (m_currentElement)
    {
        // Add this element and allow movement
        //
        EventSelection* selection = m_mParentView->getCurrentSelection();

        if (selection) 
        {
            EventSelection *newSelection;
           
            if ((e->state() & Qt::ShiftButton) ||
                selection->contains(m_currentElement->event()))
                newSelection = new EventSelection(*selection);
            else
                newSelection = new EventSelection(m_currentStaff->getSegment());

            newSelection->addEvent(m_currentElement->event());
            m_mParentView->setCurrentSelection(newSelection, true, true);
            m_mParentView->canvas()->update();
        }
        else
        {
            m_mParentView->setSingleSelectedEvent(m_currentStaff->getSegment(),
                                                  m_currentElement->event(),
						  true);
            m_mParentView->canvas()->update();
        }
    }
}

int MatrixResizer::handleMouseMove(Rosegarden::timeT newTime,
                                   int,
                                   QMouseEvent *)
{
    if (!m_currentElement || !m_currentStaff) return RosegardenCanvasView::NoFollow;
    timeT newDuration = newTime - m_currentElement->getViewAbsoluteTime();

    int initialWidth = m_currentElement->getWidth();
    double width = newDuration * m_currentStaff->getTimeScaleFactor();

    // Don't allow zero width here - always at least _fiddleFactor wide
    if (width > 0) width += _fiddleFactor;
    else if (width < 0) width -= _fiddleFactor;

    int diffWidth = initialWidth - int(width);

    EventSelection* selection = m_mParentView->getCurrentSelection();
    Rosegarden::EventSelection::eventcontainer::iterator it =
        selection->getSegmentEvents().begin();

    MatrixElement *element = 0;
    for (; it != selection->getSegmentEvents().end(); it++)
    {
        element = m_currentStaff->getElement(*it);

        if (element)
        {
            int newWidth = element->getWidth() - diffWidth;

            /*
            MATRIX_DEBUG << "MatrixResizer::handleMouseMove - "
                         << "new width = " << newWidth << endl;
                         */

            element->setWidth(newWidth);
            m_currentStaff->positionElement(element);
        }
    }

    m_mParentView->canvas()->update();
    return RosegardenCanvasView::FollowHorizontal;
}

void MatrixResizer::handleMouseRelease(Rosegarden::timeT newTime,
                                       int, QMouseEvent*)
{
    if (!m_currentElement || !m_currentStaff) return;

    timeT diffDuration = 
        newTime - m_currentElement->getViewAbsoluteTime() - 
        m_currentElement->getViewDuration();

    EventSelection *selection = m_mParentView->getCurrentSelection();

    if (selection->getAddedEvents() == 0)
        return;
    else 
    {
        QString commandLabel = i18n("Resize Event");

        if (selection->getAddedEvents() > 1)
            commandLabel = i18n("Resize Events");

        KMacroCommand *macro = new KMacroCommand(commandLabel);

        Rosegarden::EventSelection::eventcontainer::iterator it =
            selection->getSegmentEvents().begin();

        EventSelection *newSelection = 
            new EventSelection(m_currentStaff->getSegment());

        for (; it != selection->getSegmentEvents().end(); it++)
        {
            timeT eventTime = (*it)->getAbsoluteTime();
            timeT eventDuration = (*it)->getDuration() + diffDuration;

            /*
            MATRIX_DEBUG << "MatrixResizer::handleMouseRelease - "
                         << "Time = " << eventTime
                         << ", Duration = " << eventDuration << endl;
                         */

            if (eventDuration < 0)
            {
                eventTime += eventDuration;
                eventDuration = -eventDuration;
            }


            // ensure the duration is always (arbitrary) positive
            //if (eventDuration == 0)
                //eventDuration = 60;
                //m_mParentView->getSnapGrid().getSnapTime(e->x());
            
            Rosegarden::Event *newEvent =
                new Rosegarden::Event(**it,
                                      eventTime,
                                      eventDuration);

            macro->addCommand(
                    new MatrixModifyCommand(m_currentStaff->getSegment(),
                                            *it,
                                            newEvent,
                                            false,
					    false));

            newSelection->addEvent(newEvent);
        }

	macro->addCommand(new AdjustMenuNormalizeRestsCommand(*newSelection));

        m_mParentView->setCurrentSelection(0, false, false);
        m_mParentView->addCommandToHistory(macro);
        m_mParentView->setCurrentSelection(newSelection, false, false);
    }

    m_mParentView->update();
    m_currentElement = 0;
}

void MatrixResizer::ready()
{
    connect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
            this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixResizer::stow()
{
    disconnect(m_parentView->getCanvasView(), SIGNAL(contentsMoving (int, int)),
               this, SLOT(slotMatrixScrolled(int, int)));
}

void MatrixResizer::slotMatrixScrolled(int newX, int newY) 
{
    QPoint newP1(newX, newY), oldP1(m_parentView->getCanvasView()->contentsX(),
                                    m_parentView->getCanvasView()->contentsY());

    QPoint p(newX, newY);
    
    if (newP1.x() > oldP1.x()) {
        p.setX(newX + m_parentView->getCanvasView()->visibleWidth());
    }

    p = m_mParentView->inverseMapPoint(p);
    int newTime = m_mParentView->getSnapGrid().snapX(p.x());
    handleMouseMove(newTime, 0, 0);
}

//------------------------------

const QString MatrixPainter::ToolName   = "painter";
const QString MatrixEraser::ToolName    = "eraser";
const QString MatrixSelector::ToolName  = "selector";
const QString MatrixMover::ToolName     = "mover";
const QString MatrixResizer::ToolName   = "resizer";

