#ifndef _LIVE_VIDEO_PRIV_H
#define _LIVE_VIDEO_PRIV_H

#include "live_video.h"

#ifdef __cplusplus
extern "C" {
#endif

struct media_cfg {
	/* frame control specific */
	unsigned int			payload;
	unsigned int			width;
	unsigned int			height;
	unsigned int			fps;
	unsigned int			fcnt;

	void				*vstream;
	void				*video_rec;
};

#define live_video_logd(fmt, arg...) \
	ALOG(LOG_DEBUG, LOG_TAG, "<[%s %04d]> " \
		fmt, __func__, __LINE__,  ##arg);

#define live_video_loge(fmt, arg...) \
			ALOG(LOG_ERROR, LOG_TAG, "<[%s %04d]> " \
				fmt, __func__, __LINE__,  ##arg);



#ifdef __cplusplus
}
#endif

#endif /* _LIVE_VIDEO_PRIV_H */


