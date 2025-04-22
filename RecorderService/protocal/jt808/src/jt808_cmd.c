#include <stdlib.h>
#include <string.h>
#include "typedef.h"
#include "pnt_log.h"
#include "jt808_data.h"
#include "jt808_utils.h"
#include "jt808_session.h"

/* 01 BYTE, 02 WORD, 03 DWORD, 04 BCD, 05 STRING, 06 STRUCT */
BYTE JT808_Get_ParamID_Datatype(DWORD paramID)
{
	BYTE dataType = 0x03;

	switch (paramID)
	{		
		case 0x0084:
		case 0x0090:
		case 0x0091:
		case 0x0092:
		case 0x0094:
			dataType = 0x01;
			break;
		
		case 0x0031:
		case 0x005B:
		case 0x005C:
		case 0x005D:
		case 0x005E:
		case 0x0081:
		case 0x0082:
		case 0x0101:
		case 0x0103:
			dataType = 0x02;
			break;

		case 0x0110:
		case 0x0111:
		case 0x0112:
		case 0x0113:
		case 0x0114:
		case 0x0115:
		case 0x0116:
		case 0x0117:
			dataType = 0x04;
			break;
		
		case 0x0010:
		case 0x0011:
		case 0x0012:
		case 0x0013:
		case 0x0014:
		case 0x0015:
		case 0x0016:
		case 0x0017:
		case 0x001A:
		case 0x001D:
		case 0x0040:
		case 0x0041:
		case 0x0042:
		case 0x0043:
		case 0x0044:
		case 0x0048:
		case 0x0049:
		case 0x0083:
			dataType = 0x05;
			break;
		
		case 0x0075:
		case 0x0076:
		case 0x0077:
		case 0x0079:
		case 0x007B:
		case 0x007C:
		case 0xF364:
		case 0xF365:
			dataType = 0x06;
			break;

		default:
			dataType = 0x03;
			break;
	}

	return dataType;
}

int JT808_HEAD_Encoder(void* in, uint8_t* out, int len, void* pOwner)
{
    JT808MsgHeader_t* param = (JT808MsgHeader_t*)in;
	JT808Session_t* session = (JT808Session_t*)pOwner;

    int index = 0, phoneNumLen = PHONE_NUM_LEN;

	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		phoneNumLen = PHONE_NUM_LEN_2019;
	}

    index = NetbufPushWORD(out, param->mMsgID, index);
    index = NetbufPushWORD(out, param->mMsgAttribute, index);
	
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
    	index = NetbufPushBYTE(out, param->mProtocalVer, index);
	}
    index = NetbufPushBuffer(out, param->mPhoneNum, index, phoneNumLen);
    index = NetbufPushWORD(out, param->mMsgSerialNum, index);

    if (JT808MSG_ATTR_IS_PACK(param->mMsgAttribute))
    {
        index = NetbufPushWORD(out, param->mPacketTotalNum, index);
        index = NetbufPushWORD(out, param->mPacketIndex, index);
    }

    return index;
}

int JT808_HEAD_Parser(uint8_t* in, void** out, int len, void* pOwner)
{
    JT808MsgHeader_t* param = (JT808MsgHeader_t*)(*out);
	JT808Session_t* session = (JT808Session_t*)pOwner;
	
    if (NULL == param)
    {
        param = (JT808MsgHeader_t*)malloc(sizeof(JT808MsgHeader_t));
		memset(param, 0, sizeof(JT808MsgHeader_t));
		
        if (param == NULL)
        {
            return RET_FAILED;
        }
    }
    
    int index = 0, phoneNumLen = PHONE_NUM_LEN;

	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		phoneNumLen = PHONE_NUM_LEN_2019;
	}

    index = NetbufPopWORD(in, &param->mMsgID, index);
    index = NetbufPopWORD(in, &param->mMsgAttribute, index);
	
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
    	index = NetbufPopBYTE(in, &param->mProtocalVer, index);
	}
	
    index = NetbufPopBuffer(in, param->mPhoneNum, index, phoneNumLen);
    index = NetbufPopWORD(in, &param->mMsgSerialNum, index);

	//char phoneNum[32] = { 0 };
	//for (int i=0; i<phoneNumLen; i++)
	//{
	//	sprintf(phoneNum, "%s%02X", phoneNum, param->mPhoneNum[i]);
	//}
	
    //JT808SESSION_PRINT(session, "MsgHead -> | %04X | %04X | %d | %s | %04X ", 
    //    param->mMsgID, param->mMsgAttribute, param->mProtocalVer, phoneNum, param->mMsgSerialNum);
    
    if (JT808MSG_ATTR_IS_PACK(param->mMsgAttribute))
    {
        index = NetbufPopWORD(in, &param->mPacketTotalNum, index);
        index = NetbufPopWORD(in, &param->mPacketIndex, index);
        //JT808SESSION_PRINT(session, "     Has PacketItem -> %04X  %04X.", param->mPacketTotalNum, param->mPacketIndex);
    }

    *out = param;

    return index;
}

// 
int JT808_0001_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    JT808MsgBody_0001_t* param = (JT808MsgBody_0001_t*)in; 

    int index = 0;

    index = NetbufPushWORD(out, param->mResponseSerialNum, index);
    index = NetbufPushWORD(out, param->mMsgID, index);
    index = NetbufPushBYTE(out, param->mResult, index);

    return index;
} 

int JT808_0001_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    JT808MsgBody_0001_t* param = (JT808MsgBody_0001_t*)(*out); 
	JT808Session_t* session = (JT808Session_t*)pOwner;
    if (NULL == param)
    {
        param = (JT808MsgBody_0001_t*)malloc(sizeof(JT808MsgBody_0001_t));
    }

    int index = 0;

    *out = param;

    return index;
} 

int JT808_8001_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8001_t* param = (JT808MsgBody_8001_t*)in; 

    return index;
} 

int JT808_8001_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    JT808MsgBody_8001_t* param = (JT808MsgBody_8001_t*)(*out); 
	JT808Session_t* session = (JT808Session_t*)pOwner;
    if (NULL == param)
    {
        param = (JT808MsgBody_8001_t*)malloc(sizeof(JT808MsgBody_8001_t));
		memset(param, 0, sizeof(JT808MsgBody_8001_t));
    }

    int index = 0;

    index = NetbufPopWORD(in, &param->mResponseSerialNum, index);
    index = NetbufPopWORD(in, &param->mMsgID, index);
    index = NetbufPopBYTE(in, &param->mResult, index);

    *out = param;

    JT808SESSION_PRINT(session, "Msg8001 -> | %04X | %04X | %02X .", param->mResponseSerialNum, param->mMsgID, param->mResult);

    return index;
} 

int JT808_0002_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0002_t* param = (JT808MsgBody_0002_t*)in;

	// 0002 消息体为空

    return index;
} 

int JT808_0002_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0002_t* param = (JT808MsgBody_0002_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0002_t*)malloc(sizeof(JT808MsgBody_0002_t));
    }

    *out = param;

    return index;
} 

int JT808_8003_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8003_t* param = (JT808MsgBody_8003_t*)in;

    return index;
} 

int JT808_8003_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8003_t* param = (JT808MsgBody_8003_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8003_t*)malloc(sizeof(JT808MsgBody_8003_t));
		memset(param, 0, sizeof(JT808MsgBody_8003_t));
    }

	index = NetbufPopWORD(in, &param->mTargetMsgSerialNum, index);
	index = NetbufPopBYTE(in, &param->mReUploadPacksCount, index);

	param->pReUploadPackIDs = (WORD*)malloc(param->mReUploadPacksCount*sizeof(WORD));

	for (int i=0; i<param->mReUploadPacksCount; i++)
	{
		index = NetbufPopWORD(in, &param->pReUploadPackIDs[i], index);
	}

    *out = param;

    return index;
} 

int JT808_0005_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0005_t* param = (JT808MsgBody_0005_t*)in;

	index = NetbufPushWORD(out, param->mTargetMsgSerialNum, index);
	index = NetbufPushWORD(out, param->mReUploadPacksCount, index);

	for (int i=0; i<param->mReUploadPacksCount; i++)
	{
		index = NetbufPushWORD(out, param->pReUploadPackIDs[i], index);
	}

    return index;
} 

int JT808_0005_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0005_t* param = (JT808MsgBody_0005_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0005_t*)malloc(sizeof(JT808MsgBody_0005_t));
    }

    *out = param;

    return index;
} 

int JT808_0100_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0100_t* param = (JT808MsgBody_0100_t*)in;
	JT808Session_t* session = (JT808Session_t*)pOwner;

	index = NetbufPushWORD(out, param->mProvincialId, index);
	index = NetbufPushWORD(out, param->mCityCountyId, index);

	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		index = NetbufPushBuffer(out, param->mProducerId, index, PRODUCERID_LEN_2019);
		index = NetbufPushBuffer(out, param->mProductModel, index, PRODUCTMODEL_LEN_2019);
		index = NetbufPushBuffer(out, param->mTerminalId, index, TERMINALID_LEN_2019);
	}
	else
	{
		index = NetbufPushBuffer(out, param->mProducerId, index, PRODUCERID_LEN);
		index = NetbufPushBuffer(out, param->mProductModel, index, PRODUCTMODEL_LEN);
		index = NetbufPushBuffer(out, param->mTerminalId, index, TERMINALID_LEN);
	}
	index = NetbufPushBYTE(out, param->mLicensePlateColor, index);
	index = NetbufPushBuffer(out, (BYTE*)param->mLicensePlate, index, param->mLinceseSize);

    return index;
} 

