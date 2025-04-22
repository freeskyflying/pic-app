#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>

#include "utils.h"
#include "pnt_log.h"
#include "jt808_utils.h"
#include "jt1078_media_files.h"

#include "media_utils.h"
#include "media_storage.h"
#include "jt808_session.h"
#include "jt1078_rtsp.h"
#include "jt1078_params.h"

static int  JT1078MediaFiles_QueryFilesByChannel(int logicChn, unsigned char* bcdTimeStart, unsigned char* bcdTimeEnd, queue_list_t* list, long long alertFlag)
{
	if (logicChn > MAX_VIDEO_NUM || logicChn == 0)
	{
		return 0;
	}

	int count = 0;
	queue_t* head = gVideoListQueue[logicChn-1].m_head;

	time_t timeStart = convertBCDToSysTime(bcdTimeStart);
	time_t timeEnd = convertBCDToSysTime(bcdTimeEnd);
	int timeStartFlag = (bcdTimeStart[0] | bcdTimeStart[1] | bcdTimeStart[2]);
	int timeEndFlag = (bcdTimeEnd[0] | bcdTimeEnd[1] | bcdTimeEnd[2]);

	char filename[128] = { 0 };

	while (head)
	{
		time_t fileTime = (time_t)head->m_content;

		if ((!timeStartFlag || ((fileTime < timeStart && (fileTime+60) > timeStart) || fileTime >= timeStart )) && (!timeEndFlag || fileTime < timeEnd))
		{
			get_videoname_by_time(logicChn, fileTime, filename);
		
			VideoFileExtraInfo_t fileInfo = { 0 };
			get_videofile_extraInfo(filename, &fileInfo);

			if ((fileTime+fileInfo.duration) <= timeStart)
			{
				head = head->next;
				continue;
			}

			if (fileInfo.duration > 0 && fileInfo.duration <= 65 && (0 == alertFlag || (fileInfo.alertFlag & alertFlag)))
			{
				JT808MsgBody_1205_Item_t* item = (JT808MsgBody_1205_Item_t*)malloc(sizeof(JT808MsgBody_1205_Item_t));
				if (NULL != item)
				{
					memset(item, 0, sizeof(JT808MsgBody_1205_Item_t));

					item->mLogicChnNum = logicChn;
					convertSysTimeToBCD(fileTime, item->mStartBcdTime);
					convertSysTimeToBCD(fileTime + (fileInfo.duration>60?60:(fileInfo.duration)), item->mEndBcdTime);

					item->mResType = 2;
					item->mFileSize = fileInfo.fileSize;
					item->mStorageType = 1;
					item->mStreamType = 1;

					if (queue_list_push(list, item))
					{
						free(item);
					}
					else
					{
						count ++;
					}
				}
			}
		}

		head = head->next;

//		usleep(10);
	}

	return count;
}

void JT1078MediaFiles_QueryFiles(JT808MsgBody_9205_t* reqParam, JT808MsgBody_1205_t* ackParam)
{
	PNT_LOGE("start query file: %d %02X%02X%02X%02X%02X%02X-%02X%02X%02X%02X%02X%02X", reqParam->mLogicChnNum,
		reqParam->mStartBcdTime[0], reqParam->mStartBcdTime[1], reqParam->mStartBcdTime[2], reqParam->mStartBcdTime[3], reqParam->mStartBcdTime[4], reqParam->mStartBcdTime[5],
		reqParam->mEndBcdTime[0], reqParam->mEndBcdTime[1], reqParam->mEndBcdTime[2], reqParam->mEndBcdTime[3], reqParam->mEndBcdTime[4], reqParam->mEndBcdTime[5]);

	long long alertFlag = ((((long long)reqParam->mAlertFlagh)<<32)&0xFFFFFFFF00000000) + reqParam->mAlertFlagl;
		
	if (0 == reqParam->mLogicChnNum)
	{
		for (int i=1; i<=MAX_VIDEO_NUM; i++)
		{
			ackParam->mResCount += JT1078MediaFiles_QueryFilesByChannel(i, reqParam->mStartBcdTime, reqParam->mEndBcdTime, &ackParam->mResList, alertFlag);
		}
	}
	else
	{
		ackParam->mResCount = JT1078MediaFiles_QueryFilesByChannel(reqParam->mLogicChnNum, reqParam->mStartBcdTime, reqParam->mEndBcdTime, &ackParam->mResList, alertFlag);
	}
	
	PNT_LOGE("finish query file: %d %02X%02X%02X%02X%02X%02X-%02X%02X%02X%02X%02X%02X", ackParam->mResCount,
		reqParam->mStartBcdTime[0], reqParam->mStartBcdTime[1], reqParam->mStartBcdTime[2], reqParam->mStartBcdTime[3], reqParam->mStartBcdTime[4], reqParam->mStartBcdTime[5],
		reqParam->mEndBcdTime[0], reqParam->mEndBcdTime[1], reqParam->mEndBcdTime[2], reqParam->mEndBcdTime[3], reqParam->mEndBcdTime[4], reqParam->mEndBcdTime[5]);
}

