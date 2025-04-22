#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>

#include <errno.h>
#include <sys/prctl.h>
#include "dirent.h"
#include <pthread.h>

#include "pnt_log.h"
#include "media_snap.h"
#include "jt808_utils.h"
#include "jt808_session.h"
#include "jt808_activesafety.h"
#include "jt808_controller.h"
#include "jt808_activesafety_upload.h"

static const JT808_CmdNode_t JT808ACTIVESAFETYCMD_SUPPORT_LIST[] =
{
    ADD_CMD_NODE(0001, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8001, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(1210, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(1211, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(1212, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9212, NULL, CMD_TYPE_RESPONSE),
};

static int setSocketConfig(int fd)
{
    struct timeval timeoutRecv = {3, 0};
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeoutRecv, sizeof(struct timeval)) < 0)
    {
        PNT_LOGE("setsockopt SO_RCVTIMEO error:%s", strerror(errno));
        return -1;
    }
	
    struct timeval timeoutSend = {3, 0};
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeoutSend, sizeof(struct timeval)) < 0)
    {
        PNT_LOGE("setsockopt SO_SNDTIMEO error:%s", strerror(errno));
        return -1;
    }
	
    int nodelay = 1;
    if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay)) < 0)
    {
        PNT_LOGE("setsockopt TCP_NODELAY error:%s", strerror(errno));
    }
		
    return 1;
}

static int bSockBindAddress(int *fd, const char *host, int port, struct addrinfo *addrs)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = IPPROTO_TCP;

    char portStr[16] = {0};
    sprintf(portStr, "%d", port);
    int result = 0;
    PNT_LOGD("BaseSocket start to get the address of %s:%d", host, port);
    result = getaddrinfo(host, portStr, &hints, &addrs);
    if (result != 0)
    {
        PNT_LOGE("BaseSocket fatal state error!!! fail to get the address info of %s:%d", host, port);
        return RET_FAILED;
    }

    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next)
    {
        *fd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (*fd < 0)
        {
            PNT_LOGE("BaseSocket fatal state error!!! create socket of [%s:%d] error, caused by %s", host, port,
                    strerror(errno));
            freeaddrinfo(addrs);
            return RET_FAILED;
        }

        PNT_LOGD("BaseSocket the socketFd of [%s:%d] are :%d", host, port, *fd);
        setSocketConfig(*fd);
//        setNonBlocking(*fd);

        if (connect(*fd, addr->ai_addr, addr->ai_addrlen) < 0)
        {
            if (errno != EINPROGRESS)
            {
                PNT_LOGE("BaseSocket fatal state error!!! connect to [%s:%d] error! errno:%d %s", host, port, errno, strerror(errno));
                freeaddrinfo(addrs);
                return RET_FAILED;
            }
            else
            {
                PNT_LOGE("BaseSocket connection to [%s:%d] may established", host, port);
                break;
            }
        }
    }
    freeaddrinfo(addrs);

    return RET_SUCCESS;
}

