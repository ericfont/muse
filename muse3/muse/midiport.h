//=========================================================
//  MusE
//  Linux Music Editor
//  $Id: midiport.h,v 1.9.2.6 2009/11/17 22:08:22 terminator356 Exp $
//
//  (C) Copyright 1999-2004 Werner Schweer (ws@seh.de)
//  (C) Copyright 2012, 2017 Tim E. Real (terminator356 on users dot sourceforge dot net)
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; version 2 of
//  the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
//=========================================================

#ifndef __MIDIPORT_H__
#define __MIDIPORT_H__

#include "globaldefs.h"
#include "sync.h"
#include "route.h"
#include "mpevent.h"
#include "lock_free_buffer.h"

class QMenu;
class QWidget;

namespace MusECore {

class MidiDevice;
class Part;
class MidiController;
class MidiControllerList;
class MidiCtrlValListList;
class MidiCtrlValList;
class MidiInstrument;

//---------------------------------------------------------
//   MidiPort
//---------------------------------------------------------

class MidiPort {
      MidiCtrlValListList* _controller;
      MidiDevice* _device;
      QString _state;               // result of device open
      MidiInstrument* _instrument;
      AutomationType _automationType[MIDI_CHANNELS];
      // Holds sync settings and detection monitors.
      MidiSyncInfo _syncInfo;
      // Just a flag to say the port was found in the song file, even if it has no device right now.
      bool _foundInSongFile;
      // When creating a new midi track, add these global default channel routes to/from this port. Ignored if 0.
      int _defaultInChannels;    // These are bit-wise channel masks.
      int _defaultOutChannels;   //
      // Whether Init sysexes and default controller values have been sent. To be reset whenever
      //  something about the port changes like device, Jack routes, or instrument.
      bool _initializationsSent; 

      // Fifo for midi events sent from gui to audio (ex. updating hardware knobs/sliders):
      LockFreeBuffer<MidiPlayEvent> *_gui2AudioFifo;

      RouteList _inRoutes, _outRoutes;
      
      void clearDevice();

      // Prepares an event for putting into the gui2audio fifo.
      // To be called from gui thread only. Returns true if the event was staged.
      bool stageEvent(MidiPlayEvent& dst, const MidiPlayEvent& src);
      // To be called from audio thread only. Returns true if event cannot be delivered.
      bool handleGui2AudioEvent(const MidiPlayEvent&);

   public:
      MidiPort();
      ~MidiPort();

