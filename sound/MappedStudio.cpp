// -*- c-indentation-style:"stroustrup" c-basic-offset: 4 -*-

/*
    Rosegarden-4
    A sequencer and musical notation editor.

    This program is Copyright 2000-2004
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

#include <iostream>

#include "MappedStudio.h"
#include "Sequencer.h"
#include "PluginFactory.h"

#include <pthread.h> // for mutex

#define DEBUG_MAPPEDSTUDIO 1

static pthread_mutex_t _mappedObjectContainerLock;


// These four functions are stolen and adapted from Qt3 qvaluevector.h
//
// ** Copyright (C) 1992-2000 Trolltech AS.  All rights reserved.
//
QDataStream& operator>>(QDataStream& s, Rosegarden::MappedObjectPropertyList& v)
{
    v.clear();
    Q_UINT32 c;
    s >> c;
    v.resize(c);
    for(Q_UINT32 i = 0; i < c; ++i)
    {
        Rosegarden::MappedObjectProperty t;
        s >> t;
        v[i] = t;
    }
    return s;
}

QDataStream& operator<<(QDataStream& s, const Rosegarden::MappedObjectPropertyList& v)
{
    s << (Q_UINT32)v.size();
    Rosegarden::MappedObjectPropertyList::const_iterator it = v.begin();
    for( ; it != v.end(); ++it )
        s << *it;
    return s;
}

QDataStream& operator>>(QDataStream& s, Rosegarden::MappedObjectValueList& v)
{
    v.clear();
    Q_UINT32 c;
    s >> c;
    v.resize(c);
    for(Q_UINT32 i = 0; i < c; ++i)
    {
        Rosegarden::MappedObjectValue t;
        s >> t;
        v[i] = t;
    }
    return s;
}

QDataStream& operator<<(QDataStream& s, const Rosegarden::MappedObjectValueList& v)
{
    s << (Q_UINT32)v.size();
    Rosegarden::MappedObjectValueList::const_iterator it = v.begin();
    for( ; it != v.end(); ++it )
        s << *it;
    return s;
}

namespace Rosegarden
{

// Define our object properties - these can be queried and set.
//

// General things
//
const MappedObjectProperty MappedObject::Name = "name";
const MappedObjectProperty MappedObject::Instrument = "instrument";
const MappedObjectProperty MappedObject::Position = "position";

const MappedObjectProperty MappedConnectableObject::ConnectionsIn = "connectionsIn";
const MappedObjectProperty MappedConnectableObject::ConnectionsOut = "connectionsOut";

const MappedObjectProperty MappedAudioFader::Channels = "channels";
const MappedObjectProperty MappedAudioFader::FaderLevel = "faderLevel";
const MappedObjectProperty MappedAudioFader::FaderRecordLevel = "faderRecordLevel";
const MappedObjectProperty MappedAudioFader::Pan = "pan";
const MappedObjectProperty MappedAudioFader::InputChannel = "inputChannel";

const MappedObjectProperty MappedAudioBuss::BussId = "bussId";
const MappedObjectProperty MappedAudioBuss::Level = "level";
const MappedObjectProperty MappedAudioBuss::Pan = "pan";

const MappedObjectProperty MappedAudioInput::InputNumber = "inputNumber";

const MappedObjectProperty MappedPluginSlot::Identifier = "identifier";
const MappedObjectProperty MappedPluginSlot::PluginName = "pluginname";
const MappedObjectProperty MappedPluginSlot::Label = "label";
const MappedObjectProperty MappedPluginSlot::Author = "author";
const MappedObjectProperty MappedPluginSlot::Copyright = "copyright";
const MappedObjectProperty MappedPluginSlot::Category = "category";
const MappedObjectProperty MappedPluginSlot::PortCount = "portcount";
const MappedObjectProperty MappedPluginSlot::Ports = "ports";
const MappedObjectProperty MappedPluginSlot::Instrument = "instrument";
const MappedObjectProperty MappedPluginSlot::Position = "position";
const MappedObjectProperty MappedPluginSlot::Bypassed = "bypassed";

const MappedObjectProperty MappedPluginPort::PortNumber = "portnumber";
const MappedObjectProperty MappedPluginPort::Name = "name";
const MappedObjectProperty MappedPluginPort::Minimum = "minimum";
const MappedObjectProperty MappedPluginPort::Maximum = "maximum";
const MappedObjectProperty MappedPluginPort::Default = "default";
const MappedObjectProperty MappedPluginPort::DisplayHint = "displayhint";
const MappedObjectProperty MappedPluginPort::Value = "value";

// --------- MappedObject ---------
//

void
MappedObject::addChild(MappedObject *object)
{
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
        if ((*it) == object)
            return;

    m_children.push_back(object);
}

void
MappedObject::removeChild(MappedObject *object)
{
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
    {
        if ((*it) == object)
        {
            m_children.erase(it);
            return;
        }
    }
}

// Return all child ids
//
MappedObjectPropertyList
MappedObject::getChildren()
{
    MappedObjectPropertyList list;
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
        list.push_back(QString("%1").arg((*it)->getId()));

    return list;
}


// Return all child ids of a certain type
//
MappedObjectPropertyList
MappedObject::getChildren(MappedObjectType type)
{
    MappedObjectPropertyList list;
    std::vector<MappedObject*>::iterator it = m_children.begin();
    for (; it != m_children.end(); it++)
    {
        if ((*it)->getType() == type)
            list.push_back(QString("%1").arg((*it)->getId()));
    }

    return list;
}

void
MappedObject::destroyChildren()
{
    // remove references from the studio as well as from the object
    MappedObject *studioObject = getParent();
    while (!dynamic_cast<MappedStudio*>(studioObject))
        studioObject = studioObject->getParent();

    // see note in destroy() below
    
    std::vector<MappedObject *> children = m_children;
    m_children.clear();

    std::vector<MappedObject *>::iterator it = children.begin();
    for (; it != children.end(); it++)
        (*it)->destroy(); // remove from studio and destroy
}

// Destroy this object and remove it from the studio and 
// do the same for all its children.
//
void
MappedObject::destroy()
{
    MappedObject *studioObject = getParent();
    while (!dynamic_cast<MappedStudio*>(studioObject))
        studioObject = studioObject->getParent();

    MappedStudio *studio = dynamic_cast<MappedStudio*>(studioObject);

    // The destroy method on each child calls studio->clearObject,
    // which calls back on the parent (in this case us) to remove the
    // child.  (That's necessary for the case of destroying a plugin,
    // where we need to remove it from its plugin manager -- etc.)  So
    // we don't want to be iterating over m_children here, as it will
    // change from under us.

    std::vector<MappedObject *> children = m_children;
    m_children.clear();
    
    std::vector<MappedObject *>::iterator it = children.begin();
    for (; it != children.end(); it++) {
        (*it)->destroy();
    }

    (void)studio->clearObject(m_id);
    delete this;
}


// ------- MappedStudio -------
//

MappedStudio::MappedStudio() :
    MappedObject(0,
		 "MappedStudio",
		 Studio,
		 0),
    m_runningObjectId(1)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#ifdef PTHREAD_MUTEX_RECURSIVE
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
#else
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
#endif
    pthread_mutex_init(&_mappedObjectContainerLock, &attr);
}

MappedStudio::~MappedStudio()
{
#ifdef DEBUG_MAPPEDSTUDIO
    std::cout << "MappedStudio::~MappedStudio" << std::endl;
#endif
    clear();
}


// Object factory
// 
MappedObject*
MappedStudio::createObject(MappedObjectType type)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObject *mO = 0;

    // Ensure we've got an empty slot
    //
    while (getObjectById(m_runningObjectId))
        m_runningObjectId++;

    mO = createObject(type, m_runningObjectId);

    // If we've got a new object increase the running id
    //
    if (mO) m_runningObjectId++;

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return mO;
}

MappedObject*
MappedStudio::createObject(MappedObjectType type,
                           MappedObjectId id)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    // fail if the object already exists and it's not zero
    if (id != 0 && getObjectById(id)) {
	pthread_mutex_unlock(&_mappedObjectContainerLock);
	return 0;
    }

    MappedObject *mO = 0;

    if (type == MappedObject::AudioFader)
    {
        mO = new MappedAudioFader(this,
                                  id,
                                  2); // channels

        // push to the studio's child stack
        addChild(mO);
    }
    else if (type == MappedObject::AudioBuss)
    {
        mO = new MappedAudioBuss(this,
				 id);

        // push to the studio's child stack
        addChild(mO);
    }
    else if (type == MappedObject::AudioInput)
    {
        mO = new MappedAudioInput(this,
				 id);

        // push to the studio's child stack
        addChild(mO);
    }
    else if (type == MappedObject::PluginSlot)
    {
	mO = new MappedPluginSlot(this,
				  id);
	addChild(mO);
    }
    else if (type == MappedObject::PluginPort)
    {
	mO = new MappedPluginPort(this,
				  id);
	// reset the port's parent after creation outside this method
    }

    // Insert
    if (mO)
    {
//	std::cerr << "Adding object " << id << " to category " << type << std::endl;
        m_objects[type][id] = mO;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return mO;
}

MappedObject*
MappedStudio::getObjectOfType(MappedObjectType type)
{
    MappedObject *rv = 0;

    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[type];
    if (!category.empty()) rv = category.begin()->second;

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

std::vector<MappedObject *>
MappedStudio::getObjectsOfType(MappedObjectType type)
{
    std::vector<MappedObject *> rv;

    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[type];

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	rv.push_back(i->second);
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

unsigned int
MappedStudio::getObjectCount(MappedObjectType type)
{
    unsigned int count = 0;

    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[type];
    count = category.size();

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return count;
}


bool
MappedStudio::destroyObject(MappedObjectId id)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObject *obj = getObjectById(id);

    bool rv = false;

    if (obj) {
        obj->destroy();
	rv = true;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

bool
MappedStudio::connectObjects(MappedObjectId mId1, MappedObjectId mId2)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    bool rv = false;

    // objects must exist and be of connectable types
    MappedConnectableObject *obj1 =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId1));
    MappedConnectableObject *obj2 =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId2));

    if (obj1 && obj2) {
	obj1->addConnection(MappedConnectableObject::Out, mId2);
	obj2->addConnection(MappedConnectableObject::In,  mId1);
	rv = true;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

bool
MappedStudio::disconnectObjects(MappedObjectId mId1, MappedObjectId mId2)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    bool rv = false;

    // objects must exist and be of connectable types
    MappedConnectableObject *obj1 =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId1));
    MappedConnectableObject *obj2 =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId2));

    if (obj1 && obj2) {
	obj1->removeConnection(MappedConnectableObject::Out, mId2);
	obj2->removeConnection(MappedConnectableObject::In,  mId1);
	rv = true;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

bool
MappedStudio::disconnectObject(MappedObjectId mId)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    bool rv = false;

    MappedConnectableObject *obj =
	dynamic_cast<MappedConnectableObject *>(getObjectById(mId));
    
    if (obj) {
	while (1) {
	    MappedObjectValueList list = 
		obj->getConnections(MappedConnectableObject::In);
	    if (list.empty()) break;
	    MappedObjectId otherId = MappedObjectId(*list.begin());
	    disconnectObjects(otherId, mId);
	}
	while (1) {
	    MappedObjectValueList list = 
		obj->getConnections(MappedConnectableObject::Out);
	    if (list.empty()) break;
	    MappedObjectId otherId = MappedObjectId(*list.begin());
	    disconnectObjects(mId, otherId);
	}
    }
    
    rv = true;

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}

    

// Clear down the whole studio
//
void
MappedStudio::clear()
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    for (MappedObjectMap::iterator i = m_objects.begin();
	 i != m_objects.end(); ++i) {

	for (MappedObjectCategory::iterator j = i->second.begin();
	     j != i->second.end(); ++j) {

	    delete j->second;
	}
    }

    m_objects.clear();

    // reset running object id
    m_runningObjectId = 1;

    pthread_mutex_unlock(&_mappedObjectContainerLock);
}

bool
MappedStudio::clearObject(MappedObjectId id)
{
    bool rv = false;

    pthread_mutex_lock(&_mappedObjectContainerLock);

    for (MappedObjectMap::iterator i = m_objects.begin();
	 i != m_objects.end(); ++i) {

	MappedObjectCategory::iterator j = i->second.find(id);
	if (j != i->second.end()) {
	    // if the object has a parent other than the studio,
	    // persuade that parent to abandon it
	    MappedObject *parent = j->second->getParent();
	    if (parent && !dynamic_cast<MappedStudio *>(parent)) {
		parent->removeChild(j->second);
	    }

	    i->second.erase(j);
	    rv = true;
	    break;
	}
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}


MappedObjectPropertyList
MappedStudio::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        // something
    }

    return list;
}

bool
MappedStudio::getProperty(const MappedObjectProperty &,
			  MappedObjectValue &)
{
    return false;
}

MappedObject*
MappedStudio::getObjectById(MappedObjectId id)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);
    MappedObject *rv = 0;

    for (MappedObjectMap::iterator i = m_objects.begin();
	 i != m_objects.end(); ++i) {

	MappedObjectCategory::iterator j = i->second.find(id);
	if (j != i->second.end()) {
	    rv = j->second;
	    break;
	}
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

MappedObject*
MappedStudio::getObjectByIdAndType(MappedObjectId id, MappedObjectType type)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);
    MappedObject *rv = 0;

    MappedObjectCategory &category = m_objects[type];
    MappedObjectCategory::iterator i = category.find(id);
    if (i != category.end()) {
	rv = i->second;
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

MappedObject*
MappedStudio::getFirst(MappedObjectType type)
{
    return getObjectOfType(type);
}

MappedObject*
MappedStudio::getNext(MappedObject *object)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[object->getType()];

    bool next = false;
    MappedObject *rv = 0;

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	if (i->second->getId() == object->getId()) next = true;
	else if (next) {
	    rv = i->second;
	    break;
	}
    }
    
    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

void
MappedStudio::setProperty(const MappedObjectProperty &property,
                          MappedObjectValue /*value*/)
{
    if (property == "")
    {
    }

}