static int bSockConnectSock(int *fd, char* host, int port)
{
    struct addrinfo *addrs = NULL;

    if (RET_FAILED == bSockBindAddress(fd, host, port, addrs))
    {
        PNT_LOGE("Socket[%s:%d] Bind failed.", host, port);
        close(*fd);
        return RET_FAILED;
    }

    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(*fd, &readfds);
    FD_SET(*fd, &writefds);
    timeout.tv_sec = 3; //timeout is 5 second
    timeout.tv_usec = 0;

    int ret = select(*fd + 1, &readfds, &writefds, NULL, &timeout);
    if (ret <= 0)
    {
        PNT_LOGE("connection to [%s:%d] time out", host, port);
        return RET_FAILED;
    }

    if (FD_ISSET(*fd, &readfds) ||FD_ISSET(*fd, &writefds))
    {
        int error = 0;
        socklen_t length = sizeof(error);
        if (getsockopt(*fd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
        {
            PNT_LOGE("get socket of [%s:%d] option failed", host, port);
            return RET_FAILED;
        }

        if (error != 0)
        {
            PNT_LOGE("connection of [%s:%d] failed after select with the error: %d %s", host, port, error, strerror(error));
            return RET_FAILED;
        }
    }
    else
    {
        PNT_LOGE("connection [%s:%d] no events on sockfd found", host, port);
        return RET_FAILED;
    }

    FD_CLR(*fd, &readfds);
    FD_CLR(*fd, &writefds);

    return RET_SUCCESS;
}

int JT808Activesafety_SendMsg(JT808ActivesafetyUploadHandle_t* handle, WORD msgId, void* msgBody)
{
	char bodyBuff[1024] = { 0 }, msgBuff[1024] = { 0 };
	int msgBodyLen = 0, i = 0, bodyOffset = 0, sendLen = 0;
	JT808MsgHeader_t msgHead = { 0 };
	JT808Session_t* session = (JT808Session_t*)handle->owner;

	if (NULL == session)
	{
		return RET_FAILED;
	}
	
	for (i = 0; i<sizeof(JT808ACTIVESAFETYCMD_SUPPORT_LIST)/sizeof(JT808ACTIVESAFETYCMD_SUPPORT_LIST[0]); i++)
	{
		if (msgId == JT808ACTIVESAFETYCMD_SUPPORT_LIST[i].mMsgID)
		{
			msgBodyLen = JT808ACTIVESAFETYCMD_SUPPORT_LIST[i].mEncoder(msgBody, (uint8_t*)bodyBuff, 0, (JT808Session_t*)handle->owner);
			break;
		}
	}

	msgHead.mMsgID = msgId;
	
	msgHead.mProtocalVer = 1;
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		msgHead.mMsgAttribute |= (1<<14);
	}
	
	msgHead.mMsgAttribute |= (msgBodyLen&0x3FF);
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		bodyOffset = JT808MSG_BODY_OFFSET(msgHead.mMsgAttribute) + PHONE_NUM_LEN_2019;
		memcpy(msgHead.mPhoneNum, session->mPhoneNum, PHONE_NUM_LEN_2019);
	}
	else
	{
		bodyOffset = JT808MSG_BODY_OFFSET(msgHead.mMsgAttribute) + PHONE_NUM_LEN;
		memcpy(msgHead.mPhoneNum, session->mPhoneNum, PHONE_NUM_LEN);
	}

	msgHead.mMsgSerialNum = handle->msgSerialNum ++;
	
	JT808_HEAD_Encoder(&msgHead, (BYTE*)msgBuff+1, sizeof(msgBuff), session);

	if (msgBodyLen > 0)
	{
		memcpy(msgBuff+bodyOffset, bodyBuff, msgBodyLen);
	}
	
	msgBuff[0] = JT808_MSG_FLAG;
	msgBuff[bodyOffset+msgBodyLen] = JT808CalcCrcTerminal((BYTE*)msgBuff+1, bodyOffset+msgBodyLen - 1);
	msgBuff[bodyOffset+msgBodyLen+1] = JT808_MSG_FLAG;

	// 转义
	bodyBuff[0] = JT808_MSG_FLAG;
	sendLen = JT808MsgConvert(msgBuff+1, msgBodyLen+bodyOffset, (char*)bodyBuff+1, 2048, 1);
	bodyBuff[sendLen+1] = JT808_MSG_FLAG;
	
	// 发送
	PRINTF_BUFFER(session, (BYTE*)bodyBuff, sendLen+2);
	int ret = write(handle->socketFd, bodyBuff, sendLen+2);

	if (ret < 0)
	{
		handle->mState = upload_state_exit;
	}

	return ret;
}

