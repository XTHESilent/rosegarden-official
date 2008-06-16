
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_TIMEDIALOG_H_
#define _RG_TIMEDIALOG_H_

#include <kdialogbase.h>
#include <qstring.h>
#include "base/Event.h"


class QWidget;


namespace Rosegarden
{

class TimeWidget;
class Composition;


class TimeDialog : public KDialogBase
{
    Q_OBJECT
public:
    /// for absolute times
    TimeDialog(QWidget *parent, QString title, Composition *composition,
               timeT defaultTime, bool constrainToCompositionDuration);

    /// for durations
    TimeDialog(QWidget *parent, QString title, Composition *composition,
               timeT startTime, timeT defaultDuration,
               bool constrainToCompositionDuration);

    timeT getTime() const;

protected:
    TimeWidget *m_timeWidget;
};
                     


}

#endif