static int jt1078PlaybackSendRtp(JT1078MediaPlaybackCtrl_t* playback, unsigned char* buffer, int len)
{
	if (send(playback->socketFd, buffer, len, 0) == -1)
	{
		return -1;
	}

	return 0;
}

static int jt1078PlaybackPackAndSendVideoRtp(JT1078MediaPlaybackCtrl_t* playback, uint8_t* naluData, int len, unsigned int frameTime)
{
	uint8_t rtp[128] = { 0 };

	JT808Session_t* session = (JT808Session_t*)playback->owner;

	int index = 0;
	BYTE byte15 = 0, byte5 = 0;

	NetbufPushDWORD(rtp, 0x30316364, 0);
	NetbufPushBYTE(rtp, 0x81, 4);

	byte5 = RTP_1078_PAYLOAD_H264;
#if (defined VIDEO_PT_H265)
	byte5 = RTP_1078_PAYLOAD_H265;
#endif
	NetbufPushBYTE(rtp, byte5, 5);
	if (SESSION_TYPE_JT808 == session->mSessionData->mSessionType)
	{
		NetbufPushBuffer(rtp, session->mPhoneNum, 8, 6);
	}
	else
	{
		NetbufPushBuffer(rtp, session->mPhoneNum+4, 8, 6);
	}
	NetbufPushBYTE(rtp, playback->mLogicChannel, 14);
	
	NetbufPushDWORD(rtp, frameTime, 20);

	NetbufPushWORD(rtp, frameTime - playback->mLastIFrameTime, 24);
	NetbufPushWORD(rtp, frameTime - playback->mLastFrameTime, 26);
	
#if (defined VIDEO_PT_H265)
	BYTE tpye = (naluData[4]&0x7E) >> 1;
	/* 帧类别 */
	if (0x20 == tpye || 0x13 == tpye) /* IDR or I 帧 */
	{
		playback->mLastIFrameTime = frameTime;//currentTimeMillis();
		byte15 &= 0x0F;
	}
	else if (1 == tpye) /* P 帧 */
	{
		byte15 &= 0x1F;
	}
	else /* 透传其他参数序列 */
	{
		byte15 &= 0x4F;
	}
#else
	/* 帧类别 */
	if (5 == (naluData[4] & 0x1F) || 7 == (naluData[4] & 0x1F)) /* IDR or I 帧 */
	{
		playback->mLastIFrameTime = frameTime;//currentTimeMillis();
		byte15 &= 0x0F;
	}
	else if (1 == (naluData[4] & 0x1F)) /* P 帧 */
	{
		byte15 &= 0x1F;
	}
	else if (0x21 == (naluData[4] & 0x1F)) /* B 帧 */
	{
		byte15 &= 0x2F;
	}
	else /* 透传其他参数序列 */
	{
		byte15 &= 0x4F;
	}
#endif

	playback->mLastFrameTime = frameTime;//currentTimeMillis();

	int offset = 0;
	int packCnt = (len+PACKET_BODY_LEN-1)/PACKET_BODY_LEN;
	int rtpDataLen = 0;

	pthread_mutex_lock(&playback->mutex);
	
	for (int i=0; i<packCnt; i++)
	{
		if (1 == packCnt)
		{
			byte15 = (byte15&0xF0);
			byte5 |= 0x80;
			NetbufPushBYTE(rtp, byte5, 5);
		}
		else
		{
			if (0 == i)
			{
				byte15 = (byte15&0xF0) + 0x01;
			}
			else if (packCnt - 1 == i)
			{
				byte15 = (byte15&0xF0) + 0x02;
				byte5 |= 0x80;
				NetbufPushBYTE(rtp, byte5, 5);
			}
			else
			{
				byte15 = (byte15&0xF0) + 0x03;
			}
		}

		rtpDataLen = (i==(packCnt-1))?(len-offset):PACKET_BODY_LEN;
	
		NetbufPushBYTE(rtp, byte15, 15);
		
		NetbufPushWORD(rtp, rtpDataLen, 28);

		NetbufPushWORD(rtp, playback->mRtpSerialNum ++, 6);
		
		if (playback->socketFd<0)
			break;

		int resend = 0;
		for (resend=0; resend<3; resend++)
		{
			if (jt1078PlaybackSendRtp(playback, rtp, 30) == 0)
			{
				if (jt1078PlaybackSendRtp(playback, naluData+offset, rtpDataLen) == 0)
				{
					break;
				}
			}
		}
		if (resend >= 3)
		{
			pthread_mutex_unlock(&playback->mutex);
			return -1;
		}

		offset += rtpDataLen;
	}

	pthread_mutex_unlock(&playback->mutex);

	return 0;
}