void JT808Activesafety_UploadFile(JT808ActivesafetyUploadHandle_t* handle)
{
	BYTE buffer[2048+62] = { 0 };
	JT808MsgBody_1210_File_t* item = &handle->fileList[handle->currenFileIdx];

	int index = 0, dataLen = 0, fileOffset = 0;

	index = NetbufPushDWORD(buffer, 0x30316364, index);
	index = NetbufPushBuffer(buffer, item->mFilename, index, item->mFilenameLen);
	
	FILE* pfile = fopen((char*)item->mFilenameOrg, "rb");
	int retry = 0, ret = 0;
	if (pfile != 0)
	{
		PNT_LOGE("start to upload %s", item->mFilenameOrg);
		while (handle->mState != upload_state_exit)
		{
			retry = 0;
			dataLen = fread(buffer+62, 1, 2048, pfile);
			if (dataLen <= 0)
			{
				PNT_LOGE("finish upload %s [%d]", item->mFilenameOrg, handle->mState);
				break;
			}
			
			index = 54;
			index = NetbufPushDWORD(buffer, fileOffset, index);
			index = NetbufPushDWORD(buffer, dataLen, index);
retry_write:
			ret = write(handle->socketFd, buffer, (dataLen+62));
			if (ret < 0)
			{
				if (EAGAIN == errno)
				{
					retry ++;
					if (retry < 3)
						goto retry_write;
					else
					{
						fileOffset += dataLen;
						continue;
					}
				}
				PNT_LOGE("upload %s failed [%d] %s", item->mFilenameOrg, ret, strerror(errno));
				handle->mState = upload_state_exit;
				break;
			}
			fileOffset += dataLen;
		}
		fclose(pfile);
	}
}

void JT808ActiveSafety_Send1211(JT808ActivesafetyUploadHandle_t* handle)
{
	JT808MsgBody_1211_t msgBody = { 0 };
	JT808MsgBody_1210_File_t* curFile = &handle->fileList[handle->currenFileIdx];

	strcpy((char*)msgBody.mFilename, (char*)curFile->mFilename);
	msgBody.mFilenameLen = curFile->mFilenameLen;
	msgBody.mFilesize = curFile->mFilesize;
	msgBody.mFileType = curFile->mFileType;
	
	JT808Activesafety_SendMsg(handle, 0x1211, &msgBody);
}

void JT808ActiveSafety_Send1212(JT808ActivesafetyUploadHandle_t* handle)
{
	JT808MsgBody_1212_t msgBody = { 0 };
	JT808MsgBody_1210_File_t* curFile = &handle->fileList[handle->currenFileIdx];

	PNT_LOGE("upload 0x1212 %s", curFile->mFilenameOrg);

	strcpy((char*)msgBody.mFilename, (char*)curFile->mFilename);
	msgBody.mFilenameLen = curFile->mFilenameLen;
	msgBody.mFilesize = curFile->mFilesize;
	msgBody.mFileType = curFile->mFileType;
	
	JT808Activesafety_SendMsg(handle, 0x1212, &msgBody);
}

int JT808Activesafety_RecvMsgProcess(JT808ActivesafetyUploadHandle_t* handle, JT808MsgHeader_t* msgHead, void* msgBody)
{
	
	switch (msgHead->mMsgID)
	{
		case 0x8001:	// 平台通用应答
			{
				JT808MsgBody_8001_t* param = (JT808MsgBody_8001_t*)msgBody;

				switch (param->mMsgID)
				{
					case 0x1210:
						{
							JT808ActiveSafety_Send1211(handle);
							handle->mState = upload_state_report_fileinfo;
						}
						break;
					
					case 0x1211:
						{
							// 发送文件包
							JT808Activesafety_UploadFile(handle);
							if (handle->mState != upload_state_exit)
							{
								JT808ActiveSafety_Send1212(handle);
							}
						}
						break;
				}
			}
			break;
					
		case 0x9212:
			{
				JT808MsgBody_9212_t* param=(JT808MsgBody_9212_t*)msgBody;

				if (0 == param->mResult)
				{
					handle->currenFileIdx ++;
				}
				if (handle->currenFileIdx < handle->currentFileCount && !access((char*)handle->fileList[handle->currenFileIdx].mFilenameOrg, R_OK)/*±ÜÃâÎÄ¼þ²»´æÔÚµ¼ÖÂÒ»Ö±ÉÏ±¨*/)
				{
					usleep(100*1000);
					JT808ActiveSafety_Send1211(handle);
				}
				else
				{
					handle->mState = upload_state_exit;
				}
			}
			break;
	}

	return RET_SUCCESS;
}

