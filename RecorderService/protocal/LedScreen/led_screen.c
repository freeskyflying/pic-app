#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <termio.h>
#include <pthread.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <netdb.h>

#include "system_param.h"
#include "pnt_log.h"
#include "pnt_epoll.h"
#include "jt808_utils.h"

#include "led_screen.h"

LedScnParam_t* gLedScnParams = NULL;
LedScnHandle_t* gLedScreenHandle = NULL;

/********************************************************************
* 全局变量定义部分
********************************************************************/
const unsigned short BH_nf_CRCTable[256] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0,
};

/********************************************************************
* 函数名称： BH_gf_CRC16()
* 输入值 ： *pBuf : 进行 CRC 校验的数组指针
Length : 进行 CRC 校验的数组长度
* 输出值 ： 校验结果
* 功能描述： 数组生成 CRC 校验码
********************************************************************/
unsigned short BH_gf_CRC16(unsigned char *pBuf, unsigned short Length)
{
	unsigned short i;
	unsigned short temp;
	unsigned short Accum = 0;
	for( i = 0; i < Length; i++)
	{
		temp = (Accum >> 8) ^ *pBuf++;
		Accum = (Accum << 8) ^ BH_nf_CRCTable[temp];
	}
	return Accum;
}

/* 消息帧检验码计算 */
static uint8_t CalcCrc(uint8_t* pack, uint16_t packeLen, uint8_t* data, uint16_t dataLen)
{
    int sum = 0;
    int i = 0;

    for (i = 0; i < packeLen; i++)
    {
        sum ^= (int)pack[i];
    }
	
    for (i = 0; i < dataLen; i++)
    {
        sum ^= (int)data[i];
    }

    return (uint8_t)(sum & 0xFF);
}

static int uartOpen(char* name, int bdr)
{
    int fd = -1;
    
    fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0)
    {
        printf("Open %s uart fail:%s ", name, strerror(errno));
        return -1;
    }
	
	struct termios uart_cfg_opt;
	speed_t speed;
	
	if (-1 == tcgetattr(fd, &uart_cfg_opt))
		return -1;
	//Set speed.
	tcflush(fd, TCIOFLUSH);
	speed = bdr==115200?B115200:B9600;
	if (CBAUDEX != speed)
	{
		cfsetospeed(&uart_cfg_opt, speed);
		cfsetispeed(&uart_cfg_opt, speed);
	}
	if (-1 == tcsetattr(fd, TCSANOW, &uart_cfg_opt))
		return -1;
	
	tcflush(fd, TCIOFLUSH);
	
	uart_cfg_opt.c_cc[VTIME] = 0;
	uart_cfg_opt.c_cc[VMIN] = 0;
	
	/* Data length setting section */
	uart_cfg_opt.c_cflag &= ~CSIZE;
	uart_cfg_opt.c_cflag |= CS8;
	uart_cfg_opt.c_iflag &= ~INPCK;
	uart_cfg_opt.c_cflag &= ~PARODD;
	uart_cfg_opt.c_cflag &= ~CSTOPB;
	
	/* Using raw data mode */
	uart_cfg_opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	uart_cfg_opt.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
	uart_cfg_opt.c_oflag &= ~(INLCR | IGNCR | ICRNL);
	uart_cfg_opt.c_oflag &= ~(ONLCR | OCRNL);
	
	/* Apply new settings */
	if (-1 == tcsetattr(fd, TCSANOW, &uart_cfg_opt))
		return -1;
	
	tcflush(fd, TCIOFLUSH);

    return fd;
}

static void Print_Buffer(char* tag, char* buffer, int len)
{
	char str[3*128] = { 0 };
	
	memset(str, 0, sizeof(str));
	
	for (int i=0; i<(len>128?128:len); i++)
	{
		sprintf(str, "%s%02X ", str, buffer[i]);
	}
	
	PNT_LOGI("%s: %s\n", tag, str);
}