MappedAudioFader *
MappedStudio::getAudioFader(Rosegarden::InstrumentId id)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[AudioFader];
    MappedAudioFader *rv = 0;

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	MappedAudioFader *fader = dynamic_cast<MappedAudioFader *>(i->second);
	if (fader && (fader->getInstrument() == id)) {
	    rv = fader;
	    break;
	}
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

MappedAudioBuss *
MappedStudio::getAudioBuss(int bussNumber)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[AudioBuss];
    MappedAudioBuss *rv = 0;

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	MappedAudioBuss *buss = dynamic_cast<MappedAudioBuss *>(i->second);
	if (buss && (buss->getBussId() == bussNumber)) {
	    rv = buss;
	    break;
	}
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}

MappedAudioInput *
MappedStudio::getAudioInput(int inputNumber)
{
    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObjectCategory &category = m_objects[AudioInput];
    MappedAudioInput *rv = 0;

    for (MappedObjectCategory::iterator i = category.begin();
	 i != category.end(); ++i) {
	MappedAudioInput *input = dynamic_cast<MappedAudioInput *>(i->second);
	if (input && (input->getInputNumber() == inputNumber)) {
	    rv = input;
	    break;
	}
    }

    pthread_mutex_unlock(&_mappedObjectContainerLock);
    return rv;
}


