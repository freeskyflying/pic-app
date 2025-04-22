#ifndef _VIDEO_MEDIA_SUBSESSION_H
#define _VIDEO_MEDIA_SUBSESSION_H

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "OnDemandServerMediaSubsession.hh"
//#include "ei_mai_comm_define.h"
#include "ei_comm_camera.h"
#include "ei_common.h"
#include "FramedSource.hh"

class VideoSource : public FramedSource
{
public:
	VideoSource(UsageEnvironment & env, int channel = 0);
	~VideoSource(void);
public:
	virtual void doGetNextFrame();
	virtual unsigned int maxFrameSize() const;
	static void getNextFrame(void * ptr);
	int GetFrameData();
private:
	void *m_pToken;
	int channel_;
};

class VideoMediaSubsession:
	public OnDemandServerMediaSubsession {
public:
	VideoMediaSubsession(UsageEnvironment &env,
		int channel = 0, int format = 0);
	~VideoMediaSubsession(void);

public:
	virtual char const *getAuxSDPLine(RTPSink *rtpSink,
			FramedSource *inputSource);
	virtual FramedSource *createNewStreamSource(unsigned clientSessionId,
			unsigned &estBitrate); // "estBitrate" is the stream's estimated bitrate, in kbps
	virtual RTPSink *createNewRTPSink(Groupsock *rtpGroupsock,
			unsigned char rtpPayloadTypeIfDynamic, FramedSource *inputSource);

	static VideoMediaSubsession *createNew(UsageEnvironment &env,
			int channel = 0,
			int format = 0);

	static void afterPlayingDummy(void *ptr);

	static void chkForAuxSDPLine(void *ptr);
	void chkForAuxSDPLine1();

private:
	char *m_pSDPLine;
	RTPSink *m_pDummyRTPSink;
	char m_done;
	int channel_;
	int format;
};

#endif /* _VIDEO_MEDIA_SUBSESSION_H */
