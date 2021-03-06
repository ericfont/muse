//=========================================================
//  MusE
//  Linux Music Editor
//    $Id: ticksynth.cpp,v 1.8.2.7 2009/12/20 05:00:35 terminator356 Exp $
//  (C) Copyright 2004 Werner Schweer (ws@seh.de)
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

#include "audio.h"
#include "ticksynth.h"
#include "default_click.h"
#include "midi.h"
#include "popupmenu.h"
#include "gconfig.h"
#include "wave.h"

// If sysex support is ever added, make sure this number is unique among all the
//  MESS synths (including ticksynth) and DSSI, VST, LV2 and other host synths.
// 127 is reserved for special MusE system messages.
//#define METRONOME_UNIQUE_ID      7

//#define METRONOME_DEBUG

namespace MusECore {

MetronomeSynthI* metronome = 0;

class MetronomeSynth;
static MetronomeSynth* metronomeSynth = 0;

//---------------------------------------------------------
//   MetronomeSynth
//---------------------------------------------------------

class MetronomeSynth : public Synth {
   public:
      MetronomeSynth(const QFileInfo& fi) : Synth(fi, QString("Metronome"), QString("Metronome"), QString(), QString()) {}
      virtual ~MetronomeSynth() {}
      virtual Type synthType() const { return METRO_SYNTH; }
      virtual void incInstances(int) {}
      virtual void* instantiate();
      
      virtual SynthIF* createSIF(SynthI*);
      };

//---------------------------------------------------------
//   instantiate
//---------------------------------------------------------

void* MetronomeSynth::instantiate()
      {
      return 0;
      }

//---------------------------------------------------------
//   MetronomeSynthIF
//---------------------------------------------------------

class MetronomeSynthIF : public SynthIF
      {
      const float* data;
      int pos;
      int len;
      float volume;
      void process(float** buffer, int offset, int n);
      void initSamples();

      float *measSamples;
      int    measLen;
      float *beatSamples;
      int    beatLen;
      float *accent1Samples;
      int    accent1Len;
      float *accent2Samples;
      int    accent2Len;

   public:
      MetronomeSynthIF(SynthI* s) : SynthIF(s) {
            data = 0;
            beatLen = 0;
            measLen = 0;
            accent1Len = 0;
            accent2Len = 0;
            initSamples();
            }
      virtual bool initGui()     { return true; }
      virtual void guiHeartBeat()  {  }
      virtual bool guiVisible() const { return false; }
      virtual void showGui(bool) {}
      virtual bool hasGui() const { return false; }
      virtual bool nativeGuiVisible() const { return false; }
      virtual void showNativeGui(bool) { }
      virtual bool hasNativeGui() const { return false; }
      
      virtual void getGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setGeometry(int, int, int, int) {}
      virtual void getNativeGeometry(int*x, int*y, int*w, int*h) const { *x=0;*y=0;*w=0;*h=0; }
      virtual void setNativeGeometry(int, int, int, int) {}
      virtual void preProcessAlways() { }
      virtual iMPEvent getData(MidiPort*, MPEventList*, iMPEvent, unsigned pos, int ports, unsigned n, float** buffer);
      virtual bool putEvent(const MidiPlayEvent& ev);
      virtual MidiPlayEvent receiveEvent() { return MidiPlayEvent(); }
      virtual int eventsPending() const { return 0; }
      
      virtual int channels() const { return 1; }
      virtual int totalOutChannels() const { return 1; }
      virtual int totalInChannels() const { return 0; }
      virtual void deactivate3() {}
      virtual QString getPatchName(int, int, bool) const { return ""; }
      virtual void populatePatchPopup(MusEGui::PopupMenu*, int, bool) {}
      virtual void write(int, Xml&) const {}
      virtual double getParameter(unsigned long) const  { return 0.0; }
      virtual void setParameter(unsigned long, double) {}
      virtual int getControllerInfo(int, QString*, int*, int*, int*, int*) { return 0; }

      //-------------------------
      // Methods for PluginIBase:
      //-------------------------