// -------------- MappedConnectableObject -----------------
//
//
MappedConnectableObject::MappedConnectableObject(MappedObject *parent,
                                     const std::string &name,
                                     MappedObjectType type,
                                     MappedObjectId id):
    MappedObject(parent,
		 name,
		 type,
		 id)
{
}

MappedConnectableObject::~MappedConnectableObject()
{
}

void
MappedConnectableObject::setConnections(ConnectionDirection dir,
					MappedObjectValueList conns)
{
    if (dir == In)
        m_connectionsIn = conns;
    else
        m_connectionsOut = conns;
}

void
MappedConnectableObject::addConnection(ConnectionDirection dir,
				       MappedObjectId id)
{
    MappedObjectValueList &list =
	(dir == In ? m_connectionsIn : m_connectionsOut);

    for (MappedObjectValueList::iterator i = list.begin(); i != list.end(); ++i) {
	if (*i == id) {
	    return;
	}
    }
    
    list.push_back(MappedObjectValue(id));
}

void
MappedConnectableObject::removeConnection(ConnectionDirection dir,
					  MappedObjectId id)
{
    MappedObjectValueList &list =
	(dir == In ? m_connectionsIn : m_connectionsOut);

    for (MappedObjectValueList::iterator i = list.begin(); i != list.end(); ++i) {
	if (*i == id) {
	    list.erase(i);
	    return;
	}
    }
}

