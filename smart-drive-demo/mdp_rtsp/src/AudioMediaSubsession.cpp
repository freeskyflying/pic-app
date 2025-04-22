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
// Implementation

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <sys/prctl.h>

#define LOG_TAG "app_audio_subsession"
#include <log/log.h>

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include "AudioMediaSubsession.hh"
#include "uLawAudioFilter.hh"
#include "SimpleRTPSink.hh"
#include "MPEG4GenericRTPSink.hh"
#include "InputFile.hh"
#include <GroupsockHelper.hh>
#ifndef __UCLIBC__
#include <execinfo.h>
#endif
#define FRAME_PER_SEC 30

#ifdef __cplusplus
extern "C" {
#endif
extern int live_audio_get_data_from_buf(int channel,
  unsigned char *buffer,unsigned int buffer_size, unsigned int *size);
#ifdef __cplusplus
}
#endif /* __cplusplus */


AudioSource::AudioSource(UsageEnvironment &env, int channel) :
  FramedSource(env), m_pToken(0), channel_(channel) {

}

AudioSource::~AudioSource(void)
{
  envir().taskScheduler().unscheduleDelayedTask(m_pToken);
}

void AudioSource::doGetNextFrame() {
  /* according to fps，calculate wait time */
  //double delay = 1000.0 / (FRAME_PER_SEC * 4);  /* ms */
  double delay = 1000.0 / (FRAME_PER_SEC * 2);  /* ms */
  int64_t to_delay = delay * 1000;  /* us */
  int res = GetFrameData();
  if (res == -1){
    //printf("no data, schedule delay:%lld \n", to_delay);
    m_pToken = envir().taskScheduler().scheduleDelayedTask(to_delay,
      getNextFrame, this);
  }else{
    afterGetting(this);
  }
}

void AudioSource::getNextFrame(void * ptr) {
  ((AudioSource *)ptr)->doGetNextFrame();
}

int AudioSource::GetFrameData() {
  int ret = 0;
  fFrameSize = 0;
  ret = live_audio_get_data_from_buf(channel_, fTo,fMaxSize, &fFrameSize);
  if (0 == ret) {
    gettimeofday(&fPresentationTime, 0);
    if (fFrameSize > fMaxSize) {
      fNumTruncatedBytes = fFrameSize - fMaxSize;
      fFrameSize = fMaxSize;
    } else {
      fNumTruncatedBytes = 0;
    }
    return 0;
  }
  return -1;
}

AudioMediaSubsession::AudioMediaSubsession(
  UsageEnvironment &env, int channel, char const* audioType,
  int audio_channel, int audio_sample_rate) :
  OnDemandServerMediaSubsession(env, True) {
  ALOGD("%s:%d >> constructor", __func__, __LINE__);
  channel_ = channel;
  audioType_ = audioType;
  audio_channel_ = audio_channel;
  audio_sample_rate_ = audio_sample_rate;
}

AudioMediaSubsession::~AudioMediaSubsession() {
  ALOGD("%s:%d >> deconstructor", __func__, __LINE__);
}

FramedSource* AudioMediaSubsession::createNewStreamSource(
  unsigned /*clientSessionId*/, unsigned &estBitrate) {
  estBitrate = 100;
  ALOGD("%s:%d >> audioType_='%s'", __func__, __LINE__, audioType_);
  return new AudioSource(envir(), channel_);
}

RTPSink* AudioMediaSubsession::createNewRTPSink (
  Groupsock *rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
  FramedSource *inputSource) {
  ALOGD("%s:%d >> audioType_='%s',audio_sample_rate_=%d ", __func__, __LINE__, audioType_,audio_sample_rate_);
  rtpPayloadTypeIfDynamic = 96;
  /*audio only need 100k*/
  //OutPacketBuffer::maxSize = 100*1024;

  if(strcmp(audioType_, AUDIO_AAC_S) == 0) {
    return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
        rtpPayloadTypeIfDynamic, audio_sample_rate_,"audio", audioType_, "", audio_channel_);
  } else if (strcmp(audioType_, AUDIO_PCM_S) == 0){
    if(audio_sample_rate_ == 44100 && audio_channel_ == 2)
    {
      rtpPayloadTypeIfDynamic = 10;
    }
    return SimpleRTPSink::createNew(envir(), rtpGroupsock,
        rtpPayloadTypeIfDynamic, audio_sample_rate_,"audio", audioType_, audio_channel_);
  } else if (strcmp(audioType_, AUDIO_G711A_S) == 0 || strcmp(audioType_, AUDIO_G711MU_S) == 0){
    return SimpleRTPSink::createNew(envir(), rtpGroupsock,
        rtpPayloadTypeIfDynamic, audio_sample_rate_,"audio", audioType_, audio_channel_);
  } else if (strcmp(audioType_, AUDIO_ADPCM_S) == 0){
    return SimpleRTPSink::createNew(envir(), rtpGroupsock,
        rtpPayloadTypeIfDynamic, audio_sample_rate_,"audio", audioType_, audio_channel_);
  } else if (strcmp(audioType_, AUDIO_G729_S) == 0){
    rtpPayloadTypeIfDynamic = 18;
    return SimpleRTPSink::createNew(envir(), rtpGroupsock,
        rtpPayloadTypeIfDynamic, audio_sample_rate_,"audio", audioType_, audio_channel_);
  } else if (strcmp(audioType_, AUDIO_G726_S) == 0){
    rtpPayloadTypeIfDynamic = 2;
    return SimpleRTPSink::createNew(envir(), rtpGroupsock,
        rtpPayloadTypeIfDynamic, audio_sample_rate_,"audio", audioType_, audio_channel_);
  } else {
    ALOGE("%s:%d >> Do not support format:%s",__func__, __LINE__, audioType_);
    return NULL;
  }
}