int JT808_0100_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0100_t* param = (JT808MsgBody_0100_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0100_t*)malloc(sizeof(JT808MsgBody_0100_t));
    }

    *out = param;

    return index;
} 

int JT808_8100_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8100_t* param = (JT808MsgBody_8100_t*)in;

    return index;
} 

int JT808_8100_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8100_t* param = (JT808MsgBody_8100_t*)(*out);
	JT808Session_t* session = (JT808Session_t*)pOwner;
    if (NULL == param)
    {
        param = (JT808MsgBody_8100_t*)malloc(sizeof(JT808MsgBody_8100_t));
		memset(param, 0, sizeof(JT808MsgBody_8100_t));
    }

	index = NetbufPopWORD(in, &param->mResponseSerialNum, index);
	index = NetbufPopBYTE(in, &param->mResult, index);
	if (0 == param->mResult)
	{
		param->mAuthenticationCodeLen = len - 3;
		index = NetbufPopBuffer(in, (BYTE*)param->mAuthenticationCode, index, param->mAuthenticationCodeLen);
	}
		
    //JT808SESSION_PRINT(session, "Msg8100 -> | %04X | %02X | [%d]%s .", param->mResponseSerialNum, param->mResult, param->mAuthenticationCodeLen, param->mAuthenticationCode);
	
	*out = param;

    return index;
} 

int JT808_0003_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0003_t* param = (JT808MsgBody_0003_t*)in;

    return index;
} 

int JT808_0003_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0003_t* param = (JT808MsgBody_0003_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0003_t*)malloc(sizeof(JT808MsgBody_0003_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0004_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0004_t* param = (JT808MsgBody_0004_t*)in;

    return index;
} 

int JT808_0004_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0004_t* param = (JT808MsgBody_0004_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0004_t*)malloc(sizeof(JT808MsgBody_0004_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8004_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8004_t* param = (JT808MsgBody_8004_t*)in;

    return index;
} 

int JT808_8004_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8004_t* param = (JT808MsgBody_8004_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8004_t*)malloc(sizeof(JT808MsgBody_8004_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0102_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
	JT808Session_t* session = (JT808Session_t*)pOwner;
	
    JT808MsgBody_0102_t* param = (JT808MsgBody_0102_t*)in;

	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		index = NetbufPushBYTE(out, param->mAuthenticationCodeLen, index);
	}
	
	index = NetbufPushBuffer(out, (BYTE*)param->mAuthenticationCode, index, param->mAuthenticationCodeLen);
	
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		index = NetbufPushBuffer(out, param->mIMEINumber, index, sizeof(param->mIMEINumber));
		index = NetbufPushBuffer(out, param->mSoftVersion, index, sizeof(param->mSoftVersion));
	}
	
    return index;
} 

int JT808_0102_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0102_t* param = (JT808MsgBody_0102_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0102_t*)malloc(sizeof(JT808MsgBody_0102_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8103_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8103_t* param = (JT808MsgBody_8103_t*)in;

    return index;
} 

int JT808_8103_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{
    int index = 0;
    JT808MsgBody_8103_t* param = (JT808MsgBody_8103_t*)(*out);
	JT808Session_t* session = (JT808Session_t*)pOwner;
    if (NULL == param)
    {
        param = (JT808MsgBody_8103_t*)malloc(sizeof(JT808MsgBody_8103_t));
		memset(param, 0, sizeof(JT808MsgBody_8103_t));
    }

	index = NetbufPopBYTE(in, &param->mParamItemCount, index);
	
	param->pParamList = (JT808MsgBody_8103_Item_t*)malloc(sizeof(JT808MsgBody_8103_Item_t)*param->mParamItemCount);
	
	for (int i=0; i<param->mParamItemCount; i++)
	{
		JT808MsgBody_8103_Item_t* item = &param->pParamList[i];
		
		index = NetbufPopDWORD(in, &item->mParamID, index);
		index = NetbufPopBYTE(in, &item->mParamDataLen, index);
		
		item->mDataType = JT808_Get_ParamID_Datatype(item->mParamID);
		
		if (0x01 == item->mDataType)
		{
			index = NetbufPopBYTE(in, (BYTE*)&item->mParamData, index);
			item->mParamData &= 0xFF;
		}
		else if (0x02 == item->mDataType)
		{
			index = NetbufPopWORD(in, (WORD*)&item->mParamData, index);
			item->mParamData &= 0xFFFF;
		}
		else if (0x03 == item->mDataType)
		{
			index = NetbufPopDWORD(in, (DWORD*)&item->mParamData, index);
		}
		else if (0x04 == item->mDataType || 0x06 == item->mDataType || 0x05 == item->mDataType)
		{
			item->mParamData = (DWORD)malloc(item->mParamDataLen);
			index = NetbufPopBuffer(in, (BYTE*)item->mParamData, index, item->mParamDataLen);
		}
		
	}

	*out = param;

    return index;
} 

int JT808_8104_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8104_t* param = (JT808MsgBody_8104_t*)in;

    return index;
} 

int JT808_8104_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8104_t* param = (JT808MsgBody_8104_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8104_t*)malloc(sizeof(JT808MsgBody_8104_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0104_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0, count = 0;
    JT808MsgBody_0104_t* param = (JT808MsgBody_0104_t*)in;

	index = NetbufPushWORD(out, param->mResponseSerialNum, index);
	index = NetbufPushBYTE(out, param->mParamItemCount, index);

	while (1)
	{
		JT808MsgBody_8103_Item_t* item = queue_list_popup(&param->pParamList);
		if (NULL == item)
		{
			break;
		}
		
		index = NetbufPushDWORD(out, item->mParamID, index);
		index = NetbufPushBYTE(out, item->mParamDataLen, index);
		
		if (0x01 == item->mDataType)
		{
			index = NetbufPushBYTE(out, (BYTE)item->mParamData, index);
		}
		else if (0x02 == item->mDataType)
		{
			index = NetbufPushWORD(out, (WORD)item->mParamData, index);
		}
		else if (0x03 == item->mDataType)
		{
			index = NetbufPushDWORD(out, item->mParamData, index);
		}
		else if (0x04 == item->mDataType || 0x05 == item->mDataType || 0x06 == item->mDataType)
		{
			index = NetbufPushBuffer(out, (BYTE*)item->mParamData, index, item->mParamDataLen);
			free((BYTE*)item->mParamData);
		}
		
		free(item);
		count ++;
	}

	NetbufPushBYTE(out, count, 2);
	
    return index;
} 

int JT808_0104_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0104_t* param = (JT808MsgBody_0104_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0104_t*)malloc(sizeof(JT808MsgBody_0104_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8105_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8105_t* param = (JT808MsgBody_8105_t*)in;

    return index;
} 

int JT808_8105_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8105_t* param = (JT808MsgBody_8105_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8105_t*)malloc(sizeof(JT808MsgBody_8105_t));
		memset(param, 0, sizeof(JT808MsgBody_8105_t));
    }

	index = NetbufPopBYTE(in, &param->mTerminalCtrlCmd, index);
	
	param->mTerminalCtrlParamLen = len - index;
	
	if (param->mTerminalCtrlParamLen > 0)
	{
		param->pTerminalCtrlParam = (char*)malloc(param->mTerminalCtrlParamLen+1);
		memset(param->pTerminalCtrlParam, 0, param->mTerminalCtrlParamLen+1);
		
		index = NetbufPopBuffer(in, (BYTE*)param->pTerminalCtrlParam, index, param->mTerminalCtrlParamLen);
	}

	*out = param;

    return index;
} 

int JT808_8106_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8106_t* param = (JT808MsgBody_8106_t*)in;

    return index;
} 

int JT808_8106_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8106_t* param = (JT808MsgBody_8106_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8106_t*)malloc(sizeof(JT808MsgBody_8106_t));
		memset(param, 0, sizeof(JT808MsgBody_8106_t));
    }

	index = NetbufPopBYTE(in, &param->mParamItemCount, index);
	for (int i=0; i<param->mParamItemCount; i++)
	{
		index = NetbufPopDWORD(in, &param->mParamIDList[i], index);
	}

	*out = param;

    return index;
} 

int JT808_8107_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8107_t* param = (JT808MsgBody_8107_t*)in;

    return index;
} 

int JT808_8107_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8107_t* param = (JT808MsgBody_8107_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8107_t*)malloc(sizeof(JT808MsgBody_8107_t));
    }

    return index;
} 