int JT808Activesafety_RcvDataProcess(char* buff, int len, void* arg)
{
    JT808ActivesafetyUploadHandle_t* handle = (JT808ActivesafetyUploadHandle_t*)arg;
	JT808Session_t* session = (JT808Session_t*)handle->owner;

    char msg_buffer[2048] = { 0 };
    int i =0, passed_byte = 0, head_flag = 0, start_pos = 0;

    for (i = 0; i<len; i++)
    {
        if (JT808_MSG_FLAG == buff[i])
        {
            if (0 == head_flag)
            {
                head_flag = 1;
                start_pos = i;
            }
            else
            {
                int msg_len = JT808MsgConvert(buff+start_pos+1, (i - start_pos - 1), msg_buffer, 2048, 0/* 1 转义 0 还原 */);

				PRINTF_BUFFER(session, (BYTE*)msg_buffer, msg_len);
				
				JT808MsgHeader_t* head = NULL;
				
				int offset = JT808_HEAD_Parser((BYTE*)msg_buffer, (void**)&head, len, (JT808Session_t*)handle->owner);
				if (RET_FAILED != offset)
				{
					BYTE crc = JT808CalcCrcTerminal((BYTE*)msg_buffer, len - 1);
					if (msg_buffer[len-1] != crc)
					{
					}
					else
					{
						int i = 0;
						for (i = 0; i<sizeof(JT808ACTIVESAFETYCMD_SUPPORT_LIST)/sizeof(JT808ACTIVESAFETYCMD_SUPPORT_LIST[0]); i++)
						{
							if (head->mMsgID == JT808ACTIVESAFETYCMD_SUPPORT_LIST[i].mMsgID)
							{
								void* param = NULL;
					
								JT808ACTIVESAFETYCMD_SUPPORT_LIST[i].mParser((BYTE*)(msg_buffer+offset), (void**)&param, len - offset - 1, (JT808Session_t*)handle->owner);

								JT808Activesafety_RecvMsgProcess(handle, head, param);
								
								if (NULL != param)
								{
									free(param);
									param = NULL;
								}
					
								break;
							}
						}
					}
				}
				
				if (NULL != head)
				{
					free(head);
					head = NULL;
				}

                head_flag = 0;
                passed_byte += (i - start_pos + 1);
            }
        }
        else if (1 == head_flag)
        {
        }
        else
        {
            passed_byte ++;
        }
    }

    return passed_byte;
}

void JT808Activesafety_GetReportFilename(char fileType, char chn, char alertType, char seq, char* alertID, char* outname)
{
	char id[33] = { 0 }, fix[6] = { 0 };

	memcpy(id, alertID, 32);
	id[32] = 0;

	if (fileType==FILETYPE_PHOTO)
	{
		strcpy(fix, "jpg");
	}
	else if (fileType==FILETYPE_VOICE)
	{
		strcpy(fix, "wav");
	}
	else if (fileType==FILETYPE_VIDEO)
	{
		strcpy(fix, "mp4");
	}
	else if (fileType==FILETYPE_TEXT)
	{
		strcpy(fix, "bin");
	}
	
	sprintf(outname, "%02d_%02X_%02X_%02X_%s.%s",
			fileType,
			chn,
			alertType,
			seq,
			id,
			fix);
}

