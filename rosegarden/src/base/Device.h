// -*- c-basic-offset: 4 -*-

/*
    Rosegarden
    A sequencer and musical notation editor.
    Copyright 2000-2008 the Rosegarden development team.
    See the AUTHORS file for more details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "XmlExportable.h"
#include "Instrument.h"
#include <string>
#include <vector>

// A Device can query underlying hardware/sound APIs to
// generate a list of Instruments.
//

namespace Rosegarden
{

typedef unsigned int DeviceId;

class Instrument;
typedef std::vector<Instrument *> InstrumentList;

class Device : public XmlExportable
{
public:
    typedef enum 
    {
        Midi,
        Audio,
        SoftSynth
    } DeviceType;

    // special device ids
    static const DeviceId NO_DEVICE;
    static const DeviceId ALL_DEVICES;
    static const DeviceId CONTROL_DEVICE;

    Device(DeviceId id, const std::string &name, DeviceType type):
        m_name(name), m_type(type), m_id(id) { }

    virtual ~Device() 
    {
        InstrumentList::iterator it = m_instruments.begin();
        for (; it != m_instruments.end(); it++)
            delete (*it);
        m_instruments.erase(m_instruments.begin(), m_instruments.end());
    }

    void setType(DeviceType type) { m_type = type; }
    DeviceType getType() const { return m_type; }

    void setName(const std::string &name) { m_name = name; }
    std::string getName() const { return m_name; }

    void setId(DeviceId id) { m_id = id; }
    DeviceId getId() const { return m_id; }

    // Accessing instrument lists - Devices should only
    // show the world what they want it to see
    //
    virtual void addInstrument(Instrument*) = 0;

    // Two functions - one to return all Instruments on a
    // Device - one to return all Instruments that a user
    // is allowed to select (Presentation Instruments).
    //
    virtual InstrumentList getAllInstruments() const = 0;
    virtual InstrumentList getPresentationInstruments() const = 0;

    std::string getConnection() const { return m_connection; }
    void setConnection(std::string connection) { m_connection = connection; }

protected:
    InstrumentList     m_instruments;
    std::string        m_name;
    DeviceType         m_type;
    DeviceId           m_id;
    std::string        m_connection;
};

}

#endif // _DEVICE_H_
