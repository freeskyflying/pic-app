#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "utils.h"
#include "pnt_log.h"
#include "jt808_utils.h"
#include "jt1078_media_files.h"

#include "media_utils.h"
#include "media_storage.h"
#include "jt808_session.h"
#include "jt1078_rtsp.h"
#include "jt1078_params.h"

static int  JT1078MediaFiles_QueryFilesByChannel(int logicChn, unsigned char* bcdTimeStart, unsigned char* bcdTimeEnd, queue_list_t* list)
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

		if ((!timeStartFlag || fileTime >= timeStart) && (!timeEndFlag || fileTime <= timeEnd))
		{
			get_videoname_by_time(logicChn, fileTime, filename);
		
			struct stat buf = { 0 };
			memset(&buf, 0, sizeof(struct stat));
			stat(filename, &buf);
				
			int duration = get_videofile_duration(filename);

			if (duration > 0 && duration <= 65)
			{
				JT808MsgBody_1205_Item_t* item = (JT808MsgBody_1205_Item_t*)malloc(sizeof(JT808MsgBody_1205_Item_t));
				if (NULL != item)
				{
					memset(item, 0, sizeof(JT808MsgBody_1205_Item_t));

					item->mLogicChnNum = logicChn;
					convertSysTimeToBCD(fileTime, item->mStartBcdTime);
					convertSysTimeToBCD(fileTime + duration, item->mEndBcdTime);

					item->mResType = 2;
					item->mFileSize = buf.st_size;
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

		usleep(50);
		
		head = head->next;
	}

	PNT_LOGE("find ")

	return count;
}

void JT1078MediaFiles_QueryFiles(JT808MsgBody_9205_t* reqParam, JT808MsgBody_1205_t* ackParam)
{
	PNT_LOGE("start query file: %d %02X%02X%02X%02X%02X%02X-%02X%02X%02X%02X%02X%02X", reqParam->mLogicChnNum,
		reqParam->mStartBcdTime[0], reqParam->mStartBcdTime[1], reqParam->mStartBcdTime[2], reqParam->mStartBcdTime[3], reqParam->mStartBcdTime[4], reqParam->mStartBcdTime[5],
		reqParam->mEndBcdTime[0], reqParam->mEndBcdTime[1], reqParam->mEndBcdTime[2], reqParam->mEndBcdTime[3], reqParam->mEndBcdTime[4], reqParam->mEndBcdTime[5]);
	
	if (0 == reqParam->mLogicChnNum)
	{
		for (int i=1; i<=MAX_VIDEO_NUM; i++)
		{
			ackParam->mResCount += JT1078MediaFiles_QueryFilesByChannel(i, reqParam->mStartBcdTime, reqParam->mEndBcdTime, &ackParam->mResList);
		}
	}
	else
	{
		ackParam->mResCount = JT1078MediaFiles_QueryFilesByChannel(reqParam->mLogicChnNum, reqParam->mStartBcdTime, reqParam->mEndBcdTime, &ackParam->mResList);
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

static void jt1078PlaybackPackAndSendVideoRtp(JT1078MediaPlaybackCtrl_t* playback, uint8_t* naluData, int len, unsigned int frameTime)
{
	uint8_t rtp[30] = { 0 };

	JT808Session_t* session = (JT808Session_t*)playback->owner;

	int index = 0;
	BYTE byte15 = 0, byte5 = 0;

	NetbufPushDWORD(rtp, 0x30316364, 0);
	NetbufPushBYTE(rtp, 0x81, 4);

	byte5 = RTP_1078_PAYLOAD_H264;
	NetbufPushBYTE(rtp, byte5, 5);
	NetbufPushBuffer(rtp, session->mPhoneNum, 8, 6);
	NetbufPushBYTE(rtp, playback->mLogicChannel, 14);
	
	NetbufPushDWORD(rtp, frameTime, 20);

	NetbufPushWORD(rtp, currentTimeMillis() - playback->mLastIFrameTime, 24);
	NetbufPushWORD(rtp, currentTimeMillis() - playback->mLastFrameTime, 26);
	
	/* 帧类别 */
	if (5 == (naluData[4] & 0x1F) || 7 == (naluData[4] & 0x1F)) /* IDR or I 帧 */
	{
		playback->mLastIFrameTime = currentTimeMillis();
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

	playback->mLastFrameTime = currentTimeMillis();

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

		if (jt1078PlaybackSendRtp(playback, rtp, 30) == 0)
		{
			if (jt1078PlaybackSendRtp(playback, naluData+offset, rtpDataLen) == 0)
			{
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}

		offset += rtpDataLen;
	}

	pthread_mutex_unlock(&playback->mutex);
}

static void jt1078PlaybackPackAndSendAudioRtp(JT1078MediaPlaybackCtrl_t* playback, uint8_t* naluData, int len, unsigned int frameTime)
{
	uint8_t rtp[30] = { 0 };

	JT808Session_t* session = (JT808Session_t*)playback->owner;

	int index = 0;
	BYTE byte15 = 0, byte5 = 0;

	NetbufPushDWORD(rtp, 0x30316364, 0);
	NetbufPushBYTE(rtp, 0x81, 4);
	
	byte5 = RTP_1078_PAYLOAD_AACLC;
	NetbufPushBYTE(rtp, byte5, 5);
	NetbufPushBuffer(rtp, session->mPhoneNum, 8, 6);
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

		if (jt1078PlaybackSendRtp(playback, rtp, 26) == 0)
		{
			if (jt1078PlaybackSendRtp(playback, naluData+(i*PACKET_BODY_LEN), rtpDataLen) == 0)
			{
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
		
	pthread_mutex_unlock(&playback->mutex);
}

int PlaybackVideoEs(unsigned char* frame, int len, void* owner, unsigned int frameTime)
{
	JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)owner;

	long long timestart = currentTimeMillis();

//	PNT_LOGE("send video: %02X %02X %02X %02X %02X  %d", frame[0], frame[1], frame[2], frame[3], frame[4], len);

	jt1078PlaybackPackAndSendVideoRtp(playback, frame, len, frameTime);

	return (currentTimeMillis()-timestart);
}

int PlaybackAudioEs(unsigned char* frame, int len, void* owner, unsigned int frameTime)
{
	JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)owner;

	long long timestart = currentTimeMillis();

	jt1078PlaybackPackAndSendAudioRtp(playback, frame, len, frameTime);

	return (currentTimeMillis()-timestart);
}

void* JT1078PlaybackConnectServer(void* pArg)
{
	pthread_detach(pthread_self());
	
	JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)pArg;

	int socketFd = -1, connectCount = 0;
	
connect:
	socketFd = jt1078ConnectServer(playback->serverAddr, playback->serverPort);
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

	char filename[128] = { 0 };
	
	get_videoname_by_time(playback->mLogicChannel, playback->filetime, filename);

	playback->mp4Demuxer.startTimestamp = playback->filetime;
	mp4v2_Demuxer_Start(&playback->mp4Demuxer, filename);

exit:

	return NULL;
}

void JT1078MediaFiles_PlaybackRequest(JT1078MediaPlaybackCtrl_t* playback, JT808MsgBody_9201_t* reqParam, JT808MsgBody_1205_t* ackParam)
{
	if (0 == reqParam->mLogicChnNum)
	{
		return;
	}
	
	ackParam->mResCount = JT1078MediaFiles_QueryFilesByChannel(reqParam->mLogicChnNum, reqParam->mStartBcdTime, reqParam->mEndBcdTime, &ackParam->mResList);

	if (ackParam->mResCount > 0)
	{
		JT808MsgBody_1205_Item_t* item = (JT808MsgBody_1205_Item_t*)ackParam->mResList.m_head->m_content;
		if (NULL != item)
		{
			playback->filetime = convertBCDToSysTime(item->mStartBcdTime);

			playback->mRtpSerialNum = 0;
			playback->mLogicChannel = reqParam->mLogicChnNum;
			strcpy((char*)playback->serverAddr, (char*)reqParam->mIp);
			playback->serverPort = reqParam->mTCPPort;

			mp4v2_Demuxer_SetCtrl(&playback->mp4Demuxer, reqParam->mPlaybackMode, reqParam->mFastSlowRate, reqParam->mStartBcdTime);

			pthread_t pid;
			pthread_create(&pid, NULL, JT1078PlaybackConnectServer, playback);
		}
	}
}

void JT1078MediaFiles_PlaybackControl(JT1078MediaPlaybackCtrl_t* playback, JT808MsgBody_9202_t* ctrlParam)
{
	if (0 == ctrlParam->mLogicChnNum)
	{
		return;
	}

	mp4v2_Demuxer_SetCtrl(&playback->mp4Demuxer, ctrlParam->mPlaybackCtrl, ctrlParam->mFastSlowRate, ctrlParam->mDragBcdTime);
}

void JT1078MediaFiles_PlaybackInit(JT1078MediaPlaybackCtrl_t* playback, void* owner)
{
	playback->owner = owner;

	pthread_mutex_init(&playback->mutex, NULL);

	memset(&playback->mp4Demuxer, 0, sizeof(playback->mp4Demuxer));

	playback->mp4Demuxer.vidCb = PlaybackVideoEs;
	playback->mp4Demuxer.audCb = PlaybackAudioEs;
	playback->mp4Demuxer.owner = playback;
}