      //
      // manipulate active midi controller
      //
      MidiCtrlValListList* controller() { return _controller; }
      // Determine controller value at tick on channel, using values stored by ANY part.
      int getCtrl(int ch, int tick, int ctrl) const;
      // Determine controller value at tick on channel, using values stored by the SPECIFIC part.
      int getCtrl(int ch, int tick, int ctrl, Part* part) const;
      // Determine controller value at tick on channel, using values stored by ANY part,
      //  ignoring values that are OUTSIDE of their parts, or muted or off parts or tracks.
      int getVisibleCtrl(int ch, int tick, int ctrl, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const;
      // Determine controller value at tick on channel, using values stored by the SPECIFIC part,
      //  ignoring values that are OUTSIDE of the part, or muted or off part or track.
      int getVisibleCtrl(int ch, int tick, int ctrl, Part* part, bool inclMutedParts, bool inclMutedTracks, bool inclOffTracks) const;
      bool setControllerVal(int ch, int tick, int ctrl, int val, Part* part);
      // Can be CTRL_VAL_UNKNOWN until a valid state is set
      int lastValidHWCtrlState(int ch, int ctrl) const;
      int hwCtrlState(int ch, int ctrl) const;
      bool setHwCtrlState(int ch, int ctrl, int val);
      bool setHwCtrlStates(int ch, int ctrl, int val, int lastval);
      void deleteController(int ch, int tick, int ctrl, Part* part);
      void addDefaultControllers();
      
      bool guiVisible() const;
      void showGui(bool); 
      bool hasGui() const;
      bool nativeGuiVisible() const;
      void showNativeGui(bool);
      bool hasNativeGui() const;

      int portno() const;
      bool foundInSongFile() const              { return _foundInSongFile; }
      void setFoundInSongFile(bool b)           { _foundInSongFile = b; }

      MidiDevice* device() const                { return _device; }
      const QString& state() const              { return _state; }
      void setState(const QString& s)           { _state = s; }
      void setMidiDevice(MidiDevice* dev);
      const QString& portname() const;
      MidiInstrument* instrument() const   { return _instrument; }
      void setInstrument(MidiInstrument* i) { _instrument = i; }
      void changeInstrument(MidiInstrument* i);
      MidiController* midiController(int num, bool createIfNotFound = true) const;
      MidiCtrlValList* addManagedController(int channel, int ctrl);
      void tryCtrlInitVal(int chan, int ctl, int val);
      int limitValToInstrCtlRange(int ctl, int val);
      int limitValToInstrCtlRange(MidiController* mc, int val);
      MidiController* drumController(int ctl);
      // Update drum maps when patch is known.
      // If audio is running (and not idle) this should only be called by the rt audio thread.
      // Returns true if maps were changed.
      bool updateDrumMaps(int chan, int patch);
      // Update drum maps when patch is not known.
      // If audio is running (and not idle) this should only be called by the rt audio thread.
      // Returns true if maps were changed.
      bool updateDrumMaps();

      int defaultInChannels() const { return _defaultInChannels; }
      int defaultOutChannels() const { return _defaultOutChannels; }
      void setDefaultInChannels(int c) { _defaultInChannels = c; }
      void setDefaultOutChannels(int c) { _defaultOutChannels = c; }
      RouteList* inRoutes()    { return &_inRoutes; }
      RouteList* outRoutes()   { return &_outRoutes; }
      bool noInRoute() const   { return _inRoutes.empty();  }
      bool noOutRoute() const  { return _outRoutes.empty(); }
      void writeRouting(int, Xml&) const;
      
      // send events to midi device and keep track of
      // device state:
      void sendGmOn();
      void sendGsOn();
      void sendXgOn();
      void sendGmInitValues();
      void sendGsInitValues();
      void sendXgInitValues();
      void sendStart();
      void sendStop();
      void sendContinue();
      void sendSongpos(int);
      void sendClock();
      void sendSysex(const unsigned char* p, int n);
      void sendMMCLocate(unsigned char ht, unsigned char m,
                         unsigned char s, unsigned char f, unsigned char sf, int devid = -1);
      void sendMMCStop(int devid = -1);
      void sendMMCDeferredPlay(int devid = -1);

      // Send Instrument Init sequences and controller defaults etc.
      bool sendPendingInitializations(bool force = true);  // Per port
      // Send initial controller values. Called by above method, and elsewhere.
      bool sendInitialControllers(unsigned start_time = 0);
      bool initSent() const { return _initializationsSent; }  
      void clearInitSent() { _initializationsSent = false; }  
      
      // Put an event into the gui2audio fifo for playback. Calls stageEvent().
      // Called from gui thread only. Returns true if event cannot be delivered.
      bool putHwCtrlEvent(const MidiPlayEvent&);
      // Put an event into both the device and the gui2audio fifo for playback. Calls stageEvent().
      // Called from gui thread only. Returns true if event cannot be delivered.
      bool putEvent(const MidiPlayEvent&);
      // Process the gui2AudioFifo. Called from audio thread only.
      bool processGui2AudioEvents();


      bool sendHwCtrlState(const MidiPlayEvent&, bool forceSend = false );
      bool sendEvent(const MidiPlayEvent&, bool forceSend = false );
      AutomationType automationType(int channel) { return _automationType[channel]; }
      void setAutomationType(int channel, AutomationType t) {
            _automationType[channel] = t;
            }
      MidiSyncInfo& syncInfo() { return _syncInfo; }
      };

extern void initMidiPorts();

// p4.0.17 Turn off if and when multiple output routes supported.
#if 1
extern void setPortExclusiveDefOutChan(int /*port*/, int /*chan*/);
#endif

extern QMenu* midiPortsPopup(QWidget* parent = 0, int checkPort = -1, bool includeDefaultEntry = false);
extern MidiControllerList defaultManagedMidiController;

} // namespace MusECore

namespace MusEGlobal {
extern MusECore::MidiPort midiPorts[MIDI_PORTS];
}

#endif

