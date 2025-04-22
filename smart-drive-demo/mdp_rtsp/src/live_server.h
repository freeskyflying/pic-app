/*
 * live_server.h - RTSP Server Interface.
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

#ifndef _LIVE_SERVER_H
#define _LIVE_SERVER_H


#ifdef __cplusplus
extern "C" {
#endif

/* Media Server */
int is_rtsp_working(void);
int set_rtsp_work(int enable);
int get_rtsp_channel_port(void);
int set_rtsp_channel_port(int port);

#ifdef __cplusplus
};
#endif
#endif /* _LIVE_SERVER_H */