static int jt1078PlaybackPackAndSendAudioRtp(JT1078MediaPlaybackCtrl_t* playback, uint8_t* naluData, int len, unsigned int frameTime)
{
	uint8_t rtp[128] = { 0 };

	JT808Session_t* session = (JT808Session_t*)playback->owner;

	int index = 0;
	BYTE byte15 = 0, byte5 = 0;

	NetbufPushDWORD(rtp, 0x30316364, 0);
	NetbufPushBYTE(rtp, 0x81, 4);
	
	byte5 = RTP_1078_PAYLOAD_AACLC;
	NetbufPushBYTE(rtp, byte5, 5);
	if (SESSION_TYPE_JT808 == session->mSessionData->mSessionType)
	{
		NetbufPushBuffer(rtp, session->mPhoneNum, 8, 6);
	}
	else
	{
		NetbufPushBuffer(rtp, session->mPhoneNum+4, 8, 6);
	}
	NetbufPushBYTE(rtp, playback->mLogicChannel, 14);
	
	NetbufPushDWORD(rtp, frameTime, 20);

	byte15 = 0x30;

	int packCnt = (len+PACKET_BODY_LEN-1)/PACKET_BODY_LEN;
	int rtpDataLen = 0;

	pthread_mutex_lock(&playback->mutex);
	
	for (int i=0; i<packCnt; i++)
	{
		if (1 == packCnt)
		{
			byte15 = (byte15&0xF0);
			byte5 |= 0x80;
			NetbufPushBYTE(rtp, byte5, 5);
		}
		else
		{
			if (0 == i)
			{
				byte15 = (byte15&0xF0) + 0x01;
			}
			else if (packCnt - 1 == i)
			{
				byte15 = (byte15&0xF0) + 0x02;
				byte5 |= 0x80;
				NetbufPushBYTE(rtp, byte5, 5);
			}
			else
			{
				byte15 = (byte15&0xF0) + 0x03;
			}
		}
	
		NetbufPushBYTE(rtp, byte15, 15);

		rtpDataLen = (i==(packCnt-1))?(len-i*PACKET_BODY_LEN):PACKET_BODY_LEN;
		
		NetbufPushWORD(rtp, rtpDataLen, 24);

		NetbufPushWORD(rtp, playback->mRtpSerialNum ++, 6);
		
		if (playback->socketFd<0)
			break;

		int resend = 0;
		for (resend=0; resend<3; resend++)
		{
			if (jt1078PlaybackSendRtp(playback, rtp, 26) == 0)
			{
				if (jt1078PlaybackSendRtp(playback, naluData+(i*PACKET_BODY_LEN), rtpDataLen) == 0)
				{
					break;
				}
			}
		}
		if (resend >= 3)
		{
			pthread_mutex_unlock(&playback->mutex);
			return -1;
		}
	}
		
	pthread_mutex_unlock(&playback->mutex);

	return 0;
}