MappedObjectValueList
MappedConnectableObject::getConnections(ConnectionDirection dir)
{
    if (dir == In)
        return m_connectionsIn;
    else
        return m_connectionsOut;
}


// ------------ MappedAudioFader ----------------
//
MappedAudioFader::MappedAudioFader(MappedObject *parent,
                                   MappedObjectId id,
                                   MappedObjectValue channels):
    MappedConnectableObject(parent,
                      "MappedAudioFader",
                      AudioFader,
                      id),
    m_level(0.0), // dB
    m_recordLevel(0.0),
    m_instrumentId(0),
    m_pan(0),
    m_channels(channels),
    m_inputChannel(0)
{
}

MappedAudioFader::~MappedAudioFader()
{
}


MappedObjectPropertyList 
MappedAudioFader::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back(MappedAudioFader::FaderLevel);
        list.push_back(MappedAudioFader::FaderRecordLevel);
        list.push_back(MappedObject::Instrument);
        list.push_back(MappedAudioFader::Pan);
        list.push_back(MappedAudioFader::Channels);
        list.push_back(MappedConnectableObject::ConnectionsIn);
        list.push_back(MappedConnectableObject::ConnectionsOut);
    }
    else if (property == MappedObject::Instrument)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_instrumentId));
    }
    else if (property == MappedAudioFader::FaderLevel)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_level));
    }
    else if (property == MappedAudioFader::FaderRecordLevel)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_recordLevel));
    }
    else if (property == MappedAudioFader::Channels)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_channels));
    }
    else if (property == MappedAudioFader::InputChannel)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_inputChannel));
    }
    else if (property == MappedAudioFader::Pan)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_pan));
    }
    else if (property == MappedConnectableObject::ConnectionsIn)
    {
        Rosegarden::MappedObjectValueList::const_iterator 
            it = m_connectionsIn.begin();

        for( ; it != m_connectionsIn.end(); ++it)
        {
            list.push_back(QString("%1").arg(*it));
        }
    }
    else if (property == MappedConnectableObject::ConnectionsOut)
    {
        Rosegarden::MappedObjectValueList::const_iterator 
            it = m_connectionsOut.begin();

        for( ; it != m_connectionsOut.end(); ++it)
        {
            list.push_back(QString("%1").arg(*it));
        }
    }

    return list;
}

