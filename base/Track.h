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

// Representation of a Track
//
//


#ifndef _TRACK_H_
#define _TRACK_H_

#include <string>

#include "XmlExportable.h"
#include "Instrument.h"

namespace Rosegarden
{
class Composition;

typedef unsigned int TrackId;

/**
 * A Track represents a line on the SegmentCanvas on the
 * Rosegarden GUI.  A Track is owned by a Composition and
 * has reference to an Instrument from which the playback
 * characteristics of the Track can be derived.  A Track
 * has no type itself - the type comes only from the
 * Instrument relationship.
 *
 */
class Track : public XmlExportable
{
    friend class Composition;

public:
    Track();
    Track(TrackId id,
          InstrumentId instrument = 0,
          int position =0 ,
          const std::string &label = "",
          bool muted = false);

    void setId(TrackId id) { m_id = id; }

private:
    Track(const Track &);
    Track operator=(const Track &);

public:

    ~Track();

    TrackId getId() const { return m_id; }

    void setMuted(bool muted);
    bool isMuted() const { return m_muted; }

    void setPosition(int position) { m_position = position; }
    int getPosition() const { return m_position; }

    void setLabel(const std::string &label);
    std::string getLabel() const { return m_label; }

    void setInstrument(InstrumentId instrument);
    InstrumentId getInstrument() const { return m_instrument; }

    // Implementation of virtual
    //
    virtual std::string toXmlString();

    Composition* getOwningComposition() { return m_owningComposition; }

protected: // For Composition use only
    void setOwningComposition(Composition* comp) { m_owningComposition = comp; }

private:
    //--------------- Data members ---------------------------------

    TrackId        m_id;
    bool           m_muted;
    std::string    m_label;
    int            m_position;
    InstrumentId   m_instrument;

    Composition*   m_owningComposition;
};

}

#endif // _TRACK_H_
 