int JT808_0107_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0107_t* param = (JT808MsgBody_0107_t*)in;
	JT808Session_t* session = (JT808Session_t*)pOwner;

	index = NetbufPushWORD(out, param->mTerminalType, index);
	if (session->mSessionData->mSessionType == SESSION_TYPE_JT808)
	{
		index = NetbufPushBuffer(out, param->mProducerId, index, 5);
		index = NetbufPushBuffer(out, param->mTerminalModel, index, 20);
		index = NetbufPushBuffer(out, param->mTerminalId, index, 7);
		index = NetbufPushBuffer(out, param->mTerminalSIMICCIDCode, index, 10);
	}
	else
	{
		index = NetbufPushBuffer(out, param->mProducerId, index, 11);
		index = NetbufPushBuffer(out, param->mTerminalModel, index, 30);
		index = NetbufPushBuffer(out, param->mTerminalId, index, 30);
		index = NetbufPushBuffer(out, param->mTerminalSIMICCIDCode, index, 10);
	}
	index = NetbufPushBYTE(out, param->mTerminalHardwareVersionLen, index);
	index = NetbufPushBuffer(out, (BYTE*)param->mTerminalHardwareVersion, index, param->mTerminalHardwareVersionLen);
	
	index = NetbufPushBYTE(out, param->mTerminalFirmwareVersionLen, index);
	index = NetbufPushBuffer(out, (BYTE*)param->mTerminalFirmwareVersion, index, param->mTerminalFirmwareVersionLen);
	
	index = NetbufPushBYTE(out, param->mGNSSModuleAttribute, index);
	index = NetbufPushBYTE(out, param->mCommunicationModuleAttibute, index);

    return index;
} 

int JT808_0107_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0107_t* param = (JT808MsgBody_0107_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0107_t*)malloc(sizeof(JT808MsgBody_0107_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8108_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8108_t* param = (JT808MsgBody_8108_t*)in;

    return index;
} 

int JT808_8108_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8108_t* param = (JT808MsgBody_8108_t*)(*out);
	JT808Session_t* session = (JT808Session_t*)pOwner;
    if (NULL == param)
    {
        param = (JT808MsgBody_8108_t*)malloc(sizeof(JT808MsgBody_8108_t));
		memset(param, 0, sizeof(JT808MsgBody_8108_t));
    }

	index = NetbufPopBYTE(in, &param->mUpgradeType, index);
	if (session->mSessionData->mSessionType == SESSION_TYPE_JT808)
	{
		index = NetbufPushBuffer(in, param->mProducerID, index, 5);
	}
	else
	{
		index = NetbufPushBuffer(in, param->mProducerID, index, 11);
	}
	index = NetbufPopBYTE(in, &param->mTerminalFirmwareVersionLen, index);
	index = NetbufPopBuffer(in, (BYTE*)param->mTerminalFirmwareVersion, index, param->mTerminalFirmwareVersionLen);
	index = NetbufPopDWORD(in, &param->mTerminalFirmwareDataLen, index);
	param->pTerminalFirmwareData = (char*)(in + index);
		
	*out = param;

    return index;
} 

int JT808_0108_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0108_t* param = (JT808MsgBody_0108_t*)in;

	index = NetbufPushBYTE(out, param->mUpgradeType, index);
	index = NetbufPushBYTE(out, param->mUpgradeResult, index);

    return index;
} 