bool
MappedAudioFader::getProperty(const MappedObjectProperty &property,
			      MappedObjectValue &value)
{
    if (property == FaderLevel) {
	value = m_level;
    } else if (property == Instrument) {
	value = m_instrumentId;
    } else if (property == FaderRecordLevel) {
	value = m_recordLevel;
    } else if (property == Channels) {
	value = m_channels;
    } else if (property == InputChannel) {
	value = m_inputChannel;
    } else if (property == Pan) {
	value = m_pan;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioFader::getProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
        return false;
    }
    return true;
}

void
MappedAudioFader::setProperty(const MappedObjectProperty &property,
                              MappedObjectValue value)
{
    if (property == MappedAudioFader::FaderLevel)
    {
        m_level = value;
    }
    else if (property == MappedObject::Instrument)
    {
        m_instrumentId = Rosegarden::InstrumentId(value);
    }
    else if (property ==  MappedAudioFader::FaderRecordLevel)
    {
        m_recordLevel = value;
    }
    else if (property ==  MappedAudioFader::Channels)
    {
        m_channels = value;
    }
    else if (property ==  MappedAudioFader::InputChannel)
    {
        m_inputChannel = value;
    }
    else if (property ==  MappedAudioFader::Pan)
    {
        m_pan = value;
    }
    else if (property == MappedConnectableObject::ConnectionsIn)
    {
        m_connectionsIn.clear();
        m_connectionsIn.push_back(value);
    }
    else if (property == MappedConnectableObject::ConnectionsOut)
    {
        m_connectionsOut.clear();
        m_connectionsOut.push_back(value);
    }
    else
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioFader::setProperty - "
                  << "unsupported property" << std::endl;
#endif
        return;
    }

    /*
    std::cout << "MappedAudioFader::setProperty - "
              << property << " = " << value << std::endl;
              */


}

