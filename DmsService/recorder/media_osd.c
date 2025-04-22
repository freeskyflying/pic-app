#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include "dirent.h"
#include <pthread.h>

#include "sample_comm.h"
#include "typedef.h"
#include "pnt_log.h"

#include "media_osd.h"
#include "font_utils.h"
#include "pnt_ipc_gps.h"
#include "pnt_ipc.h"

#include "system_param.h"
#include "6zhentan.h"
#include "strnormalize.h"

static pthread_t gOsdPid;
static bool_t gMediaOsdRuning = FALSE;
MediaOsdPara_t gMediaOsdPara = { 0 };
static uint8_t* gMediaOsdBuffer = NULL;
static char gMediaOsdHandleId[MAX_VIDEO_NUM*2] = { 0 };
static PNTIPC_t gLocationIPC = { 0 };

static char turnRight[6]={0xD3,0xD2,0xD7,0xAA};
static char turnLeft[6]={0xD7,0xF3,0xD7,0xAA};
static char Nlamp_Status[6]={0xBD,0xFC,0xB9,0xE2};
static char Flamp_Status[6]={0xD4,0xB6,0xB9,0xE2};
static char Brake_Status[6]={0xD6,0xC6,0xB6,0xAF};
static char Reverse_gear[6]={0xB5,0XB9,0XB5,0XB5};
static char forward_Retarder[6]={0xD5,0xFD,0xD7,0xAA};//  ?
static char reverse_Retarder[6]={0xB7,0xB4,0xD7,0xAA};//  ?

static char tird[]={0xc6,0xa3,0xc0,0xcd,0x00};
static char cellphone[]={0xb4,0xf2,0xb5,0xe7,0xbb,0xb0,0x00};
static char smok[]={0xb3,0xe9,0xd1,0xcc,0x00};
static char distract[]={0xb7,0xd6,0xd0,0xc4,0x00};
static char nodriver[]={0xbc,0xdd,0xca,0xbb,0xd4,0xb1,0xc0,0xeb,0xb8,0xda,0x00};
static char drivererr[]={0xbc,0xdd,0xca,0xbb,0xd4,0xb1,0xb9,0xa6,0xc4,0xdc,0xca,0xa7,0xd0,0xa7,0x00};
static char drivermatch[]={0xbc,0xdd,0xca,0xbb,0xd4,0xb1,0xc6,0xa5,0xc5,0xe4,0x00};
static char icerr[]={0x69,0x63,0xb6,0xc1,0xbf,0xa8,0xca,0xa7,0xb0,0xdc,0x00};
static char night[]={0xd2,0xb9,0xbc,0xe4,0xbd,0xfb,0xd0,0xd0,0x00};
static char overdriver[]={0xd2,0xb9,0xbc,0xe4,0xbd,0xfb,0xd0,0xd0,0x00};

static char fcw[]={0xc7,0xb0,0xcf,0xf2,0xc5,0xf6,0xd7,0xb2,0x00};
static char lcw[]={0xb3,0xb5,0xb5,0xc0,0xc6,0xab,0xc0,0xeb,0x00};
static char near[]={0xb3,0xb5,0xbe,0xe0,0xb9,0xfd,0xbd,0xfc,0x00};
static char ped[]={0xd0,0xd0,0xc8,0xcb,0xc5,0xf6,0xd7,0xb2,0x00};
static char adaserr[]={0x41,0x64,0x61,0x73,0xb9,0xa6,0xc4,0xdc,0xca,0xa7,0xd0,0xa7,0x00};
static char overspeed[]={0xb5,0xc0,0xc2,0xb7,0xb3,0xac,0xcb,0xd9,0x00};
static char overload[]={0xb3,0xac,0xd4,0xd8,0x00};
static char crash[]={0xc5,0xf6,0xd7,0xb2,0xb2,0xe0,0xb7,0xad,0x00};

