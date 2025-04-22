#ifndef _JT1078_PARAMS_H_
#define _JT1078_PARAMS_H_

#include "jt808_data.h"

#define VIDEO_MAX_CHN		(8)
#define AUDIO_MAX_CHN		(8)

// the 1078 RTP payload format specification from GB_T 1078 : FORM-12
typedef enum 
{

	RTP_1078_PAYLOAD_G_721					= 1, // audio
	RTP_1078_PAYLOAD_G_722					= 2, // audio
	RTP_1078_PAYLOAD_G_723					= 3, // audio
	RTP_1078_PAYLOAD_G_728					= 4, // audio
	RTP_1078_PAYLOAD_G_729					= 5, // audio
	RTP_1078_PAYLOAD_G_711A = 6, // audio
	RTP_1078_PAYLOAD_G_711U = 7, // audio
	RTP_1078_PAYLOAD_G_726 = 8, // audio
	RTP_1078_PAYLOAD_G_729A = 9, // audio
	RTP_1078_PAYLOAD_DVI4_3 = 10, // audio
	RTP_1078_PAYLOAD_DVI4_4 = 11, // audio
	RTP_1078_PAYLOAD_DVI4_8K = 12, // audio
	RTP_1078_PAYLOAD_DVI4_16K = 13, // audio
	RTP_1078_PAYLOAD_LPC = 14, // audio
	RTP_1078_PAYLOAD_S16BE_STEREO = 15, // audio
	RTP_1078_PAYLOAD_S16BE_MONO = 16, // audio
	RTP_1078_PAYLOAD_MPEGAUDIO = 17, // audio
	RTP_1078_PAYLOAD_LPCM = 18, // audio
	RTP_1078_PAYLOAD_AAC = 19, // audio
	RTP_1078_PAYLOAD_WMA9STD = 20, // audio
	RTP_1078_PAYLOAD_HEAAC = 21, // audio
	RTP_1078_PAYLOAD_PCM_VOICE = 22, // audio
	RTP_1078_PAYLOAD_PCM_AUDIO = 23, // audio
	RTP_1078_PAYLOAD_AACLC = 24, // audio
	RTP_1078_PAYLOAD_MP3 = 25, // audio
	RTP_1078_PAYLOAD_ADPCMA = 26, // audio
	RTP_1078_PAYLOAD_MP4AUDIO = 27, // audio
	RTP_1078_PAYLOAD_AMR = 28, // audio
	RTP_1078_PLAYLOAD_TRANSPARENT = 91,
	RTP_1078_PAYLOAD_H264 = 98, // video
	RTP_1078_PAYLOAD_H265 = 99, // video
	RTP_1078_PAYLOAD_AVS = 100, // video
	RTP_1078_PAYLOAD_SVAC = 101, // video
	RTP_1078_PAYLOAD_MIX = 102, //video + audio
	
} PayloadType;

/* 音视频参数设置 对应808参数设置 0x0075 */
typedef struct
{

	BYTE 					mRealTimeEncMode;				/* 实时流编码模式，0-固定码率，1-可变码率，2-平均码率 */
	BYTE 					mRealTimeSize;					/* 实时流分辨率，0-QCIF，1-CIF，2-WCIF，3-D1，4-WD1，5-720P，6-1080P */
	WORD 					mRealTimeIFrameInterval;		/* 实时流关键帧间隔，1-100 */
	BYTE 					mRealTimeFps;					/* 实时流目标帧率，1-120 */
	DWORD 					mRealTimeBitrate;				/* 实时流目标码率，kbps */
	BYTE 					mSaveEncMode;					/* 存储流编码模式，0-固定码率，1-可变码率，2-平均码率 */
	BYTE 					mSaveSize;						/* 存储流分辨率，0-QCIF，1-CIF，2-WCIF，3-D1，4-WD1，5-720P，6-1080P */
	WORD 					mSaveIFrameInterval;			/* 存储流关键帧间隔，1-100 */
	BYTE 					mSaveFps;						/* 存储流帧率，1-120 */
	DWORD 					mSaveBitrate;					/* 存储流目标码率，kbps */
	WORD 					mOsd;							/* OSD字幕叠加设置，为0不叠加，为1叠加，
																bit0-日期和时间，bit1-车牌号码，bit2-逻辑通道号，bit3-经纬度，bit4-形式记录速度，bit5-卫星定位速度，bit6-连续驾驶时间，bit7~bit10-保留，bit11~bit15，自定义*/
	BYTE 					mIsAudioEnable;					/* 是否启用音频输出，0-不启用，1-启用 */

} JT1078AVParam_t;