int JT808Activesafety_InsertRecord(JT808ActivesafetyUploadHandle_t* handle, BYTE* ip, int port, BYTE* alertID, BYTE* alertFlag)
{
	pthread_mutex_lock(&handle->mMutex);

    char sql[1024] = {0};
    bool_t flag = TRUE;

    sqlite3_stmt *stmt = NULL;

    sprintf(sql, kSqlInsertOneUpload, ip, port, alertID, 0);
	
    if(sqlite3_prepare_v2(handle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(handle->mDB), __FUNCTION__);
		pthread_mutex_unlock(&handle->mMutex);
        return -1;
    }

    sqlite3_exec(handle->mDB, "begin", 0, 0, NULL);
    sqlite3_bind_blob(stmt,1, alertFlag, 16, NULL);
    int ret = sqlite3_step(stmt);
	
    if(ret != SQLITE_DONE)
	{
        PNT_LOGE("sqlite3_step error!%d", ret);
        flag = FALSE;
    }
	
    if(!flag)
	{
        sqlite3_exec(handle->mDB, "rollback", 0, 0, NULL);;
    }
	else
	{
        sqlite3_exec(handle->mDB, "commit", 0, 0, NULL);
    }
    sqlite3_finalize(stmt);

	unsigned int mediaId = sqlite3_last_insert_rowid(handle->mDB);;

	pthread_mutex_unlock(&handle->mMutex);

	return mediaId;
}

void JT808Activesafety_DeleteRecordByMediaID(JT808ActivesafetyUploadHandle_t* handle, DWORD mediaId)
{
    char sql[1024] = {0};
	
	pthread_mutex_lock(&handle->mMutex);
	
	sprintf(sql, kSqlDeleteOneUpload, mediaId);
    ExecSql(handle->mDB, sql, NULL, NULL);
	
	pthread_mutex_unlock(&handle->mMutex);
}

void JT808Activesafety_GetUploadList(JT808ActivesafetyUploadHandle_t* handle)
{
	pthread_mutex_lock(&handle->mMutex);
	
    char sql[1024] = {0};
	sprintf(sql, kQueryUploadBySeq, handle->reqList.m_max_count);
	
    sqlite3_stmt *stmt = NULL;

    if(sqlite3_prepare_v2(handle->mDB, sql, strlen(sql), &stmt, NULL) != SQLITE_OK)
	{
        PNT_LOGE("sqlite3_prepare_v2 error! %s %s", sqlite3_errmsg(handle->mDB), __FUNCTION__);
    }
	else
	{
	    while(sqlite3_step(stmt) == SQLITE_ROW)
		{
			JT808ActivesafetyRequest_t* item = (JT808ActivesafetyRequest_t*)malloc(sizeof(JT808ActivesafetyRequest_t));

			if (NULL == item)
			{
				continue;
			}
			memset(item, 0, sizeof(JT808ActivesafetyRequest_t));
			
	        int column = sqlite3_column_count(stmt);
			
	        for(int i = 0; i < column; i++)
			{
	            const char* name = sqlite3_column_name(stmt, i);
	            if(!strcmp(name, "id"))
				{
	                item->mID = sqlite3_column_int(stmt, i);
	            }
				else if(!strcmp(name, "port"))
				{
	                item->mTCPPort = sqlite3_column_int(stmt, i);
	            }
				else if(!strcmp(name, "ip"))
				{
	                const unsigned char* data = sqlite3_column_text(stmt, i);
					strcpy((char*)item->mIp, (char*)data);
	            }
				else if(!strcmp(name, "alertID"))
				{
	                const unsigned char* data = sqlite3_column_text(stmt, i);
					strcpy((char*)item->mAlertID, (char*)data);
	            }
				else if(!strcmp(name, "alertFlag"))
				{
	                const void* data = sqlite3_column_blob(stmt, i);
					memcpy(item->mAlertFlag, data, sizeof(item->mAlertFlag));
	            }
	        }
			
			if (queue_list_push(&handle->reqList, item))
			{
				free(item);
				item = NULL;
			}
	    }
		
	    sqlite3_finalize(stmt);
	}
	
	pthread_mutex_unlock(&handle->mMutex);
}