int JT808_0108_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0108_t* param = (JT808MsgBody_0108_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0108_t*)malloc(sizeof(JT808MsgBody_0108_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0200_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0200_t* param = (JT808MsgBody_0200_t*)in;

	index = NetbufPushDWORD(out, param->mAlertFlag, index);
	index = NetbufPushDWORD(out, param->mStateFlag, index);
	index = NetbufPushDWORD(out, param->mLatitude, index);
	index = NetbufPushDWORD(out, param->mLongitude, index);
	index = NetbufPushWORD(out, param->mAltitude, index);
	index = NetbufPushWORD(out, param->mSpeed, index);
	index = NetbufPushWORD(out, param->mDirection, index);
	index = NetbufPushBuffer(out, param->mDateTime, index, sizeof(param->mDateTime));

	while (1)
	{
		JT808MsgBody_0200Ex_t* item = queue_list_popup(&param->mExtraInfoList);
		if (NULL == item)
		{
			break;
		}

		if (item->mExtraInfoID > 0)
		{
			index = NetbufPushBYTE(out, item->mExtraInfoID, index);
			if (item->mExtraInfoLen > 0)
			{
				index = NetbufPushBYTE(out, item->mExtraInfoLen, index);
				index = NetbufPushBuffer(out, item->mExtraInfoData, index, item->mExtraInfoLen);
			}
		}

		free(item);
	}
	
    return index;
} 

int JT808_0200_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0200_t* param = (JT808MsgBody_0200_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0200_t*)malloc(sizeof(JT808MsgBody_0200_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8201_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8201_t* param = (JT808MsgBody_8201_t*)in;

    return index;
} 

int JT808_8201_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8201_t* param = (JT808MsgBody_8201_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8201_t*)malloc(sizeof(JT808MsgBody_8201_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0201_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0201_t* param = (JT808MsgBody_0201_t*)in;

	index = NetbufPushWORD(out, param->mResponseSerialNum, index);

	JT808MsgBody_0200_t* loc = &param->mLocationReport;

	index = NetbufPushDWORD(out, loc->mAlertFlag, index);
	index = NetbufPushDWORD(out, loc->mStateFlag, index);
	index = NetbufPushDWORD(out, loc->mLatitude, index);
	index = NetbufPushDWORD(out, loc->mLongitude, index);
	index = NetbufPushWORD(out, loc->mAltitude, index);
	index = NetbufPushWORD(out, loc->mSpeed, index);
	index = NetbufPushWORD(out, loc->mDirection, index);
	index = NetbufPushBuffer(out, loc->mDateTime, index, sizeof(loc->mDateTime));

	while (loc->mExtraInfoCount > 0)
	{
		JT808MsgBody_0200Ex_t* item = queue_list_popup(&loc->mExtraInfoList);
		if (NULL == item)
		{
			break;
		}

		if (item->mExtraInfoID > 0)
		{
			index = NetbufPushBYTE(out, item->mExtraInfoID, index);
			if (item->mExtraInfoLen > 0)
			{
				index = NetbufPushBYTE(out, item->mExtraInfoLen, index);
				index = NetbufPushBuffer(out, item->mExtraInfoData, index, item->mExtraInfoLen);
			}
		}

		free(item);
	}


    return index;
} 

int JT808_0201_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0201_t* param = (JT808MsgBody_0201_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0201_t*)malloc(sizeof(JT808MsgBody_0201_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8202_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8202_t* param = (JT808MsgBody_8202_t*)in;

    return index;
} 

int JT808_8202_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8202_t* param = (JT808MsgBody_8202_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8202_t*)malloc(sizeof(JT808MsgBody_8202_t));
		memset(param, 0, sizeof(JT808MsgBody_8202_t));
    }

	index = NetbufPopWORD(in, &param->mTimeIntervel, index);
	index = NetbufPopDWORD(in, &param->mValidDate, index);
		
	*out = param;

    return index;
} 

int JT808_8301_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8301_t* param = (JT808MsgBody_8301_t*)in;

    return index;
} 

int JT808_8301_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8301_t* param = (JT808MsgBody_8301_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8301_t*)malloc(sizeof(JT808MsgBody_8301_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0301_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0301_t* param = (JT808MsgBody_0301_t*)in;

    return index;
} 

int JT808_0301_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0301_t* param = (JT808MsgBody_0301_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0301_t*)malloc(sizeof(JT808MsgBody_0301_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8302_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8302_t* param = (JT808MsgBody_8302_t*)in;

    return index;
} 

int JT808_8302_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8302_t* param = (JT808MsgBody_8302_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8302_t*)malloc(sizeof(JT808MsgBody_8302_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0302_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0302_t* param = (JT808MsgBody_0302_t*)in;

    return index;
} 

int JT808_0302_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0302_t* param = (JT808MsgBody_0302_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0302_t*)malloc(sizeof(JT808MsgBody_0302_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8303_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8303_t* param = (JT808MsgBody_8303_t*)in;

    return index;
} 

int JT808_8303_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8303_t* param = (JT808MsgBody_8303_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8303_t*)malloc(sizeof(JT808MsgBody_8303_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0303_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0303_t* param = (JT808MsgBody_0303_t*)in;

    return index;
} 

int JT808_0303_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0303_t* param = (JT808MsgBody_0303_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0303_t*)malloc(sizeof(JT808MsgBody_0303_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8304_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8304_t* param = (JT808MsgBody_8304_t*)in;

    return index;
} 

int JT808_8304_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8304_t* param = (JT808MsgBody_8304_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8304_t*)malloc(sizeof(JT808MsgBody_8304_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8400_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8400_t* param = (JT808MsgBody_8400_t*)in;

    return index;
} 

int JT808_8400_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8400_t* param = (JT808MsgBody_8400_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8400_t*)malloc(sizeof(JT808MsgBody_8400_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8401_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8401_t* param = (JT808MsgBody_8401_t*)in;

    return index;
} 

int JT808_8401_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8401_t* param = (JT808MsgBody_8401_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8401_t*)malloc(sizeof(JT808MsgBody_8401_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8500_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8500_t* param = (JT808MsgBody_8500_t*)in;

    return index;
} 

int JT808_8500_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8500_t* param = (JT808MsgBody_8500_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8500_t*)malloc(sizeof(JT808MsgBody_8500_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0500_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0500_t* param = (JT808MsgBody_0500_t*)in;

    return index;
} 

int JT808_0500_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0500_t* param = (JT808MsgBody_0500_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0500_t*)malloc(sizeof(JT808MsgBody_0500_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8600_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8600_t* param = (JT808MsgBody_8600_t*)in;

    return index;
} 

int JT808_8600_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8600_t* param = (JT808MsgBody_8600_t*)(*out);
	JT808Session_t* session = (JT808Session_t*)pOwner;
	
    if (NULL == param)
    {
        param = (JT808MsgBody_8600_t*)malloc(sizeof(JT808MsgBody_8600_t));
		memset(param, 0, sizeof(JT808MsgBody_8600_t));
    }

	index = NetbufPopBYTE(in, &param->mSettingOption, index);
	index = NetbufPopBYTE(in, &param->mAreaCount, index);
	if (param->mAreaCount > 0)
	{
		param->pCircleAreaItems = (JT808_Circle_Area_t*)malloc(param->mAreaCount*sizeof(JT808_Circle_Area_t));
		memset(param->pCircleAreaItems, 0, param->mAreaCount*sizeof(JT808_Circle_Area_t));
		
		if (NULL != param->pCircleAreaItems)
		{			
			for (int i=0; i<param->mAreaCount; i++)
			{
				JT808_Circle_Area_t* item = &param->pCircleAreaItems[i];
			
				index = NetbufPopDWORD(in, &item->mAreaID, index);
				index = NetbufPopWORD(in, &item->mAreaAttribute, index);
				index = NetbufPopDWORD(in, &item->mAreaRoundotLatitude, index);
				index = NetbufPopDWORD(in, &item->mAreaRoundotLongitude, index);
				index = NetbufPopDWORD(in, &item->mAreaRadius, index);
				if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
				{
					index = NetbufPopBuffer(in, item->mStartTime, index, 6);
					index = NetbufPopBuffer(in, item->mStopTime, index, 6);
				}
				if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
				{
					index = NetbufPopWORD(in, &item->mSpeedLimit, index);
					index = NetbufPopBYTE(in, &item->mOverspeedContinueTime, index);
				}
				if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
				{
					if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
					{
						index = NetbufPopWORD(in, &item->mSpeedLimitNight, index);
					}
					index = NetbufPopWORD(in, &item->mAreaNameLen, index);
					index = NetbufPopBuffer(in, (BYTE*)item->mAreaName, index, item->mAreaNameLen);
				}
				if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_LATITUDE_FLAG)
				{
					item->mAreaRoundotLatitude = -item->mAreaRoundotLatitude;
				}
				if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_LONGITUDE_FLAG)
				{
					item->mAreaRoundotLongitude = -item->mAreaRoundotLongitude;
				}
			}
		}
	}
		
	*out = param;

    return index;
} 

int JT808_8601_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8601_t* param = (JT808MsgBody_8601_t*)in;

    return index;
} 

int JT808_8601_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8601_t* param = (JT808MsgBody_8601_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8601_t*)malloc(sizeof(JT808MsgBody_8601_t));
		memset(param, 0, sizeof(JT808MsgBody_8601_t));
    }

	index = NetbufPopBYTE(in, &param->mDeleteAreaCount, index);
	param->mDeleteAreaCount = (param->mDeleteAreaCount>125)?125:(param->mDeleteAreaCount);
	for (int i=0; i<param->mDeleteAreaCount; i++)
	{
		index = NetbufPopDWORD(in, &param->mDeleteAreaIDs[i], index);
	}
		
	*out = param;

    return index;
} 

int JT808_8602_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8602_t* param = (JT808MsgBody_8602_t*)in;

    return index;
} 

int JT808_8602_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8602_t* param = (JT808MsgBody_8602_t*)(*out);
	JT808Session_t* session = (JT808Session_t*)pOwner;
	
    if (NULL == param)
    {
        param = (JT808MsgBody_8602_t*)malloc(sizeof(JT808MsgBody_8602_t));
		memset(param, 0, sizeof(JT808MsgBody_8602_t));
    }

	index = NetbufPopBYTE(in, &param->mSettingOption, index);
	index = NetbufPopBYTE(in, &param->mAreaCount, index);
	if (param->mAreaCount > 0)
	{
		param->pRectangleAreaItems = (JT808_Rectangle_Area_t*)malloc(param->mAreaCount*sizeof(JT808_Rectangle_Area_t));
		memset(param->pRectangleAreaItems, 0, param->mAreaCount*sizeof(JT808_Rectangle_Area_t));
		
		if (NULL != param->pRectangleAreaItems)
		{			
			for (int i=0; i<param->mAreaCount; i++)
			{
				JT808_Rectangle_Area_t* item = &param->pRectangleAreaItems[i];
			
				index = NetbufPopDWORD(in, &item->mAreaID, index);
				index = NetbufPopWORD(in, &item->mAreaAttribute, index);
				index = NetbufPopDWORD(in, &item->mAreaLeftTopLatitude, index);
				index = NetbufPopDWORD(in, &item->mAreaLeftTopLongitude, index);
				index = NetbufPopDWORD(in, &item->mAreaRightBottomLatitude, index);
				index = NetbufPopDWORD(in, &item->mAreaRightBottomLongitude, index);
				if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
				{
					index = NetbufPopBuffer(in, item->mStartTime, index, 6);
					index = NetbufPopBuffer(in, item->mStopTime, index, 6);
				}
				if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
				{
					index = NetbufPopWORD(in, &item->mSpeedLimit, index);
					index = NetbufPopBYTE(in, &item->mOverspeedContinueTime, index);
				}
				if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
				{
					if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
					{
						index = NetbufPopWORD(in, &item->mSpeedLimitNight, index);
					}
					index = NetbufPopWORD(in, &item->mAreaNameLen, index);
					index = NetbufPopBuffer(in, (BYTE*)item->mAreaName, index, item->mAreaNameLen);
				}
				if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_LATITUDE_FLAG)
				{
					item->mAreaLeftTopLatitude = -item->mAreaLeftTopLatitude;
					item->mAreaRightBottomLatitude = -item->mAreaRightBottomLatitude;
				}
				if (item->mAreaAttribute & JT808_AREA_ATTRIBUTE_LONGITUDE_FLAG)
				{
					item->mAreaLeftTopLongitude = -item->mAreaLeftTopLongitude;
					item->mAreaRightBottomLongitude = -item->mAreaRightBottomLongitude;
				}
			}
		}
	}
		
	*out = param;

    return index;
} 

int JT808_8603_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8603_t* param = (JT808MsgBody_8603_t*)in;

    return index;
} 

int JT808_8603_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8603_t* param = (JT808MsgBody_8603_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8603_t*)malloc(sizeof(JT808MsgBody_8603_t));
		memset(param, 0, sizeof(JT808MsgBody_8603_t));
    }

	index = NetbufPopBYTE(in, &param->mDeleteAreaCount, index);
	param->mDeleteAreaCount = (param->mDeleteAreaCount>125)?125:(param->mDeleteAreaCount);
	for (int i=0; i<param->mDeleteAreaCount; i++)
	{
		index = NetbufPopDWORD(in, &param->mDeleteAreaIDs[i], index);
	}

	*out = param;

    return index;
} 

int JT808_8604_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8604_t* param = (JT808MsgBody_8604_t*)in;

    return index;
} 

int JT808_8604_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8604_t* param = (JT808MsgBody_8604_t*)(*out);
	JT808Session_t* session = (JT808Session_t*)pOwner;
	
    if (NULL == param)
    {
        param = (JT808MsgBody_8604_t*)malloc(sizeof(JT808MsgBody_8604_t));
		memset(param, 0, sizeof(JT808MsgBody_8604_t));
    }

	index = NetbufPopDWORD(in, &param->mAreaID, index);
	index = NetbufPopWORD(in, &param->mAreaAttribute, index);
	if (param->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_TIMEREGION)
	{
		index = NetbufPopBuffer(in, param->mStartTime, index, 6);
		index = NetbufPopBuffer(in, param->mStopTime, index, 6);
	}
	if (param->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
	{
		index = NetbufPopWORD(in, &param->mSpeedLimit, index);
		index = NetbufPopBYTE(in, &param->mOverspeedContinueTime, index);
	}
	
	index = NetbufPopWORD(in, &param->mPointCount, index);
	if (param->mPointCount > 0)
	{
		param->pPoints = (JT808_Area_Point_t*)malloc(param->mPointCount*sizeof(JT808_Area_Point_t));
		memset(param->pPoints, 0, param->mPointCount*sizeof(JT808_Area_Point_t));
		for (int i=0; i<param->mPointCount; i++)
		{
			index = NetbufPopDWORD(in, &param->pPoints[i].mLatitude, index);
			index = NetbufPopDWORD(in, &param->pPoints[i].mLongitude, index);
			if (param->mAreaAttribute & JT808_AREA_ATTRIBUTE_LATITUDE_FLAG)
			{
				param->pPoints[i].mLatitude = -param->pPoints[i].mLatitude;
			}
			if (param->mAreaAttribute & JT808_AREA_ATTRIBUTE_LONGITUDE_FLAG)
			{
				param->pPoints[i].mLongitude = -param->pPoints[i].mLongitude;
			}
		}
	}
	
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		if (param->mAreaAttribute & JT808_AREA_ATTRIBUTE_USE_SPEEDLIMIT)
		{
			index = NetbufPopWORD(in, &param->mSpeedLimitNight, index);
		}
		index = NetbufPopWORD(in, &param->mAreaNameLen, index);
		index = NetbufPopBuffer(in, (BYTE*)param->mAreaName, index, param->mAreaNameLen);
	}
		
	*out = param;

    return index;
} 

