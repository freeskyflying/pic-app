#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include <sys/prctl.h>

#define LOG_TAG "app_video_subsession"
#include <log/log.h>

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include <stdio.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#endif
#include "VideoMediaSubsession.hh"




#define FRAME_PER_SEC 20


extern int live_video_get_data_from_buf(int channel,
  unsigned char *buffer,unsigned int buffer_size, unsigned int *size);



VideoSource::VideoSource(UsageEnvironment & env, int channel) :
  FramedSource(env), m_pToken(0), channel_(channel) {

}

VideoSource::~VideoSource(void) {
  envir().taskScheduler().unscheduleDelayedTask(m_pToken);
//	printf("[MEDIA SERVER] rtsp channel[%d] connection closed\n", channel_);
}

void VideoSource::doGetNextFrame() {
  double delay = 1000.0 / (FRAME_PER_SEC * 20);  /* ms */
  int64_t to_delay = delay * 1000;  /* us */
  //to_delay = 2000;
  int res = GetFrameData();

  if (res == -1){
    //printf("no data, schedule delay:%lld \n", to_delay);
    m_pToken = envir().taskScheduler().scheduleDelayedTask(to_delay, getNextFrame,
      this);
  }else{
    afterGetting(this);
  }
  
}

unsigned int VideoSource::maxFrameSize() const
{
  return 1024 * 200;
}

void VideoSource::getNextFrame(void * ptr)
{
  ((VideoSource *)ptr)->doGetNextFrame();
}

int flag = 0;
int VideoSource::GetFrameData()
{
  fFrameSize = 0;
  unsigned size = 0;
  int res = 0;
  res = live_video_get_data_from_buf(channel_, fTo,fMaxSize, &size);
  if (flag == 0) {
	  flag++;
	  printf("fMaxSize = %d \n", fMaxSize);
  }
  /*
  if (size != 0)
	printf("size = %d \n", size);
  */
  if (res == 0){
    gettimeofday(&fPresentationTime, 0);
    fFrameSize = size;

	if (fMaxSize <= size){
        printf("RTSP: data size > fMaxSize, part of data may be dropped\n");
	}
    //fDurationInMicroseconds = 40000;
    if (fFrameSize > fMaxSize) {
      fNumTruncatedBytes = fFrameSize - fMaxSize;
      printf("fNumTruncatedBytes:%u fMaxSize:%u fFrameSize:%u \n",
              fNumTruncatedBytes, fMaxSize, fFrameSize);
      fFrameSize = fMaxSize;
    } else {
      fNumTruncatedBytes = 0;
    }
    return 0;
  }

  return -1;
}

VideoMediaSubsession::VideoMediaSubsession(
  UsageEnvironment &env, int channel, int format) :
  OnDemandServerMediaSubsession(env, True)
{
  m_pSDPLine = 0;
  this->channel_ = channel;
  this->format = format;
}

VideoMediaSubsession::~VideoMediaSubsession(void) {
  if (m_pSDPLine) {
    free(m_pSDPLine);
  }
}

VideoMediaSubsession* VideoMediaSubsession::createNew(
  UsageEnvironment &env, int channel, int format) {
    ALOGE("%s:%d >> createNew format:%d channel:%d",
      __func__, __LINE__, format, channel);

  return new VideoMediaSubsession(env, channel, format);
}

FramedSource * VideoMediaSubsession::createNewStreamSource(
  unsigned clientSessionId, unsigned & estBitrate) {
  estBitrate = 300; /*kbs*/
  if (PT_H265 == format){
      ALOGE("%s:%d >> H265VideoStreamFramer format:%d channel_:%d",
        __func__, __LINE__, format, channel_);

    return H265VideoStreamFramer::createNew(envir(),
      new VideoSource(envir(), channel_));
  }
  else if (PT_H264 == format){
      ALOGE("%s:%d >> H264VideoStreamFramer format:%d channel_:%d",
        __func__, __LINE__, format, channel_);

    return H264VideoStreamFramer::createNew(envir(),
      new VideoSource(envir(), channel_));
 }
  else {
    ALOGE("%s:%d >> Do not support format:%d",
      __func__, __LINE__, format);
    return NULL;
  }
}

RTPSink * VideoMediaSubsession::createNewRTPSink(
  Groupsock * rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic,
  FramedSource * inputSource) {


  OutPacketBuffer::maxSize = 300*1024;

  if (PT_H265 == format)
    return H265VideoRTPSink::createNew(envir(), rtpGroupsock,
      rtpPayloadTypeIfDynamic);
  else if (PT_H264 == format)
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock,
      rtpPayloadTypeIfDynamic);
  else {
    ALOGE("%s:%d >> Do not support format:%d",
      __func__, __LINE__, format);
    return NULL;
  }
}

char const * VideoMediaSubsession::getAuxSDPLine(
  RTPSink * rtpSink, FramedSource * inputSource) {
  if (m_pSDPLine) {
    return m_pSDPLine;
  }

  m_pDummyRTPSink = rtpSink;

  /* mp_dummy_rtpsink->startPlaying(*source, afterPlayingDummy, this); */
  m_pDummyRTPSink->startPlaying(*inputSource, 0, 0);

  chkForAuxSDPLine(this);

  m_done = 0;

  envir().taskScheduler().doEventLoop(&m_done);

  if (m_pDummyRTPSink) {
    m_pSDPLine = strdup(m_pDummyRTPSink->auxSDPLine());
    m_pDummyRTPSink->stopPlaying();
    m_pDummyRTPSink = NULL;
  }

  return m_pSDPLine;
}

void VideoMediaSubsession::afterPlayingDummy(void * ptr)
{
  VideoMediaSubsession * This = (VideoMediaSubsession *)ptr;

  This->m_done = 0xff;
}

void VideoMediaSubsession::chkForAuxSDPLine(void * ptr)
{
  VideoMediaSubsession * This = (VideoMediaSubsession *)ptr;
  This->chkForAuxSDPLine1();
}

void VideoMediaSubsession::chkForAuxSDPLine1()
{
  if (m_pDummyRTPSink && m_pDummyRTPSink->auxSDPLine()) {
    m_done = 0xff;
  } else {
    double delay = 1000.0 / (FRAME_PER_SEC);  /* ms */
    int to_delay = delay * 1000;  /* us */

    nextTask() = envir().taskScheduler().scheduleDelayedTask(
      to_delay, chkForAuxSDPLine, this);
  }
}