      virtual bool addScheduledControlEvent(unsigned long /*i*/, float /*val*/, unsigned /*frame*/) { return true; }    // returns true if event cannot be delivered
      };

//---------------------------------------------------------
//   getData
//---------------------------------------------------------

iMPEvent MetronomeSynthIF::getData(MidiPort*, MPEventList* el, iMPEvent i, unsigned pos, int/*ports*/, unsigned n, float** buffer)
      {
      // Added by Tim. p3.3.18
      #ifdef METRONOME_DEBUG
      printf("MusE: MetronomeSynthIF::getData\n");
      #endif

      if (((MidiPlayEvent&)*i).dataA() == MusECore::reloadClickSounds) {
          initMetronome();
      }

      //set type to unsigned , due to compiler warning: comparison signed/unsigned
      unsigned int curPos      = pos;             //prevent compiler warning: comparison signed/unsigned
      unsigned int endPos      = pos + n;         //prevent compiler warning: comparison signed/unsigned
      unsigned int off         = pos;             //prevent compiler warning: comparison signed/unsigned
      int frameOffset = MusEGlobal::audio->getFrameOffset();

      for (; i != el->end(); ++i) {
            unsigned int frame = i->time() - frameOffset; //prevent compiler warning: comparison signed /unsigned
            if (frame >= endPos)
                  break;
            if (frame > curPos) {
                  if (frame < pos)
                        printf("should not happen: missed event %d\n", pos -frame);
                  else
                        process(buffer, curPos-pos, frame - curPos);
                  curPos = frame;
                  }
            putEvent(*i);
            }
      if (endPos - curPos)
            process(buffer, curPos - off, endPos - curPos);
      return el->end();
      }

//---------------------------------------------------------
//   initSamples
//---------------------------------------------------------

void MetronomeSynthIF::initSamples()
{
    if (beatLen)
      delete beatSamples;
    if (measLen)
      delete measSamples;
    if (accent1Len)
      delete accent1Samples;
    if (accent2Len)
      delete accent2Samples;
    beatLen = 0;
    measLen = 0;
    accent1Len = 0;
    accent2Len = 0;

    SndFile beat(MusEGlobal::museGlobalShare + "/metronome/" + MusEGlobal::config.beatSample);
    if (!beat.openRead(false)) {
      beatLen = beat.samples();
      beatSamples = new float[beatLen];
      beat.read(1, &beatSamples, beatLen);
    }

    SndFile meas(MusEGlobal::museGlobalShare  + "/metronome/" + MusEGlobal::config.measSample);
    if (!meas.openRead(false)) {
      measLen = meas.samples();
      measSamples = new float[measLen];
      meas.read(1, &measSamples, measLen);
    }

    SndFile accent1(MusEGlobal::museGlobalShare +  "/metronome/" + MusEGlobal::config.accent1Sample);
    if (!accent1.openRead(false)) {
      accent1Len = accent1.samples();
      accent1Samples = new float[accent1Len];
      accent1.read(1, &accent1Samples, accent1Len);
    }

    SndFile accent2(MusEGlobal::museGlobalShare +  "/metronome/" + MusEGlobal::config.accent2Sample);
    if (!accent2.openRead(false)) {
      accent2Len = accent2.samples();
      accent2Samples = new float[accent2Len];
      accent2.read(1, &accent2Samples, accent2Len);
    }

}



//---------------------------------------------------------
//   putEvent
//---------------------------------------------------------

bool MetronomeSynthIF::putEvent(const MidiPlayEvent& ev)
{
    if(ev.type() != MusECore::ME_NOTEON)
      return false;
    if (ev.dataA() == MusECore::measureSound) {
        if (MusEGlobal::clickSamples == MusEGlobal::origSamples) {
            data = defaultClickEmphasis;
            len  = defaultClickEmphasisLength;
        }
        else {
              data = measSamples;
              len  = measLen;
        }
        volume = MusEGlobal::measClickVolume;
    }
    else if (ev.dataA() == MusECore::beatSound) {
        if (MusEGlobal::clickSamples == MusEGlobal::origSamples) {
            data = defaultClick;
            len  = defaultClickLength;
        } else {
            data = beatSamples;
            len  = beatLen;
        }
        volume = MusEGlobal::beatClickVolume;
    }
    else if (ev.dataA() == MusECore::accent1Sound) {
             data = accent1Samples;
             len  = accent1Len;
             volume = MusEGlobal::accent1ClickVolume;
             if (MusEGlobal::clickSamples == MusEGlobal::origSamples) {
                 volume=0.0;
             }
    }
    else if (ev.dataA() == MusECore::accent2Sound) {
             data = accent2Samples;
             len  = accent2Len;
             volume = MusEGlobal::accent2ClickVolume;
             if (MusEGlobal::clickSamples == MusEGlobal::origSamples) {
                 volume=0.0;
             }
    }


    pos = 0;
    return false;
}

//---------------------------------------------------------
//   createSIF
//---------------------------------------------------------

SynthIF* MetronomeSynth::createSIF(SynthI* s)
      {
      return new MetronomeSynthIF(s);
      }

//---------------------------------------------------------
//   process
//    synthesize n samples into buffer+offset
//---------------------------------------------------------

void MetronomeSynthIF::process(float** buffer, int offset, int n)
      {
      // Added by Tim. p3.3.18
      #ifdef METRONOME_DEBUG
      printf("MusE: MetronomeSynthIF::process data:%p offset:%d n:%d\n", data, offset, n);
      #endif

      if (data == 0)
        return;

      const float* s = data + pos;
      float* d       = *buffer + offset;
      int l          = std::min(n, len);

      for (int i = 0; i < l; ++i)
            *d++ += *s++ * MusEGlobal::audioClickVolume * volume;
      pos += l;
      len -= l;
      if (len <= 0)
            data = 0;
      }

//---------------------------------------------------------
//   initMetronome
//---------------------------------------------------------

void initMetronome()
      {
      QFileInfo fi;
      metronomeSynth = new MetronomeSynth(fi);
      metronome = new MetronomeSynthI();
      
      QString name("metronome");
      metronome->initInstance(metronomeSynth, name);
      }

//---------------------------------------------------------
//   exitMetronome
//---------------------------------------------------------

void exitMetronome()
{
      if(metronome)
        delete metronome;
      metronome = 0;  
      
      if(metronomeSynth)
        delete metronomeSynth;
      metronomeSynth = 0;
}

} // namespace MusECore