int JT808_8605_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8605_t* param = (JT808MsgBody_8605_t*)in;

    return index;
} 

int JT808_8605_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8605_t* param = (JT808MsgBody_8605_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8605_t*)malloc(sizeof(JT808MsgBody_8605_t));
		memset(param, 0, sizeof(JT808MsgBody_8605_t));
    }
	
	index = NetbufPopBYTE(in, &param->mDeleteAreaCount, index);
	param->mDeleteAreaCount = (param->mDeleteAreaCount>125)?125:(param->mDeleteAreaCount);
	for (int i=0; i<param->mDeleteAreaCount; i++)
	{
		index = NetbufPopDWORD(in, &param->mDeleteAreaIDs[i], index);
	}
	
	*out = param;

    return index;
} 

int JT808_8606_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8606_t* param = (JT808MsgBody_8606_t*)in;

    return index;
} 

int JT808_8606_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8606_t* param = (JT808MsgBody_8606_t*)(*out);
	JT808Session_t* session = (JT808Session_t*)pOwner;
	
    if (NULL == param)
    {
        param = (JT808MsgBody_8606_t*)malloc(sizeof(JT808MsgBody_8606_t));
		memset(param, 0, sizeof(JT808MsgBody_8606_t));
    }

	index = NetbufPopDWORD(in, &param->mRouteID, index);
	index = NetbufPopWORD(in, &param->mRouteAttribute, index);
	if (param->mRouteAttribute & JT808_ROUTE_ATTRIBUTE_USE_TIMEREGION)
	{
		index = NetbufPopBuffer(in, param->mStartTime, index, 6);
		index = NetbufPopBuffer(in, param->mStopTime, index, 6);
	}
	
	index = NetbufPopWORD(in, &param->mInflectionPointCount, index);
	if (param->mInflectionPointCount > 0)
	{
		param->pInflectionPoints = (JT808_InflectionPoint_t*)malloc(param->mInflectionPointCount*sizeof(JT808_InflectionPoint_t));
		memset(param->pInflectionPoints, 0, param->mInflectionPointCount*sizeof(JT808_InflectionPoint_t));
		
		for (int i=0; i<param->mInflectionPointCount; i++)
		{
			index = NetbufPopDWORD(in, &param->pInflectionPoints[i].mInflectionPointID, index);
			index = NetbufPopDWORD(in, &param->pInflectionPoints[i].mRoadID, index);
			index = NetbufPopDWORD(in, &param->pInflectionPoints[i].mInflectPointLatitude, index);
			index = NetbufPopDWORD(in, &param->pInflectionPoints[i].mInflectPointLongitude, index);
			index = NetbufPopBYTE(in, &param->pInflectionPoints[i].mRoadWidth, index);
			index = NetbufPopBYTE(in, &param->pInflectionPoints[i].mRoadAttribute, index);
			if (param->pInflectionPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_TIMELIMIT)
			{
				index = NetbufPopWORD(in, &param->pInflectionPoints[i].mRoadLongTimeThreashold, index);
				index = NetbufPopWORD(in, &param->pInflectionPoints[i].mRoadInsufficientThreashold, index);
			}
			if (param->pInflectionPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_USE_SPEEDLIMIT)
			{
				index = NetbufPopWORD(in, &param->pInflectionPoints[i].mSpeedLimit, index);
				index = NetbufPopBYTE(in, &param->pInflectionPoints[i].mOverspeedContinueTime, index);
				if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
				{
					index = NetbufPopWORD(in, &param->pInflectionPoints[i].mSpeedLimitNight, index);
				}
			}
			if (param->pInflectionPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_LATITUDE_FLAG)
			{
				param->pInflectionPoints[i].mInflectPointLatitude = -param->pInflectionPoints[i].mInflectPointLatitude;
			}
			if (param->pInflectionPoints[i].mRoadAttribute & JT808_ROAD_ATTRIBUTE_LONGITUDE_FLAG)
			{
				param->pInflectionPoints[i].mInflectPointLatitude = -param->pInflectionPoints[i].mInflectPointLatitude;
			}
		}
	}
	
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		index = NetbufPopWORD(in, &param->mAreaNameLen, index);
		index = NetbufPopBuffer(in, (BYTE*)param->mAreaName, index, param->mAreaNameLen);
	}
		
	*out = param;

    return index;
} 

int JT808_8607_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8607_t* param = (JT808MsgBody_8607_t*)in;

    return index;
} 

int JT808_8607_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{
    int index = 0;
    JT808MsgBody_8607_t* param = (JT808MsgBody_8607_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8607_t*)malloc(sizeof(JT808MsgBody_8607_t));
		memset(param, 0, sizeof(JT808MsgBody_8607_t));
    }
	
	index = NetbufPopBYTE(in, &param->mDeleteRouteCount, index);
	param->mDeleteRouteCount = (param->mDeleteRouteCount>125)?125:(param->mDeleteRouteCount);
	for (int i=0; i<param->mDeleteRouteCount; i++)
	{
		index = NetbufPopDWORD(in, &param->mDeleteRouteIDs[i], index);
	}
	
	*out = param;

    return index;
} 

int JT808_8700_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8700_t* param = (JT808MsgBody_8700_t*)in;

    return index;
} 

int JT808_8700_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8700_t* param = (JT808MsgBody_8700_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8700_t*)malloc(sizeof(JT808MsgBody_8700_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0700_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0700_t* param = (JT808MsgBody_0700_t*)in;

    return index;
} 

int JT808_0700_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0700_t* param = (JT808MsgBody_0700_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0700_t*)malloc(sizeof(JT808MsgBody_0700_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8701_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8701_t* param = (JT808MsgBody_8701_t*)in;

    return index;
} 

int JT808_8701_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8701_t* param = (JT808MsgBody_8701_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8701_t*)malloc(sizeof(JT808MsgBody_8701_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8203_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8203_t* param = (JT808MsgBody_8203_t*)in;

    return index;
} 

int JT808_8203_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8203_t* param = (JT808MsgBody_8203_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8203_t*)malloc(sizeof(JT808MsgBody_8203_t));
		memset(param, 0, sizeof(JT808MsgBody_8203_t));
    }

	index = NetbufPopWORD(in, &param->mAlertMsgSerialNumber, index);
	index = NetbufPopDWORD(in, &param->mAlertType, index);

	*out = param;

    return index;
} 

int JT808_8204_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8204_t* param = (JT808MsgBody_8204_t*)in;

    return index;
} 

int JT808_8204_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8204_t* param = (JT808MsgBody_8204_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8204_t*)malloc(sizeof(JT808MsgBody_8204_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8300_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8300_t* param = (JT808MsgBody_8300_t*)in;

    return index;
} 

int JT808_8300_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8300_t* param = (JT808MsgBody_8300_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8300_t*)malloc(sizeof(JT808MsgBody_8300_t));
		memset(param, 0, sizeof(JT808MsgBody_8300_t));
    }

	JT808Session_t* session = (JT808Session_t*)pOwner;

	index = NetbufPopBYTE(in, &param->mTextFlag, index);
	if (SESSION_TYPE_JT808_2019 == session->mSessionData->mSessionType)
	{
		index = NetbufPopBYTE(in, &param->mTextType, index);
	}
	index = NetbufPopBuffer(in, (BYTE*)param->mTextContent, index, len-index);
		
	*out = param;

    return index;
} 

int JT808_8702_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8702_t* param = (JT808MsgBody_8702_t*)in;

    return index;
} 

