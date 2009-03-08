
/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-2009 the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef _RG_GUIPALETTE_H_
#define _RG_GUIPALETTE_H_

#include "base/Colour.h"
#include <map>
#include <QColor>




namespace Rosegarden
{



/**
 * Definitions of colours to be used throughout the Rosegarden GUI.
 * 
 * They're in this file not so much to allow them to be easily
 * changed, as to ensure a certain amount of consistency between
 * colours for different functions that might end up being seen
 * at the same time.
 */

class GUIPalette 
{
public:
    static QColor getColour(const char* const colourName);

    static Colour convertColour(const QColor &input);
    static QColor convertColour(const Colour &input);

    static const char* const ActiveRecordTrack;

    static const char* const SegmentCanvas;
    static const char* const SegmentBorder;

    static const char* const RecordingInternalSegmentBlock;
    static const char* const RecordingAudioSegmentBlock;
    static const char* const RecordingSegmentBorder;

    static const char* const RepeatSegmentBorder;

    static const char* const SegmentAudioPreview;
    static const char* const SegmentInternalPreview;
    static const char* const SegmentLabel;
    static const char* const SegmentSplitLine;

    static const char* const MatrixElementBorder;
    static const char* const MatrixElementBlock;
    static const char* const MatrixOverlapBlock;

    static const char* const LoopRulerBackground;
    static const char* const LoopRulerForeground;
    static const char* const LoopHighlight;

    static const char* const RulerForeground;
    static const char* const RulerBackground;

    static const char* const TempoBase;

    static const char* const TextRulerBackground;
    static const char* const TextRulerForeground;

    static const char* const ChordNameRulerBackground;
    static const char* const ChordNameRulerForeground;

    static const char* const RawNoteRulerBackground;
    static const char* const RawNoteRulerForeground;

    static const char* const LevelMeterGreen;
    static const char* const LevelMeterOrange;
    static const char* const LevelMeterRed;

    static const char* const LevelMeterSolidGreen;
    static const char* const LevelMeterSolidOrange;
    static const char* const LevelMeterSolidRed;

    static const char* const BarLine;
    static const char* const BarLineIncorrect;
    static const char* const BeatLine;
    static const char* const SubBeatLine;
    static const char* const StaffConnectingLine;
    static const char* const StaffConnectingTerminatingLine;

    static const char* const Pointer;
    static const char* const PointerRuler;

    static const char* const InsertCursor;
    static const char* const InsertCursorRuler;

    static const char* const TrackDivider;
    static const char* const MovementGuide;
    static const char* const SelectionRectangle;
    static const char* const SelectedElement;

    static const int SelectedElementHue;
    static const int SelectedElementMinValue;
    static const int HighlightedElementHue;
    static const int HighlightedElementMinValue;
    static const int QuantizedNoteHue;
    static const int QuantizedNoteMinValue;
    static const int TriggerNoteHue;
    static const int TriggerNoteMinValue;
    static const int OutRangeNoteHue;
    static const int OutRangeNoteMinValue;

    static const int CollisionHaloHue;
    static const int CollisionHaloSaturation;

    static const char* const TextAnnotationBackground;

    static const char* const TextLilyPondDirectiveBackground;

    static const char* const AudioCountdownBackground;
    static const char* const AudioCountdownForeground;

    static const char* const RotaryFloatBackground;
    static const char* const RotaryFloatForeground;

    static const char* const RotaryPastelBlue;
    static const char* const RotaryPastelRed;
    static const char* const RotaryPastelGreen;
    static const char* const RotaryPastelOrange;
    static const char* const RotaryPastelYellow;

    static const char* const MatrixKeyboardFocus;

    static const char* const RotaryPlugin;

    static const char* const RotaryMeter;

    static const char* const MarkerBackground;

    static const char* const QuickMarker;

    static const char* const MuteTrackLED;
    static const char* const RecordMIDITrackLED;
    static const char* const RecordAudioTrackLED;

    static const char* const PlaybackFaderOutline;
    static const char* const RecordFaderOutline;

    static const int AudioDefaultIndex;
    
protected:
    GUIPalette();
    QColor getDefaultColour(const char* const colourName);

    //--------------- Data members ---------------------------------
    static GUIPalette* getInstance();
    static GUIPalette* m_instance;

    typedef std::map<const char* const, QColor> colourmap;
    
    colourmap m_defaultsMap;
};
 


}

#endif
