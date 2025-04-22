#ifndef _JT808_UTILS_H_
#define _JT808_UTILS_H_

#include "jt808_data.h"
#include "jt808_session.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JT808_MSG_FLAG              (0x7E)
#define JT808_MSG_FLAG_CONVERT      (0x7D)


#define JT808SESSION_PRINT(session, ...) { \
		char tag[128] = { 0 };\
		sprintf(tag, "%s-%d %s:", __FUNCTION__, __LINE__, session->mSessionData->mAddress);\
		LogW(lev_info, (const char*)tag, session->mSessionData->mPort, __VA_ARGS__); \
	}

int NetbufPushBYTE(BYTE* buff, BYTE d, int offset);

int NetbufPushWORD(BYTE* buff, WORD d, int offset);

int NetbufPushDWORD(BYTE* buff, DWORD d, int offset);

int NetbufPushBuffer(BYTE* buff, BYTE* b, int offset, int len);

int NetbufPopBYTE(BYTE* buff, BYTE* d, int offset);

int NetbufPopWORD(BYTE* buff, WORD* d, int offset);

int NetbufPopDWORD(BYTE* buff, DWORD* d, int offset);

int NetbufPopBuffer(BYTE* buff, BYTE* d, int offset, int len);

BYTE JT808CalcCrcTerminal(BYTE* buff, int len);

int JT808MsgConvert(char* src_buff/*src_buff不能带消息头尾标志*/, int src_len, char* dst_buff, int dst_len, int forward /* 1 转义 0 还原 */);

void ConvertPhoneNomToBCD(unsigned char *phoneBcdCode, int len/* 协议手机号字节数 */, const char *phoneNO);

int ConvertHexStrToBCD(unsigned char* out, const char* str);

void PRINTF_BUFFER(JT808Session_t* session, BYTE* buff, int len);

#ifdef __cplusplus
}
#endif

#endif