int JT808_8702_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8702_t* param = (JT808MsgBody_8702_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8702_t*)malloc(sizeof(JT808MsgBody_8702_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0702_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0702_t* param = (JT808MsgBody_0702_t*)in;

    return index;
} 

int JT808_0702_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0702_t* param = (JT808MsgBody_0702_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0702_t*)malloc(sizeof(JT808MsgBody_0702_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0704_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0704_t* param = (JT808MsgBody_0704_t*)in;

	index = NetbufPushWORD(out, param->mItemsCount, index);
	index = NetbufPushBYTE(out, param->mItemsType, index);

	for (int i=0; i<param->mItemsCount; i++)
	{
		JT808MsgBody_0704_Item_t* item = &param->pItemsList[i];
		
		index = NetbufPushWORD(out, item->mDataLen, index);
		index = NetbufPushBuffer(out, item->mData, index, item->mDataLen);
	}

    return index;
} 

int JT808_0704_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0704_t* param = (JT808MsgBody_0704_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0704_t*)malloc(sizeof(JT808MsgBody_0704_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0705_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0705_t* param = (JT808MsgBody_0705_t*)in;

    return index;
} 

int JT808_0705_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0705_t* param = (JT808MsgBody_0705_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0705_t*)malloc(sizeof(JT808MsgBody_0705_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0800_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0800_t* param = (JT808MsgBody_0800_t*)in;

	index = NetbufPushDWORD(out, param->mMultiMediaDataID, index);
	index = NetbufPushBYTE(out, param->mMultiMediaDataType, index);
	index = NetbufPushBYTE(out, param->mMultiMediaDataEncodeType, index);
	index = NetbufPushBYTE(out, param->mEventType, index);
	index = NetbufPushBYTE(out, param->mChannelID, index);

    return index;
} 

int JT808_0800_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0800_t* param = (JT808MsgBody_0800_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0800_t*)malloc(sizeof(JT808MsgBody_0800_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0801_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0801_t* param = (JT808MsgBody_0801_t*)in;

    return index;
} 

int JT808_0801_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0801_t* param = (JT808MsgBody_0801_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0801_t*)malloc(sizeof(JT808MsgBody_0801_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8800_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8800_t* param = (JT808MsgBody_8800_t*)in;

    return index;
} 

int JT808_8800_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8800_t* param = (JT808MsgBody_8800_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8800_t*)malloc(sizeof(JT808MsgBody_8800_t));
		memset(param, 0, sizeof(JT808MsgBody_8800_t));
    }

	index = NetbufPopDWORD(in, &param->mMultiMediaDataID, index);
	if (len > 4)
	{
		index = NetbufPopBYTE(in, &param->mReuploadPackCount, index);
		for (int i=0; i<param->mReuploadPackCount; i++)
		{
			index = NetbufPopWORD(in, &param->mReuploadPackIdxs[i], index);
		}
	}
	
	*out = param;

    return index;
} 

int JT808_8801_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8801_t* param = (JT808MsgBody_8801_t*)in;

    return index;
} 

int JT808_8801_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8801_t* param = (JT808MsgBody_8801_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8801_t*)malloc(sizeof(JT808MsgBody_8801_t));
		memset(param, 0, sizeof(JT808MsgBody_8801_t));
	}

	index = NetbufPopBYTE(in, &param->mCameraChannelID, index);
	index = NetbufPopWORD(in, &param->mCaptureCommand, index);
	index = NetbufPopWORD(in, &param->mCaptureIntervel, index);
	index = NetbufPopBYTE(in, &param->mStoreFlag, index);
	index = NetbufPopBYTE(in, &param->mResolution, index);
	index = NetbufPopBYTE(in, &param->mQuatity, index);
	index = NetbufPopBYTE(in, &param->mBrightness, index);
	index = NetbufPopBYTE(in, &param->mConstraction, index);
	index = NetbufPopBYTE(in, &param->mSaturation, index);
	index = NetbufPopBYTE(in, &param->mChrominance, index);
		
	*out = param;

    return index;
} 

int JT808_0805_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0805_t* param = (JT808MsgBody_0805_t*)in;

	index = NetbufPushWORD(out, param->mResponseSerialNum, index);
	index = NetbufPushBYTE(out, param->mResult, index);
	if (0 == param->mResult)
	{
		index = NetbufPushWORD(out, param->mIDCount, index);
		for (int i=0; i<param->mIDCount; i++)
		{
			index = NetbufPushDWORD(out, param->pIDList[i], index);
		}
	}

    return index;
} 

int JT808_0805_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0805_t* param = (JT808MsgBody_0805_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0805_t*)malloc(sizeof(JT808MsgBody_0805_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8802_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8802_t* param = (JT808MsgBody_8802_t*)in;

    return index;
} 

int JT808_8802_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8802_t* param = (JT808MsgBody_8802_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8802_t*)malloc(sizeof(JT808MsgBody_8802_t));
		memset(param, 0, sizeof(JT808MsgBody_8802_t));
    }

	index = NetbufPopBYTE(in, &param->mMultiMediaDataType, index);
	index = NetbufPopBYTE(in, &param->mChannelID, index);
	index = NetbufPopBYTE(in, &param->mEventType, index);

	index = NetbufPopBuffer(in, param->mStartTime, index, sizeof(param->mStartTime));
	index = NetbufPopBuffer(in, param->mEndTime, index, sizeof(param->mEndTime));
		
	*out = param;

    return index;
} 

int JT808_0802_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0802_t* param = (JT808MsgBody_0802_t*)in;

	index = NetbufPushWORD(out, param->mResponseSerialNum, index);
	if (NULL != param->pItemDatas)
	{
		index = NetbufPushWORD(out, param->mItemsCount, index);
		index = NetbufPushBuffer(out, param->pItemDatas, index, param->mDataLen);
	}
	else
	{
		param->mItemsCount = 0;
		index = NetbufPushWORD(out, param->mItemsCount, index);
	}

    return index;
} 

int JT808_0802_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0802_t* param = (JT808MsgBody_0802_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0802_t*)malloc(sizeof(JT808MsgBody_0802_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8608_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8608_t* param = (JT808MsgBody_8608_t*)in;

    return index;
} 

int JT808_8608_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8608_t* param = (JT808MsgBody_8608_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8608_t*)malloc(sizeof(JT808MsgBody_8608_t));
		memset(param, 0, sizeof(JT808MsgBody_8608_t));
    }

	index = NetbufPopBYTE(in, &param->mQueryType, index);
	index = NetbufPopDWORD(in, &param->mQueryIDCount, index);
	param->pQueryIDs = (DWORD*)malloc(sizeof(DWORD)*param->mQueryIDCount);
	if (NULL != param->pQueryIDs)
	{
		memset(param->pQueryIDs, 0, (sizeof(DWORD)*param->mQueryIDCount));
		for (int i=0; i<param->mQueryIDCount; i++)
		{
			index = NetbufPopDWORD(in, &param->pQueryIDs[i], index);
		}
	}
	else
	{
		param->mQueryIDCount = 0;
	}
		
	*out = param;

    return index;
} 

int JT808_0608_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0608_t* param = (JT808MsgBody_0608_t*)in;

	index = NetbufPushBYTE(out, param->mQueryType, index);
	index = NetbufPushDWORD(out, param->mQueryDataCount, index);
	index = NetbufPushBuffer(out, param->mDatas, index, param->mDataLen);

    return index;
} 

int JT808_0608_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0608_t* param = (JT808MsgBody_0608_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0608_t*)malloc(sizeof(JT808MsgBody_0608_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0701_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0701_t* param = (JT808MsgBody_0701_t*)in;

    return index;
} 

int JT808_0701_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0701_t* param = (JT808MsgBody_0701_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0701_t*)malloc(sizeof(JT808MsgBody_0701_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8803_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8803_t* param = (JT808MsgBody_8803_t*)in;

    return index;
} 

int JT808_8803_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8803_t* param = (JT808MsgBody_8803_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8803_t*)malloc(sizeof(JT808MsgBody_8803_t));
		memset(param, 0, sizeof(JT808MsgBody_8803_t));
    }

	index = NetbufPopBYTE(in, &param->mMultiMediaDataType, index);
	index = NetbufPopBYTE(in, &param->mChannelID, index);
	index = NetbufPopBYTE(in, &param->mEventType, index);
	index = NetbufPopBuffer(in, param->mStartTime, index, sizeof(param->mStartTime));
	index = NetbufPopBuffer(in, param->mEndTime, index, sizeof(param->mEndTime));

	index = NetbufPopBYTE(in, &param->mDeleteFlag, index);
	
	*out = param;

    return index;
} 

int JT808_8804_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8804_t* param = (JT808MsgBody_8804_t*)in;

    return index;
} 

int JT808_8804_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8804_t* param = (JT808MsgBody_8804_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8804_t*)malloc(sizeof(JT808MsgBody_8804_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8805_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8805_t* param = (JT808MsgBody_8805_t*)in;

    return index;
} 

int JT808_8805_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8805_t* param = (JT808MsgBody_8805_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8805_t*)malloc(sizeof(JT808MsgBody_8805_t));
		memset(param, 0, sizeof(JT808MsgBody_8805_t));
    }

	index = NetbufPopDWORD(in, &param->mMultiMediaDataID, index);
	index = NetbufPopBYTE(in, &param->mDeleteFlag, index);
		
	*out = param;

    return index;
} 

int JT808_8900_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8900_t* param = (JT808MsgBody_8900_t*)in;

    return index;
} 