// ---------------- MappedAudioBuss -------------------
//
//
MappedAudioBuss::MappedAudioBuss(MappedObject *parent,
                                 MappedObjectId id) :
    MappedConnectableObject(parent,
                      "MappedAudioBuss",
                      AudioBuss,
                      id),
    m_bussId(0),
    m_level(0),
    m_pan(0)
{
}

MappedAudioBuss::~MappedAudioBuss()
{
}

MappedObjectPropertyList
MappedAudioBuss::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back(MappedAudioBuss::BussId);
        list.push_back(MappedAudioBuss::Level);
        list.push_back(MappedAudioBuss::Pan);
        list.push_back(MappedConnectableObject::ConnectionsIn);
        list.push_back(MappedConnectableObject::ConnectionsOut);
    }
    else if (property == BussId)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_bussId));
    }
    else if (property == Level)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_level));
    }
    else if (property == MappedConnectableObject::ConnectionsIn)
    {
        Rosegarden::MappedObjectValueList::const_iterator 
            it = m_connectionsIn.begin();

        for( ; it != m_connectionsIn.end(); ++it)
        {
            list.push_back(QString("%1").arg(*it));
        }
    }
    else if (property == MappedConnectableObject::ConnectionsOut)
    {
        Rosegarden::MappedObjectValueList::const_iterator 
            it = m_connectionsOut.begin();

        for( ; it != m_connectionsOut.end(); ++it)
        {
            list.push_back(QString("%1").arg(*it));
        }
    }

    return list;
}

bool
MappedAudioBuss::getProperty(const MappedObjectProperty &property,
			     MappedObjectValue &value)
{
    if (property == BussId) {
	value = m_bussId;
    } else if (property == Level) {
	value = m_level;
    } else if (property == Pan) {
	value = m_pan;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioBuss::getProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
        return false;
    }
    return true;
}
    
void
MappedAudioBuss::setProperty(const MappedObjectProperty &property,
			     MappedObjectValue value)
{
    if (property == MappedAudioBuss::BussId)
    {
        m_bussId = value;
    }
    else if (property == MappedAudioBuss::Level)
    {
        m_level = value;
    }
    else if (property == MappedAudioBuss::Pan)
    {
        m_pan = value;
    }
    else if (property == MappedConnectableObject::ConnectionsIn)
    {
        m_connectionsIn.clear();
        m_connectionsIn.push_back(value);
    }
    else if (property == MappedConnectableObject::ConnectionsOut)
    {
        m_connectionsOut.clear();
        m_connectionsOut.push_back(value);
    }
    else
    {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedAudioBuss::setProperty - "
                  << "unsupported property" << std::endl;
#endif
        return;
    }
}

std::vector<InstrumentId>
MappedAudioBuss::getInstruments()
{
    std::vector<InstrumentId> rv;

    pthread_mutex_lock(&_mappedObjectContainerLock);

    MappedObject *studioObject = getParent();
    while (!dynamic_cast<MappedStudio *>(studioObject))
        studioObject = studioObject->getParent();

    std::vector<MappedObject *> objects =
	static_cast<MappedStudio *>(studioObject)->
	getObjectsOfType(MappedObject::AudioFader);
    
    for (std::vector<MappedObject *>::iterator i = objects.begin();
	 i != objects.end(); ++i) {
	MappedAudioFader *fader = dynamic_cast<MappedAudioFader *>(*i);
	if (fader) {
	    MappedObjectValueList connections = fader->getConnections
		(MappedConnectableObject::Out);
	    if (!connections.empty() && (*connections.begin() == getId())) {
		rv.push_back(fader->getInstrument());
	    }
	}
    }
    
    pthread_mutex_unlock(&_mappedObjectContainerLock);

    return rv;
}
    

// ---------------- MappedAudioInput -------------------
//
//
MappedAudioInput::MappedAudioInput(MappedObject *parent,
				   MappedObjectId id) :
    MappedConnectableObject(parent,
			    "MappedAudioInput",
			    AudioInput,
			    id)
{
}

MappedAudioInput::~MappedAudioInput()
{
}