static void getOsdtime(char* buf)
{
    char szContentBuf[200] ="";
    time_t timep;
    struct tm p;
    time(&timep);
    localtime_r(&timep,&p);
    int year = 1900 + p.tm_year;
    if(year<0) year = 2000;
    sprintf(szContentBuf,"%04d-%02d-%02d %02d:%02d:%02d",year,(1+p.tm_mon),p.tm_mday,p.tm_hour,p.tm_min,p.tm_sec);
    strcpy(buf,szContentBuf);
}

static EI_S32 MediaOsd_Update2REG(uint8_t* pbuf, EI_U32 u32Width, EI_U32 u32Height, RGN_HANDLE handleId)
{
    EI_S32 s32Ret;
    SIZE_S stSize;
    BITMAP_S stBitmap;
    RGN_CANV_INFO_S stCanvasInfo;
    RGN_HANDLE Handle;
    Handle = handleId;
    
    s32Ret = EI_MI_RGN_GetCanvInfo(Handle,&stCanvasInfo);
    if(RET_SUCCESS != s32Ret)
    {
        PNT_LOGE("HI_MPI_RGN_GetCanvasInfo failed with %#x!", s32Ret);
        return RET_FAILED;
    }
    int height = stCanvasInfo.stSize.u32Height;
    int width = stCanvasInfo.stSize.u32Width;//*2;//       
    int x = 0;
    int y = 0;
    int h1 = u32Height;
    int w1 = u32Width;//?    
    int l1 = h1 * w1* 2;
    int stride = w1 * 2;
    void *pOrigbuf = (void*)stCanvasInfo.pVirtAddr;
    memset(pOrigbuf,0,height*width);
    for(int i = (y * width + x) * 2, j = 0; i < (((y + h1) * width - (width - x)) * 2) && j < l1; i+= width*2, j+=stride)
    {
        memcpy(pOrigbuf + i, pbuf + j, stride);
    }

    s32Ret = EI_MI_RGN_UpdateCanv(Handle);
    if(RET_SUCCESS != s32Ret)
    {
        PNT_LOGE("HI_MPI_RGN_UpdateCanvas failed with %#x!", s32Ret);
        return RET_FAILED;
    }
    return s32Ret;
}

static int MediaOsd_UpdateOsd(int chn, OsdInfo_t* osdInfo)
{
    if (1 != gMediaOsdHandleId[chn])
    {
        return EXIT_FAILURE;
    }

    if (NULL == gMediaOsdBuffer)
    {
        gMediaOsdBuffer = (uint8_t*)malloc(OSD_AREA_WIDTH*OSD_AREA_HEIGHT*2);
    }
    if (NULL == gMediaOsdBuffer)
    {
        PNT_LOGE("fatal error !!! gMediaOsdBuffer null.");
        return EXIT_FAILURE;
    }
    
    RGN_HANDLE handleId = chn;
    
    memset(gMediaOsdBuffer, 0, OSD_AREA_WIDTH*OSD_AREA_HEIGHT*2);
    
    int fontsize = osdInfo->textSzie;
    if(fontsize > 3){
        fontsize = 2;
    }

    unsigned short color = HIFB_RED_1555;
    if(osdInfo->textColor == GREEN){
        color = HIFB_GREEN_1555;
    }else if(osdInfo->textColor == BLUE){
        color = HIFB_BLUE_1555;
    }else{
        color = HIFB_RED_1555;
    }

    uint32_t destWidth = 0, destHeight = 0, buffwidth = 0;
    int s32Ret = 0;

    if (RES_1080p == gMediaOsdPara.res[chn])
    {
        buffwidth = 1920;
    }
    else if (RES_720p == gMediaOsdPara.res[chn])
    {
        buffwidth = 1280;
    }
    else if (RES_D1 == gMediaOsdPara.res[chn])
    {
        buffwidth = 720;
    }

    s32Ret = osd_str_to_argb(osdInfo->text, fontsize, color, 0, buffwidth, (unsigned short*)gMediaOsdBuffer, &destWidth, &destHeight);

    s32Ret = MediaOsd_Update2REG(gMediaOsdBuffer, destWidth, destHeight, handleId);
    if(EI_SUCCESS != s32Ret)
    {
        PNT_LOGE("MediaOsd_Update2REG failed!");
        return EI_FAILURE;
    }

    return 0;
}

