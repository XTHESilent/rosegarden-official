
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

#ifndef _RG_REMAPINSTRUMENTDIALOG_H_
#define _RG_REMAPINSTRUMENTDIALOG_H_

#include "base/Studio.h"
#include <kdialogbase.h>


class QWidget;
class QRadioButton;
class QButtonGroup;
class KCommand;
class KComboBox;


namespace Rosegarden
{


class RosegardenGUIDoc;
class MultiViewCommandHistory;


class RemapInstrumentDialog : public KDialogBase
{
    Q_OBJECT
public:
    RemapInstrumentDialog(QWidget *parent,
                          RosegardenGUIDoc *doc);

    void populateCombo(int id);

    void addCommandToHistory(KCommand *command);
    MultiViewCommandHistory* getCommandHistory();

public slots:
    void slotRemapReleased(int id);

    void slotOk();
    void slotApply();

protected:

    RosegardenGUIDoc    *m_doc;

    QRadioButton        *m_deviceButton;
    QRadioButton        *m_instrumentButton;

    QButtonGroup        *m_buttonGroup;
    KComboBox           *m_fromCombo;
    KComboBox           *m_toCombo;

    DeviceList m_devices;
    InstrumentList m_instruments;
};


}

#endif