/* 音视频通道列表设置 对应808参数设置 0x0076 */
typedef struct
{

	BYTE					mChannelCount;					/* 音视频通道总数 */
	BYTE					mAudioChnCount;					/* 音频通道总数 */
	BYTE					mVideoChnCount;					/* 视频通道总数 */
	BYTE					mChannelList[(VIDEO_MAX_CHN+AUDIO_MAX_CHN)*4];			
															/* 音视频通道对照表, 
																BYTE0-物理通道号，从1开始
																BYTE1-逻辑通道号，按照JT/T 1076-2016 表2
																BYTE2-通道类型，0-音视频，1-音频，2-视频
																BYTE3-是否连接云台
															*/

} JT1078AVChannel_t;

/* 单独视频参数设置 对应808参数设置 0x0077 */
typedef struct
{

	BYTE					mLogicChnNUm;					/* 逻辑通道号 */
	BYTE 					mRealTimeEncMode;				/* 实时流编码模式，0-固定码率，1-可变码率，2-平均码率 */
	BYTE 					mRealTimeSize;					/* 实时流分辨率，0-QCIF，1-CIF，2-WCIF，3-D1，4-WD1，5-720P，6-1080P */
	WORD 					mRealTimeIFrameInterval;		/* 实时流关键帧间隔，1-100 */
	BYTE 					mRealTimeFps;					/* 实时流目标帧率，1-120 */
	DWORD 					mRealTimeBitrate;				/* 实时流目标码率，kbps */
	BYTE 					mSaveEncMode;					/* 存储流编码模式，0-固定码率，1-可变码率，2-平均码率 */
	BYTE 					mSaveSize;						/* 存储流分辨率，0-QCIF，1-CIF，2-WCIF，3-D1，4-WD1，5-720P，6-1080P */
	WORD 					mSaveIFrameInterval;			/* 存储流关键帧间隔，1-100 */
	BYTE 					mSaveFps;						/* 存储流帧率，1-120 */
	DWORD 					mSaveBitrate;					/* 存储流目标码率，kbps */
	WORD 					mOsd;							/* OSD字幕叠加设置，为0不叠加，为1叠加，
																bit0-日期和时间，bit1-车牌号码，bit2-逻辑通道号，bit3-经纬度，bit4-形式记录速度，bit5-卫星定位速度，bit6-连续驾驶时间，bit7~bit10-保留，bit11~bit15，自定义*/

} JT1078VideoParam_t;
typedef struct
{

	BYTE					mChannelCount;					/* 需要单独设置的视频通道数量 */
	JT1078VideoParam_t*		pList;

} JT1078VideosParam_t;

/* 特殊报警录像参数设置 对应808参数设置 0x0079 */
typedef struct
{

	BYTE					mStorageUsageDuty;				/* 特殊报警视频存储占用主存储器比值，默认20，取值1-99 */
	BYTE					mDurationMaxValue;				/* 特殊报警视频录制最长持续时间，单位分钟，默认5 */
	BYTE					mCacheBeforeDuration;			/* 特殊报警发生前进行标记的录像时间，单位分钟，默认1 */

} JT1078SpecialAlertParam_t;

/* 图像分析报警参数设置 对应808参数设置 0x007B */
typedef struct
{

	BYTE					mNuclearLoadCount;				/* 核载人数，视频分析结果超过时报警 */
	BYTE					mFatigueThreashold;				/* 疲劳驾驶阈值，视频分析结果超过时报警 */

} JT1078VideoAnaliseAlertParam_t;

/* 终端休眠唤醒模式设置 对应808参数设置 0x007C */
typedef struct
{

	BYTE					mWakeupMode;					/* 唤醒模式，0-条件唤醒，1-定时唤醒，2-手动唤醒 */
	BYTE					mWakeupCondition;				/* 唤醒条件，bit0-紧急报警，bit1-碰撞侧翻报警，bit2-车辆开门 */
	BYTE					mWakeupWeekday;					/* 唤醒日设置，bit0-星期一~bit6-星期六 */
	BYTE					mValidFlag;						/* 时间段唤醒启用标志，bit0-时间段1，bit1-时间段2，bit2-时间段3，bit3-时间段4 */
	BYTE					mTime1Wakeup[2];				/* 唤醒时间，HHMM，取值00：00~23：59 */
	BYTE					mTime1Sleep[2];					/* 关闭时间，HHMM，取值00：00~23：59 */
	BYTE					mTime2Wakeup[2];
	BYTE					mTime2Sleep[2];
	BYTE					mTime3Wakeup[2];
	BYTE					mTime3Sleep[2];
	BYTE					mTime4Wakeup[2];
	BYTE					mTime4Sleep[2];

} JT1078TerminalWakeupParam_t;

#endif