static int constructOsdChnTitle(MediaOsdPara_t *para)
{
    if(para == NULL)
    {
        PNT_LOGE("para is null!");
        return -1;
    }

    char timename[100]={0};
    getOsdtime(timename);
    OsdInfo_t  info = { 0 };

    info.textColor = RED;
    for(int i=0;i<MAX_VIDEO_NUM*2;i++)
    {    
        memset(info.text,0,sizeof(info.text));
        if(para->res[i] == RES_1080p)
        {
            info.textSzie = 2;
            info.relativeY = 14;
            info.relativeX = 20;
        }
        else if((para->res[i] == RES_D1)||(para->res[i] == RES_720p))
        {
            info.textSzie = 1;
            info.relativeY = 14;
            info.relativeX = 10;
        }
        else if(para->res[i] == RES_CIF)
        {
            info.textSzie = 0;
            info.relativeY = 14;
            info.relativeX = 10;
        }

        if(para->isShowChn[i] == TRUE)
        {
           sprintf((char*)info.text,"CH %d",i%MAX_VIDEO_NUM + 1);
        }
        if(para->isShowTime[i] == TRUE)
        {
           sprintf((char*)info.text,"%s %s",(char*)info.text,timename+2);
        }

        if((para->isShowPlateNumber[i] == TRUE)&&(strlen(para->plateNumber)))
        {
            sprintf((char*)info.text,"%s %s",(char*)info.text,para->plateNumber);
            //gs_driver_info.car_id_valid=0;
        }
        
        if(para->isShowSpeed[i] == TRUE)
        {
            sprintf((char*)info.text,"%s %dkm/h",(char*)info.text,para->speed);
        }
        
        if(para->signal.Rturn_Status == 1)
        {
            sprintf((char*)info.text,"%s %s",(char*)info.text,turnRight);
        }
        if(para->signal.Lturn_Status == 1)
        {
            sprintf((char*)info.text,"%s %s",(char*)info.text,turnLeft);
        }
        if(para->signal.Nlamp_Status == 1)
        {
            sprintf((char*)info.text,"%s %s",(char*)info.text,Nlamp_Status);
        }
        if(para->signal.Flamp_Status == 1)
        {
            sprintf((char*)info.text,"%s %s",(char*)info.text,Flamp_Status);
        }
        if(para->signal.Brake_Status == 1)
        {
            sprintf((char*)info.text,"%s %s",(char*)info.text,Brake_Status);
        }
        if(para->signal.Reverse_gear == 1)
        {
            sprintf((char*)info.text,"%s %s",(char*)info.text,Reverse_gear);
        }
        if(para->signal.Retarder == 1)
        {
            sprintf((char*)info.text,"%s %s",(char*)info.text,forward_Retarder);
        }else if(para->signal.Retarder == 2)
        {
            sprintf((char*)info.text,"%s %s",(char*)info.text,reverse_Retarder);
        }
        
        if(para->isShowGPS[i] == TRUE)
        {
            sprintf((char*)info.text,"%s %.6f%c %.6f%c",(char*)info.text,para->latitude,para->lat,para->longitude,para->lon);
        }

        if (para->isShowUniqueCode)
        {
            sprintf((char*)info.text, "%s %s %s", (char*)info.text, para->terminalid, para->macAddr);
        }

        MediaOsd_UpdateOsd(i, &info);
    }
    return 0;
}

static int LocationIPCIRcvDataProcess(char* buff, int len, int fd, void* arg)
{
    GpsLocation_t location = { 0 };
    extern int gCurrentSpeed;

    if (len >= sizeof(GpsLocation_t))
    {
        memcpy(&location, buff, sizeof(GpsLocation_t));

        gMediaOsdPara.latitude = location.latitude;
        gMediaOsdPara.longitude = location.longitude;
        gMediaOsdPara.lat = location.lat;
        gMediaOsdPara.lon = location.lon;

		gCurrentSpeed = location.speed;
        gMediaOsdPara.speed = gCurrentSpeed;

        return len;
    }
    else
    {
        return 0;
    }
}

