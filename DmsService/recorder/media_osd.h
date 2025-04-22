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
 * OSD��Ϣ
 */
typedef struct
{
    /**
     * ��������϶����X����.
     */
    uint16_t relativeX;
    /**
     * ��������϶����Y����.
     */
    uint16_t relativeY;
    /**
     * OSD�ı���Ϣ����.
     */
    uint8_t text[256];
    /**
     * OSD�ı�����ɫ.
     */
    Color textColor;
    /**
     * OSD�ı������С.
     */
    uint8_t textSzie;

} OsdInfo_t;

typedef struct
{

    uint8_t Nlamp_Status;   //����Ƶ�״̬
    uint8_t Flamp_Status;   //Զ���״̬
    uint8_t Rturn_Status;   //�ҵ�״̬
    uint8_t Lturn_Status;   //���״̬
    uint8_t Brake_Status;   //�ƶ�״̬
    uint8_t Reverse_gear;   //����״̬
    uint8_t Retarder;       //ת��״̬

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
