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

#include <qcheckbox.h>
#include <qcombobox.h>

#ifndef _GUIELEMENTS_H_
#define _GUIELEMENTS_H_

// Create out own check box which is always Tristate 
// and allows us to click only between on and off
// and only to _show_ the third ("Some") state 
//
//
class RosegardenTristateCheckBox : public QCheckBox
{
Q_OBJECT
public:
    RosegardenTristateCheckBox(QWidget *parent=0,
                               const char *name=0):QCheckBox(parent, name)
        { setTristate(true) ;}

    virtual ~RosegardenTristateCheckBox() {;}

protected:
    // don't emit when the button is released
    virtual void mouseReleaseEvent(QMouseEvent *) {;}

private:
};

// Turn a normal QComboBox into one that accepts mouse wheel
// events to change the value
//
class RosegardenComboBox : public QComboBox
{
Q_OBJECT
public:
    RosegardenComboBox(bool reverse, QWidget *parent=0, const char *name=0):
        QComboBox(parent, name) {;}

    RosegardenComboBox(bool reverse, bool rw,
                       QWidget *parent=0, const char *name=0):
        QComboBox(rw, parent, name), m_reverse(reverse) {;}


protected:
    virtual void wheelEvent(QWheelEvent *e)
    {
        e->accept();

        int value = e->delta();

        if (m_reverse)
             value = -value;
       
        if (value < 0)
        {
            if (currentItem() < count() - 1)
            {
                setCurrentItem(currentItem() + 1);
                emit propagate(currentItem());
            }
        }
        else
        {
            if (currentItem() > 0)
            {
                setCurrentItem(currentItem() - 1);
                emit propagate(currentItem());
            }
        }
    }

signals:
    void propagate(int); // update the Segment with new value

private:
    bool m_reverse;

};


#endif // _GUIELEMENTS_H_