void* MediaOsd_ThreadLoop(void* arg)
{
    time_t nowtime = time(NULL);
    struct tm p;
    localtime_r(&nowtime,&p);
    time_t oldtime = 0;
    int first = 0;

    LibPnt_IPCConnect(&gLocationIPC, PNT_IPC_GPS_NAME, LocationIPCIRcvDataProcess, NULL, 256, NULL);
	
	AppPlatformParams_t param;
	getAppPlatformParams(&param, 0);
	if (strlen(param.plateNum))
	{
		str_normalize_init();
		unsigned int to_len = strlen(param.plateNum) + 1;
		char* buffer = (char*)malloc(to_len);
		memset(buffer, 0, to_len);
		utf8_to_gbk(param.plateNum, strlen(param.plateNum), &buffer, &to_len);
		strncpy(gMediaOsdPara.plateNumber, buffer, sizeof(gMediaOsdPara.plateNumber)-1);
		free(buffer);
	}

    while (gMediaOsdRuning)
    {
        nowtime = time(NULL);
        localtime_r(&nowtime,&p);
        if(nowtime != oldtime)
        {
            oldtime = nowtime;
            if(first==0)
            {
                first = 1;
            }
			if (gGlobalSysParam->osdEnable)
			{
            	constructOsdChnTitle(&gMediaOsdPara);
			}
        }
        usleep(1000*20);
    }

	LibPnt_IPCClientRelease(&gLocationIPC);

    pthread_exit(NULL);
    return NULL;
}

EI_S32 MediaOsd_Init(void)
{
//    memset(&gMediaOsdPara, 0, sizeof(gMediaOsdPara));

    for(int i=0;i<MAX_VIDEO_NUM;i++)
    {
        //gMediaOsdPara.res[i] = RES_1080p;
        gMediaOsdPara.isUpdate[i] = TRUE;
        gMediaOsdPara.isShowTime[i] = TRUE;
        gMediaOsdPara.isShowPlateNumber[i] = TRUE;
        gMediaOsdPara.isShowChn[i] = TRUE;
        gMediaOsdPara.isShowGPS[i] = TRUE;
        gMediaOsdPara.isShowSpeed[i] = TRUE;
        gMediaOsdPara.speed = 0;
        gMediaOsdPara.signal.Nlamp_Status = 0;   //    ? ??
        gMediaOsdPara.signal.Flamp_Status = 0;   //?   ??
        gMediaOsdPara.signal.Rturn_Status = 0;   // ? ??
        gMediaOsdPara.signal.Lturn_Status = 0;   //   ??
        gMediaOsdPara.signal.Brake_Status = 0;   // ? ??
        gMediaOsdPara.signal.Reverse_gear = 0;   //    ??
        gMediaOsdPara.signal.Retarder = 0;       //?    ??
        gMediaOsdPara.latitude = 0.0;
        gMediaOsdPara.lat = 'N';
        gMediaOsdPara.longitude = 0.0;
        gMediaOsdPara.lon = 'E';
    }

    for(int i=MAX_VIDEO_NUM;i<MAX_VIDEO_NUM*2;i++)
    {
        //gMediaOsdPara.res[i] = RES_D1;
        gMediaOsdPara.isUpdate[i] = TRUE;
        gMediaOsdPara.isShowTime[i] = TRUE;
        gMediaOsdPara.isShowPlateNumber[i] = TRUE;
        gMediaOsdPara.isShowChn[i] = TRUE;
        gMediaOsdPara.isShowGPS[i] = TRUE;
        gMediaOsdPara.isShowSpeed[i] = TRUE;
        gMediaOsdPara.speed = 0;
        gMediaOsdPara.signal.Nlamp_Status = 0;   //    ? ??
        gMediaOsdPara.signal.Flamp_Status = 0;   //?   ??
        gMediaOsdPara.signal.Rturn_Status = 0;   // ? ??
        gMediaOsdPara.signal.Lturn_Status = 0;   //   ??
        gMediaOsdPara.signal.Brake_Status = 0;   // ? ??
        gMediaOsdPara.signal.Reverse_gear = 0;   //    ??
        gMediaOsdPara.signal.Retarder = 0;       //?    ??
        gMediaOsdPara.latitude = 0.0;
        gMediaOsdPara.lat = 'N';
        gMediaOsdPara.longitude = 0.0;
        gMediaOsdPara.lon = 'E';
    }

    return RET_SUCCESS;
}

