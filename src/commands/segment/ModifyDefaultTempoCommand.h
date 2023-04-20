
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2023 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef RG_MODIFYDEFAULTTEMPOCOMMAND_H
#define RG_MODIFYDEFAULTTEMPOCOMMAND_H

#include "document/Command.h"
#include <QString>
#include <QCoreApplication>
#include "base/Composition.h" // for tempoT



namespace Rosegarden
{


class ModifyDefaultTempoCommand : public NamedCommand
{
    Q_DECLARE_TR_FUNCTIONS(Rosegarden::ModifyDefaultTempoCommand)

public:
    ModifyDefaultTempoCommand(Composition *composition,
                              tempoT tempo):
        NamedCommand(getGlobalName()),
        m_composition(composition),
        m_tempo(tempo) {}

    ~ModifyDefaultTempoCommand() override {}

    static QString getGlobalName() { return tr("Modify &Default Tempo..."); }

    void execute() override;
    void unexecute() override;

private:
    Composition *m_composition;
    tempoT       m_tempo;
    tempoT       m_oldTempo;
};



}

#endif
