#ifndef _JT808_CONTROLLER_H_
#define _JT808_CONTROLLER_H_

#include "jt808_session.h"
#include "jt808_database.h"
#include "ttsmain.h"
#include "pnt_ipc_gps.h"
#include "pnt_ipc.h"
#include "jt808_activesafety.h"
#include "jt1078_fileupload.h"
#include "jt808_overspeed.h"


#define JT808SESSION_COUNT_INTIME				(5)

typedef struct
{

	bool_t						mStartFlag;

	uint32_t					mCurrentAlertFlag;						/* 当前报警标志 */
	uint32_t					mCurrentTermState;						/* 当前状态标志 */

	uint32_t					mVehicalDriveDistance;					/* 车辆行驶距离 */

	TTSMain_t					mTTSMain;

	JT808Database_t				mJT808DB;								/* session、params数据库 */

	JT808Session_t*				mSession[JT808SESSION_COUNT_INTIME];	/* 808 session，暂定同时支持5个 */

	JT808ActiveSafety_t			mActivesafety;

	char						mDeviceID[7];
	char						mPlateNum[64];
	char						mPlateColor;
	char						mProtocalType;

	PNTIPC_t					mLocationIPC;
	GpsLocation_t				mLocation;

	JT1078FileUploadHandle_t	m1078FileUpload;
	JT808Overspeed_t			mOverspeed;

} JT808Controller_t;

extern JT808Controller_t gJT808Controller;

void JT808Controller_Stop(void);

void JT808Controller_Start(void);

void JT808Controller_StopSession(int id);

void JT808Controller_StartSession(int id);

void JT808Contorller_InsertFrame(int chnVenc, int sub, unsigned char* buff1, int len1, unsigned char* buff2, int len2);

void JT808Contorller_InsertAudio(int chnVenc, unsigned char* buff, int len);

void JT808Controller_Insert0200Ext(JT808MsgBody_0200Ex_t* ext);

#endif


