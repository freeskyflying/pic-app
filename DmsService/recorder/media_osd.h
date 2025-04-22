#ifndef _MEDIA_OSD_H_
#define _MEDIA_OSD_H_

#include "typedef.h"
#include "media_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define OSD_AREA_HEIGHT         160
#define OSD_AREA_WIDTH          1920

/**
 * OSD信息
 */
typedef struct
{
    /**
     * 相对于左上顶点的X坐标.
     */
    uint16_t relativeX;
    /**
     * 相对于坐上顶点的Y坐标.
     */
    uint16_t relativeY;
    /**
     * OSD文本信息内容.
     */
    uint8_t text[256];
    /**
     * OSD文本的颜色.
     */
    Color textColor;
    /**
     * OSD文本字体大小.
     */
    uint8_t textSzie;

} OsdInfo_t;

typedef struct
{

    uint8_t Nlamp_Status;   //近光灯灯状态
    uint8_t Flamp_Status;   //远光灯状态
    uint8_t Rturn_Status;   //右灯状态
    uint8_t Lturn_Status;   //左灯状态
    uint8_t Brake_Status;   //制动状态
    uint8_t Reverse_gear;   //倒车状态
    uint8_t Retarder;       //转速状态

} ExtendedSignal_t;

typedef struct
{

    bool_t          isUpdate[MAX_VIDEO_NUM*2];
    bool_t          isShowTime[MAX_VIDEO_NUM*2];
    bool_t          isShowPlateNumber[MAX_VIDEO_NUM*2];
    bool_t          isShowChn[MAX_VIDEO_NUM*2];
    bool_t          isShowGPS[MAX_VIDEO_NUM*2];
    bool_t          isShowSpeed[MAX_VIDEO_NUM*2];
    bool_t          isShowUniqueCode;
    Resolution      res[MAX_VIDEO_NUM*2];
    char            plateNumber[20];
    char            terminalid[16];
    char            macAddr[16];
    int             speed;
    double          latitude;
    char            lat;
    double          longitude;
    char            lon;
    ExtendedSignal_t  signal;

} MediaOsdPara_t;

EI_S32 MediaOsd_Init(void);

EI_S32 MediaOsd_Stop(void);

EI_S32 MediaOsd_Start(void);

EI_S32 MediaOsd_InitChn(MDP_CHN_S *stRgnChn, EI_U32 chnWidth, EI_U32 chnHeight, EI_U32 handleId);

EI_S32 MediaOsd_DeInitChn(MDP_CHN_S *stRgnChn, EI_U32 handleId);

#ifdef __cplusplus
}
#endif

#endif
