/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)
This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.
You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2013, Live Networks, Inc.  All rights reserved

#include <pthread.h>
#include <sys/prctl.h>

#include "version.hh"
#include "DynamicRTSPServer.hh"
#include "BasicUsageEnvironment.hh"
#include "VideoMediaSubsession.hh"

#include "AudioMediaSubsession.hh"




#ifdef __cplusplus
extern "C" {
#endif


#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "app_live_server"
#endif
#include <log/log.h>

static char watchVariable = 1;
static int rtsp_port = 554;
pthread_t thread_server;




static void *rtsp_server_task(void *p)
{
  prctl(PR_SET_NAME, "rtsp_server");

  char * url;
	/* Begin by setting up our usage environment: */
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	//TaskScheduler* scheduler = EpollTaskScheduler::createNew();
	UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

	UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
	/* To implement client access control to the RTSP server, do the following: */

	authDB = new UserAuthenticationDatabase;
	authDB->addUserRecord("username1", "password1"); /* replace these with real strings */
	/* Repeat the above with each <username>, <password> that you wish to allow */
	/* access to the server. */
#endif

	/* Create the RTSP server: */
  ALOGD("%s:%d >> rtsp_port=%d", __func__, __LINE__, rtsp_port);

	RTSPServer* rtspServer = RTSPServer::createNew(*env, rtsp_port, authDB);
	if (rtspServer == NULL) {
		*env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
		exit(1);
	}

	/* Add live stream */
	//VideoSource * videoSource = 0;
	ServerMediaSession * sms_main = ServerMediaSession::createNew(*env, "live0", NULL, "sdrive live");
	sms_main->addSubsession(VideoMediaSubsession::createNew(*env, 0, PT_H265));
	rtspServer->addServerMediaSession(sms_main);
	
	ServerMediaSession * sms_sec = ServerMediaSession::createNew(*env, "live1", NULL, "sdrive live");
	sms_sec->addSubsession(VideoMediaSubsession::createNew(*env, 1, PT_H265));
	rtspServer->addServerMediaSession(sms_sec);
	
	
	url = rtspServer->rtspURL(sms_main);
	*env << "visit h265 url \"" << url << "\"\n";
	delete[] url;

	/* Run loop */
	env->taskScheduler().doEventLoop(&watchVariable);

	rtspServer->removeServerMediaSession(sms_main);
	rtspServer->removeServerMediaSession(sms_sec);

	Medium::close(rtspServer);

	env->reclaim();

	delete scheduler;

	return NULL;
}

/* Media Server */
int rtsp_server_init()
{
	int ret = 0;
  ALOGD("%s:%d >> enter", __func__, __LINE__);

  if (1 == watchVariable) {
    watchVariable = 0;
    ret = pthread_create(&thread_server, NULL, rtsp_server_task, NULL);
  }

  return ret;
}

void rtsp_server_deinit()
{
  ALOGD("%s:%d >> enter", __func__, __LINE__);
  watchVariable = 1;
  pthread_join(thread_server, NULL);
}

int is_rtsp_working(void) {
  ALOGD("%s:%d >> watchVariable=%d", __func__, __LINE__, watchVariable);
  return 0 == watchVariable?1:0;
}

int set_rtsp_work(int enable) {
  ALOGD("%s:%d >> enable=%d, watchVariable=%d", __func__, __LINE__,
    enable, watchVariable);
  if (enable != 0) {
    if (watchVariable != 0) {
      rtsp_server_init();
    }
  } else {
    if (watchVariable == 0) {
      rtsp_server_deinit();
    }
  }
  return 0;
}

int get_rtsp_channel_port(void) {
  ALOGD("%s:%d >> rtsp_port=%d", __func__, __LINE__, rtsp_port);
  return rtsp_port;
}

void set_rtsp_channel_port(int port) {
  rtsp_port = port;
  ALOGD("%s:%d >> rtsp_port=%d", __func__, __LINE__, rtsp_port);
}

#ifdef __cplusplus
};
#endif
