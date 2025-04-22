#ifndef __OSD_H__
#define __OSD_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

typedef enum UVC_REGION_TYPE {
    RGN_TYPE_CHANNEL = 0,
    RGN_TYPE_TIME,
    RGN_TYPE_3A_AwbMode,
    RGN_TYPE_3A_ColorTempId,
    RGN_TYPE_3A_AeMode,
    RGN_TYPE_3A_FlickerMode,
    RGN_TYPE_3A_ISO,
    RGN_TYPE_3A_ExposureTime,
    RGN_TYPE_3A_IspDgain,
    RGN_TYPE_3A_Again,
    RGN_TYPE_3A_Dgain,
    RGN_TYPE_3A_Ratio,
    RGN_TYPE_3A_BV,
    RGN_TYPE_3A_FNumber,
    RGN_TYPE_MAX,
} UVC_REGION_TYPE_E;

extern int create_rgn(UVC_REGION_TYPE_E type, int devId, int chn, int bmp_width, int bmp_height, int px, int py);
extern int destory_rgn(UVC_REGION_TYPE_E type, int devId, int chn);
extern int update_rgn(UVC_REGION_TYPE_E type, int chn, char *text);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __OSD_H__ */