MappedObjectPropertyList
MappedAudioInput::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "")
    {
        list.push_back(MappedAudioInput::InputNumber);
    }
    else if (property == InputNumber)
    {
        list.push_back(MappedObjectProperty("%1").arg(m_inputNumber));
    }

    return list;
}

bool
MappedAudioInput::getProperty(const MappedObjectProperty &property,
			     MappedObjectValue &value)
{
    if (property == InputNumber) {
	value = m_inputNumber;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
	std::cerr << "MappedAudioInput::getProperty - "
		  << "no properties available" << std::endl;
#endif
    }
    return false;
}

void
MappedAudioInput::setProperty(const MappedObjectProperty &property,
			      MappedObjectValue value)
{
    if (property == InputNumber) {
	m_inputNumber = value;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
	std::cerr << "MappedAudioInput::setProperty - "
		  << "no properties available" << std::endl;
#endif
    }
    return;
}


MappedPluginSlot::MappedPluginSlot(MappedObject *parent, MappedObjectId id) :
    MappedObject(parent, "MappedPluginSlot", PluginSlot, id)
{
}

MappedPluginSlot::~MappedPluginSlot()
{
    if (m_identifier != "") {

	// shut down and remove the plugin instance we have running

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

	if (studio) {
	    Rosegarden::Sequencer *seq = studio->getSequencer();

	    if (seq) {
		seq->removePluginInstance(m_instrument, m_position);
	    }
	}
    }
}

MappedObjectPropertyList
MappedPluginSlot::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "") {
	list.push_back(PortCount);
	list.push_back(Instrument);
	list.push_back(Bypassed);
	list.push_back(PluginName);
	list.push_back(Label);
	list.push_back(Author);
	list.push_back(Copyright);
	list.push_back(Category);
    } else {
	std::cerr << "MappedPluginSlot::getPropertyList: not a list property"
		  << std::endl;
    }

    return list;
}

bool
MappedPluginSlot::getProperty(const MappedObjectProperty &property,
			      MappedObjectValue &value)
{
    if (property == PortCount) {
	value = m_portCount;
    } else if (property == Instrument) {
	value = m_instrument;
    } else if (property == Position) {
	value = m_position;
    } else if (property == Bypassed) {
	value = m_bypassed;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginSlot::getProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
        return false;
    }
    return true;
}

bool
MappedPluginSlot::getProperty(const MappedObjectProperty &property,
			      QString &value)
{
    if (property == Identifier) {
	value = m_identifier;
    } else if (property == PluginName) {
	value = m_name;
    } else if (property == Label) {
	value = m_label;
    } else if (property == Author) {
	value = m_author;
    } else if (property == Copyright) {
	value = m_copyright;
    } else if (property == Category) {
	value = m_category;
    } else {

#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginSlot::getProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
        return false;
    }
    return true;
}

void
MappedPluginSlot::setProperty(const MappedObjectProperty &property,
			      MappedObjectValue value)
{
    if (property == Instrument) {
        m_instrument = Rosegarden::InstrumentId(value);
    } else if (property == PortCount) {
        m_portCount = int(value);
    } else if (property == Position) {
        m_position = int(value);
    } else if (property == Bypassed) {
        m_bypassed = bool(value);

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

	if (studio) {
	    studio->getSequencer()->setPluginInstanceBypass(m_instrument,
							    m_position,
							    m_bypassed);
	}
    }
}

void
MappedPluginSlot::setProperty(const MappedObjectProperty &property,
			      QString value)
{
    if (property == Identifier) {

        if (m_identifier == value) return;

        // shut down and remove the plugin instance we have running

        MappedStudio *studio =
            dynamic_cast<MappedStudio*>(getParent());

	if (studio) {
	    Rosegarden::Sequencer *seq = studio->getSequencer();

	    if (seq) {

		// We don't call seq->removePluginInstance at this
		// point: the sequencer will deal with that when we
		// call setPluginInstance below.  If we removed the
		// instance here, we might cause the library we want
		// for the new plugin instance to be unloaded and then
		// loaded again, which is hardly the most efficient.

		m_identifier = value;

		// populate myself and my ports
		PluginFactory *factory = PluginFactory::instanceFor(m_identifier);
		if (!factory) {
		    std::cerr << "WARNING: MappedPluginSlot::setProperty(identifier): No plugin factory for identifier " << m_identifier << "!" << std::endl;
		    m_identifier = "";
		    return;
		}

		factory->populatePluginSlot(m_identifier, *this);
		
		// now create the new instance
		seq->setPluginInstance(m_instrument,
				       m_identifier,
				       m_position);
	    }
	}

    } else if (property == PluginName) {
	m_name = value;
    } else if (property == Label) {
	m_label = value;
    } else if (property == Author) {
	m_author = value;
    } else if (property == Copyright) {
	m_copyright = value;
    } else if (property == Category) {
	m_category = value;
    } else {

#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginSlot::setProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
    }
}

