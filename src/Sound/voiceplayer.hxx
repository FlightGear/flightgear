// voiceplayer.hxx -- voice/sound sample player
//
// Written by Jean-Yves Lefort, started September 2005.
//
// Copyright (C) 2005, 2006  Jean-Yves Lefort - jylefort@FreeBSD.org
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.


#ifndef __SOUND_VOICEPLAYER_HXX
#define __SOUND_VOICEPLAYER_HXX

#include <assert.h>

#include <vector>
#include <deque>
#include <map>

#include <simgear/props/props.hxx>
#include <simgear/props/tiedpropertylist.hxx>
#include <simgear/sound/sample_openal.hxx>
using std::vector;
using std::deque;
using std::map;

class SGSampleGroup;

#include <Main/globals.hxx>

#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable: 4355 )
#endif

/////////////////////////////////////////////////////////////////////////////
// FGVoicePlayer /////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class FGVoicePlayer
{
public:

    /////////////////////////////////////////////////////////////////////////////
    // MK::RawValueMethodsData /////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////

    template <class C, class VT, class DT>
    class RawValueMethodsData : public SGRawValue<VT>
    {
    public:
      typedef VT (C::*getter_t) (DT) const;
      typedef void (C::*setter_t) (DT, VT);

      RawValueMethodsData (C &obj, DT data, getter_t getter = 0, setter_t setter = 0)
        : _obj(obj), _data(data), _getter(getter), _setter(setter) {}

      virtual VT getValue () const
      {
        if (_getter)
      return (_obj.*_getter)(_data);
        else
      return SGRawValue<VT>::DefaultValue();
      }
      virtual bool setValue (VT value)
      {
        if (_setter)
      {
        (_obj.*_setter)(_data, value);
        return true;
      }
        else
      return false;
      }
      virtual SGRawValue<VT> *clone () const 
      {
        return new RawValueMethodsData<C,VT,DT>(_obj, _data, _getter, _setter);
      }

    private:
      C       &_obj;
      DT      _data;
      getter_t    _getter;
      setter_t    _setter;
    };
    
    class PropertiesHandler : public simgear::TiedPropertyList
    {
    public:

      template <class T>
      inline void tie (SGPropertyNode *node, const SGRawValue<T> &raw_value)
      {
          Tie(node,raw_value);
      }

      template <class T>
      inline void tie (SGPropertyNode *node,
               const char *relative_path,
               const SGRawValue<T> &raw_value)
      {
        Tie(node->getNode(relative_path, true),raw_value);
      }

      PropertiesHandler() {};

      void unbind () {Untie();}
    };

  ///////////////////////////////////////////////////////////////////////////
  // FGVoicePlayer::Voice ////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  class Voice
  {
  public:

    /////////////////////////////////////////////////////////////////////////
    // FGVoicePlayer::Voice::Element ////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    class Element
    {
    public:
        bool silence;

        virtual ~Element() {}
        virtual inline void play (float volume) {}
        virtual inline void stop () {}
        virtual bool is_playing () = 0;
        virtual inline void set_volume (float volume) {}
    };

    /////////////////////////////////////////////////////////////////////////
    // FGVoicePlayer::Voice::SampleElement ///////////////////////////
    /////////////////////////////////////////////////////////////////////////

    class SampleElement : public Element
    {
        SGSharedPtr<SGSoundSample>  _sample;
        float               _volume;

    public:
        inline SampleElement (SGSharedPtr<SGSoundSample> sample, float volume = 1.0)
          : _sample(sample), _volume(volume) { silence = false; }

        virtual inline void play (float volume) { if (_sample && (volume > 0.05)) { set_volume(volume); _sample->play_once(); } }
        virtual inline void stop () { if (_sample) _sample->stop(); }
        virtual inline bool is_playing () { return _sample ? _sample->is_playing() : false; }
        virtual inline void set_volume (float volume) { if (_sample) _sample->set_volume(volume * _volume); }
    };

    /////////////////////////////////////////////////////////////////////////
    // FGVoicePlayer::Voice::SilenceElement //////////////////////////
    /////////////////////////////////////////////////////////////////////////

    class SilenceElement : public Element
    {
        double _duration;
        double start_time;

    public:
        inline SilenceElement (double duration)
          : _duration(duration) { silence = true; }

        virtual inline void play (float volume) { start_time = globals->get_sim_time_sec(); }
        virtual inline bool is_playing () { return globals->get_sim_time_sec() - start_time < _duration; }
    };

    /////////////////////////////////////////////////////////////////////////
    // FGVoicePlayer::Voice (continued) //////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    Element *element;

    inline Voice (FGVoicePlayer *_player)
      : element(NULL), player(_player), volume(1.0) {}

    virtual ~Voice ();

    inline void append (Element *_element) { elements.push_back(_element); }

    void play ();
    void stop (bool now);
    void set_volume (float _volume);
    void volume_changed ();
    void update ();

  private:
      FGVoicePlayer *player;

      float volume;

      vector<Element *>         elements;
      vector<Element *>::iterator   iter;

      inline float get_volume () const { return player->volume * player->speaker.volume * volume; }
  };

  ///////////////////////////////////////////////////////////////////////////
  // FGVoicePlayer (continued) ///////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  struct
  {
    float volume;
  } conf;

  float volume;

  Voice *voice;
  Voice *next_voice;
  bool paused;
  string dev_name;
  string dir_prefix;

  inline FGVoicePlayer (PropertiesHandler* properties_handler, string _dev_name)
    : volume(1.0), voice(NULL), next_voice(NULL), paused(false),
      dev_name(_dev_name), dir_prefix(""),
      speaker(this,properties_handler) {}

  virtual ~FGVoicePlayer ();

  void init ();
  void pause();
  void resume();
  bool is_playing() { return (voice!=NULL);}

  enum
  {
    PLAY_NOW      = 1 << 0,
    PLAY_LOOPED   = 1 << 1
  };
  void play (Voice *_voice, unsigned int flags = 0);

  enum
  {
    STOP_NOW      = 1 << 0
  };
  void stop (unsigned int flags = 0);

  void set_volume (float _volume);
  void update ();

  void bind (SGPropertyNode *node, const char* default_dir_prefix);

public:

  ///////////////////////////////////////////////////////////////////////////
  // FGVoicePlayer::Speaker //////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  class Speaker
  {
    FGVoicePlayer *player;
    PropertiesHandler* properties_handler;

    double    pitch;

    template <class T>
    inline void tie (SGPropertyNode *node, const char *name, T *ptr)
    {
    properties_handler->tie
    (node, (string("speaker/") + name).c_str(),
     RawValueMethodsData<FGVoicePlayer::Speaker,T,T*>
     (*this, ptr,
      &FGVoicePlayer::Speaker::get_property,
      &FGVoicePlayer::Speaker::set_property));
    }

  public:
    template <class T>
    inline void set_property (T *ptr, T value) { *ptr = value; update_configuration(); }

    template <class T>
    inline T get_property (T *ptr) const { return *ptr; }

    float volume;

    inline Speaker (FGVoicePlayer *_player,PropertiesHandler* _properties_handler)
  : player(_player),
    properties_handler(_properties_handler),
    pitch(1),
    volume(1)
    {
    }

    void bind (SGPropertyNode *node);
    void update_configuration ();
  };

protected:
  ///////////////////////////////////////////////////////////////////////////
  // FGVoicePlayer (continued) ///////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////

  SGSharedPtr<SGSampleGroup> _sgr;
  Speaker speaker;

  map< string, SGSharedPtr<SGSoundSample> >   samples;
  vector<Voice *>         _voices;

  bool looped;
  bool next_looped;

  SGSoundSample *get_sample (const char *name);

  inline void append (Voice *voice, Voice::Element *element) { voice->append(element); }
  inline void append (Voice *voice, const char *sample_name) { voice->append(new Voice::SampleElement(get_sample(sample_name))); }
  inline void append (Voice *voice, double silence) { voice->append(new Voice::SilenceElement(silence)); }

  inline void make_voice (Voice **voice) { *voice = new Voice(this); _voices.push_back(*voice); }

  template <class T1>
  inline void make_voice (Voice **voice, T1 e1) { make_voice(voice); append(*voice, e1); }
  template <class T1, class T2>
  inline void make_voice (Voice **voice, T1 e1, T2 e2) { make_voice(voice, e1); append(*voice, e2); }
  template <class T1, class T2, class T3>
  inline void make_voice (Voice **voice, T1 e1, T2 e2, T3 e3) { make_voice(voice, e1, e2); append(*voice, e3); }
  template <class T1, class T2, class T3, class T4>
  inline void make_voice (Voice **voice, T1 e1, T2 e2, T3 e3, T4 e4) { make_voice(voice, e1, e2, e3); append(*voice, e4); }
};

#endif // __SOUND_VOICEPLAYER_HXX
