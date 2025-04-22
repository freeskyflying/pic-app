/*
 * live_video.h - Get H264 Real-time video data interface.
 *
 * Copyright (C) 2016-2018, LomboTech Co.Ltd.
 * Author: lomboswer <lomboswer@lombotech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef _LIVE_VIDEO_H
#define _LIVE_VIDEO_H

#include "live_server.h"
#include "frame_queue.h"

typedef enum{
  LIVE_VIDEO_DEINIT,
  LIVE_VIDEO_INIT,
  LIVE_VIDEO_PAUSE,
  LIVE_VIDEO_RUNING
} live_video_state_t;

typedef struct live_video_para {
	/*enc*/
	int frame_rate;
	int height;
	int width;
	int bitrate;
} live_video_para_t;

int live_video_init(void *rgb_hdl);
//void live_video_face_cb(faces_t *faces);
int live_video_deinit(void);
int live_video_pause(int channel);
int live_video_resume(int channel);
live_video_state_t live_get_video_state(int channel);
int live_video_set_para(int channel, 
								const live_video_para_t *para);
int live_video_get_para(int channel,
								live_video_para_t *para);
								
int live_video_set_data_to_buf(dword dwFrameQueueNum, frame_dsc_s *FrameDsc);

#endif /* _LIVE_VIDEO_H */