static void getTimeDate(char* buffer)
{
	time_t t;
	struct tm * lt;
	time(&t);
	lt = localtime(&t);
	lt->tm_mon += 1;
	lt->tm_year = (lt->tm_year + 1900) % 100;
	buffer[0] = lt->tm_year;
	buffer[1] = lt->tm_mon;
	buffer[2] = lt->tm_mday;
	buffer[3] = lt->tm_hour;
	buffer[4] = lt->tm_min;
	buffer[5] = lt->tm_sec;

	struct timeval tv;
	gettimeofday(&tv, NULL);
	buffer[6] = tv.tv_usec>>8;
	buffer[7] = tv.tv_usec;
}

static void uartSendPacket(LedScnHandle_t* handle, char* buffer, int len)
{
	if (handle->uartFd <= 0)
	{
		return;
	}
	
	Print_Buffer("Uart Send", buffer, len);
	
	pthread_mutex_lock(&handle->mutex);

	write(handle->uartFd, buffer, len);

	pthread_mutex_unlock(&handle->mutex);
}

static int UdpConnectServer(uint8_t* url, uint16_t port)
{
	int socketFd = 0;
	
    struct addrinfo *addrs = NULL;

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;//IPPROTO_UDP;

    char portStr[16] = {0};
    sprintf(portStr, "%d", port);
    int result = 0;
	
    PNT_LOGD("BaseSocket start to get the address of %s : %d", url, port);
	
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

        PNT_LOGI("BaseSocket the socketFd of [%s:%d] are :%d", url, port, socketFd);

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

static int UdpDataParserProc(LedScnHandle_t* handle, uint8_t* buffer, int len)
{
	int i = 0, passed = 0;

	LedScnPlatCmd_t pack = { 0 };

	Print_Buffer("UDP Recv", buffer, len);

	while (i<len)
	{
		if (buffer[i] == 'B' && buffer[i+1] == 'B')
		{
			NetbufPopWORD(buffer+i+2, &pack.packLen, 0);

			if (len-i < pack.packLen+7) // 不是完整包
			{
				break;
			}

			if (buffer[i+pack.packLen+5] == 0x0D && buffer[i+pack.packLen+6] == 0x0A)
			{
				//NetbufPopBuffer(buffer+i+4, pack.resv, 0, sizeof(pack.resv));
				pack.flag = *(buffer+i+14);
				//NetbufPopBuffer(buffer+i+15, pack.termID, 0, sizeof(pack.termID));
				NetbufPopWORD(buffer+i+35, &pack.cmdID, 0);
				//NetbufPopBuffer(buffer+i+37, pack.time, 0, sizeof(pack.time));
				NetbufPopWORD(buffer+i+45, &pack.dataLen, 0);

				pack.data = buffer+i+47;

				pack.crc = *(buffer+i+pack.packLen+4);

				uint8_t crc = CalcCrc(buffer+i+2, 45, pack.data, pack.dataLen);

				if (crc != pack.crc)
				{
					PNT_LOGE("crc check fail : %02X  calc-crc: %02X.", pack.crc, crc);
				}

				PNT_LOGI("recv cmd : %04X", pack.cmdID);

				if (0x0081 == pack.cmdID)
				{
					uartSendPacket(handle, (char*)pack.data, pack.dataLen);
				}
				
				passed += pack.packLen + 7; // 跳过整包
				i += pack.packLen + 7;
				continue;
			}
			else // 不是正确包，跳过包头
			{
				passed += 2;
			}
		}
		else
		{
			passed ++;
		}
		i++;
	}

	if (passed >= len)
	{
		memset(buffer, 0, len);
		return 0;
	}

	len -= passed;
	memcpy(buffer, buffer+passed, len);

	return len;
}

static void udpSendPacket(LedScnHandle_t* handle, uint16_t cmdID, char* data, int dataLen)
{
	if (handle->udpFd <= 0)
	{
		return;
	}
	
	uint8_t buffer[128] = { 0 };

	int offset = 0;

	pthread_mutex_lock(&handle->udpMutex);

	handle->lastPacketTime = currentTimeMillis() / 1000;

	buffer[offset++] = 'B';
	buffer[offset++] = 'B';

	int packLen = 43 + dataLen;
	offset = NetbufPushWORD(buffer, packLen, offset);
	offset += 10;
	buffer[offset++] = handle->param->mFactory;

	offset = NetbufPushBuffer(buffer, (uint8_t*)handle->param->mTermID, offset, 20);
	
	offset = NetbufPushWORD(buffer, cmdID, offset);

	getTimeDate(&buffer[offset]);
	offset += 8;
	
	offset = NetbufPushWORD(buffer, dataLen, offset);

	uint8_t crc = CalcCrc(buffer+2, 45, (uint8_t*)data, dataLen);

	send(handle->udpFd, buffer, offset, 0);

	if (dataLen > 0)
		send(handle->udpFd, data, dataLen, 0);

	buffer[0] = crc;
	buffer[1] = 0x0D;
	buffer[2] = 0x0A;
	
	send(handle->udpFd, buffer, 3, 0);
	
	pthread_mutex_unlock(&handle->udpMutex);
}

static void SendCmdPacket_TIME(void)
{
	
}

static int UartDataParserProc(LedScnHandle_t* handle, uint8_t* buffer, int len)
{
	int i = 0, passed = 0, start_flag = 0, start_pos = 0;

	Print_Buffer("Uart Recv", buffer, len);

	while (i<len)
	{
		if (1 == start_flag)
		{
			if (buffer[i] == 0x7E)
			{
				char* pack = buffer+start_pos;
				
				udpSendPacket(handle, 0x0080, pack, i-start_pos+1);

				if (0 == memcmp(pack+6, "PWON", 4))
				{
					SendCmdPacket_TIME();
				}
				
				passed += i-start_pos+1;
				start_flag = 0;
			}
		}
		else
		{
			if (buffer[i] == 0x7E)
			{
				start_flag = 1;
				start_pos = i;
			}
			else
			{
				passed ++;
			}
		}
		i++;
	}

	if (passed >= len)
	{
		memset(buffer, 0, len);
		return 0;
	}

	len -= passed;
	memcpy(buffer, buffer+passed, len);

	Print_Buffer("Uart Recv", buffer, len);

	return len;
}

void LedScreenParamInit(void)
{
	LedScnParam_t param;

	if (access("/data/etc", F_OK))
	{
		mkdir("/data/etc", 660);
	}
	
	memset(&param, 0, sizeof(param));
	
	param.mBaudrate = 115200;
	
	int fd = open(LEDSCN_PARAMS_FILE, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("create %s failed.\n", LEDSCN_PARAMS_FILE);
		return;
	}
	
	int size = lseek(fd, 0, SEEK_END);

	if (size < sizeof(LedScnParam_t))
	{
		PNT_LOGE("%s resize %d", LEDSCN_PARAMS_FILE, size);
		
		if (0 < size)
		{
			lseek(fd, 0, SEEK_SET);
			read(fd, &param, size);
		}
		ftruncate(fd, sizeof(LedScnParam_t)-size);
		lseek(fd, 0, SEEK_SET);
		write(fd, &param, sizeof(LedScnParam_t));
	}

	gLedScnParams = mmap(NULL, sizeof(LedScnParam_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	
	if (MAP_FAILED == gLedScnParams)
	{
		PNT_LOGE("mmap file %s failed.\n", LEDSCN_PARAMS_FILE);
		return;
	}
}

void LedScreenParamDeInit(void)
{
	if (gLedScnParams)
	{
		munmap(gLedScnParams, sizeof(LedScnParam_t));
	}
}

static int read_datas(int fd, char *rcv_buf, int buffLen, int rcv_wait_ms)
{
	int retval;
	fd_set rfds;
	struct timeval tv;
	int ret,pos;

	tv.tv_sec = rcv_wait_ms/1000;      // wait 2.5s
	tv.tv_usec = rcv_wait_ms*1000;

	pos = 0; // point to rceeive buf

	while (1)
	{
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);

		retval = select(fd+1 , &rfds, NULL, NULL, &tv);

		if (retval == -1)
		{
			break;
		}
		else if (retval)
		{
			ret = read(fd, rcv_buf+pos, buffLen);
			pos += ret;
			if (pos >= buffLen)
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	return pos;
}


void* LedScreenUartThread(void* parg)
{
	int flag = 0;
	char point[128] = { 0 };
	int ret = 0, bufferLen = 0;

	LedScnHandle_t* handle = (LedScnHandle_t*)parg;
	
open_uart:

	while (gGlobalSysParam->ExtDeviceType[0] != LED_SCREEN) SleepMs(1000);

	flag = 0;
	handle->uartFd = -1;

	for (int i=0; i<10; i++)
	{
		sprintf(point, "/dev/ttyACM%d", i);
		if (0 == access(point, F_OK))
		{
			PNT_LOGI("find kwatch node %s", point);
			flag = 1;
			break;
		}
	}

	if (flag)
	{
	    handle->uartFd = uartOpen(point, handle->param->mBaudrate);
	    if (handle->uartFd < 0)
	    {
	        sleep(2);
	        goto open_uart;
	    }
	}
	else
	{
		sleep(2);
		goto open_uart;
	}

	PNT_LOGE("led screen start.");

	while (gGlobalSysParam->ExtDeviceType[0] == LED_SCREEN)
	{
		ret = read_datas(handle->uartFd, handle->buffer+bufferLen, sizeof(handle->buffer)-bufferLen, 50);
		if (ret > 0)
		{
			bufferLen += ret;

			if (bufferLen >= 13)
			{
				bufferLen = UartDataParserProc(handle, (uint8_t*)handle->buffer, bufferLen);
			}
		}
	}

	close(handle->uartFd);
	handle->uartFd = -1;
	
	PNT_LOGE("led screen closed.");

goto open_uart;

	return NULL;
}

void* LedScreenUdpThread(void* parg)
{
	LedScnHandle_t* handle = (LedScnHandle_t*)parg;

	int ret = 0, bufferLen = 0;
	
reConnect:

	while (gGlobalSysParam->ExtDeviceType[0] != LED_SCREEN) SleepMs(1000);
	
	handle->udpFd = UdpConnectServer((uint8_t*)handle->param->mUrl, handle->param->mPort);

	if (handle->udpFd <= 0)
	{
		SleepMs(2000);
		goto reConnect;
	}
	
	while (gGlobalSysParam->ExtDeviceType[0] == LED_SCREEN)
	{
		ret = recv(handle->udpFd, handle->udpBuffer+bufferLen, sizeof(handle->udpBuffer)-bufferLen, 0);
		if (ret > 0)
		{
			bufferLen += ret;
			
			if (bufferLen > 50)
			{
				bufferLen = UdpDataParserProc(handle, (uint8_t*)handle->udpBuffer, bufferLen);
			}
		}
		else
		{
			SleepMs(50);
		}

		if ((currentTimeMillis()/1000) - handle->lastPacketTime >= (2*60))
		{
			
		}
	}
	
	close(handle->udpFd);
	handle->udpFd = -1;
	
	PNT_LOGE("led screen closed.");

	goto reConnect;
	
	return NULL;
}

void LedScreenStart(void)
{
	if (NULL == gLedScnParams)
	{
		LedScreenParamInit();
		if (NULL == gLedScnParams)
		{
			PNT_LOGE("fatal error: led screen param init failed.");
			return;
		}
	}
	
    pthread_t pid;

	gLedScreenHandle = (LedScnHandle_t*)malloc(sizeof(LedScnHandle_t));
	if (NULL == gLedScreenHandle)
	{
		PNT_LOGE("fatal error: led screen param init failed.");
		return;
	}

	memset(gLedScreenHandle, 0, sizeof(LedScnHandle_t));

    pthread_mutex_init(&gLedScreenHandle->mutex, NULL);
    pthread_mutex_init(&gLedScreenHandle->udpMutex, NULL);

	gLedScreenHandle->uartFd = -1;
	gLedScreenHandle->udpFd = -1;
	gLedScreenHandle->param = gLedScnParams;	

    int ret = pthread_create(&pid, 0, LedScreenUartThread, gLedScreenHandle);
    if (ret)
    {
        PNT_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    }
	ret = pthread_create(&pid, 0, LedScreenUdpThread, gLedScreenHandle);
    if (ret)
    {
        PNT_LOGE("errno=%d, reason(%s)\n", errno, strerror(errno));
    }

	strcpy(gLedScnParams->mUrl, "192.168.169.2");
	gLedScnParams->mBaudrate = 115200;
	gLedScnParams->mPort = 9000;
	gGlobalSysParam->ExtDeviceType[0] = LED_SCREEN;
}

