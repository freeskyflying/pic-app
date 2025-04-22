#ifndef __MEDIA_H__
#define __MEDIA_H__


#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#include "uvc_i.h"

extern EI_S32 start_uvc_stream(stream_param_t *param);
extern EI_S32 stop_uvc_stream(stream_param_t *param);
extern EI_S32 start_video_source(void);
extern EI_S32 stop_video_source(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __MEDIA_H__ */