void
MappedPluginSlot::setPort(unsigned long portNumber, float value)
{
    std::vector<MappedObject*> ports = getChildObjects();
    std::vector<MappedObject*>::iterator it = ports.begin();
    MappedPluginPort *port = 0;

    for (; it != ports.end(); it++) {
        port = dynamic_cast<MappedPluginPort *>(*it);
        if (port && port->getPortNumber() == portNumber) {
            port->setValue(value);
	}
    }
}



MappedPluginPort::MappedPluginPort(MappedObject *parent, MappedObjectId id) :
    MappedObject(parent, "MappedPluginPort", PluginPort, id)
{
}

MappedPluginPort::~MappedPluginPort()
{
}

MappedObjectPropertyList
MappedPluginPort::getPropertyList(const MappedObjectProperty &property)
{
    MappedObjectPropertyList list;

    if (property == "") {
	list.push_back(PortNumber);
	list.push_back(Minimum);
	list.push_back(Maximum);
	list.push_back(Default);
	list.push_back(DisplayHint);
	list.push_back(Value);
	list.push_back(Name);
    } else {
	std::cerr << "MappedPluginSlot::getPropertyList: not a list property"
		  << std::endl;
    }

    return list;
}

bool
MappedPluginPort::getProperty(const MappedObjectProperty &property,
			      MappedObjectValue &value)
{
    if (property == PortNumber) {
	value = m_portNumber;
    } else if (property == Minimum) {
	value = m_minimum;
    } else if (property == Maximum) {
	value = m_maximum;
    } else if (property == Default) {
	value = m_default;
    } else if (property == DisplayHint) {
	value = m_displayHint;
    } else if (property == Value) {
	value = m_value;
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginPort::getProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
        return false;
    }
    return true;
}

bool
MappedPluginPort::getProperty(const MappedObjectProperty &property,
			      QString &value)
{
    if (property == Name) {
	value = m_name;
    } else {

#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginPort::getProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
        return false;
    }
    return true;
}

void
MappedPluginPort::setValue(MappedObjectValue value)
{
    m_value = value;

    MappedPluginSlot *slot =
	dynamic_cast<MappedPluginSlot *>(getParent());

    if (slot) {
    
	MappedStudio *studio =
	    dynamic_cast<MappedStudio *>(slot->getParent());
    
	if (studio) {
	    Rosegarden::Sequencer *seq = studio->getSequencer();
	
	    if (seq) {
		seq->setPluginInstancePortValue(slot->getInstrument(),
						slot->getPosition(),
						m_portNumber, m_value);
	    }
	}
    }
}    

void
MappedPluginPort::setProperty(const MappedObjectProperty &property,
			      MappedObjectValue value)
{
    if (property == PortNumber) {
        m_portNumber = int(value);
    } else if (property == Minimum) {
        m_minimum = value;
    } else if (property == Maximum) {
        m_maximum = value;
    } else if (property == Default) {
        m_default = value;
    } else if (property == DisplayHint) {
        m_displayHint = PluginPort::PortDisplayHint(value);
    } else if (property == Value) {
	setValue(value);
    } else {
#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginPort::setProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
    }
}


void
MappedPluginPort::setProperty(const MappedObjectProperty &property,
			      QString value)
{
    if (property == Name) {
	m_name = value;
    } else {

#ifdef DEBUG_MAPPEDSTUDIO
        std::cerr << "MappedPluginPort::setProperty - "
                  << "unsupported or non-scalar property" << std::endl;
#endif
    }
}


}


