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
#include "jt808_circlearea.h"
#include "jt808_rectangearea.h"
#include "jt808_polygonarea.h"
#include "jt808_roadarea.h"
#include "jt808_fatiguedrive.h"


#define JT808SESSION_COUNT_INTIME				(2)

typedef struct
{

	volatile bool_t				mStartFlag;

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

	PNTIPC_t					mLocationIPC;
	GpsLocation_t				mLocation;

	JT1078FileUploadHandle_t	m1078FileUpload;
	JT808Overspeed_t			mOverspeed;
	JT808CircleArea_t			mCircleArea;
	JT808RectArea_t				mRectArea;
	JT808PolygonArea_t			mPolygonArea;
	JT808RoadArea_t				mRoadArea;
	JT808FatigueDrive_t			mFatigueDrive;

} JT808Controller_t;

extern JT808Controller_t gJT808Controller;

void JT808Controller_Init(void);

void JT808Controller_Stop(void);

void JT808Controller_Start(void);

void JT808Controller_StopSession(int id);

void JT808Controller_StartSession(int id);

void JT808Controller_AddSession(char* ip, int port, char* phoneNum, int sessionType);

void JT808Controller_Insert0200Ext(JT808MsgBody_0200Ex_t* ext);

void JT808Controller_ReportPassengers(uint32_t start_time, int pg_count_on, int pg_count_off);

#endif