int JT808_8900_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8900_t* param = (JT808MsgBody_8900_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8900_t*)malloc(sizeof(JT808MsgBody_8900_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0900_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0900_t* param = (JT808MsgBody_0900_t*)in;

    return index;
} 

int JT808_0900_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0900_t* param = (JT808MsgBody_0900_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0900_t*)malloc(sizeof(JT808MsgBody_0900_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0901_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0901_t* param = (JT808MsgBody_0901_t*)in;

    return index;
} 

int JT808_0901_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0901_t* param = (JT808MsgBody_0901_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0901_t*)malloc(sizeof(JT808MsgBody_0901_t));
    }
		
	*out = param;

    return index;
} 

int JT808_8A00_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8A00_t* param = (JT808MsgBody_8A00_t*)in;

    return index;
} 

int JT808_8A00_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_8A00_t* param = (JT808MsgBody_8A00_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_8A00_t*)malloc(sizeof(JT808MsgBody_8A00_t));
    }
		
	*out = param;

    return index;
} 

int JT808_0A00_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0A00_t* param = (JT808MsgBody_0A00_t*)in;

    return index;
} 

int JT808_0A00_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_0A00_t* param = (JT808MsgBody_0A00_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_0A00_t*)malloc(sizeof(JT808MsgBody_0A00_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9003_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9003_t* param = (JT808MsgBody_9003_t*)in;

    return index;
} 

int JT808_9003_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9003_t* param = (JT808MsgBody_9003_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9003_t*)malloc(sizeof(JT808MsgBody_9003_t));
    }
		
	*out = param;

    return index;
} 

int JT808_1003_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1003_t* param = (JT808MsgBody_1003_t*)in;

	index = NetbufPushBYTE(out, param->mAudioEncType, index);
	index = NetbufPushBYTE(out, param->mAudioTrackCount, index);
	index = NetbufPushBYTE(out, param->mAudioSampleRate, index);
	index = NetbufPushBYTE(out, param->mAudioSampleBit, index);
	index = NetbufPushWORD(out, param->mAudioFrameLen, index);
	index = NetbufPushBYTE(out, param->mIsSupportAudioOut, index);
	index = NetbufPushBYTE(out, param->mVideoEncType, index);
	index = NetbufPushBYTE(out, param->mAudioChnCount, index);
	index = NetbufPushBYTE(out, param->mVideoChnCount, index);

    return index;
} 

int JT808_1003_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1003_t* param = (JT808MsgBody_1003_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_1003_t*)malloc(sizeof(JT808MsgBody_1003_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9101_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9101_t* param = (JT808MsgBody_9101_t*)in;

    return index;
} 

int JT808_9101_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9101_t* param = (JT808MsgBody_9101_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9101_t*)malloc(sizeof(JT808MsgBody_9101_t));
		memset(param, 0, sizeof(JT808MsgBody_9101_t));
    }

	index = NetbufPopBYTE(in, &param->mIpLen, index);
	index = NetbufPopBuffer(in, param->mIp, index, param->mIpLen);
	index = NetbufPopWORD(in, &param->mTCPPort, index);
	index = NetbufPopWORD(in, &param->mUDPPort, index);
	index = NetbufPopBYTE(in, &param->mLogicChnNum, index);
	index = NetbufPopBYTE(in, &param->mDataType, index);
	index = NetbufPopBYTE(in, &param->mStreamType, index);
		
	*out = param;

    return index;
} 

int JT808_1005_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1005_t* param = (JT808MsgBody_1005_t*)in;

	index = NetbufPushBuffer(out, param->mStartBcdTime, index, 6);
	index = NetbufPushBuffer(out, param->mEndBcdTime, index, 6);
	index = NetbufPushWORD(out, param->mGetOnCount, index);
	index = NetbufPushWORD(out, param->mGetOffCount, index);

    return index;
} 

int JT808_1005_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1005_t* param = (JT808MsgBody_1005_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_1005_t*)malloc(sizeof(JT808MsgBody_1005_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9102_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9102_t* param = (JT808MsgBody_9102_t*)in;

    return index;
} 

int JT808_9102_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9102_t* param = (JT808MsgBody_9102_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9102_t*)malloc(sizeof(JT808MsgBody_9102_t));
		memset(param, 0, sizeof(JT808MsgBody_9102_t));
    }

	index = NetbufPopBYTE(in, &param->mLogicChnNum, index);
	index = NetbufPopBYTE(in, &param->mCtrlCmd, index);
	index = NetbufPopBYTE(in, &param->mStopType, index);
	index = NetbufPopBYTE(in, &param->mSwitchStreamType, index);
	
	*out = param;

    return index;
} 

int JT808_9105_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9105_t* param = (JT808MsgBody_9105_t*)in;

    return index;
} 

int JT808_9105_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9105_t* param = (JT808MsgBody_9105_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9105_t*)malloc(sizeof(JT808MsgBody_9105_t));
		memset(param, 0, sizeof(JT808MsgBody_9105_t));
    }

	index = NetbufPopBYTE(in, &param->mLogicChnNum, index);
	index = NetbufPopBYTE(in, &param->mLostPacketRate, index);
	
	*out = param;

    return index;
} 

int JT808_9205_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9205_t* param = (JT808MsgBody_9205_t*)in;

    return index;
} 

int JT808_9205_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9205_t* param = (JT808MsgBody_9205_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9205_t*)malloc(sizeof(JT808MsgBody_9205_t));
		memset(param, 0, sizeof(JT808MsgBody_9205_t));
    }

	index = NetbufPopBYTE(in, &param->mLogicChnNum, index);
	index = NetbufPopBuffer(in, param->mStartBcdTime, index, 6);
	index = NetbufPopBuffer(in, param->mEndBcdTime, index, 6);
	index = NetbufPopDWORD(in, &param->mAlertFlagh, index);
	index = NetbufPopDWORD(in, &param->mAlertFlagl, index);
	index = NetbufPopBYTE(in, &param->mResType, index);
	index = NetbufPopBYTE(in, &param->mStreamType, index);
	index = NetbufPopBYTE(in, &param->mStorageType, index);
	
	*out = param;

    return index;
} 

int JT808_1205_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1205_t* param = (JT808MsgBody_1205_t*)in;

	index = NetbufPushWORD(out, param->mReqSerialNum, index);
	index = NetbufPushDWORD(out, param->mResCount, index);

	while (1)
	{
		JT808MsgBody_1205_Item_t* item = (JT808MsgBody_1205_Item_t*)queue_list_popup(&param->mResList);
		if (NULL == item)
		{
			break;
		}
		
		index = NetbufPushBYTE(out, item->mLogicChnNum, index);
		index = NetbufPushBuffer(out, item->mStartBcdTime, index, 6);
		index = NetbufPushBuffer(out, item->mEndBcdTime, index, 6);
		index = NetbufPushDWORD(out, item->mAlertFlagh, index);
		index = NetbufPushDWORD(out, item->mAlertFlagl, index);
		index = NetbufPushBYTE(out, item->mResType, index);
		index = NetbufPushBYTE(out, item->mStreamType, index);
		index = NetbufPushBYTE(out, item->mStorageType, index);
		index = NetbufPushDWORD(out, item->mFileSize, index);

		free(item);
	}

    return index;
} 

int JT808_1205_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1205_t* param = (JT808MsgBody_1205_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_1205_t*)malloc(sizeof(JT808MsgBody_1205_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9201_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9201_t* param = (JT808MsgBody_9201_t*)in;

    return index;
} 

int JT808_9201_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9201_t* param = (JT808MsgBody_9201_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9201_t*)malloc(sizeof(JT808MsgBody_9201_t));
		memset(param, 0, sizeof(JT808MsgBody_9201_t));
    }

	index = NetbufPopBYTE(in, &param->mIpLen, index);
	index = NetbufPopBuffer(in, param->mIp, index, param->mIpLen);
	index = NetbufPopWORD(in, &param->mTCPPort, index);
	index = NetbufPopWORD(in, &param->mUDPPort, index);
	index = NetbufPopBYTE(in, &param->mLogicChnNum, index);
	index = NetbufPopBYTE(in, &param->mResType, index);
	index = NetbufPopBYTE(in, &param->mStreamType, index);
	index = NetbufPopBYTE(in, &param->mStorageType, index);
	index = NetbufPopBYTE(in, &param->mPlaybackMode, index);
	index = NetbufPopBYTE(in, &param->mFastSlowRate, index);
	index = NetbufPopBuffer(in, param->mStartBcdTime, index, 6);
	index = NetbufPopBuffer(in, param->mEndBcdTime, index, 6);

	if (param->mFastSlowRate > 5)
		param->mFastSlowRate = 0;
	else
		param->mFastSlowRate = (1 << (param->mFastSlowRate-1));
	
	*out = param;

    return index;
} 

int JT808_9202_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9202_t* param = (JT808MsgBody_9202_t*)in;

    return index;
} 

