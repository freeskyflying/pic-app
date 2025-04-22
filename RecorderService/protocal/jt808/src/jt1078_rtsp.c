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

#include "media_audio.h"

#include "pnt_log.h"
#include "jt808_cmd_node.h"
#include "queue_list.h"
#include "jt1078_params.h"
#include "utils.h"
#include "jt808_utils.h"
#include "jt1078_rtsp.h"
#include "jt808_session.h"
#include "media_utils.h"
#include "media_video.h"

int jt1078ConnectServer(uint8_t* url, uint16_t port)
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

		int sendSize = 8192*10;
        if (setsockopt(socketFd, SOL_SOCKET, SO_RCVBUF, &sendSize, sizeof(sendSize)) < 0)
        {
            PNT_LOGE("setsockopt SO_RCVBUF error:%s", strerror(errno));
			close(socketFd);
            return -1;
        }
		
		int recvSize = 8192*10;
        if (setsockopt(socketFd, SOL_SOCKET, SO_SNDBUF, &recvSize, sizeof(recvSize)) < 0)
        {
            PNT_LOGE("setsockopt SO_RCVBUF error:%s", strerror(errno));
			close(socketFd);
            return -1;
        }

		int nodelay = 1;
		if (setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
		{
			PNT_LOGE("setsockopt TCP_NODELAY error:%s", strerror(errno));
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

static int jt1078RtspSendRtp(JT1078Rtsp_t* rtsp, unsigned char* buffer, int len)
{
	int retry = 0;
retry_send:
	if (send(rtsp->mSocketFd, buffer, len, 0) == -1)
	{
		if (EAGAIN == errno)
		{
			retry ++;
			if (retry<3)
			{
				goto retry_send;
			}
			return 0;
		}
		return -1;
	}

	return 0;
}

void TEST_PRINTF_BUFFER(BYTE* buff, int len)
{
	char strbuff[1024] = { 0 };
	
	int end = 0, total = len;
	if (len > 128)
	{
		end = len - 8;
		len = 128;
	}
	
	memset(strbuff, 0, sizeof(strbuff));

	for (int i = 0; i<len; i++)
	{
		sprintf(strbuff, "%s %02X", strbuff, buff[i]);
	}

	if (end > 0)
	{
		sprintf(strbuff, "%s (%d) ", strbuff, total-len);
		for (int i = 0; i<8; i++)
		{
			sprintf(strbuff, "%s %02X", strbuff, buff[i+end]);
		}
	}

	PNT_LOGE("%s", strbuff);
}

static void jt1078RtspPackAndSendVideoRtp(JT1078Rtsp_t* rtsp, uint8_t* naluData, int len)
{
	uint8_t rtp[128] = { 0 };

	JT1078RtspControl_t* controller = (JT1078RtspControl_t*)rtsp->mOwner;
	JT808Session_t* session = (JT808Session_t*)controller->mOwner;

	int index = 0;
	BYTE byte15 = 0xF0, byte5 = 0;

	NetbufPushDWORD(rtp, 0x30316364, 0);
	NetbufPushBYTE(rtp, 0x81, 4);
	
	byte5 = RTP_1078_PAYLOAD_H264;
#if (defined VIDEO_PT_H265)
	if (0 == rtsp->mStreamType)
	{
		byte5 = RTP_1078_PAYLOAD_H265;
	}
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

	NetbufPushBYTE(rtp, rtsp->mLogicChannel, 14);
	
	NetbufPushDWORD(rtp, controller->mTimestamp, 20);

	NetbufPushWORD(rtp, controller->mTimestamp - rtsp->mLastIFrameTime, 24);
	NetbufPushWORD(rtp, controller->mTimestamp - rtsp->mLastFrameTime, 26);

#if (defined VIDEO_PT_H265)
	if (0 == rtsp->mStreamType)
	{
		BYTE tpye = (naluData[4]&0x7E) >> 1;
		/* 帧类别 */
		if (0x20 == tpye || 0x13 == tpye) /* IDR or I 帧 */
		{
			rtsp->mLastIFrameTime = currentTimeMillis();
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
	
	}
	else
	{
		/* 帧类别 */
		if (5 == (naluData[4] & 0x1F) || 7 == (naluData[4] & 0x1F)) /* IDR or I 帧 */
		{
			rtsp->mLastIFrameTime = currentTimeMillis();
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
	}
#else
	/* 帧类别 */
	if (5 == (naluData[4] & 0x1F) || 7 == (naluData[4] & 0x1F)) /* IDR or I 帧 */
	{
		rtsp->mLastIFrameTime = currentTimeMillis();
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
	rtsp->mLastFrameTime = currentTimeMillis();

	int offset = 0;
	int packCnt = (len+PACKET_BODY_LEN-1)/PACKET_BODY_LEN;
	int rtpDataLen = 0;

	pthread_mutex_lock(&rtsp->mWriteMutex);
	
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

		NetbufPushWORD(rtp, rtsp->mRtpSerialNum ++, 6);

		if (jt1078RtspSendRtp(rtsp, rtp, 30) == 0)
		{
			//PRINTF_BUFFER("rtphead: ", rtp, 30);
			if (jt1078RtspSendRtp(rtsp, naluData+offset, rtpDataLen) == 0)
			{
				//PRINTF_BUFFER("rtpdata: ", naluData+offset, 10);
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

	pthread_mutex_unlock(&rtsp->mWriteMutex);
}

static void jt1078RtspPackAndSendAudioRtp(JT1078Rtsp_t* rtsp, uint8_t* naluData, int len)
{
	uint8_t rtp[128] = { 0 };

	JT1078RtspControl_t* controller = (JT1078RtspControl_t*)rtsp->mOwner;
	JT808Session_t* session = (JT808Session_t*)controller->mOwner;

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
	NetbufPushBYTE(rtp, rtsp->mLogicChannel, 14);
	
	NetbufPushDWORD(rtp, controller->mTimestamp, 20);

	byte15 = 0x30;

	int packCnt = (len+PACKET_BODY_LEN-1)/PACKET_BODY_LEN;
	int rtpDataLen = 0;

	pthread_mutex_lock(&rtsp->mWriteMutex);
	
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

		NetbufPushWORD(rtp, rtsp->mRtpSerialNum ++, 6);

		if (jt1078RtspSendRtp(rtsp, rtp, 26) == 0)
		{
			if (jt1078RtspSendRtp(rtsp, naluData+(i*PACKET_BODY_LEN), rtpDataLen) == 0)
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
		
	pthread_mutex_unlock(&rtsp->mWriteMutex);
}

int isAoInit = 0;
void* jt1078RtspIntercomRecvProcess(void* pArg)
{
    pthread_detach(pthread_self());
	JT1078Rtsp_t* rtsp = (JT1078Rtsp_t*)pArg;
	JT1078RtspControl_t* controller = (JT1078RtspControl_t*)rtsp->mOwner;

	const unsigned int SAMPLERATE[] = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000, 11025, 8000, 7350, 8000, 8000, 8000, 8000};
	int chnNum = 1;
	int buffer_len = 0, read_len = 0, slider = 0;
	short rtp_data_len = 0;
	char readBuffer[2048] = { 0 };

	struct timeval tv;
	tv.tv_sec = 5; // 超时时间设置为10秒
	tv.tv_usec = 0;
	setsockopt(rtsp->mSocketFd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
	
	int nodelay = 1;
	if (setsockopt(rtsp->mSocketFd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
	{
		PNT_LOGE("setsockopt TCP_NODELAY error:%s", strerror(errno));
	}

	while (rtsp->mState == JT1078RtspStatePlaying)
	{
		if (-1 == rtsp->mSocketFd)
		{
			break;
		}
		read_len = read(rtsp->mSocketFd, readBuffer+buffer_len, sizeof(readBuffer)-buffer_len);
		if (read_len > 0)
		{
			buffer_len += read_len;
			slider = 0;
			while(buffer_len > slider+26)
			{
				if (0x30 == readBuffer[slider] && 0x31 == readBuffer[slider+1] && 0x63 == readBuffer[slider+2] && 0x64 == readBuffer[slider+3] && 0x81 == readBuffer[slider+4])
				{
					rtp_data_len = (((short)readBuffer[slider+24] << 8) & 0xFF00) + readBuffer[slider+25];
					
					if (rtp_data_len + slider + 26 <= buffer_len)
					{
						char* tmp = readBuffer + slider;
						
						if (0x93 == tmp[5] || 0x98 == tmp[5])
						{
							if (!isAoInit)
							{
								int rateIndex = (tmp[28] & 0x3C) >> 2;
								int chnNum = ((tmp[28+1] >> 6)&0x03) + (tmp[28]&0x01);
								if (!MediaAudio_DecAoStart(PT_AAC, chnNum, SAMPLERATE[rateIndex]))
								{
									isAoInit = 1;
								}
							}
							if (isAoInit)
							{
								MediaAudio_DecAoSendFrame(tmp+26, rtp_data_len);
							}
						}

						slider = slider + 26 + rtp_data_len;
					}
				}
				else
				{
					slider ++;
				}
			}

			if (buffer_len < slider)
			{
				buffer_len = 0;
			}
			else
			{
				buffer_len -= slider;
			}
		}
		else if (read_len < 0)
		{
			if (EAGAIN == errno)
			{
				continue;
			}
			PNT_LOGE("disconnect.");
			break;
		}
		else
		{
			usleep(20*1000);
		}
	}

	PNT_LOGE("intercom recv exit.");

	return NULL;
}

void* jt1078RtspSessionProcessor(void* pArg)
{
    pthread_detach(pthread_self());

	int connetCount = 0;
	int socketFd = -1;
	
	JT1078Rtsp_t* rtsp = (JT1078Rtsp_t*)pArg;
	JT1078RtspControl_t* controller = (JT1078RtspControl_t*)rtsp->mOwner;

	if (2 == rtsp->mDataType || 4 == rtsp->mDataType)
	{
		if (!isAoInit)
		{
			if (!MediaAudio_DecAoStart(PT_AAC, 1, 8000))
			{
				isAoInit = 1;
			}
		}
	}

connect:
	socketFd = jt1078ConnectServer(rtsp->mServerUrl, rtsp->mServerTCPPort);
	if (RET_FAILED == socketFd)
	{
		connetCount ++;
		if (connetCount>2)
		{
			goto exit;
		}
		else
		{
			goto connect;
		}
	}

	rtsp->mSocketFd = socketFd;

	pthread_mutex_init(&rtsp->mWriteMutex, NULL);

	rtsp->mState = JT1078RtspStatePlaying;
	
	if (2 == rtsp->mDataType || 4 == rtsp->mDataType)
	{
		pthread_t pid;
		int threadRet = pthread_create(&pid, NULL, jt1078RtspIntercomRecvProcess, rtsp);
	}

	int start = 0, wait = 0;
	unsigned long long timeStart = 0;

	FrameInfo_t frame = { 0 };
	FrameInfo_t frameAudio = { 0 };

	int nodelay = 1;
	if (setsockopt(rtsp->mSocketFd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
	{
		PNT_LOGE("setsockopt TCP_NODELAY error:%s", strerror(errno));
	}

	frame.timeStamp = get_curr_framepts(rtsp->mLogicChannel-1+rtsp->mStreamType*MAX_VIDEO_NUM);

	while (TRUE)
	{
		if (rtsp->mState == JT1078RtspStateStop)
		{
			break;
		}

		wait = 2;

		if (rtsp->mStreamState&0x01)
		{
			if (0 == get_video_stream(rtsp->mLogicChannel-1+rtsp->mStreamType*MAX_VIDEO_NUM, &frame))
			{
				if (frame.frameType == DATA_TYPE_I_FRAME || frame.frameType == DATA_TYPE_INITPACKET)
				{
					//timeStart = currentTimeMillis();
					jt1078RtspPackAndSendVideoRtp(rtsp, (unsigned char*)frame.frameBuffer, frame.frameSize);
					//skipCnt = (currentTimeMillis() - timeStart)*15/1000;
					//printf("costtime: %lld skipframe: %d.\n", (currentTimeMillis() - timeStart), skipCnt);
					start = 1;
				}
				else if (1 == start)
				{
					jt1078RtspPackAndSendVideoRtp(rtsp, (unsigned char*)frame.frameBuffer, frame.frameSize);
				}
			}
			wait --;
		}

		if (rtsp->mStreamState&0x02)
		{
			if (0 == get_audio_stream(AUDIO_BIND_CHN, &frameAudio))
			{
				jt1078RtspPackAndSendAudioRtp(rtsp, (unsigned char*)frameAudio.frameBuffer, frameAudio.frameSize);
			}
			wait --;
		}
		
		if (2 == wait)
		{
			SleepMs(20);
		}
		
		controller->mTimestamp = currentTimeMillis();
	}
	
	pthread_mutex_destroy(&rtsp->mWriteMutex);

	close(socketFd);
	rtsp->mSocketFd = -1;

	if (frame.frameBuffer)
		free(frame.frameBuffer);
	if (frameAudio.frameBuffer)
		free(frameAudio.frameBuffer);

exit:
	PNT_LOGI(" rtsp client exit.");

	if (2 == rtsp->mDataType || 4 == rtsp->mDataType)
	{
		if (isAoInit)
		{
			MediaAudio_DecAoStop(1);
			isAoInit = 0;
		}
	}

	pthread_mutex_lock(&controller->mRtspHandleList.m_mutex);

	queue_t* head = controller->mRtspHandleList.m_head;
	queue_t* pre = head;

	while (head)
	{
		if (rtsp == head->m_content)
		{
			if (controller->mRtspHandleList.m_head == head)
			{
				controller->mRtspHandleList.m_head = head->next;
			}
			else
			{
				pre->next = head->next;
			}
			free(head->m_content);
			free(head);
			break;
		}
		pre = head;
		head = head->next;
	}
	
	pthread_mutex_unlock(&controller->mRtspHandleList.m_mutex);

    pthread_exit(NULL);
    return NULL;
}

void JT1078Rtsp_ControlInit(JT1078RtspControl_t* controller, void* owner)
{
	if (NULL == controller)
	{
		PNT_LOGE("param 1 cannot be NULL.");
		return;
	}

	memset(controller, 0, sizeof(JT1078RtspControl_t));

	controller->mOwner = owner;

	queue_list_create(&controller->mRtspHandleList, 100);
}

int JT1078Rtsp_ChannelCreate(JT1078RtspControl_t* controller, JT808MsgBody_9101_t* param)
{
	int ret = RET_SUCCESS;

	if (0 == param->mTCPPort)
	{
		PNT_LOGE("jt1078 not support udp.");
		return RET_FAILED;
	}

	unsigned char streamState = 0;
	if (0 == param->mDataType || 1 == param->mDataType)
	{
		streamState |= 0x01;
	}
	if (0 == param->mDataType || 2 == param->mDataType || 3 == param->mDataType || 4 == param->mDataType)
	{
		streamState |= 0x02;
	}
	if (4 < param->mDataType)
	{
		return RET_FAILED;
	}

	pthread_mutex_lock(&controller->mRtspHandleList.m_mutex);

	queue_t* head = controller->mRtspHandleList.m_head;

	while (head)
	{
		JT1078Rtsp_t* rtsp = (JT1078Rtsp_t*)head->m_content;
		if (rtsp->mLogicChannel == param->mLogicChnNum)
		{
			if (rtsp->mDataType == param->mDataType && rtsp->mStreamType == param->mStreamType)
			{
				PNT_LOGE("Chn[%d-%d] RTSP is running.", rtsp->mLogicChannel, rtsp->mStreamType);
				pthread_mutex_unlock(&controller->mRtspHandleList.m_mutex);
				return RET_SUCCESS;
			}
		}
		
		head = head->next;
	}
	
	pthread_mutex_unlock(&controller->mRtspHandleList.m_mutex);

	JT1078Rtsp_t* rtsp = (JT1078Rtsp_t*)malloc(sizeof(JT1078Rtsp_t));

	memset(rtsp, 0, sizeof(JT1078Rtsp_t));

	rtsp->mLogicChannel = param->mLogicChnNum;
	rtsp->mServerTCPPort = param->mTCPPort;
	rtsp->mServerUDPPort = param->mUDPPort;
	strcpy((char*)rtsp->mServerUrl, (char*)param->mIp);
	rtsp->mStreamType = param->mStreamType;
	rtsp->mDataType = param->mDataType;
	rtsp->mStreamState = streamState;

	rtsp->mOwner = controller;

	queue_list_push(&controller->mRtspHandleList, rtsp);

	pthread_t pid;
	int threadRet = pthread_create(&pid, NULL, jt1078RtspSessionProcessor, rtsp);
	if (threadRet != 0)
    {
        PNT_LOGE("fail to create channel[%d] rtsp, because %s", param->mLogicChnNum, strerror(threadRet));
        ret = RET_FAILED;
    }

	return ret;
}

void JT1078Rtsp_PlayControl(JT1078RtspControl_t* controller, JT808MsgBody_9102_t* param)
{
	pthread_mutex_lock(&controller->mRtspHandleList.m_mutex);

	queue_t* head = controller->mRtspHandleList.m_head;

	while (head)
	{
		JT1078Rtsp_t* rtsp = (JT1078Rtsp_t*)head->m_content;
		if (rtsp->mLogicChannel == param->mLogicChnNum)
		{
			PNT_LOGI("Ctrl Chn[%d] RTSP cmd [%d].", rtsp->mLogicChannel, param->mCtrlCmd);
			if (0 == param->mCtrlCmd)
			{
				if (0 == param->mStopType)
				{
					rtsp->mState = JT1078RtspStateStop;
				}
				else if (1 == param->mStopType)
				{
					rtsp->mStreamState &= (~(0x02));
				}
				else if (2 == param->mStopType)
				{
					rtsp->mStreamState &= (~(0x01));
				}
				if (0 == rtsp->mStreamState)
				{
					rtsp->mState = JT1078RtspStateStop;
				}
			}
			else if (1 == param->mCtrlCmd)
			{
				rtsp->mState = JT1078RtspStatePlaying;
				if (param->mSwitchStreamType != rtsp->mStreamType)
				{
					rtsp->mStreamType = param->mSwitchStreamType;
				}
			}
			else if (2 == param->mCtrlCmd)
			{
				rtsp->mState = JT1078RtspStatePause;
			}
			else if (3 == param->mCtrlCmd)
			{
				rtsp->mState = JT1078RtspStatePlaying;
			}
			else if (4 == param->mCtrlCmd)
			{
				if (2 == rtsp->mDataType)
				{
					rtsp->mState = JT1078RtspStateStop;
				}
			}
		}
		
		head = head->next;
	}
	
	pthread_mutex_unlock(&controller->mRtspHandleList.m_mutex);
}

void JT1078Rtsp_GetMediaAttribute(JT808MsgBody_1003_t* ack)
{
	ack->mAudioChnCount = 1;
	ack->mAudioEncType = RTP_1078_PAYLOAD_AACLC;
	ack->mAudioFrameLen = 1024;
	ack->mAudioSampleBit = 1;
	ack->mAudioSampleRate = 0;
	ack->mAudioTrackCount = 1;
	ack->mIsSupportAudioOut = 1;
	ack->mVideoChnCount = MAX_VIDEO_NUM;
	ack->mVideoEncType = RTP_1078_PAYLOAD_H264;
}


