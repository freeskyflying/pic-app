#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include "utils.h"
#include "pnt_log.h"
#include "jt808_cmd_node.h"
#include "jt808_session.h"
#include "jt808_data.h"
#include "jt808_utils.h"
#include "jt808_controller.h"
#include "pnt_epoll.h"
#include "ttsmain.h"
#include "strnormalize.h"
#include "rild.h"

extern BoardSysInfo_t gBoardInfo;

static const JT808_CmdNode_t JT808CMD_SUPPORT_LIST[] =
{
    ADD_CMD_NODE(0001, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8001, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(0002, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8003, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0005, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0100, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8100, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(0003, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0004, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8004, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(0102, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8103, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8104, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0104, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8105, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8106, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8107, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0107, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8108, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0108, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(0200, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8201, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0201, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8202, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8301, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0301, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8302, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0302, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8303, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0303, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8304, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8400, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8401, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8500, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0500, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8600, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8601, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8602, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8603, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8604, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8605, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8606, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8607, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8700, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0700, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8701, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8203, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8204, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8300, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8702, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0702, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0704, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0705, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0800, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0801, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8800, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8801, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0805, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8802, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0802, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(8608, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0608, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(0701, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8803, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8804, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8805, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8900, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0900, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0901, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(8A00, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(0A00, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9003, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(1003, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(9101, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(1005, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9102, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9105, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9205, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(1205, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(9201, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9202, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9206, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(1206, NULL, CMD_TYPE_RESPONSE),
    ADD_CMD_NODE(9207, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9301, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9302, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9303, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9304, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9305, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9306, NULL, CMD_TYPE_INITIATE),
    ADD_CMD_NODE(9208, NULL, CMD_TYPE_INITIATE),
};
	
int gJt808LogOutLen = 4096;
char* gJt808LogOut = NULL;
pthread_mutex_t gJt808LogOutMutex;

extern BoardSysInfo_t gBoardInfo;

void* JT808Session_UploadBlindLocationData(void* arg);

int TTSMainCompleteNotify(char* path)
{
	return 0;
}

/*
 * 808 8105 FTP 升级
 * 文件名格式：
 * 系统升级：model_fileVersion_targetVersion_md5.zip
 * 系统增量升级：model_fileVersion_targetVersion_md5.diff
 * MCU升级：model_fileVersion_targetVersion_md5.bin
*/
void JT808FtpUpgradeDownloadComplete(void* arg)
{
	FtpUtil_t* pFtpUtil = (FtpUtil_t*)arg;

    char finalFullPath[256] = { 0 };

    char originFullPath[256] = { 0 };
    sprintf(originFullPath, "%s/%s", pFtpUtil->mLocalPath, pFtpUtil->mFileName);

	char md5sum[33] = { 0 };
	computeFileMd5(originFullPath, md5sum);
	PNT_LOGI("md5sum : %s.", md5sum);

	if (NULL == strstr(pFtpUtil->mFileName, md5sum)) // 文件名包含md5参数
	{
		PNT_LOGE("fatal error !!! md5 check failed(%s, %s).", pFtpUtil->mFileName, md5sum);
	}
	else
	{
        if (strstr(pFtpUtil->mFileName, ".zip"))
        {
            sprintf(finalFullPath, "%s/%s", TERMINAL_OTA_PATH, pFtpUtil->mFileName);
        }
        else if (strstr(pFtpUtil->mFileName, ".diff"))
        {
            sprintf(finalFullPath, "%s/%s", TERMINAL_OTA_PATH, pFtpUtil->mFileName);
            int len = strlen(finalFullPath);
            finalFullPath[len-4] = 'z';
            finalFullPath[len-3] = 'i';
            finalFullPath[len-2] = 'p';
            finalFullPath[len-1] = '\0';
        }
        else if (strstr(pFtpUtil->mFileName, ".bin"))
        {
            sprintf(finalFullPath, "%s/%s", TERMINAL_OTA_PATH, pFtpUtil->mFileName);
        }
        if (copyFile(finalFullPath, originFullPath) != 0)
		{
            PNT_LOGE("fatal state error!!! --> fail to copy the ota file from tmp path:%s to target path:%s, caused by:%s",
                    originFullPath, finalFullPath, strerror(errno));
        }
	}

	clearTargetDir(pFtpUtil->mLocalPath);
}

void JT808FtpUpgradeDownloadFailed(void* arg)
{
	//FtpUtil_t* pFtpUtil = (FtpUtil_t*)arg;
}

int JT808FtpUpgradeProc(char* str)
{
	FtpUtil_t ftpUtil = { 0 };
	memset(&ftpUtil, 0, sizeof(ftpUtil));
	
	int i = 0, strLen = strlen(str), j = 0;
	while (i<strLen)
	{
		if (str[i] == ';')
		{
			i++;
			ftpUtil.mRemotePath[j] = 0;
			break;
		}
		ftpUtil.mRemotePath[j++] = str[i];
		i++;
	}
	i++;
	j = 0;
	while (i<strLen)
	{
		if (str[i] == ';')
		{
			i++;
			ftpUtil.mUserName[j] = 0;
			break;
		}
		ftpUtil.mUserName[j++] = str[i];
		i++;
	}
	
	j = 0;
	while (i<strLen)
	{
		if (str[i] == ';')
		{
			i++;
			ftpUtil.mPassWord[j] = 0;
			break;
		}
		ftpUtil.mPassWord[j++] = str[i];
		i++;
	}
	
	strcpy(ftpUtil.mFileName, strrchr(ftpUtil.mRemotePath, '/')+1);

	PNT_LOGI("ftpUtil.mRemotePath: %s.", ftpUtil.mRemotePath);
	PNT_LOGI("ftpUtil.mUserName: %s.", ftpUtil.mUserName); 
	PNT_LOGI("ftpUtil.mPassWord: %s.", ftpUtil.mPassWord);
	PNT_LOGI("ftpUtil.mFileName: %s.", ftpUtil.mFileName);

	ftpUtil.mComplete = JT808FtpUpgradeDownloadComplete;
	ftpUtil.mFailed = JT808FtpUpgradeDownloadFailed;

	strcpy(ftpUtil.mLocalPath, JT808_FTP_UPGRADE_TMP_PATH);
	if (FALSE == ensureTargetDirExist(ftpUtil.mLocalPath))
	{
		PNT_LOGE("fatal error !!! %s dir create failed.", ftpUtil.mLocalPath);
		return RET_FAILED;
	}
	if (FALSE == ensureTargetDirExist(TERMINAL_OTA_PATH))
	{
		PNT_LOGE("fatal error !!! %s dir create failed.", ftpUtil.mLocalPath);
		return RET_FAILED;
	}

	return FtpUtil_DownloadCreate(&ftpUtil);
}

JT808Session_t* JT808Session_Create(JT808SessionData_t* sessionData, void* owner)
{
    JT808Session_t* session = NULL;

	if (NULL == owner)
	{
        JT808SESSION_PRINT(session, "session create failed, owner must set and must be a JT808Controller_t.");
	
		return NULL;
	}

    session = (JT808Session_t*)malloc(sizeof(JT808Session_t));
    if (NULL == session)
    {
        JT808SESSION_PRINT(session, "session create failed, session malloc error.");
        return NULL;
    }

	memset(session, 0, sizeof(JT808Session_t));

    session->mSessionData = sessionData;
    session->mOnwer = owner;

    queue_list_create(&session->mRecvMsgQueue, 1000);
    
    queue_list_create(&session->mSendMsgQueue, 1000);
	
    queue_list_create(&session->mWaitAckMsgQueue, 1000);
	
    queue_list_create(&session->mPacketedSendMsgQueue, 1000);

	queue_list_create(&session->mPacketedRecvMsgQueue, 1000);

	queue_list_create(&session->mLocationExtraInfoList, 100);

	JT1078Rtsp_ControlInit(&session->mRtspController, session);

	for (int i=0; i<MAX_VIDEO_NUM; i++)
	{
		JT1078MediaFiles_PlaybackInit(&session->mMediaPlayback[i], session);
	}

    PNT_LOGI("session create success.");
    return session;
}

int JT808Session_SendPacketMsgs(JT808Session_t* session, JT808MsgNode_t* msg, WORD serialNum, WORD* idxs, int count/* 0 发送全部 */)
{
	int i = 0, ret = 0, j = 0, need = 1;
	
	int packetCount = (msg->mMsgBodyLen+BODY_PACKET_LEN-1)/BODY_PACKET_LEN;
	
	for (i=0; i<packetCount; i++)
	{
		if (0 != count)
		{
			need = 0;
		}
		for (; j<count; j++)// idxs中序号是按顺序的，因此只需要遍历一次
		{
			if (idxs[j] == i)
			{
				need = 1;
				break;
			}
		}
		if (0 == need) // 不在列表内，不需要补传
		{
			continue;
		}
		JT808MsgNode_t* nodeT = (JT808MsgNode_t*)malloc(sizeof(JT808MsgNode_t));
		memset(nodeT, 0, sizeof(JT808MsgNode_t));
		nodeT->mMsgHead = (JT808MsgHeader_t*)malloc(sizeof(JT808MsgHeader_t));
		memset(nodeT->mMsgHead, 0, sizeof(JT808MsgHeader_t));
		nodeT->mMsgHead->mMsgID = msg->mMsgHead->mMsgID;
		nodeT->mMsgHead->mMsgAttribute |= 0x2000;
		nodeT->mMsgHead->mPacketTotalNum = packetCount;
		nodeT->mMsgHead->mPacketIndex = (i+1);
		nodeT->mMsgHead->mMsgSerialNum = serialNum ++;

		if (packetCount == (i+1) && (msg->mMsgBodyLen%BODY_PACKET_LEN))
		{
			nodeT->mMsgBodyLen = msg->mMsgBodyLen%BODY_PACKET_LEN;
		}
		else
		{
			nodeT->mMsgBodyLen = BODY_PACKET_LEN;
		}

		nodeT->mMsgBody = (char*)malloc(nodeT->mMsgBodyLen);
		
		memcpy(nodeT->mMsgBody, msg->mMsgBody+i*BODY_PACKET_LEN, nodeT->mMsgBodyLen);
		
	 	PNT_LOGI("insert Msg[%04X] pack[%d, %d] to send queue.", nodeT->mMsgHead->mMsgID, nodeT->mMsgHead->mPacketTotalNum, nodeT->mMsgHead->mPacketIndex);

		if (0 != queue_list_push(&session->mSendMsgQueue, nodeT))
		{
			free(nodeT->mMsgBody);
			free(nodeT->mMsgHead);
			free(nodeT);
			break;
		}
		ret ++;
	}

	return ret;
}

int JT808Session_SendMsg(JT808Session_t* session, WORD msgId, void* msgBody, int msgBodyLen, char msgBodyEncFlag/* 数据量比较大的在调用此函数前将消息体编码 */)
{
	int i = 0, ret = RET_SUCCESS;
	char buff[1024] = { 0 }, *temp = NULL;

	pthread_mutex_lock(&session->mSendMsgMutex);

	JT808MsgNode_t* node = (JT808MsgNode_t*)malloc(sizeof(JT808MsgNode_t));
	memset(node, 0, sizeof(JT808MsgNode_t));
	
	node->mMsgHead = (JT808MsgHeader_t*)malloc(sizeof(JT808MsgHeader_t));
	memset(node->mMsgHead, 0, sizeof(JT808MsgHeader_t));
	
	node->mMsgHead->mMsgID = msgId;

	if (msgBodyEncFlag)
	{
		temp = msgBody;
		node->mMsgBodyLen = msgBodyLen;
	}
	else
	{
		for (i = 0; i<sizeof(JT808CMD_SUPPORT_LIST)/sizeof(JT808CMD_SUPPORT_LIST[0]); i++)
		{
			if (msgId == JT808CMD_SUPPORT_LIST[i].mMsgID)
			{
				node->mMsgBodyLen = JT808CMD_SUPPORT_LIST[i].mEncoder(msgBody, (uint8_t*)buff, 0, session);
				temp = buff;

				break;
			}
		}
	}

	if (NULL != temp && 0 < node->mMsgBodyLen)
	{
		node->mMsgBody = (char*)malloc(node->mMsgBodyLen);
		if (NULL != node->mMsgBody)
		{
			memcpy(node->mMsgBody, temp, node->mMsgBodyLen);
		}
	}

	if (node->mMsgBodyLen <= BODY_PACKET_LEN)
	{
  	 	PNT_LOGI("insert Msg[%04X] to send queue.", node->mMsgHead->mMsgID);
		node->mMsgHead->mMsgSerialNum = session->mMsgSerialNum ++;
		if (0 != queue_list_push(&session->mSendMsgQueue, node))
		{
			free(node->mMsgBody);
			free(node->mMsgHead);
			free(node);
			ret = RET_FAILED;
		}
	}
	else
	{
		node->mMsgHead->mMsgSerialNum = session->mMsgSerialNum; // 第一个包消息流水号
		
		session->mMsgSerialNum += JT808Session_SendPacketMsgs(session, node, session->mMsgSerialNum, NULL, 0);		

		for (i = 0; i<sizeof(JT808CMD_SUPPORT_LIST)/sizeof(JT808CMD_SUPPORT_LIST[0]); i++)
		{
			if (node->mMsgHead->mMsgID == JT808CMD_SUPPORT_LIST[i].mMsgID)
			{
				if (CMD_TYPE_INITIATE == JT808CMD_SUPPORT_LIST[i].mType /* && 重传次数>0 */) // 主动发送的消息，需要入等待应答队列
				{
					PNT_LOGI("insert Msg[%04X] [%04X]to packetSend queue.", node->mMsgHead->mMsgID, node->mMsgHead->mMsgSerialNum);
					if (0 != queue_list_push(&session->mPacketedSendMsgQueue, node))
					{
						free(node->mMsgBody);
						free(node->mMsgHead);
						free(node);
						ret = RET_FAILED;
					}
				}
				break;
			}
		}
		if (i == sizeof(JT808CMD_SUPPORT_LIST)/sizeof(JT808CMD_SUPPORT_LIST[0])) // 不需要入等待应答队列
		{
			free(node->mMsgBody);
			free(node->mMsgHead);
			free(node);
		}
	}
	
	pthread_mutex_unlock(&session->mSendMsgMutex);
	
	return ret;
}

int JT808Session_AddWaitAckMsg(JT808Session_t* session, char* dataBuf, int dataLen, WORD msgId, WORD msgSerialNum, char* bcdtime)
{
	JT808MsgWaitAckNode_t* waitAckNode = (JT808MsgWaitAckNode_t*)malloc(sizeof(JT808MsgWaitAckNode_t));
	waitAckNode->mMsgData = (uint8_t*)dataBuf;
	waitAckNode->mMsgDataLen = dataLen;
	waitAckNode->mLastTimeStamp = (uint32_t)currentTimeMillis();
	waitAckNode->mReplyCount = 0;
	waitAckNode->mTimeoutValue = 3; // 设置参数，TCP_MSG_RESPONSE_TIMEOUT
	waitAckNode->mMsgId = msgId;
	waitAckNode->mMsgSerilNum = msgSerialNum;
	memcpy(waitAckNode->mTimeBCD, bcdtime, 6);

	if (0 != queue_list_push(&session->mWaitAckMsgQueue, waitAckNode))
	{
		free(waitAckNode);
		
		return RET_FAILED;
	}
	
	return RET_SUCCESS;
}

int JT808Session_DeleteWaitAckMsg(JT808Session_t* session, WORD msgId, WORD msgSerialNum)
{
	queue_list_t* plist = &session->mWaitAckMsgQueue;

	pthread_mutex_lock(&plist->m_mutex);
	
	queue_t* head = plist->m_head;
	queue_t* prehead = head;

	while (head != NULL)
	{
		JT808MsgWaitAckNode_t* node = (JT808MsgWaitAckNode_t*)head->m_content;

		if (msgId == node->mMsgId && (msgSerialNum == 0x0000 || msgSerialNum == node->mMsgSerilNum))
		{
			PNT_LOGI("Delete WaitQueueMsg[%04X, %04X].", msgId, msgSerialNum);
			if (0x0200 == node->mMsgId)
			{
				//JT808BlindUpload_DeleteLocationDataByBCDTime(&session->mBlineGpsData, node->mTimeBCD);
			}
			else if (0x0704 == node->mMsgId)
			{
				JT808BlindUpload_DeleteLocationDataByBatchSerialNum(&session->mBlineGpsData, msgSerialNum);
			}
			
			// 删除节点
			if (head != plist->m_head)
			{
				prehead->next = head->next;
			}
			
			free(node->mMsgData);
			free(node);
			free(head);
			
			if (head == plist->m_head)
			{
				plist->m_head = NULL;
				head = NULL;
			}

			break;
		}
		
		prehead = head;
		head = head->next;
	}

	pthread_mutex_unlock(&plist->m_mutex);

	return RET_SUCCESS;
}

int JT808Session_DeletePacketMsgSendQueue(JT808Session_t* session, WORD msgId, WORD msgSerialNum)
{
	queue_list_t* plist = &session->mPacketedSendMsgQueue;

	pthread_mutex_lock(&plist->m_mutex);
	
	queue_t* head = plist->m_head;
	queue_t* prehead = head;

	while (head != NULL)
	{
		JT808MsgNode_t* node = (JT808MsgNode_t*)head->m_content;

		if (msgId == node->mMsgHead->mMsgID && msgSerialNum == (node->mMsgHead->mMsgSerialNum - node->mMsgHead->mPacketTotalNum))
		{
			PNT_LOGI("Delete Packet Msg[%04X, %04X][%d:%d].", msgId, msgSerialNum, node->mMsgHead->mPacketTotalNum, node->mMsgHead->mPacketIndex);
			// 删除节点
			if (head != plist->m_head)
			{
				prehead->next = head->next;
			}
			
			free(node->mMsgBody);
			free(node->mMsgHead);
			free(node);
			free(head);
			
			if (head == plist->m_head)
			{
				plist->m_head = NULL;
				head = NULL;
			}

			break;
		}
		
		prehead = head;
		head = head->next;
	}

	pthread_mutex_unlock(&plist->m_mutex);

	return RET_SUCCESS;
}

int JT808Session_QueryPacketMsgSendQueueAndResend(JT808Session_t* session, JT808MsgBody_8003_t* seq)
{
	queue_list_t* plist = &session->mPacketedSendMsgQueue;

	pthread_mutex_lock(&plist->m_mutex);
	
	queue_t* head = plist->m_head;
	queue_t* prehead = head;

	while (head != NULL)
	{
		JT808MsgNode_t* node = (JT808MsgNode_t*)head->m_content;

		if (seq->mTargetMsgSerialNum == node->mMsgHead->mMsgSerialNum)
		{
			PNT_LOGI("Resend Packet Msg[%04X][%d:%d].", node->mMsgHead->mMsgID, node->mMsgHead->mPacketTotalNum, node->mMsgHead->mPacketIndex);
			
			JT808Session_SendPacketMsgs(session, node, node->mMsgHead->mMsgSerialNum, seq->pReUploadPackIDs, seq->mReUploadPacksCount);	
			
			// 删除节点
			if (head != plist->m_head)
			{
				prehead->next = head->next;
			}
			
			free(node->mMsgBody);
			free(node->mMsgHead);
			free(node);
			free(head);
			
			if (head == plist->m_head)
			{
				plist->m_head = NULL;
				head = NULL;
			}

			break;
		}
		
		prehead = head;
		head = head->next;
	}

	pthread_mutex_unlock(&plist->m_mutex);

	return RET_SUCCESS;
}

int JT808Session_RcvMsgProcess(JT808Session_t* session, JT808MsgHeader_t* msgHead, void* msgBody)
{
	JT808Controller_t* controller = (JT808Controller_t*)session->mOnwer;

	JT808SESSION_PRINT(session, "Process RecvMsg[%04X].", msgHead->mMsgID);

	switch (msgHead->mMsgID)
	{
		case 0x8001:	// 平台通用应答
			{
				JT808MsgBody_8001_t* param = (JT808MsgBody_8001_t*)msgBody;

				if (1 != param->mResult)
				{
					JT808Session_DeleteWaitAckMsg(session, param->mMsgID, param->mResponseSerialNum);
				}
				if (2 != param->mResult && 1 != param->mResult)
				{
					JT808Session_DeletePacketMsgSendQueue(session, param->mMsgID, param->mResponseSerialNum);
				}

				switch (param->mMsgID)
				{
					case 0x0102:
						{
							if (0 == param->mResult)
							{
								session->mMainProcessState = JT808SessionState_Running;

								pthread_t pid;
							    pthread_create(&pid, NULL, JT808Session_UploadBlindLocationData, session);
							}
							else
							{
								session->mMainProcessState = JT808SessionState_StartRegister;
							}
						}
						break;
				}
			}
			break;

		case 0x8003:	// 补传分包请求
			{
    			JT808MsgBody_8003_t* param = (JT808MsgBody_8003_t*)param;
				// 查看发送分包队列，并开始补传
				JT808Session_QueryPacketMsgSendQueueAndResend(session, param);
				
				if (NULL != param->pReUploadPackIDs)
					free(param->pReUploadPackIDs);
			}
			break;

		case 0x8100:	// 注册应答
    		{
				JT808MsgBody_8100_t* param = (JT808MsgBody_8100_t*)msgBody;
				if (0 == param->mResult)
				{
					PNT_LOGI("AUTHCODE %s.", param->mAuthenticationCode);
					
					if (JT808SessionState_Waiting == session->mMainProcessState)
					{
						if (memcmp(session->mSessionData->mAuthenticode, param->mAuthenticationCode, param->mAuthenticationCodeLen) || 
							strlen((char*)session->mSessionData->mAuthenticode) != param->mAuthenticationCodeLen)
						{
							memcpy(session->mSessionData->mAuthenticode, param->mAuthenticationCode, param->mAuthenticationCodeLen);
							JT808Database_UpdateSessionData(&controller->mJT808DB, session->mSessionData);
						}
						session->mMainProcessState = JT808SessionState_StartAuthentication;
					}
					
					JT808Session_DeleteWaitAckMsg(session, 0x0100, param->mResponseSerialNum);
				}
			}
			break;

		case 0x8104:	// 查询终端参数
			{
				uint8_t* encBuf = (uint8_t*)malloc(1024*4);
				JT808MsgBody_0104_t param = { 0 };
				// 获取param参数列表
				JT808Database_GetAllJT808Param(&controller->mJT808DB, &param.pParamList);
				param.mResponseSerialNum = msgHead->mMsgSerialNum;
				int len = JT808_0104_CmdEncoder(&param, encBuf, 0, session);
				queue_list_destroy(&param.pParamList);
				
				JT808Session_SendMsg(session, 0x0104, encBuf, len, 1);

				free(encBuf);
			}
			break;

		case 0x8103:	// 设置终端参数
			{
				JT808MsgBody_8103_t* param = (JT808MsgBody_8103_t*)msgBody;
				for (int i=0; i<param->mParamItemCount; i++)
				{
					JT808MsgBody_8103_Item_t* item = &param->pParamList[i];
					
					JT808Database_SetJT808Param(&controller->mJT808DB, item);

					if (0x03 < item->mDataType)
					{
						free((void*)item->mParamData);
						item->mParamData = 0;
					}
				}
				if (NULL != param->pParamList)
				{
					free(param->pParamList);
					param->pParamList = NULL;
				}
				
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_SUCCESS;

				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}
			break;

		case 0x8106:	// 查询指定参数
			{
				uint8_t* encBuf = (uint8_t*)malloc(1024*4);
				
				JT808MsgBody_8106_t* param = (JT808MsgBody_8106_t*)(msgBody);
				
				JT808MsgBody_0104_t paramAck = { 0 };

				paramAck.mParamItemCount = param->mParamItemCount;
				paramAck.mResponseSerialNum = msgHead->mMsgSerialNum;
				
				queue_list_create(&paramAck.pParamList, 1000);
				
				for (int i=0; i<param->mParamItemCount; i++)
				{
					JT808Param_t* paramData = NULL;
					
					JT808Database_GetJT808Param(&controller->mJT808DB, param->mParamIDList[i], &paramData);
					
					if (NULL != paramData)
					{
						JT808Param_t* item = malloc(sizeof(JT808Param_t));
						item->mDataType = paramData->mDataType;
						item->mParamDataLen = paramData->mParamDataLen;
						item->mParamID = paramData->mParamID;
						if (0x03 < paramData->mDataType)
						{
							item->mParamData = (DWORD)malloc(item->mParamDataLen);
							memcpy((char*)item->mParamData, (char*)paramData->mParamData, item->mParamDataLen);
						}
						else
						{
							item->mParamData = paramData->mParamData;
						}
						queue_list_push(&paramAck.pParamList, item);
					}
				}
				
				int len = JT808_0104_CmdEncoder(&paramAck, encBuf, 0, session);
				queue_list_destroy(&paramAck.pParamList);
				
				JT808Session_SendMsg(session, 0x0104, encBuf, len, 1);
				free(encBuf);
			}
			break;

		case 0x8201:	// 查询位置信息
			{
				JT808MsgBody_0201_t ack = { 0 };
				
				ack.mLocationReport.mAlertFlag = controller->mCurrentAlertFlag;
				ack.mLocationReport.mStateFlag = controller->mCurrentTermState;
				getLocalBCDTime((char*)ack.mLocationReport.mDateTime);
				ack.mLocationReport.mSpeed = (WORD)(controller->mLocation.speed*10);
				ack.mLocationReport.mAltitude = (WORD)(controller->mLocation.altitude);
				ack.mLocationReport.mLatitude = (DWORD)(controller->mLocation.latitude*1000000);
				ack.mLocationReport.mLongitude = (DWORD)(controller->mLocation.longitude*1000000);
				ack.mLocationReport.mDirection = (WORD)controller->mLocation.bearing;
				ack.mLocationReport.mExtraInfoCount = 0;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				
				JT808Session_SendMsg(session, 0x0201, &ack, sizeof(ack), 0);
			}
			break;

		case 0x8300:
			{
				JT808MsgBody_8300_t* param = (JT808MsgBody_8300_t*)msgBody;

				if (param->mTextFlag & 0x08)
				{
					str_normalize_init();
		            int size = strlen(param->mTextContent) * 2 + 1;
		            char *buffer = (char *) malloc(size);
		            memset(buffer, 0, size);
		            unsigned int outSize = 0;
		            gbk_to_utf8(param->mTextContent, strlen(param->mTextContent), &buffer, &outSize);
					TTSMain_Start(&controller->mTTSMain, buffer, TTSMainCompleteNotify);
					free(buffer);
				}
				if (param->mTextFlag & 0x04)
				{
				}
				
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_SUCCESS;
				
				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}
			break;

		case 0x9003:	// 查询终端音视频属性
			{
				JT808MsgBody_1003_t ack = { 0 };

				JT808Session_SendMsg(session, 0x1003, &ack, sizeof(ack), 0);
			}
			break;

		case 0x9101:
			{
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_SUCCESS;
				
				JT808MsgBody_9101_t* param = (JT808MsgBody_9101_t*)msgBody;
				if (RET_SUCCESS != JT1078Rtsp_ChannelCreate(&session->mRtspController, param))
				{
					ack.mResult = COM_RESULT_FAILED;
				}
				
				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}
			break;

		case 0x9102:
			{
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_SUCCESS;
				
				JT808MsgBody_9102_t* param = (JT808MsgBody_9102_t*)msgBody;

				JT1078Rtsp_PlayControl(&session->mRtspController, param);
				
				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}
			break;

		case 0x9208:
			{
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_SUCCESS;

				JT808MsgBody_9208_t* param = (JT808MsgBody_9208_t*)msgBody;

				JT808ActiveSafety_UploadRequest(&session->mAlarmUpload, param);
				
				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}
			break;

		case 0x8105:
			{
				JT808MsgBody_8105_t* param = (JT808MsgBody_8105_t*)msgBody;
				
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_SUCCESS;
				
				switch(param->mTerminalCtrlCmd)
				{
					case 1:
						{
							PNT_LOGI("Terminal ftp update [%s]", param->pTerminalCtrlParam);
							if (JT808FtpUpgradeProc(param->pTerminalCtrlParam))
							{
								ack.mResult = COM_RESULT_FAILED;
							}
						}
						break;
					case 2:
						break;
					case 3:
						break;
					case 4:
						{
							JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
							system("reboot");
							return RET_SUCCESS;
						}
						break;
					case 5:
						break;
					case 6:
						break;
					case 7:
						break;
				}
				
				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);

				if (param->pTerminalCtrlParam)
				{
					free(param->pTerminalCtrlParam);
				}
			}
			break;

		case 0x9205:
			{
				JT808MsgBody_1205_t ack = { 0 };
				JT808MsgBody_9205_t* param = (JT808MsgBody_9205_t*)msgBody;

				memset(&ack, 0, sizeof(JT808MsgBody_1205_t));
				
				queue_list_create(&ack.mResList, 99999);

				ack.mReqSerialNum = msgHead->mMsgSerialNum;
				
				JT1078MediaFiles_QueryFiles(param, &ack);
				PNT_LOGE("finish query file: %d", ack.mResCount);
				if (ack.mResCount > 20) // 数据体长
				{
					uint8_t* buf = malloc(ack.mResCount*sizeof(JT808MsgBody_1205_Item_t)+8);
					if (NULL != buf)
					{
						int ackBodyLen = JT808_1205_CmdEncoder(&ack, buf, 0, session);
						
						JT808Session_SendMsg(session, 0x1205, buf, ackBodyLen, 1);

						free(buf);
					}
				}
				else
				{
					JT808Session_SendMsg(session, 0x1205, &ack, 0, 0);
				}

				queue_list_destroy(&ack.mResList);
			}
			break;

		case 0x9201:
			{
				JT808MsgBody_1205_t ack = { 0 };
				JT808MsgBody_9201_t* param = (JT808MsgBody_9201_t*)msgBody;

				memset(&ack, 0, sizeof(JT808MsgBody_1205_t));
				
				queue_list_create(&ack.mResList, 99999);

				ack.mReqSerialNum = msgHead->mMsgSerialNum;
				
				JT1078MediaFiles_PlaybackRequest(&session->mMediaPlayback[param->mLogicChnNum-1], param, &ack);
				
				if (ack.mResCount > 20) // 数据体长
				{
					uint8_t* buf = malloc(ack.mResCount*sizeof(JT808MsgBody_1205_Item_t)+8);
					if (NULL != buf)
					{
						int ackBodyLen = JT808_1205_CmdEncoder(&ack, buf, 0, session);
						
						JT808Session_SendMsg(session, 0x1205, buf, ackBodyLen, 1);

						free(buf);
					}
				}
				else
				{
					JT808Session_SendMsg(session, 0x1205, &ack, 0, 0);
				}

				queue_list_destroy(&ack.mResList);
			}
			break;

		case 0x9202:
			{
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_SUCCESS;
				
				JT808MsgBody_9202_t* param = (JT808MsgBody_9202_t*)msgBody;
				JT1078MediaFiles_PlaybackControl(&session->mMediaPlayback[param->mLogicChnNum-1], param);

				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}
			break;

		case 0x9206:
			{
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_SUCCESS;
				
				JT808MsgBody_9206_t* param = (JT808MsgBody_9206_t*)msgBody;
				JT1078FileUpload_RequestProcess(msgHead->mMsgSerialNum, param, &controller->m1078FileUpload, session);

				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}
			break;

		case 0x9207:
			{
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_SUCCESS;
				
				JT808MsgBody_9207_t* param = (JT808MsgBody_9207_t*)msgBody;
				JT1078FileUpload_RequestControl(&controller->m1078FileUpload, param);

				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}
			break;

		default:
			{
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = msgHead->mMsgID;
				ack.mResponseSerialNum = msgHead->mMsgSerialNum;
				ack.mResult = COM_RESULT_UNSUPPORT;

				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}
			break;
		
	}

	return RET_SUCCESS;
}

int JT808Session_MsgDataParser(JT808Session_t* session, char* data, WORD len)
{
    JT808MsgHeader_t* head = NULL;

    int offset = JT808_HEAD_Parser((BYTE*)data, (void**)&head, len, session);
    if (RET_FAILED == offset)
    {
        return RET_FAILED;
    }
    
    //PNT_LOGI("recv Msg[%04X], start to parser.", head->mMsgID);

    // 检查校验码
    BYTE crc = JT808CalcCrcTerminal((BYTE*)data, len - 1);
	
    if (data[len-1] != crc)
    {
        JT808SESSION_PRINT(session, "Check CRC error local[%02X] remote[%02X].", crc, data[len-1]);

		JT808MsgBody_0001_t ack = { 0 };
		ack.mMsgID = head->mMsgID;
		ack.mResponseSerialNum = head->mMsgSerialNum;
		ack.mResult = COM_RESULT_ERROR;

		JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
		
		free(head);
    }
	else
	{
		if (JT808MSG_ATTR_IS_PACK(head->mMsgAttribute)) // 分包消息
		{
			JT808MsgRecvPacketNode_t* node = NULL;
			JT808MsgRecvPacketNode_t* tmp = NULL;

			pthread_mutex_lock(&session->mPacketedRecvMsgQueue.m_mutex);
			
			queue_t* pHead = session->mPacketedRecvMsgQueue.m_head;
			while (pHead)
			{
				tmp = pHead->m_content;
				if (tmp->mMsgHead.mMsgID == head->mMsgID)
				{
					node = tmp;
					break;
				}
				if (NULL == pHead->next)
				{
					break; // 留住尾节点，以便后面新增节点使用
				}
				pHead = pHead->next;
			}
			
			if (NULL == node)
			{
				JT808MsgRecvPacketNode_t* node = (JT808MsgRecvPacketNode_t*)malloc(sizeof(JT808MsgRecvPacketNode_t));
				memset(node, 0, sizeof(JT808MsgRecvPacketNode_t));
				node->mMsgHead = *head;
				node->mPackIdxList = (BYTE*)malloc(node->mMsgHead.mPacketTotalNum);
				memset(node->mPackIdxList, 0, node->mMsgHead.mPacketTotalNum);
				node->mMsgBody = (BYTE*)malloc(node->mMsgHead.mPacketTotalNum*(1024+4)); // 前4字节存放包数据长度，1024为最长长度
				if (NULL == node->mMsgBody || NULL == node->mPackIdxList)
				{
					JT808SESSION_PRINT(session, "packet msg [%04X] malloc failed.", node->mMsgHead.mMsgID);
					if (node->mPackIdxList)
						free(node->mPackIdxList);
					if (node->mMsgBody)
						free(node->mMsgBody);
					free(node);
					node = NULL;
				}
				else
				{
					queue_t* pNode = (queue_t*)malloc(sizeof(queue_t)); // 添加到队列
					memset(pNode, 0, sizeof(queue_t));
					pNode->m_content = node;
					if (NULL == pHead)
					{
						session->mPacketedRecvMsgQueue.m_head = pNode;
					}
					else
					{
						pHead->next = pNode;
					}
				}
			}
			
			if (NULL != node)
			{
				int offsetBody = (1024+4)*(node->mMsgHead.mPacketIndex-1);
				int bodylen = JT808MSG_ATTR_LENGTH(node->mMsgHead.mMsgAttribute);
				memcpy(node->mMsgBody+offsetBody, &bodylen, 4);
				memcpy(node->mMsgBody+offsetBody+4, data+offset, node->mMsgBodyLen);
				node->mPackIdxList[node->mMsgHead.mPacketIndex-1] = 1;
				node->mLastTimeStamp = currentTimeMillis();
				node->mMsgBodyLen += bodylen;
				if (node->mMsgHead.mPacketIndex == node->mMsgHead.mPacketTotalNum)
				{
					node->mFinishFlag = 1;
				}
			}

			pthread_mutex_unlock(&session->mPacketedRecvMsgQueue.m_mutex);
			
			JT808MsgBody_0001_t ack = { 0 };
			ack.mMsgID = head->mMsgID;
			ack.mResponseSerialNum = head->mMsgSerialNum;
			ack.mResult = COM_RESULT_SUCCESS;
			
			JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);

			free(head);
		}
		else
		{
	    	JT808MsgNode_t* node = (JT808MsgNode_t*)malloc(sizeof(JT808MsgNode_t));
			node->mMsgHead = head;
			node->mMsgBodyLen = len - offset - 1;
			node->mMsgBody = (char*)malloc(node->mMsgBodyLen);
				
			if (NULL == node->mMsgBody)
			{
	            JT808SESSION_PRINT(session, "malloc failed.", head->mMsgID);
				free(head);
			}
			else
			{
				memcpy(node->mMsgBody, data+offset, node->mMsgBodyLen);
				
				if (0 != queue_list_push(&session->mRecvMsgQueue, node))
				{
					free(node->mMsgHead);
					free(node->mMsgBody);
					free(node);
					return RET_FAILED;
				}
			}
		}
	}
	
    return RET_SUCCESS;
}

int JT808Session_RcvDataProcess(char* buff, int len, void* arg)
{
    PNTSocket_t* sock = (PNTSocket_t*)arg;
    JT808Session_t* session = (JT808Session_t*)sock->mOwner;

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

                JT808Session_MsgDataParser(session, msg_buffer, msg_len);

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

void* JT808Session_RecvQueueProcess(void* arg)
{
    pthread_detach(pthread_self());
	
    JT808Session_t* session = (JT808Session_t*)arg;

    JT808MsgNode_t* node = NULL;

    while (session->mRunningFlag)
    {    
        node = (JT808MsgNode_t*)queue_list_popup(&session->mRecvMsgQueue);
        if (NULL == node)
        {
            SleepMs(100);
            continue;
        }
        else
        {
    		JT808MsgHeader_t* head = node->mMsgHead;
	
			int i = 0;
		    for (i = 0; i<sizeof(JT808CMD_SUPPORT_LIST)/sizeof(JT808CMD_SUPPORT_LIST[0]); i++)
		    {
		        if (head->mMsgID == JT808CMD_SUPPORT_LIST[i].mMsgID)
		        {
		            void* param = NULL;

		            JT808CMD_SUPPORT_LIST[i].mParser(node->mMsgBody, (void**)&param, node->mMsgBodyLen, session);

					JT808Session_RcvMsgProcess(session, head, param);

		            if (NULL != param)
		            {
		                free(param);
		            }

		            break;
		        }
		    }
			if (i == sizeof(JT808CMD_SUPPORT_LIST)/sizeof(JT808CMD_SUPPORT_LIST[0]))
			{
		        // Not Support CMD
				JT808MsgBody_0001_t ack = { 0 };
				ack.mMsgID = head->mMsgID;
				ack.mResponseSerialNum = head->mMsgSerialNum;
				ack.mResult = COM_RESULT_UNSUPPORT;

				JT808Session_SendMsg(session, 0x0001, &ack, sizeof(ack), 0);
			}

            if (NULL != node->mMsgHead)
            {
                free(node->mMsgHead);
                node->mMsgHead = NULL;
            }
            if (NULL != node->mMsgBody)
            {
                free(node->mMsgBody);
                node->mMsgBody = NULL;
            }
        }
    }

	JT808SESSION_PRINT(session, "thread task exit.");

	session->mRecvQueueProcessPid = 0;

    pthread_exit(NULL);
    return NULL;
}

void JT808Session_RecvPacketQueueProcess(JT808Session_t* session)
{
	queue_list_t* plist = &session->mPacketedRecvMsgQueue;

	pthread_mutex_lock(&plist->m_mutex);
	
	queue_t* head = plist->m_head;
	queue_t* prehead = head;

	while (head != NULL)
	{
		JT808MsgRecvPacketNode_t* node = (JT808MsgRecvPacketNode_t*)head->m_content;

		if (node->mFinishFlag || (currentTimeMillis()-node->mLastTimeStamp > 10*1000)/*10s内未收到新的包*/)
		{
			JT808MsgBody_0005_t req = { 0 };
			
			char complete_flag = 1;
			for (int i=0; i<node->mMsgHead.mPacketTotalNum; i++)
			{
				if (node->mPackIdxList[i] == 0) // 有丢包
				{
					complete_flag = 0;
					break;
				}
			}

			if (complete_flag)
			{
		    	JT808MsgNode_t* nodet = (JT808MsgNode_t*)malloc(sizeof(JT808MsgNode_t));
				if (nodet)
				{
					nodet->mMsgHead = (JT808MsgHeader_t*)malloc(sizeof(JT808MsgHeader_t));
					if (nodet->mMsgHead)
					{
						nodet->mMsgBodyLen = node->mMsgBodyLen;
						nodet->mMsgBody = (char*)malloc(nodet->mMsgBodyLen);
							
						if (NULL == node->mMsgBody)
						{
				            JT808SESSION_PRINT(session, "malloc failed.", node->mMsgHead.mMsgID);
							free(nodet);
							free(head);
						}
						else
						{
							int offset = 0, len = 0;
							for (int i=0; i<node->mMsgHead.mPacketTotalNum; i++)
							{
								memcpy(&len, node->mMsgBody+i*(1024+4), 4);
								memcpy(nodet->mMsgBody+offset, node->mMsgBody+i*(1024+4)+4, len);
								offset += len;
							}
							
							if (0 != queue_list_push(&session->mRecvMsgQueue, nodet))
							{
								free(nodet->mMsgHead);
								free(nodet->mMsgBody);
								free(nodet);
							}
						}
					}
					else
					{
						free(nodet);
					}
				}
				
				// 删除节点
				if (head != plist->m_head)
				{
					prehead->next = head->next;
				}
				
				free(node->mMsgBody);
				free(node->mPackIdxList);
				free(node);
				free(head);
				
				if (head == plist->m_head)
				{
					plist->m_head = NULL;
					head = NULL;
				}
				else
				{
					head = prehead->next;
				}
				
				continue;
			}
			else // 需要补传
			{
				req.pReUploadPackIDs = (WORD*)malloc(sizeof(WORD)*node->mMsgHead.mPacketTotalNum);
				if (req.pReUploadPackIDs)
				{
					for (int i=0; i<node->mMsgHead.mPacketTotalNum; i++)
					{
						if (node->mPackIdxList[i] == 0) // 有丢包
						{
							req.pReUploadPackIDs[req.mReUploadPacksCount] = i+1;
							req.mReUploadPacksCount ++;
						}
					}
					req.mTargetMsgSerialNum = node->mMsgHead.mMsgSerialNum;
					node->mFinishFlag = 0;
					node->mLastTimeStamp = currentTimeMillis();

					JT808Session_SendMsg(session, 0x0005, &req, sizeof(req), 0);

					free(req.pReUploadPackIDs);
				}
				else
				{
					JT808SESSION_PRINT(session, "packet msg[%04X] req recv malloc failed.", node->mMsgHead.mMsgID);
				}
			}
		}
		
		prehead = head;
		head = head->next;
	}

	pthread_mutex_unlock(&plist->m_mutex);

}

void* JT808Session_SendQueueProcess(void* arg)
{
    pthread_detach(pthread_self());
	
    JT808Session_t* session = (JT808Session_t*)arg;

    JT808MsgNode_t* node = NULL;
	int releaseFlag = 0;

	char* msgbuf = (char*)malloc(2048);
	memset(msgbuf, 0, 2048);
	if (NULL == msgbuf)
	{
		JT808SESSION_PRINT(session, "malloc failed.");
		goto exit;
	}

    while (session->mRunningFlag)
    {
        node = (JT808MsgNode_t*)queue_list_popup(&session->mSendMsgQueue);
        if (NULL == node)
        {
            SleepMs(100);
            continue;
        }
        else
        {
			int i = 0;
			int sendLen = 0, bodyOffset = 0;
			releaseFlag = 1;
	
			BYTE* sendbuf = (BYTE*)malloc(2048);
			memset(sendbuf, 0, 2048);
			if (NULL == sendbuf)
			{
				continue;
			}

			node->mMsgHead->mProtocalVer = 1;
			if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
			{
				node->mMsgHead->mMsgAttribute |= (1<<14);
			}

			node->mMsgHead->mMsgAttribute |= (node->mMsgBodyLen&0x3FF);
			if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
			{
				bodyOffset = JT808MSG_BODY_OFFSET(node->mMsgHead->mMsgAttribute) + PHONE_NUM_LEN_2019;
				memcpy(node->mMsgHead->mPhoneNum, session->mPhoneNum, PHONE_NUM_LEN_2019);
			}
			else //if (SESSION_TYPE_JT808 == session->mSessionData->mSessionType)
			{
				bodyOffset = JT808MSG_BODY_OFFSET(node->mMsgHead->mMsgAttribute) + PHONE_NUM_LEN;
				memcpy(node->mMsgHead->mPhoneNum, session->mPhoneNum, PHONE_NUM_LEN);
			}

			if (0 < node->mMsgBodyLen)
			{
				memcpy(msgbuf+bodyOffset, node->mMsgBody, node->mMsgBodyLen);
				
				if (0x0200 == node->mMsgHead->mMsgID)
				{
					// 保存盲区数据
					//JT808BlindUpload_InsertLocationData(&session->mBlineGpsData, node->mMsgHead->mMsgSerialNum, node->mMsgBodyLen, node->mMsgBody);
				}
			}

			JT808_HEAD_Encoder(node->mMsgHead, (BYTE*)msgbuf+1, 2048, session);

			msgbuf[0] = JT808_MSG_FLAG;
			msgbuf[bodyOffset+node->mMsgBodyLen] = JT808CalcCrcTerminal((BYTE*)msgbuf+1, bodyOffset+node->mMsgBodyLen - 1);
			msgbuf[bodyOffset+node->mMsgBodyLen+1] = JT808_MSG_FLAG;

			// 转义
			sendbuf[0] = JT808_MSG_FLAG;
			sendLen = JT808MsgConvert(msgbuf+1, node->mMsgBodyLen+bodyOffset, (char*)sendbuf+1, 2048, 1);
			sendbuf[sendLen+1] = JT808_MSG_FLAG;
			
			// 发送
			LibPnt_SocketWrite(&session->mMainSocket, sendbuf, sendLen+2);
			
			PRINTF_BUFFER(session, (BYTE*)sendbuf, sendLen+2);

			if (0 == JT808MSG_ATTR_IS_PACK(node->mMsgHead->mMsgAttribute)) // 分包消息由另一条等待应答处理机制
			{
				for (i = 0; i<sizeof(JT808CMD_SUPPORT_LIST)/sizeof(JT808CMD_SUPPORT_LIST[0]); i++)
				{
					if (node->mMsgHead->mMsgID == JT808CMD_SUPPORT_LIST[i].mMsgID)
					{
						if (CMD_TYPE_INITIATE == JT808CMD_SUPPORT_LIST[i].mType /* && 重传次数>0 */) // 主动发送的消息，需要入等待应答队列
						{
							releaseFlag = 0;
							char bcdtime[6] = { 0 };
							if (0x0200 == node->mMsgHead->mMsgID)
							{
								memcpy(bcdtime, node->mMsgBody+22, 6);
							}

							if (RET_FAILED == JT808Session_AddWaitAckMsg(session, (char*)sendbuf, sendLen+2, node->mMsgHead->mMsgID, node->mMsgHead->mMsgSerialNum, bcdtime))
							{
								releaseFlag = 1;
							}
						}
						break;
					}
				}
			}
			if (i == sizeof(JT808CMD_SUPPORT_LIST)/sizeof(JT808CMD_SUPPORT_LIST[0]))
			{
	            PNT_LOGE("Msg[%04X] not support.", node->mMsgHead->mMsgID);
			}

			if (releaseFlag)
			{
				free(sendbuf);
			}

            if (NULL != node->mMsgHead)
            {
                free(node->mMsgHead);
                node->mMsgHead = NULL;
            }
            if (NULL != node->mMsgBody)
            {
                free(node->mMsgBody);
                node->mMsgBody = NULL;
            }
        }
    }

exit:

	JT808SESSION_PRINT(session, "thread task exit.");

	if (NULL != msgbuf)
	{
		free(msgbuf);
	}
	session->mSendQueueProcessPid = 0;

    pthread_exit(NULL);
    return NULL;
}

static void insert0200BlindData(JT808Session_t* session, JT808MsgWaitAckNode_t* node)
{
	char msgOrg[2048] = { 0 };

	int bodyOffset = 0;
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		bodyOffset = 7 + PHONE_NUM_LEN_2019;
	}
	else
	{
		bodyOffset = 6 + PHONE_NUM_LEN;
	}

	int orgLen = JT808MsgConvert((char*)(node->mMsgData+1), node->mMsgDataLen-2, msgOrg, sizeof(msgOrg), 0);
	
	// 保存盲区数据
	JT808BlindUpload_InsertLocationData(&session->mBlineGpsData, node->mMsgSerilNum, orgLen-bodyOffset, (BYTE*)(msgOrg+bodyOffset));
}

void* JT808Session_WaitAckMsgQueueProcess(void* arg)
{
    pthread_detach(pthread_self());
	
    JT808Session_t* session = (JT808Session_t*)arg;

	queue_list_t* plist = &session->mWaitAckMsgQueue;
	
	while (session->mRunningFlag)
	{
		pthread_mutex_lock(&plist->m_mutex);

		queue_t* head = plist->m_head;
		queue_t* prehead = head;

		while (head != NULL)
		{
			JT808MsgWaitAckNode_t* node = (JT808MsgWaitAckNode_t*)head->m_content;

			if (((uint32_t)currentTimeMillis() - node->mLastTimeStamp) > (uint32_t)(node->mTimeoutValue*1000))
			{
				if (node->mReplyCount >= 2) // 设置参数，TCP_MSG_RETRY_COUNT
				{
					PNT_LOGI("MSG[%04X][%04X] reSend over time, offline.", node->mMsgId, node->mMsgSerilNum);

					if (session->mMainSocket.mConnectStatus)
					{
						LibPnt_SocketClose(&session->mMainSocket);
					}
					
					if (0x0200 == node->mMsgId)
					{
						insert0200BlindData(session, node);
					}
					
					session->mMainProcessState = JT808SessionState_Offline;

					// 删除节点
					if (head == plist->m_head)
					{
						plist->m_head = head->next;
						prehead = head->next;
					}
					else
					{
						prehead->next = head->next;
					}
					
					free(node->mMsgData);
					free(node);
					free(head);

					head = prehead;
					continue;
				}
				else
				{
					LibPnt_SocketWrite(&session->mMainSocket, node->mMsgData, node->mMsgDataLen);
					node->mReplyCount ++;
					node->mTimeoutValue = node->mTimeoutValue*(node->mReplyCount+1);
					node->mLastTimeStamp = (uint32_t)currentTimeMillis();
					//PNT_LOGI("MSG[%04X][%04X] reSend next [%d %d].", node->mMsgId, node->mMsgSerilNum, node->mReplyCount, node->mTimeoutValue);
				}
			}

			prehead = head;
			head = head->next;
		}

		pthread_mutex_unlock(&plist->m_mutex);

		SleepMs(100);
	}
	
	JT808SESSION_PRINT(session, "thread task exit.");

	session->mWaitAckMsgQueueProcessPid = 0;

    pthread_exit(NULL);
    return NULL;
}

void* JT808Session_LocationMsgReportProcess(void* arg)
{
    pthread_detach(pthread_self());
	
	JT808MsgBody_0200_t body = { 0 };

	queue_list_create(&body.mExtraInfoList, 100);

    JT808Session_t* session = (JT808Session_t*)arg;
	JT808Controller_t* controller = (JT808Controller_t*)session->mOnwer;

	DWORD* Cmd0200ReportStrategy = (DWORD*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x0020, NULL);
	DWORD* Cmd0200ReportTimeInterval = (DWORD*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x0029, NULL);
	DWORD* Cmd0200ReportDistInterval = (DWORD*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x002C, NULL);

	long long lastReportTime = 0, lastReportTime2 = 0, lastReportDistance = 0;
	DWORD reportTimeInterval = 10, reportDistInterval = 100, reportStrategy = 0;
	unsigned int lastAlertFlag = 0, lastStateFlag = 0;
	bool_t report = 0;

	while (session->mRunningFlag)
	{
		if (NULL != Cmd0200ReportTimeInterval)
		{
			reportTimeInterval = *Cmd0200ReportTimeInterval;
		}
		if (NULL != Cmd0200ReportDistInterval)
		{
			reportDistInterval = *Cmd0200ReportDistInterval;
		}
		if (NULL != Cmd0200ReportStrategy)
		{
			reportStrategy = *Cmd0200ReportStrategy;
		}

		// 每隔1s获取报警/状态变化量队列，有数据则上报，无则根据上报策略计算上报
		if (currentTimeMillis() - lastReportTime2 >= 1000)
		{
			if (lastAlertFlag != controller->mCurrentAlertFlag || lastStateFlag != controller->mCurrentTermState)
			{
				report = 1;
			}
		}

		if (0x01 != reportStrategy) // 定时
		{
			if (currentTimeMillis() - lastReportTime >= reportTimeInterval*1000)
			{
				lastReportTime = currentTimeMillis();
				report = 1;
			}
		}
		if (0x00 != reportStrategy) // 定距
		{
			if (controller->mVehicalDriveDistance - lastReportDistance >= reportDistInterval)
			{
				lastReportDistance = controller->mVehicalDriveDistance;
				report = 1;
			}
		}

		if (report)
		{
			body.mExtraInfoCount = 0;
			
			lastReportTime2 = currentTimeMillis();

			lastAlertFlag = controller->mCurrentAlertFlag;
			lastStateFlag = controller->mCurrentTermState;
			
			body.mAlertFlag = controller->mCurrentAlertFlag;
			body.mStateFlag = controller->mCurrentTermState;
			getLocalBCDTime((char*)body.mDateTime);
			body.mSpeed = (WORD)(controller->mLocation.speed*10);
			body.mAltitude = (WORD)(controller->mLocation.altitude);
			body.mLatitude = (DWORD)(controller->mLocation.latitude*1000000);
			body.mLongitude = (DWORD)(controller->mLocation.longitude*1000000);
			body.mDirection = (WORD)controller->mLocation.bearing;
			
			JT808MsgBody_0200Ex_t ext = { 0 };
			ext.mExtraInfoID = 0x30; // 4G信号强度
			ext.mExtraInfoLen = 1;
			ext.mExtraInfoData[0] = gNet4GRild.signalStrength;
			JT808Session_InsertLocationExtra(session, &ext);
			
			ext.mExtraInfoID = 0x31; // GPS卫星数
			ext.mExtraInfoLen = 1;
			ext.mExtraInfoData[0] = controller->mLocation.satellites;
			JT808Session_InsertLocationExtra(session, &ext);

			if (checkTFCard(NULL))
			{
				ext.mExtraInfoID = 0x17; // TF卡故障
				ext.mExtraInfoLen = 2;
				ext.mExtraInfoData[1] = 0xFF;
				ext.mExtraInfoData[1] = 0xFE;
				JT808Session_InsertLocationExtra(session, &ext);
			}

			while (1)
			{
				JT808MsgBody_0200Ex_t* item = queue_list_popup(&session->mLocationExtraInfoList);
				if (NULL == item)
				{
					break;
				}
				queue_list_push(&body.mExtraInfoList, item);
				body.mExtraInfoCount ++;
			}

			JT808Session_SendMsg(session, 0x0200, &body, sizeof(body), 0);
			
			report = 0;
		}

		SleepMs(100);
	}
	
	JT808SESSION_PRINT(session, "thread task exit.");

	queue_list_destroy(&body.mExtraInfoList);

    pthread_exit(NULL);
    return NULL;
}

void JT808Session_InsertLocationExtra(JT808Session_t* session, JT808MsgBody_0200Ex_t* ext)
{
	int haveSame = 0;

	pthread_mutex_lock(&session->mLocationExtraInfoList.m_mutex);

	queue_t* head = session->mLocationExtraInfoList.m_head;
	while (head)
	{
		JT808MsgBody_0200Ex_t* item = head->m_content;
		if (item->mExtraInfoID == ext->mExtraInfoID)
		{
			haveSame = 1;
			break;
		}
		head = head->next;
	}
	
	pthread_mutex_unlock(&session->mLocationExtraInfoList.m_mutex);

	if (haveSame)
	{
		memcpy(head->m_content, ext, sizeof(JT808MsgBody_0200Ex_t));
	}
	else
	{
		JT808MsgBody_0200Ex_t* tmp = (JT808MsgBody_0200Ex_t*)malloc(sizeof(JT808MsgBody_0200Ex_t));
		if (NULL == tmp)
		{
			return;
		}
		memcpy(tmp, ext, sizeof(JT808MsgBody_0200Ex_t));
		queue_list_push(&session->mLocationExtraInfoList, tmp);
	}
}

void* JT808Session_UploadBlindLocationData(void* arg)
{
    pthread_detach(pthread_self());

    JT808Session_t* session = (JT808Session_t*)arg;

	JT808BlindUpload_DeleteLocationDataByState(&session->mBlineGpsData);

	JT808SESSION_PRINT(session, "thread task start.");

	int timestop = JT808BlindUpload_GetCacheNewestGpsTime(&session->mBlineGpsData) - 60; /* 前移1分钟，0200消息发送时就会保存盲区，收到应答才会删除，所以避免等待应答过程的消息被补报 */
	int timestart = 0;
	DWORD timeList[kUploadGpsCount] = { 0 };
	
	queue_list_t list = { 0 };

	JT808MsgBody_0704_t msg0704 = { 0 };

	msg0704.pItemsList = (JT808MsgBody_0704_Item_t*)malloc(sizeof(JT808MsgBody_0704_Item_t)*kUploadGpsCount);
	msg0704.mItemsType = 1; /* 盲区补报 */

	queue_list_create(&list, kUploadGpsCount);

	while (session->mMainProcessState == JT808SessionState_Running && session->mRunningFlag)
	{
		JT808SESSION_PRINT(session, "blind data starttime %d.", timestart);

		memset(timeList, 0, sizeof(DWORD)*kUploadGpsCount);
		msg0704.mItemsCount = 0;
		
		int count = JT808BlindUpload_GetDatasList(&session->mBlineGpsData, &list, kUploadGpsCount, timestop, timestart);

		if (0 == count)
		{
			JT808SESSION_PRINT(session, "blind data upload over.");
			break;
		}
		else
		{
			while (TRUE)
			{
				JT808LocationData_t* location = (JT808LocationData_t*)queue_list_popup(&list);
				
				if (location)
				{
					JT808SESSION_PRINT(session, "blind data [%d].", location->mTime);

					timestart = location->mTime+1;
					
					if (location->pData)
					{
						msg0704.pItemsList[msg0704.mItemsCount].mDataLen = location->mSize;
						memcpy(msg0704.pItemsList[msg0704.mItemsCount].mData, location->pData, location->mSize);
						
						timeList[msg0704.mItemsCount] = location->mTime;
							
						msg0704.mItemsCount ++;
						
						free(location->pData);
					}
					free(location);
				}
				else
				{
					break;
				}
			}

			if (RET_SUCCESS == JT808Session_SendMsg(session, 0x0704, &msg0704, sizeof(JT808MsgBody_0704_t), 0))
			{
				WORD serialNum = session->mMsgSerialNum-1;
				for (int i=0; i<count; i++)
				{
					if (timeList[i] != 0)
					{
						JT808BlindUpload_SetBatchSerialNum(&session->mBlineGpsData, timeList[i], serialNum);
					}
				}
			}
		}

		SleepMs(100);
	}

	queue_list_destroy(&list);

	if (msg0704.pItemsList)
	{
		free(msg0704.pItemsList);
	}

	JT808SESSION_PRINT(session, "thread task exit.");

    pthread_exit(NULL);
    return NULL;
}

int  JT808Session_OnTimerEventProcess(int fd, unsigned int event, void* arg)
{
	PNTEpoll_t* timerEpoll = (PNTEpoll_t*)arg;
    JT808Session_t* session = (JT808Session_t*)timerEpoll->mOwner;

	if (!(event & EPOLLIN))
	{
		return RET_FAILED;
	}

	if (fd == session->mHeartBeatTimerFd)
	{
	
	}
	else if (fd == session->mLocationReportTimerFd)
	{
	}

	uint64_t exp = 0;
	read(fd, &exp, sizeof(uint64_t));

	return RET_SUCCESS;
}

void JT808Session_Register(JT808Session_t* session)
{
	JT808MsgBody_0100_t body = { 0 };
	JT808Controller_t* controller = (JT808Controller_t*)session->mOnwer;

	body.mProvincialId = 44;
	body.mCityCountyId = 306;

	memcpy(body.mProducerId, "44030", sizeof(body.mProducerId));
	strcpy((char*)body.mProductModel, gBoardInfo.mProductModel);

	body.mLicensePlateColor = controller->mPlateColor;
	strcpy((char*)body.mLicensePlate, controller->mPlateNum);
	body.mLinceseSize = strlen(body.mLicensePlate);
	
	JT808Session_SendMsg(session, 0x0100, &body, sizeof(body), 0);
}

void JT808Session_Authentication(JT808Session_t* session)
{    
	JT808MsgBody_0102_t body = { 0 };

	strcpy(body.mAuthenticationCode, (char*)session->mSessionData->mAuthenticode);
	body.mAuthenticationCodeLen = strlen(body.mAuthenticationCode) + 1;
	strcpy((char*)body.mSoftVersion, gBoardInfo.mSwVersion);
	
	JT808Session_SendMsg(session, 0x0102, &body, sizeof(body), 0);
}

int JT808Session_ConnectStatusChange(int status, void* arg)
{
	PNTSocket_t* sock = (PNTSocket_t*)arg;
	JT808Session_t* session = (JT808Session_t*)sock->mOwner;

	if (TRUE == status)
	{
		session->mMainProcessState = JT808SessionState_StartRegister;
	}
	else//if (FALSE == status)
	{
		session->mMainProcessState = JT808SessionState_Offline;
	}

    return RET_SUCCESS;
}

void* JT808Session_MainProcess(void* arg)
{
    pthread_detach(pthread_self());
	
    JT808Session_t* session = (JT808Session_t*)arg;

	int phoneNumLen = PHONE_NUM_LEN;
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		phoneNumLen = PHONE_NUM_LEN_2019;
	}

	ConvertPhoneNomToBCD(session->mPhoneNum, phoneNumLen/* 协议手机号字节数 */, session->mSessionData->mPhoneNum);

	JT808BlindUpload_DBInit(&session->mBlineGpsData, session);

	session->mRunningFlag = TRUE;

	JT808ActiveSafety_UploadInit(&session->mAlarmUpload, session);
    
    int threadRet = pthread_create(&session->mSendQueueProcessPid, NULL, JT808Session_SendQueueProcess, session);
    if (threadRet != 0)
    {
        JT808SESSION_PRINT(session, "fail to create thead of JT808Session_SendQueueProcess [%s:%d], caused by:%s", session->mSessionData->mAddress, session->mSessionData->mPort, strerror(threadRet));
        goto exit;
    }
    
    threadRet = pthread_create(&session->mRecvQueueProcessPid, NULL, JT808Session_RecvQueueProcess, session);
    if (threadRet != 0)
    {
        JT808SESSION_PRINT(session, "fail to create thead of JT808Session_RecvQueueProcess [%s:%d], caused by:%s", session->mSessionData->mAddress, session->mSessionData->mPort, strerror(threadRet));
        goto exit;
    }
    
    threadRet = pthread_create(&session->mWaitAckMsgQueueProcessPid, NULL, JT808Session_WaitAckMsgQueueProcess, session);
    if (threadRet != 0)
    {
        JT808SESSION_PRINT(session, "fail to create thead of JT808Session_WaitAckMsgQueueProcess [%s:%d], caused by:%s", session->mSessionData->mAddress, session->mSessionData->mPort, strerror(threadRet));
        goto exit;
    }

	threadRet = pthread_create(&session->mLocationReportProcessPid, NULL, JT808Session_LocationMsgReportProcess, session);
    if (threadRet != 0)
    {
        JT808SESSION_PRINT(session, "fail to create thead of JT808Session_LocationMsgReportProcess [%s:%d], caused by:%s", session->mSessionData->mAddress, session->mSessionData->mPort, strerror(threadRet));
        goto exit;
    }

	pthread_mutex_init(&session->mSendMsgMutex, NULL);

	LibPnt_EpollCreate(&session->mTimerEpoll, 1024, session);
	LibPnt_EpollSetOnEvent(&session->mTimerEpoll, JT808Session_OnTimerEventProcess);
    LibPnt_EpollSetState(&session->mTimerEpoll, TRUE);

	session->mLocationReportTimerFd = LibPnt_EpollCreateTimer(&session->mTimerEpoll, 10*1000);
	
	// 注册，鉴权，保存鉴权码
	// 重连后直接鉴权
	while (session->mRunningFlag)
	{
		switch (session->mMainProcessState)
		{
			case JT808SessionState_Waiting:
				break;

			case JT808SessionState_StartRegister:
				{
					JT808Session_Register(session);
					
					session->mMainProcessState = JT808SessionState_Waiting;
				}
				break;

			case JT808SessionState_StartAuthentication:
				{
					JT808Session_Authentication(session);
					
					session->mMainProcessState = JT808SessionState_Waiting;
				}
				break;

			case JT808SessionState_Running:
				break;

			case JT808SessionState_Offline:
				break;
		}

		JT808Session_RecvPacketQueueProcess(session);

		SleepMs(100);
	}

	LibPnt_EpollDestroyTimer(&session->mTimerEpoll, session->mLocationReportTimerFd);

    LibPnt_EpollSetState(&session->mTimerEpoll, FALSE);

	LibPnt_EpollClose(&session->mTimerEpoll);

exit:

	session->mRunningFlag = FALSE;

	session->mMainProcessPid = 0;

	JT808SESSION_PRINT(session, "thread task exit.");

    pthread_exit(NULL);
    return NULL;
}

int JT808Session_Start(JT808Session_t* session)
{
    /* 初始化Socket */
    int beatheart_interval = 60*1000; // 1min钟

	if (NULL == session || NULL == session->mSessionData)
	{
        JT808SESSION_PRINT(session, "Param error");
        return RET_PARAMERR;
	}
	
    LibPnt_SocketInit(&session->mMainSocket, (char*)session->mSessionData->mAddress, (int)session->mSessionData->mPort, beatheart_interval, session);

    LibPnt_SocketSetRcvDataProcess(&session->mMainSocket, JT808Session_RcvDataProcess);
    
    LibPnt_SocketSetOnConnStatusChange(&session->mMainSocket, JT808Session_ConnectStatusChange);

    int ret = LibPnt_SocketStart(&session->mMainSocket);

    if (RET_FAILED == ret)
    {
        JT808SESSION_PRINT(session, "fail to create socket [%s:%d]", session->mSessionData->mAddress, session->mSessionData->mPort);
        return RET_FAILED;
    }
    
    int threadRet = pthread_create(&session->mMainProcessPid, NULL, JT808Session_MainProcess, session);
    if (threadRet != 0)
    {
        JT808SESSION_PRINT(session, "fail to create thead of JT808Session_MainProcess [%s:%d], caused by:%s", session->mSessionData->mAddress, session->mSessionData->mPort, strerror(threadRet));
        return RET_FAILED;
    }

	JT808SESSION_PRINT(session, "start success.");

    return RET_SUCCESS;    
}

int JT808Session_Release(JT808Session_t* session)
{
	if (NULL == session)
	{
		return RET_PARAMERR;
	}

	session->mRunningFlag = FALSE;

	LibPnt_SocketStop(&session->mMainSocket);

	while (0 != session->mMainProcessPid || 
		0 != session->mWaitAckMsgQueueProcessPid || 
		0 != session->mRecvQueueProcessPid || 
		0 != session->mSendQueueProcessPid) 
		SleepMs(100);

	while(1)
	{
		JT808MsgNode_t* node = queue_list_popup(&session->mRecvMsgQueue);
		if (NULL == node)
		{
			break;
		}
		if (NULL != node->mMsgHead)
		{
			free(node->mMsgHead);
		}
		if (NULL != node->mMsgBody)
		{
			free(node->mMsgBody);
		}
		free(node);
	}
	queue_list_destroy(&session->mRecvMsgQueue);
	
	while(1)
	{
		JT808MsgNode_t* node = queue_list_popup(&session->mSendMsgQueue);
		if (NULL == node)
		{
			break;
		}
		if (NULL != node->mMsgHead)
		{
			free(node->mMsgHead);
		}
		if (NULL != node->mMsgBody)
		{
			free(node->mMsgBody);
		}
		free(node);
	}
	queue_list_destroy(&session->mSendMsgQueue);
	
	while(1)
	{
		JT808MsgWaitAckNode_t* node = queue_list_popup(&session->mWaitAckMsgQueue);
		if (NULL == node)
		{
			break;
		}
		if (NULL != node->mMsgData)
		{
			free(node->mMsgData);
		}
		free(node);
	}
	queue_list_destroy(&session->mWaitAckMsgQueue);
	
	while(1)
	{
		JT808MsgNode_t* node = queue_list_popup(&session->mPacketedSendMsgQueue);
		if (NULL == node)
		{
			break;
		}
		if (NULL != node->mMsgHead)
		{
			free(node->mMsgHead);
		}
		if (NULL != node->mMsgBody)
		{
			free(node->mMsgBody);
		}
		free(node);
	}
	queue_list_destroy(&session->mPacketedSendMsgQueue);
	
	while(1)
	{
		JT808MsgRecvPacketNode_t* node = queue_list_popup(&session->mPacketedRecvMsgQueue);
		if (NULL == node)
		{
			break;
		}
		if (NULL != node->mPackIdxList)
		{
			free(node->mPackIdxList);
		}
		if (NULL != node->mMsgBody)
		{
			free(node->mMsgBody);
		}
		free(node);
	}
	queue_list_destroy(&session->mPacketedRecvMsgQueue);
	
	while(1)
	{
		JT808MsgBody_0200Ex_t* node = queue_list_popup(&session->mLocationExtraInfoList);
		if (NULL == node)
		{
			break;
		}
		free(node);
	}
	queue_list_destroy(&session->mLocationExtraInfoList);

	pthread_mutex_destroy(&session->mSendMsgMutex);

	PNT_LOGI("Session Part release.");

	return RET_SUCCESS;
}

