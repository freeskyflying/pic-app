#ifndef _JT808_MEDIAEVENT_H_
#define _JT808_MEDIAEVENT_H_

#include "queue_list.h"

#define JT808_DATABASE_MEDIAEVENT			"/data/etc/jt808_mediaevent_%s.db"

#define JT808_MEDIAEVENT_DB_VERSION			1

#define kSqlCreateMediaTable			"CREATE TABLE IF NOT EXISTS jt808Media(id INTEGER PRIMARY KEY AUTOINCREMENT, eventType INT, mediaType INT, chn INT, time INT, upload INT, flag INT, location BLOB)"
#define kSqlInsertOneMedia		  		"INSERT INTO jt808Media VALUES (NULL, %d, %d, %d, %d, %d, %d, ?);"
#define kSqlDeleteOneMedia 				"DELETE FROM jt808Media WHERE id=%d;"
#define kSqlRefreshMedias				"DELETE FROM jt808Media WHERE upload=2 AND flag=1;"
#define kSqlUpdateUploadFlag			"UPDATE jt808Media SET upload=%d, flag=%d WHERE id=%d;"
#define kSqlUpdateUpload				"UPDATE jt808Media SET upload=%d WHERE id=%d;"
#define kSqlDropMediaTable  			"DROP TABLE jt808Media;"

#define kSqlMediaGetVersion 		 	"PRAGMA user_version;"
#define kSqlMediaSetVersion  			"PRAGMA user_version = %d;"

typedef enum
{

	JT808MediaType_Photo,
	JT808MediaType_Audio,
	JT808MediaType_Video

} JT808MediaType_e;

#define JT808MEDIAEVENT_FILESUFIX(mt)			(mt==JT808MediaType_Photo?"jpg":(mt==JT808MediaType_Video?"mp4":"aac"))

#define JT808MEDIAEVENT_FILEENCTYPE(mt)			(mt==JT808MediaType_Photo?0:(mt==JT808MediaType_Video?4:3))

#define JT808MEDIAEVENT_FILENAME(id,f,mt,et,chn)	{ sprintf(f, EVENT_PATH"/%08x_%01x_%02x.%s", id, et, chn, JT808MEDIAEVENT_FILESUFIX(mt)); }

typedef enum
{

	JT808MediaEventType_PlatCmd,
	JT808MediaEventType_RegularTime,
	JT808MediaEventType_EmergeAlarm,
	JT808MediaEventType_Rollover,
	JT808MediaEventType_DoorOpen,
	JT808MediaEventType_DoorClose,
	JT808MediaEventType_DoorCloseSpeedHigh,
	JT808MediaEventType_RegularDistance,

} JT808MediaEventType_e;

typedef enum
{

	MediaPhotoResolution_160x120 = 0,
	MediaPhotoResolution_320x240,
	MediaPhotoResolution_640x480,
	MediaPhotoResolution_800x600,
	MediaPhotoResolution_1024x704,
	MediaPhotoResolution_176x144,
	MediaPhotoResolution_352x288,
	MediaPhotoResolution_704x288,
	MediaPhotoResolution_704x576,
	MediaPhotoResolution_1280x720,

} JT808MediaPhotoResolution_e;

typedef struct
{

	unsigned char mChannel;
	unsigned char mSaveFlag;
	unsigned char mEventType;
	unsigned char mResolution;
	unsigned short mCount;
	unsigned short mIntervel; /* 单位s */
	
	void* owner;

} JT808MediaEvent_PhotoTask_t;

typedef struct
{

	pthread_mutex_t			mMutex;
	sqlite3*				mDB;

	queue_list_t			mUploadList;

	volatile unsigned char 	mUploadFlag;

	void* owner;

} JT808MediaEvent_t;

void JT808MediaEvent_MediaVideoEvent(JT808MediaEvent_t* handle, BYTE eventType, BYTE chnNum, BYTE before, BYTE after);

void JT808MediaEvent_MediaPhotoEvent(JT808MediaEvent_t* handle, BYTE eventType, BYTE chnNum, int count, char saveFlag, int intervel, JT808MediaPhotoResolution_e resolution);

void JT808MediaEvent_DeleteRecordByMediaID(JT808MediaEvent_t* handle, DWORD mediaId);

void JT808MediaEvent_ReportEvent(JT808MediaEvent_t* handle, unsigned int mediaId, BYTE mediaType, BYTE eventType, BYTE chnNum);

void JT808MediaEvent_Init(JT808MediaEvent_t* handle, void* owner);

void JT808MediaEvent_QueryMediaFiles(JT808MediaEvent_t* handle, JT808MsgBody_8802_t* req, JT808MsgBody_0802_t* ack);

void JT808MediaEvent_UploadMediaFilesReq(JT808MediaEvent_t* handle, JT808MsgBody_8803_t* req);

void JT808MediaEvent_UploadMediaFileReq(JT808MediaEvent_t* handle, JT808MsgBody_8805_t* req);

void JT808MediaEvent_UploadMediaFileStatus(JT808MediaEvent_t* handle, JT808MsgBody_8800_t* req);

#endif