int PlaybackVideoEs(unsigned char* frame, int len, void* owner, unsigned int frameTime)
{
	JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)owner;

	long long timestart = currentTimeMillis();
	char filename[128] = { 0 };

	if (frame[4] == 0x67)
	{
		get_videoname_by_time(playback->mLogicChannel, playback->filetime, filename);
		PNT_LOGI("send video: %s  %d %d\n", filename, frameTime, len);
	}

	if (jt1078PlaybackPackAndSendVideoRtp(playback, frame, len, frameTime))
	{
		get_videoname_by_time(playback->mLogicChannel, playback->filetime, filename);
		PNT_LOGE("%s send pack failed. exit", filename);
		playback->mp4Demuxer.state = mp4v2_demuxer_ctrl_stop;
	}

	return (currentTimeMillis()-timestart);
}

int PlaybackAudioEs(unsigned char* frame, int len, void* owner, unsigned int frameTime)
{
	JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)owner;

	long long timestart = currentTimeMillis();

	if (jt1078PlaybackPackAndSendAudioRtp(playback, frame, len, frameTime))
	{
		char filename[128] = { 0 };
		get_videoname_by_time(playback->mLogicChannel, playback->filetime, filename);
		PNT_LOGE("%s send pack failed. exit", filename);
		playback->mp4Demuxer.state = mp4v2_demuxer_ctrl_stop;
	}

	return (currentTimeMillis()-timestart);
}

int jt1078PlaybackConnect(uint8_t* url, uint16_t port)
{
	int socketFd = 0;
	
    struct addrinfo *addrs = NULL;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    char portStr[16] = {0};
    sprintf(portStr, "%d", port);
    int result = 0;
    PNT_LOGI("BaseSocket start to get the address of %s : %d", url, port);
    result = getaddrinfo((char*)url, portStr, &hints, &addrs);
    if (result != 0)
    {
        PNT_LOGE("BaseSocket fatal state error!!! fail to get the address info of %s:%d", url, port);
        return RET_FAILED;
    }

    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next)
    {
        socketFd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (socketFd < 0)
        {
            PNT_LOGE("BaseSocket fatal state error!!! create socket of [%s:%d] error, caused by %s", url, port,
                    strerror(errno));
            freeaddrinfo(addrs);
            return RET_FAILED;
        }

        PNT_LOGD("BaseSocket the socketFd of [%s:%d] are :%d", url, port, socketFd);
		
		struct timeval timeoutRecv = {3, 0};
		if (setsockopt(socketFd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeoutRecv, sizeof(struct timeval)) < 0)
		{
			PNT_LOGE("setsockopt SO_RCVTIMEO error:%s", strerror(errno));
		}
		
		struct timeval timeoutSend = {3, 0};
		if (setsockopt(socketFd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeoutSend, sizeof(struct timeval)) < 0)
		{
			PNT_LOGE("setsockopt SO_SNDTIMEO error:%s", strerror(errno));
		}

		int recvSize = 8192*10;
        if (setsockopt(socketFd, SOL_SOCKET, SO_SNDBUF, &recvSize, sizeof(recvSize)) < 0)
        {
            PNT_LOGE("setsockopt SO_RCVBUF error:%s", strerror(errno));
			close(socketFd);
            return -1;
        }

        if (connect(socketFd, addr->ai_addr, addr->ai_addrlen) < 0)
        {
            if (errno != EINPROGRESS)
            {
                PNT_LOGE("BaseSocket fatal state error!!! connect to [%s:%d] error! errno:%d %s", url, port, errno, strerror(errno));
                freeaddrinfo(addrs);
				close(socketFd);
                return RET_FAILED;
            }
            else
            {
                PNT_LOGE("BaseSocket connection to [%s:%d] may established", url, port);
                break;
            }
        }
    }
    freeaddrinfo(addrs);

    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(socketFd, &readfds);
    FD_SET(socketFd, &writefds);
    timeout.tv_sec = 3; //timeout is 5 second
    timeout.tv_usec = 0;

    int ret = select(socketFd + 1, &readfds, &writefds, NULL, &timeout);
    if (ret <= 0)
    {
        PNT_LOGE("connection to [%s:%d] time out", url, port);
		close(socketFd);
        return RET_FAILED;
    }

    if (FD_ISSET(socketFd, &readfds) ||FD_ISSET(socketFd, &writefds))
    {
        int error = 0;
        socklen_t length = sizeof(error);
        if (getsockopt(socketFd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
        {
            PNT_LOGE("get socket of [%s:%d] option failed", url, port);
			close(socketFd);
            return RET_FAILED;
        }

        if (error != 0)
        {
            PNT_LOGE("connection of [%s:%d] failed after select with the error: %d %s", url, port, error, strerror(error));
			close(socketFd);
            return RET_FAILED;
        }
    }
    else
    {
        PNT_LOGE("connection [%s:%d] no events on sockfd found", url, port);
		close(socketFd);
        return RET_FAILED;
    }

    FD_CLR(socketFd, &readfds);
    FD_CLR(socketFd, &writefds);

    return socketFd;
}

void* JT1078PlaybackConnectServer(void* pArg)
{
	pthread_detach(pthread_self());
	
	JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)pArg;

	int socketFd = -1, connectCount = 0;

	char filename[128] = { 0 };

	PNT_LOGI("start connect server");
	
	pthread_mutex_init(&playback->mutex, NULL);
				
connect:
	socketFd = jt1078PlaybackConnect(playback->serverAddr, playback->serverPort);
	if (RET_FAILED == socketFd)
	{
		connectCount ++;
		if (connectCount>2)
		{
			goto exit;
		}
		else
		{
			goto connect;
		}
	}

	playback->socketFd = socketFd;
	
	get_videoname_by_time(playback->mLogicChannel, playback->filetime, filename);

	mp4v2_Demuxer_Start(&playback->mp4Demuxer, filename);

exit:

	if (socketFd > 0)
	{
		close(socketFd);
	}
	
	pthread_mutex_destroy(&playback->mutex);
//	free(playback);
	playback->mp4Demuxer.state = mp4v2_demuxer_ctrl_release;
	
	PNT_LOGE("%s exit replay", filename);

	return NULL;
}

