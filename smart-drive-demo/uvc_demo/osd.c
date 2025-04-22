#include <sys/prctl.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "sample_comm.h"
#include "libuvcg.h"
#include "uvc_stream.h"
#include "osd.h"
#include "font.h"

#define MAX_STREAM_NUMBER 3

/* 这边需要保证每个区域的返回值不一样 */
static RGN_HANDLE _get_rgn_handle(UVC_REGION_TYPE_E type, int chn)
{
    return (type + 1) * MAX_STREAM_NUMBER + chn;
}

/* 创建区域 */
int create_rgn(UVC_REGION_TYPE_E type, int devId, int chn, int bmp_width, int bmp_height, int px, int py)
{
    
    RGN_HANDLE handle = _get_rgn_handle(type, chn);

    _DBG("type = %d, chn = %d, handle = %d, bmp_width = %d, bmp_height = %d, px = %d, py = %d\n", 
        type, chn, handle, bmp_width, bmp_height, px, py);
    RGN_ATTR_S rgn_attr;
    memset(&rgn_attr, 0, sizeof(RGN_ATTR_S));
    rgn_attr.enType = OVERLAYEX_RGN;
    rgn_attr.unAttr.stOverlayEx.enPixelFmt = PIX_FMT_ARGB_8888;//PIX_FMT_ARGB_1555;/* 用ARGB1555消耗资源比PIX_FMT_ARGB_8888少*/
    rgn_attr.unAttr.stOverlayEx.u32BgColor = 0x00ffffff;
    rgn_attr.unAttr.stOverlayEx.stSize.u32Width = bmp_width;
    rgn_attr.unAttr.stOverlayEx.stSize.u32Height = bmp_height;
    rgn_attr.unAttr.stOverlayEx.u32CanvasNum = 2;

    int ret = EI_MI_RGN_Create(handle, &rgn_attr);
    if (ret < 0)
    {
        _DBG("EI_MI_RGN_Create Failed. handle = %d, ret = 0x%X\n", handle, ret);
        return -1;
    }

    MDP_CHN_S stRgnChn = {0};
    memset(&stRgnChn, 0, sizeof(MDP_CHN_S));
   /*绑定 VPU[这个子码流就要单独绑定了]   */ 
    stRgnChn.enModId = EI_ID_IPPU;
    stRgnChn.s32DevId = devId;
    stRgnChn.s32ChnId = chn;    /* 编码通道 */

    RGN_CHN_ATTR_S rgn_chn_attr;
    memset(&rgn_chn_attr, 0, sizeof(RGN_CHN_ATTR_S));
    rgn_chn_attr.bShow = EI_TRUE;
    rgn_chn_attr.enType = OVERLAYEX_RGN;
    rgn_chn_attr.unChnAttr.stOverlayExChn.stPoint.s32X = px;
    rgn_chn_attr.unChnAttr.stOverlayExChn.stPoint.s32Y = py;
    rgn_chn_attr.unChnAttr.stOverlayExChn.u32FgAlpha = 255;
    //rgn_chn_attr.unChnAttr.stOverlayExChn.stInvertColorAttr.bInvertColorEn = EI_TRUE;//加上去就看不到

    ret = EI_MI_RGN_AddToChn(handle, &stRgnChn, &rgn_chn_attr);
    if (ret < 0)
    {
        _DBG("EI_MI_RGN_AddToChn Failed. handle = %d, ret = 0x%X\n", handle, ret);
        return -1;
    }
    
    return ret;
}

/* 销毁区域 */
int destory_rgn(UVC_REGION_TYPE_E type, int devId, int chn)
{
    //_DBG("type = %d, chn = %d", type, chn);
    RGN_HANDLE handle = _get_rgn_handle(type, chn);

    MDP_CHN_S stRgnChn = {0};
    memset(&stRgnChn, 0, sizeof(MDP_CHN_S));
    stRgnChn.enModId = EI_ID_IPPU;
    stRgnChn.s32DevId = devId;
    stRgnChn.s32ChnId = chn;    /* 子码流编码通道 */

    EI_MI_RGN_DelFromChn(handle, &stRgnChn);
    EI_MI_RGN_Destroy(handle);
    
    return 0;
}

