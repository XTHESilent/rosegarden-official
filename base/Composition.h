
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

#ifndef _COMPOSITION_H_
#define _COMPOSITION_H_

#include <set>

#include "Track.h"
#include "MappedEvent.h"

namespace Rosegarden 
{
    
/**
 * A set of tracks.
 * This class owns the event lists it is holding
 * It will delete them on destruction.
 */
class Composition
{
    
public:
    typedef std::set<Track*, Track::TrackCmp> trackcontainer;

    typedef trackcontainer::iterator iterator;
    typedef trackcontainer::const_iterator const_iterator;

    Composition();
    ~Composition();

    trackcontainer& tracks() { return m_tracks; }

    iterator addTrack(Track*);

    void deleteTrack(iterator);
    bool deleteTrack(Track*);

    unsigned int getNbTracks() const { return m_tracks.size(); }

    // returns the nb of time steps of the longest track
    unsigned int getNbTimeSteps() const;
    void         clear();

    unsigned int getNbTicksPerBar() const { return m_nbTicksPerBar; }
    void setNbTicksPerBar(unsigned int n) { m_nbTicksPerBar = n; }

    // Some vector<> API delegation
    iterator       begin()       { return m_tracks.begin(); }
    const_iterator begin() const { return m_tracks.begin(); }
    iterator       end()         { return m_tracks.end(); }
    const_iterator end() const   { return m_tracks.end(); }

    MappedComposition* getMappedComposition(const unsigned int &sliceStart,
                                            const unsigned int &sliceEnd);


//     Track*       operator[](int i)       { return m_tracks[i]; }
//     const Track* operator[](int i) const { return m_tracks[i]; }

    unsigned int getTempo() const { return m_tempo; }
    void setTempo(const int &tempo) { m_tempo = tempo; }

protected:
    trackcontainer m_tracks;

    unsigned int m_nbTicksPerBar;
    unsigned int m_tempo;
};

}


#endif