void JT1078MediaFiles_PlaybackRequest(void* owner, JT808MsgBody_9201_t* reqParam, JT808MsgBody_1205_t* ackParam)
{
	if (0 == reqParam->mLogicChnNum)
	{
		return;
	}

	uint8_t sameClient = 0;
	char filename[128] = { 0 };
	JT808Session_t* session = (JT808Session_t*)owner;
	
	ackParam->mResCount = JT1078MediaFiles_QueryFilesByChannel(reqParam->mLogicChnNum, reqParam->mStartBcdTime, reqParam->mEndBcdTime, &ackParam->mResList, 0);

	if (ackParam->mResCount > 0)
	{
		JT808MsgBody_1205_Item_t* item = (JT808MsgBody_1205_Item_t*)ackParam->mResList.m_head->prev->m_content;
		if (NULL != item)
		{
			pthread_mutex_lock(&session->mMediaPlaybackList.m_mutex);
			queue_t* head = session->mMediaPlaybackList.m_head;
			while (head)
			{
				JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)head->m_content;
				if (playback && playback->filetime == convertBCDToSysTime(item->mStartBcdTime) && playback->mLogicChannel == reqParam->mLogicChnNum &&
					(mp4v2_demuxer_ctrl_playing == playback->mp4Demuxer.state || mp4v2_demuxer_ctrl_init == playback->mp4Demuxer.state))
				{
					//playback->mp4Demuxer.state = mp4v2_demuxer_ctrl_stop;
					sameClient = 1;
				}
				head = head->next;
			}
			pthread_mutex_unlock(&session->mMediaPlaybackList.m_mutex);

			if (sameClient)
			{
				get_videoname_by_time(reqParam->mLogicChnNum, convertBCDToSysTime(item->mStartBcdTime), filename);
				PNT_LOGE("%s had same client.", filename);
				return;
			}
			
			JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)malloc(sizeof(JT1078MediaPlaybackCtrl_t));

			if (playback)
			{
				memset(playback, 0, sizeof(JT1078MediaPlaybackCtrl_t));
				playback->owner = owner;
				
				playback->filetime = convertBCDToSysTime(item->mStartBcdTime);
				playback->mRtpSerialNum = 0;
				playback->mLogicChannel = reqParam->mLogicChnNum;
				strcpy((char*)playback->serverAddr, (char*)reqParam->mIp);
				playback->serverPort = reqParam->mTCPPort;
		
				memset(&playback->mp4Demuxer, 0, sizeof(playback->mp4Demuxer));

				playback->mp4Demuxer.vidCb = PlaybackVideoEs;
				playback->mp4Demuxer.audCb = PlaybackAudioEs;
				playback->mp4Demuxer.owner = playback;
				playback->mp4Demuxer.state = mp4v2_demuxer_ctrl_stop;
				playback->mp4Demuxer.mode = reqParam->mPlaybackMode;
				playback->mp4Demuxer.gain = reqParam->mFastSlowRate;
				playback->mp4Demuxer.startTimestamp = playback->filetime;
				unsigned int dragPos = convertBCDToSysTime(reqParam->mStartBcdTime);
				playback->mp4Demuxer.dragOffset = dragPos - playback->filetime;
				if (playback->mp4Demuxer.dragOffset >= 60)
				{
					playback->mp4Demuxer.dragOffset = 0;
				}
				
				if (queue_list_push(&session->mMediaPlaybackList, playback))
				{
					free(playback);
				}
				else
				{
					pthread_t pid;
					pthread_create(&pid, NULL, JT1078PlaybackConnectServer, playback);
				}
			}
			else
			{
				get_videoname_by_time(reqParam->mLogicChnNum, playback->filetime, filename);
				PNT_LOGE("failed to replay %s", filename);
			}
		}
	}
}