EI_S32 MediaOsd_Start(void)
{
    gMediaOsdRuning = TRUE;

    MediaOsd_Init();

    int s32Ret = pthread_create(&gOsdPid, 0, MediaOsd_ThreadLoop, &gMediaOsdPara);
    if (s32Ret != 0)
    {
        PNT_LOGE("create osdProc failed!");
    }
    PNT_LOGI("record osd start finish!");

    return RET_SUCCESS;
}

EI_S32 MediaOsd_Stop(void)
{
    if (gMediaOsdRuning == TRUE)
    {
        gMediaOsdRuning= FALSE;
        pthread_join(gOsdPid, 0);
    }

    return RET_SUCCESS;
}

EI_S32 MediaOsd_InitChn(MDP_CHN_S *stRgnChn, EI_U32 chnWidth, EI_U32 chnHeight, EI_U32 handleId)
{
    RGN_ATTR_S stRegion = { 0 };
    RGN_CHN_ATTR_S stRgnChnAttr = { 0 };
    EI_S32 s32Ret;
    RGN_HANDLE Handle;
    Handle = handleId;

    if (1920 == chnWidth)
    {
        gMediaOsdPara.res[handleId] = RES_1080p;
    }
    else if (1280 == chnWidth || 960 == chnWidth)
    {
        gMediaOsdPara.res[handleId] = RES_720p;
    }
    else
    {
        gMediaOsdPara.res[handleId] = RES_D1;
    }
    
    stRegion.enType = OVERLAYEX_RGN;
    stRegion.unAttr.stOverlayEx.enPixelFmt = PIX_FMT_ARGB_1555;
    stRegion.unAttr.stOverlayEx.stSize.u32Width = chnWidth;
    stRegion.unAttr.stOverlayEx.stSize.u32Height = chnHeight;
    stRegion.unAttr.stOverlayEx.u32CanvasNum = 2;

    s32Ret = EI_MI_RGN_Create(Handle, &stRegion);
    if (s32Ret)
    {
        return RET_FAILED;
    }
    
    stRgnChnAttr.bShow = EI_TRUE;
    stRgnChnAttr.enType = OVERLAYEX_RGN;
    stRgnChnAttr.unChnAttr.stOverlayExChn.stPoint.s32X = 30;
    stRgnChnAttr.unChnAttr.stOverlayExChn.stPoint.s32Y = 30;
    stRgnChnAttr.unChnAttr.stOverlayExChn.u32FgAlpha = 255;
    stRgnChnAttr.unChnAttr.stOverlayExChn.stInvertColorAttr.bInvertColorEn = EI_FALSE;

    s32Ret = EI_MI_RGN_AddToChn(Handle, stRgnChn, &stRgnChnAttr);
    if (s32Ret)
    {
        return RET_FAILED;
    }

    gMediaOsdHandleId[handleId] = 1;

    return RET_SUCCESS;
}

EI_S32 MediaOsd_DeInitChn(MDP_CHN_S *stRgnChn, EI_U32 handleId)
{
    RGN_HANDLE Handle;
    Handle = handleId;

    EI_MI_RGN_DelFromChn(Handle, stRgnChn);
    EI_MI_RGN_Destroy(Handle);

    return EI_SUCCESS;
}
