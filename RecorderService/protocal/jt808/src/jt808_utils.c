#include <string.h>
#include <stdlib.h>
#include "jt808_utils.h"
#include "pnt_log.h"


int NetbufPushBYTE(BYTE* buff, BYTE d, int offset)
{
    buff[offset++] = d;
    
    return offset;
}

int NetbufPushWORD(BYTE* buff, WORD d, int offset)
{
    buff[offset++] = (BYTE)(d >> 8);
    buff[offset++] = (BYTE)(d);

    return offset;
}

int NetbufPushDWORD(BYTE* buff, DWORD d, int offset)
{
    buff[offset++] = (BYTE)(d >> 24);
    buff[offset++] = (BYTE)(d >> 16);
    buff[offset++] = (BYTE)(d >> 8);
    buff[offset++] = (BYTE)(d);

    return offset;
}

int NetbufPushBuffer(BYTE* buff, BYTE* b, int offset, int len)
{
    memcpy(buff+offset, b, len);

    offset += len;
    return offset;
}

int NetbufPopBYTE(BYTE* buff, BYTE* d, int offset)
{
    *d = buff[offset++];

    return offset;
}

int NetbufPopWORD(BYTE* buff, WORD* d, int offset)
{
    *d = ((buff[offset+0] << 8)&0xff00) |
        ((buff[offset+1])&0x00ff);

    return (offset+2);
}

int NetbufPopDWORD(BYTE* buff, DWORD* d, int offset)
{
    *d = ((buff[offset+0] << 24)&0xff000000) |
        ((buff[offset+1] << 16)&0x00ff0000) |
        ((buff[offset+2] << 8)&0x0000ff00) |
        ((buff[offset+3])&0x00ff);

    return (offset+4);
}

int NetbufPopBuffer(BYTE* buff, BYTE* d, int offset, int len)
{
    memcpy(d, buff+offset, len);

    offset += len;
    return offset;
}

/* 消息帧检验码计算 */
BYTE JT808CalcCrcTerminal(BYTE* buff, int len)
{
    int sum = 0;
    int i = 0;

    for (i = 0; i < len; i++)
    {
        sum ^= (int)buff[i];
    }

    return (BYTE)(sum & 0xFF);
}

/* 转义处理，返回处理字节数，传入的src_buff不能带消息头尾标志 */
int JT808MsgConvert(char* src_buff, int src_len, char* dst_buff, int dst_len, int forward /* 1 转义 0 还原 */)
{
    int ret = 0, i = 0, j = 0;

    /* 转义与还原 */
    if (forward) /* 转义处理 */
    {
        for (i = 0; i < src_len; i++)
        {
            if (JT808_MSG_FLAG == src_buff[i])
            {
                dst_buff[j++] = JT808_MSG_FLAG_CONVERT;
                dst_buff[j++] = 0x02; /* 转义 */
            }
            else if (JT808_MSG_FLAG_CONVERT == src_buff[i])
            {
                dst_buff[j++] = src_buff[i];
                dst_buff[j++] = 0x01; /* 转义 */
            }
            else
            {
                dst_buff[j++] = src_buff[i];
            }
        }

        /* 返回转义处理字节数 */
        ret = j;
    }
    else /* 还原处理 */
    {
        for (i = 0; i < src_len; i++)
        {
            if (JT808_MSG_FLAG_CONVERT == src_buff[i])
            {
                if (0x01 == src_buff[i + 1])
                {
                    dst_buff[j++] = JT808_MSG_FLAG_CONVERT;
                    i++; /* 跳过转义符 */
                }
                else if (0x02 == src_buff[i + 1])
                {
                    dst_buff[j++] = JT808_MSG_FLAG;
                    i++; /* 跳过转义符 */
                }
                else
                {
                    PNT_LOGE("convert failed (forward %d), MESSAGE_FLAG_CONVERT found, but next byte not 0x01 or 0x02.", forward);
                    ret = -1;
                    goto convert_end;
                }
            }
            else
            {
                dst_buff[j++] = src_buff[i];
            }
        }

        /* 返回转义处理字节数 */
        ret = j;
    }

convert_end:

    return ret;
}

void ConvertPhoneNomToBCD(unsigned char *phoneBcdCode, int len/* 协议手机号字节数 */, const char *phoneNO)
{
	char PhoneNum[32] = { 0 }, str[64] = { 0 };
	int i = 0, j = 0;

	if (strlen(phoneNO) <= len*2)
	{
		for (i=0; i<len*2-strlen(phoneNO); i++)
		{
			PhoneNum[i] = '0';
		}
		strcpy(PhoneNum+i, phoneNO);
	}
	else
	{
		memcpy(PhoneNum, phoneNO, 2*len);
	}

    for (i = 0; i < 2*len; i += 2)
	{
         phoneBcdCode[j++] = (BYTE)(((PhoneNum[i]-'0')<<4)&0xF0) + (BYTE)(PhoneNum[i+1]-'0');
		 sprintf(str, "%s %02X", str, phoneBcdCode[j-1]);
    }

	PNT_LOGI("PhoneNum: %s convert to BCD[%s]", PhoneNum, str);
}

int ConvertHexStrToBCD(unsigned char* out, const char* str)
{
	int i = 0, j = 0;
	int len = strlen(str);
	BYTE l = 0, h = 0;
	
    for (i = 0; i < len; i++)
	{
		if (str[i]>='A' && str[i] < 'a')
		{
			h = str[i] - 'A' + 10;
		}
		else if (str[i] >= 'a')
		{
			h = str[i] - 'a' + 10;
		}
		else
		{
			h = str[i] - '0';
		}

		i++;
		if (str[i]>='A' && str[i] < 'a')
		{
			l = str[i] - 'A' + 10;
		}
		else if (str[i] >= 'a')
		{
			l = str[i] - 'a' + 10;
		}
		else
		{
			l= str[i] - '0';
		}

		out[j++] = l + ((h<<4)&0xF0);
    }

	return j;
}

void PRINTF_BUFFER(JT808Session_t* session, BYTE* buff, int len)
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

	JT808SESSION_PRINT(session, "%s", strbuff);
}