void JT1078MediaFiles_PlaybackControl(void* owner, JT808MsgBody_9202_t* ctrlParam)
{
	if (0 == ctrlParam->mLogicChnNum)
	{
		return;
	}

	JT808Session_t* session = (JT808Session_t*)owner;

	pthread_mutex_lock(&session->mMediaPlaybackList.m_mutex);
	queue_t* head = session->mMediaPlaybackList.m_head;

	while (head)
	{
		JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)head->m_content;

		if (playback)
		{
			if (ctrlParam->mLogicChnNum == playback->mLogicChannel)
			{
				if (mp4v2_demuxer_ctrl_release != playback->mp4Demuxer.state && mp4v2_demuxer_ctrl_stop != playback->mp4Demuxer.state)
				{
					mp4v2_Demuxer_SetCtrl(&playback->mp4Demuxer, ctrlParam->mPlaybackCtrl, ctrlParam->mFastSlowRate, ctrlParam->mDragBcdTime);
				}
				break;
			}
		}
		
		head = head->next;
	}

	pthread_mutex_unlock(&session->mMediaPlaybackList.m_mutex);
}

void* JT1078MediaFiles_PlaybackTask(void* pArg)
{
	pthread_detach(pthread_self());

	JT808Session_t* session = (JT808Session_t*)pArg;
	char filename[128] = { 0 };
	
	SleepMs(1000);

	while (session->mRunningFlag)
	{
		pthread_mutex_lock(&session->mMediaPlaybackList.m_mutex);
		
		queue_t* head = session->mMediaPlaybackList.m_head;
		queue_t* prehead = head, *tmp = NULL;

		while (head)
		{
			JT1078MediaPlaybackCtrl_t* playback = head->m_content;

			if (playback)
			{
				if (mp4v2_demuxer_ctrl_release == playback->mp4Demuxer.state)
				{
					if (head == session->mMediaPlaybackList.m_head)
					{
						session->mMediaPlaybackList.m_head = head->next;
						prehead = head->next;
					}
					else
					{
						prehead->next = head->next;
					}
					
					get_videoname_by_time(playback->mLogicChannel, playback->filetime, filename);
					PNT_LOGE("%s replay release", filename, playback->mp4Demuxer.state);
					
					free(playback);
					playback = NULL;
					free(head);
					head = prehead;
					continue;
				}
				else
				{
					get_videoname_by_time(playback->mLogicChannel, playback->filetime, filename);
					PNT_LOGE("%s replay state: %d", filename, playback->mp4Demuxer.state);
				}
			}

			prehead = head;
			head = head->next;
		}

		pthread_mutex_unlock(&session->mMediaPlaybackList.m_mutex);
		
		SleepMs(2000);
	}
	
	return NULL;
}

void JT1078MediaFiles_PlaybackInit(void* owner)
{
	JT808Session_t* session = (JT808Session_t*)owner;

	queue_list_create(&session->mMediaPlaybackList, 100);
		
	pthread_t pid;
	pthread_create(&pid, NULL, JT1078MediaFiles_PlaybackTask, session);
}