int JT808_9202_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9202_t* param = (JT808MsgBody_9202_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9202_t*)malloc(sizeof(JT808MsgBody_9202_t));
		memset(param, 0, sizeof(JT808MsgBody_9202_t));
    }
	
	index = NetbufPopBYTE(in, &param->mLogicChnNum, index);
	index = NetbufPopBYTE(in, &param->mPlaybackCtrl, index);
	index = NetbufPopBYTE(in, &param->mFastSlowRate, index);
	index = NetbufPopBuffer(in, param->mDragBcdTime, index, 6);

	if (param->mFastSlowRate > 5)
		param->mFastSlowRate = 0;
	else
		param->mFastSlowRate = (1 << (param->mFastSlowRate-1));
	
	*out = param;

    return index;
} 

int JT808_9206_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9206_t* param = (JT808MsgBody_9206_t*)in;

    return index;
} 

int JT808_9206_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9206_t* param = (JT808MsgBody_9206_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9206_t*)malloc(sizeof(JT808MsgBody_9206_t));
		memset(param, 0, sizeof(JT808MsgBody_9206_t));
    }

	index = NetbufPopBYTE(in, &param->mAddressLen, index);
	index = NetbufPopBuffer(in, param->mAddresss, index, param->mAddressLen);
	index = NetbufPopWORD(in, &param->mFTPPort, index);
	index = NetbufPopBYTE(in, &param->mUserNameLen, index);
	index = NetbufPopBuffer(in, param->mUserName, index, param->mUserNameLen);
	index = NetbufPopBYTE(in, &param->mPswdNameLen, index);
	index = NetbufPopBuffer(in, param->mPswdName, index, param->mPswdNameLen);
	index = NetbufPopBYTE(in, &param->mFilePathLen, index);
	index = NetbufPopBuffer(in, param->mFilePath, index, param->mFilePathLen);
	index = NetbufPopBYTE(in, &param->mLogicChnNum, index);
	index = NetbufPopBuffer(in, param->mStartBcdTime, index, 6);
	index = NetbufPopBuffer(in, param->mEndBcdTime, index, 6);
	index = NetbufPopDWORD(in, &param->mAlertFlagh, index);
	index = NetbufPopDWORD(in, &param->mAlertFlagl, index);
	index = NetbufPopBYTE(in, &param->mResType, index);
	index = NetbufPopBYTE(in, &param->mStreamType, index);
	index = NetbufPopBYTE(in, &param->mStorageType, index);
	index = NetbufPopBYTE(in, &param->mExcuteCondition, index);
	
	*out = param;

    return index;
} 

int JT808_1206_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1206_t* param = (JT808MsgBody_1206_t*)in;

	index = NetbufPushWORD(out, param->mReqSerialNum, index);
	index = NetbufPushBYTE(out, param->mResult, index);

    return index;
} 

int JT808_1206_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1206_t* param = (JT808MsgBody_1206_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_1206_t*)malloc(sizeof(JT808MsgBody_1206_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9207_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9207_t* param = (JT808MsgBody_9207_t*)in;

    return index;
}

int JT808_9207_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9207_t* param = (JT808MsgBody_9207_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9207_t*)malloc(sizeof(JT808MsgBody_9207_t));
    }
	
	index = NetbufPopWORD(in, &param->mReqSerialNum, index);
	index = NetbufPopBYTE(in, &param->mControlCmd, index);
	
	*out = param;

    return index;
} 

int JT808_9301_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9301_t* param = (JT808MsgBody_9301_t*)in;

    return index;
} 

int JT808_9301_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9301_t* param = (JT808MsgBody_9301_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9301_t*)malloc(sizeof(JT808MsgBody_9301_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9302_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9302_t* param = (JT808MsgBody_9302_t*)in;

    return index;
} 

int JT808_9302_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9302_t* param = (JT808MsgBody_9302_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9302_t*)malloc(sizeof(JT808MsgBody_9302_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9303_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9303_t* param = (JT808MsgBody_9303_t*)in;

    return index;
} 

int JT808_9303_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9303_t* param = (JT808MsgBody_9303_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9303_t*)malloc(sizeof(JT808MsgBody_9303_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9304_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9304_t* param = (JT808MsgBody_9304_t*)in;

    return index;
} 

int JT808_9304_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9304_t* param = (JT808MsgBody_9304_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9304_t*)malloc(sizeof(JT808MsgBody_9304_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9305_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9305_t* param = (JT808MsgBody_9305_t*)in;

    return index;
} 

int JT808_9305_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9305_t* param = (JT808MsgBody_9305_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9305_t*)malloc(sizeof(JT808MsgBody_9305_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9306_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9306_t* param = (JT808MsgBody_9306_t*)in;

    return index;
} 

int JT808_9306_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9306_t* param = (JT808MsgBody_9306_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9306_t*)malloc(sizeof(JT808MsgBody_9306_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9208_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9208_t* param = (JT808MsgBody_9208_t*)in;

    return index;
} 

int JT808_9208_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{
    int index = 0;
    JT808MsgBody_9208_t* param = (JT808MsgBody_9208_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9208_t*)malloc(sizeof(JT808MsgBody_9208_t));
		memset(param, 0, sizeof(JT808MsgBody_9208_t));
    }

	index = NetbufPopBYTE(in, &param->mIpLen, index);
	index = NetbufPopBuffer(in, param->mIp, index, param->mIpLen);
	index = NetbufPopWORD(in, &param->mTCPPort, index);
	index = NetbufPopWORD(in, &param->mUDPPort, index);
	index = NetbufPopBuffer(in, param->mAlertFlag, index, sizeof(param->mAlertFlag));
	index = NetbufPopBuffer(in, param->mAlertID, index, sizeof(param->mAlertID));
	index = NetbufPopBuffer(in, param->mResv, index, sizeof(param->mResv));

	*out = param;

    return index;
} 

int JT808_1210_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1210_t* param = (JT808MsgBody_1210_t*)in;

	index = NetbufPushBuffer(out, param->mTerminalID, index, sizeof(param->mTerminalID));
	index = NetbufPushBuffer(out, param->mAlertFlag, index, sizeof(param->mAlertFlag));
	index = NetbufPushBuffer(out, param->mAlertID, index, sizeof(param->mAlertID));
	index = NetbufPushBYTE(out, param->mInfoType, index);
	index = NetbufPushBYTE(out, param->mFileCount, index);

	for (int i=0; i<param->mFileCount; i++)
	{
		JT808MsgBody_1210_File_t* item = &param->mFileList[i];

		index = NetbufPushBYTE(out, item->mFilenameLen, index);
		index = NetbufPushBuffer(out, item->mFilename, index, item->mFilenameLen);
		index = NetbufPushDWORD(out, item->mFilesize, index);
	}
	
    return index;
} 

int JT808_1210_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1210_t* param = (JT808MsgBody_1210_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_1210_t*)malloc(sizeof(JT808MsgBody_1210_t));
    }
		
	*out = param;

    return index;
} 

int JT808_1211_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1211_t* param = (JT808MsgBody_1211_t*)in;

	index = NetbufPushBYTE(out, param->mFilenameLen, index);
	index = NetbufPushBuffer(out, param->mFilename, index, param->mFilenameLen);
	index = NetbufPushBYTE(out, param->mFileType, index);
	index = NetbufPushDWORD(out, param->mFilesize, index);

    return index;
} 

int JT808_1211_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1211_t* param = (JT808MsgBody_1211_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_1211_t*)malloc(sizeof(JT808MsgBody_1211_t));
    }
		
	*out = param;

    return index;
} 

int JT808_1212_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1212_t* param = (JT808MsgBody_1212_t*)in;

	index = NetbufPushBYTE(out, param->mFilenameLen, index);
	index = NetbufPushBuffer(out, param->mFilename, index, param->mFilenameLen);
	index = NetbufPushBYTE(out, param->mFileType, index);
	index = NetbufPushDWORD(out, param->mFilesize, index);

    return index;
} 

int JT808_1212_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_1212_t* param = (JT808MsgBody_1212_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_1212_t*)malloc(sizeof(JT808MsgBody_1212_t));
    }
		
	*out = param;

    return index;
} 

int JT808_9212_CmdEncoder(void* in, uint8_t* out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9212_t* param = (JT808MsgBody_9212_t*)in;

    return index;
} 

int JT808_9212_CmdParser(uint8_t* in, void** out, int len, void* pOwner)
{ 
    int index = 0;
    JT808MsgBody_9212_t* param = (JT808MsgBody_9212_t*)(*out);
    if (NULL == param)
    {
        param = (JT808MsgBody_9212_t*)malloc(sizeof(JT808MsgBody_9212_t));
		memset(param, 0, sizeof(JT808MsgBody_9212_t));
    }

	index = NetbufPopBYTE(in, &param->mFilenameLen, index);
	index = NetbufPopBuffer(in, param->mFilename, index, param->mFilenameLen);
	index = NetbufPopBYTE(in, &param->mFileType, index);
	index = NetbufPopBYTE(in, &param->mResult, index);
	index = NetbufPopBYTE(in, &param->mPackCount, index);

	for (int i=0; i<param->mPackCount; i++)
	{
		JT808MsgBody_9212_Item_t* item = &param->items[i];
		index = NetbufPopDWORD(in, &item->mOffset, index);
		index = NetbufPopDWORD(in, &item->mLength, index);
	}

	*out = param;

    return index;
} 



