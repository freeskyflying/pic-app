/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// A 'ServerMediaSubsession' object that creates new, unicast, "RTPSink"s
// on demand, from an WAV audio file.
// C++ header

#ifndef _AUDIO_MEDIA_SUBSESSION_HH
#define _AUDIO_MEDIA_SUBSESSION_HH

#ifndef _ON_DEMAND_SERVER_MEDIA_SUBSESSION_HH
#include "OnDemandServerMediaSubsession.hh"
#endif
#include "FramedSource.hh"

#define AUDIO_AAC_S "AAC-hbr"
#define AUDIO_PCM_S "L16"
#define AUDIO_G711A_S "PCMA"
#define AUDIO_G711MU_S "PCMU"
#define AUDIO_ADPCM_S "DVI4"
#define AUDIO_G729_S "G729"
#define AUDIO_G726_S "G726-32"

class AudioSource: public FramedSource {
public:
  AudioSource(UsageEnvironment &env, int channel = 0);
  ~AudioSource(void);

  static void getNextFrame(void *ptr);
  int GetFrameData();
private:
  // redefined virtual functions:
  virtual void doGetNextFrame();
  void *m_pToken;
  int channel_;
  char * audioType_;
};

class AudioMediaSubsession: public OnDemandServerMediaSubsession{
public:
	AudioMediaSubsession(UsageEnvironment& env,
		int channel = 0, char const* audioType = AUDIO_AAC_S,
		int audio_channel = 2, int audio_sample_rate = 16000);
	virtual ~AudioMediaSubsession();

protected: // redefined virtual functions
	virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
		unsigned& estBitrate);
	virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
		unsigned char rtpPayloadTypeIfDynamic,
		FramedSource* inputSource);

private:
	int channel_;
	char const* audioType_;
	int audio_channel_;
	int audio_sample_rate_;
};

#endif