int update_rgn(UVC_REGION_TYPE_E type, int chn, char *text)
{
    RGN_ATTR_S rgn_attr = {0};
    EI_S32 s32BytesPerPix = 2;
    BITMAP_S stBitmap;
    int width, height; 
    int ret;
    RGN_HANDLE handle = _get_rgn_handle(type, chn);

    
    memset(&stBitmap, 0, sizeof(BITMAP_S));
    memset(&rgn_attr, 0, sizeof(RGN_ATTR_S));

    //_DBG("Update rgn handl:%d\n", handle);
    ret = EI_MI_RGN_QueryAttr(handle, &rgn_attr);
    if (ret != 0) {
        _DBG("EI_MI_RGN_QueryAttr fail. handle:%d(type:%d chn:%d) %#x\n", handle, type, chn, ret);
        return -1;
    }
    width = rgn_attr.unAttr.stOverlayEx.stSize.u32Width;
    height = rgn_attr.unAttr.stOverlayEx.stSize.u32Height;

    if (text == NULL)
    {
        _DBG("[ERROR]Text is NULL!\n");
        return -1;
    }
    
    //width = strlen(text) * 8;
    //height = 16;
#if 0
    /* 注意：text的文本长度变化，需要重新创建区域 */
    static int default_chn_str_len = strlen("CHN5000");
    if (strlen(text) != default_chn_str_len)
    {
        _DBG("No Same Size[default_chn_str_len = %d, text_len = %d]", default_chn_str_len, strlen(text));
        _start_destory_rgn(type, chn);

        int chn_px = 120;
        int chn_py = 80;
        _start_create_rgn(type, chn, width, height, chn_px, chn_py);
        default_chn_str_len = strlen(text);
    }
#endif        
    
    /* osd0 */
    stBitmap.enPixelFormat = PIX_FMT_ARGB_8888;//PIX_FMT_ARGB_1555;
    stBitmap.u32Width = width;
    stBitmap.u32Height = height;
    switch (stBitmap.enPixelFormat) {
    case PIX_FMT_GREY_Y8: s32BytesPerPix = 1; break;
    case PIX_FMT_ARGB_4444: s32BytesPerPix = 2; break;
    case PIX_FMT_ARGB_1555: s32BytesPerPix = 2; break;
    case PIX_FMT_ARGB_8888: s32BytesPerPix = 4; break;
    default: break;
    }
    
    stBitmap.pData = calloc(stBitmap.u32Width * stBitmap.u32Height * s32BytesPerPix + 256, 1);
    if (!stBitmap.pData) {
        _DBG("Request memory fail.\n");
        return -1;
    }

   // _DBG("text:%s\n", text);
    text_to_bmp2((unsigned char *)text, width/16, stBitmap.pData);
#if 1
    static int flag[RGN_TYPE_MAX] = {0};
    static FILE *fp[RGN_TYPE_MAX] = {NULL}; 
    if (flag[type] < 1)
    {
        flag[type] += 1;
        if (!fp[type]) {
            char path[128] = {0};
            sprintf(path, "/data/text_%d_%d_%d.argb", type, stBitmap.u32Width, stBitmap.u32Height);
            fp[type] = fopen(path, "wb");
        }
        fwrite(stBitmap.pData, stBitmap.u32Width * stBitmap.u32Height * s32BytesPerPix, 1, fp[type]);
    } else {
        if (fp[type]) {
            fclose(fp[type]);
            fp[type] = NULL;
        }
    }
#endif

    ret = EI_MI_RGN_SetBitMap(handle, &stBitmap);
    if (ret < 0)
    {
        _DBG("EI_MI_RGN_SetBitMap Failed. type = %d, chn = %d, handle = %d, ret = %d, 0x%X\n", 
            type, chn, handle, ret, ret);
    }
    
   // _DBG("\n");
    free(stBitmap.pData);
   // _DBG("\n");

    return ret;
}