void* activesafety_upload_task(void* pArg)
{
    char prname[30]={0};
	JT808ActivesafetyUploadHandle_t* handle = (JT808ActivesafetyUploadHandle_t*)pArg;
	JT808Session_t* session = (JT808Session_t*)handle->owner;
	
    sprintf(prname,"activeSafetyUpload");
    prctl(PR_SET_NAME, prname, 0,0,0);

	int reconnect_times = 0;

	char recvBuffer[1024] = { 0 };
		
	JT808MsgBody_1210_t msg1210 = { 0 };
	jt808_alert_flag_t alertFlag = { 0 };
	char filename[128] = { 0 };
	int index = 0, photoCnt = 0, dataLen = 0;

	msg1210.mFileList = (JT808MsgBody_1210_File_t*)malloc(sizeof(JT808MsgBody_1210_File_t)*11);
	handle->fileList = msg1210.mFileList;

	while (session->mRunningFlag)
	{
		index = 0;
		reconnect_times = 0;
		
		JT808ActivesafetyRequest_t* req = (JT808ActivesafetyRequest_t*)queue_list_popup(&handle->reqList);
		if (NULL == req)
		{
			JT808Activesafety_GetUploadList(handle);
			sleep(1);
			continue;
		}
		
		index = NetbufPopBuffer(req->mAlertFlag, alertFlag.m_dev_id, index, 7);
		index = NetbufPopBuffer(req->mAlertFlag, alertFlag.m_datetime, index, 6);
		index = NetbufPopBYTE(req->mAlertFlag, &alertFlag.m_event_id, index);
		index = NetbufPopBYTE(req->mAlertFlag, &alertFlag.m_files_cnt, index);
		index = NetbufPopBYTE(req->mAlertFlag, &alertFlag.m_resv, index);

		char alertType = alertFlag.m_resv & 0x3F;
		char alertChn = (alertFlag.m_resv&ACTIVESAFETY_ALARM_ADAS)?0x64:0x65;

		time_t timeAlert = convertBCDToSysTime(alertFlag.m_datetime);
		
		char timebcd[6] = { 0 };
		getLocalBCDTime(timebcd);
		time_t now = convertBCDToSysTime((uint8_t*)timebcd);		
		if (now < 20 + timeAlert) // 报警附件30s后上传
		{
			queue_list_push(&handle->reqList, req);
			usleep(100*1000);
			continue;
		}

		PNT_LOGE("start to report alarm [20%02x%02x%02x-%02x%02x%02x]", alertFlag.m_datetime[0], alertFlag.m_datetime[1], alertFlag.m_datetime[2]
																		, alertFlag.m_datetime[3], alertFlag.m_datetime[4], alertFlag.m_datetime[5]);

		memcpy(msg1210.mTerminalID, alertFlag.m_dev_id, sizeof(msg1210.mTerminalID));
		memcpy(msg1210.mAlertFlag, req->mAlertFlag, sizeof(msg1210.mAlertFlag));
		memcpy(msg1210.mAlertID, req->mAlertID, sizeof(msg1210.mAlertID));
		msg1210.mInfoType = 0;
		msg1210.mFileCount = 0;

		JT808MsgBody_1210_File_t* fileItem = NULL;

		photoCnt = alertFlag.m_files_cnt;
		
		JT808ActiveSafety_GetFilename(&alertFlag, filename, FILETYPE_VIDEO);
		if (0 == access(filename, R_OK))
		{
			fileItem = &msg1210.mFileList[msg1210.mFileCount];
			
			strcpy((char*)fileItem->mFilenameOrg, filename);
			fileItem->mFilesize = getFileSize((const char*)fileItem->mFilenameOrg);

			JT808Activesafety_GetReportFilename(FILETYPE_VIDEO, alertChn, alertType, 0,(char*)req->mAlertID, (char*)fileItem->mFilename);
			fileItem->mFilenameLen = strlen((char*)fileItem->mFilename);
			fileItem->mFileType = FILETYPE_VIDEO;
			
			msg1210.mFileCount ++;
			photoCnt -= 1;
		}
		else
		{
			PNT_LOGE("%s file not exit.", filename);
		}
		
		JT808ActiveSafety_GetFilename(&alertFlag, filename, FILETYPE_PHOTO);
		for (int i=0; i<photoCnt; i++)
		{
			fileItem = &msg1210.mFileList[msg1210.mFileCount];
			
			MediaSnap_RenameBySequence((char*)fileItem->mFilenameOrg, filename, photoCnt, i);
			
			if (0 == access((char*)fileItem->mFilenameOrg, R_OK))
			{
				fileItem->mFilesize = getFileSize((const char*)fileItem->mFilenameOrg);
				
				JT808Activesafety_GetReportFilename(FILETYPE_PHOTO, alertChn, alertType, i, (char*)req->mAlertID, (char*)fileItem->mFilename);
				fileItem->mFilenameLen = strlen((char*)fileItem->mFilename);
				fileItem->mFileType = FILETYPE_PHOTO;
				
				msg1210.mFileCount ++;
			}
			else
			{
				PNT_LOGE("%s file not exit.", fileItem->mFilenameOrg);
			}
		}

		if (0 == msg1210.mFileCount)
		{
			JT808Activesafety_DeleteRecordByMediaID(handle, req->mID);
			free(req);
			req = NULL;
			usleep(100*1000);
			continue;
		}
		
		handle->currenFileIdx = 0;
		handle->currentFileCount = msg1210.mFileCount;
		
reconnect:

		handle->socketFd = 0;
		if (RET_SUCCESS != bSockConnectSock(&handle->socketFd, req->mIp, req->mTCPPort))
		{
			close(handle->socketFd);
			SleepSs(3);
			reconnect_times ++;
			if (reconnect_times > 3)
			{
				queue_list_push(&handle->reqList, req);
				usleep(100*1000);
				continue;
			}
			goto reconnect;
		}

		JT808Activesafety_SendMsg(handle, 0x1210, &msg1210);

		handle->mState = upload_state_report_attinfo;

		struct timeval timeout = {3,0};
		char time_out_cnt = 0;
		while (handle->mState != upload_state_exit)
		{
			setsockopt(handle->socketFd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
			setsockopt(handle->socketFd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(struct timeval));
			
			int readLen = recv(handle->socketFd, recvBuffer+dataLen, 1024-dataLen, 0);

			if (readLen > 0)
			{
				dataLen += readLen;				
				int parsed = JT808Activesafety_RcvDataProcess(recvBuffer, dataLen, handle);
				if (parsed < dataLen)
				{
					memcpy(recvBuffer, recvBuffer+parsed, dataLen-parsed);
				}
				dataLen -= parsed;
			}
			else //if (time_out_cnt >= 5)
			{
				time_out_cnt ++;
				if (time_out_cnt >= 5)
				{
					PNT_LOGE("%s:%d timeout , reconnect.", req->mIp, req->mTCPPort);
					close(handle->socketFd);
					goto reconnect;
				}
			}
		}

		close(handle->socketFd);
		handle->socketFd = -1;
		
		pthread_mutex_lock(&handle->reqList.m_mutex);
		queue_t* head = handle->reqList.m_head;
		while (head)
		{
			JT808ActivesafetyRequest_t* node = (JT808ActivesafetyRequest_t*)head->m_content;
			if (NULL != node)
			{
				if (0 == memcmp(node->mAlertFlag, req->mAlertFlag, sizeof(req->mAlertFlag)))
				{
					PNT_LOGE("%s alarm had uploaded.", node->mAlertID);
					JT808Activesafety_DeleteRecordByMediaID(handle, node->mID);

					if (head != handle->reqList.m_head)
					{
						head->prev->next = head->next;
					}
					else
					{
						handle->reqList.m_head = head->next;
					}
					
					queue_t* tmp = head;
					head = head->next;
					
					free(tmp->m_content);
					free(tmp);
					tmp->m_content = NULL;
					tmp = NULL;
					
					continue;
				}
			}
			head = head->next;
		}
		pthread_mutex_unlock(&handle->reqList.m_mutex);

		if (handle->currenFileIdx >= handle->currentFileCount || req->mRetryCount >= 3)
		{
			JT808Activesafety_DeleteRecordByMediaID(handle, req->mID);
			free(req);
			req = NULL;
		}
		else
		{
			req->mRetryCount ++;
			queue_list_push(&handle->reqList, req);
			usleep(100*1000);
			continue;
		}
	}

	PNT_LOGI("task exit.");

	return NULL;
}
	
int JT808ActiveSafety_GetVersion(JT808ActivesafetyUploadHandle_t* handle)
{
	sqlite3_stmt *stmt = NULL;
	
	if (sqlite3_prepare_v2(handle->mDB, kSqlUploadGetVersion, strlen(kSqlUploadGetVersion), &stmt, NULL) != SQLITE_OK)
	{
		PNT_LOGE("sqlite3_prepare_v2 error!");
		return -1;
	}
	if (sqlite3_step(stmt) != SQLITE_ROW)
	{
		PNT_LOGE("sqlite3_step!");
		return -1;
	}

	int version = sqlite3_column_int(stmt, 0);
	
	sqlite3_finalize(stmt);
	
	return version;
}

void JT808ActiveSafety_SetVersion(JT808ActivesafetyUploadHandle_t* handle, int version)
{
	char sql[1024] = {0};
	
	sprintf(sql, kSqlUploadSetVersion, version);
	
	ExecSql(handle->mDB, sql, NULL, NULL);
}

void JT808ActiveSafety_UploadInit(JT808ActivesafetyUploadHandle_t * handle, void* owner)
{
	handle->owner = owner;

	JT808Session_t* session = (JT808Session_t*)handle->owner;
	
	pthread_mutex_init(&handle->mMutex, NULL);

	char dbFilename[128] = { 0 };

	sprintf(dbFilename, JT808ACTIVE_DATABASE_UPLOAD, session->mSessionData->mAddress);

	int ret = sqlite3_open_v2(dbFilename, &handle->mDB, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	int oldVersion = JT808ActiveSafety_GetVersion(handle);

	if (oldVersion != -1 && oldVersion < JT808ACTIVE_UPLOAD_DB_VERSION)
	{
        ExecSql(handle->mDB, kSqlDropUploadTable, NULL, NULL);
		
        JT808ActiveSafety_SetVersion(handle, JT808ACTIVE_UPLOAD_DB_VERSION);
	}

	if (SQLITE_OK != ret)
	{
		PNT_LOGE("open/create DB %s failed.", dbFilename);
		return;
	}

	PNT_LOGI("start to create session table");
	ExecSql(handle->mDB, kSqlCreateUploadTable, NULL, NULL);

	queue_list_create(&handle->reqList, 20);

	pthread_t pid;
	
	pthread_create(&pid, NULL, activesafety_upload_task, handle);
}

void JT808ActiveSafety_UploadRequest(JT808ActivesafetyUploadHandle_t* handle, JT808MsgBody_9208_t* req)
{
	pthread_mutex_lock(&handle->reqList.m_mutex);
	queue_t* head = handle->reqList.m_head;
	while (head)
	{
		JT808ActivesafetyRequest_t* node = (JT808ActivesafetyRequest_t*)head->m_content;
		if (NULL != node)
		{
			if (0 == memcmp(node->mAlertFlag, req->mAlertFlag, sizeof(req->mAlertFlag)))
			{
				PNT_LOGE("%s alarm is in uploading list.", node->mAlertID);
				pthread_mutex_unlock(&handle->reqList.m_mutex);
				return;
			}
		}
		head = head->next;
	}
	pthread_mutex_unlock(&handle->reqList.m_mutex);

	JT808ActivesafetyRequest_t* item = (JT808ActivesafetyRequest_t*)malloc(sizeof(JT808ActivesafetyRequest_t));

	if (NULL == item)
	{
		return;
	}

	memset(item, 0, sizeof(JT808ActivesafetyRequest_t));
	
	strncpy(item->mIp, (char*)req->mIp, sizeof(item->mIp)-1);
	memcpy(item->mAlertFlag, req->mAlertFlag, sizeof(item->mAlertFlag));
	memcpy(item->mAlertID, req->mAlertID, sizeof(item->mAlertID));
	item->mTCPPort = req->mTCPPort;

	if (queue_list_push(&handle->reqList, item))
	{
		free(item);
	}
	else
	{
		item->mID = JT808Activesafety_InsertRecord(handle, (BYTE*)item->mIp, item->mTCPPort, item->mAlertID, item->mAlertFlag);
	}
}

