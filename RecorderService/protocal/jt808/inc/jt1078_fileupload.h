#ifndef _JT1078_FILEUPLOAD_H_
#define _JT1078_FILEUPLOAD_H_


#include <pthread.h>
#include "jt808_data.h"
#include "jt808_session.h"
#include "utils.h"

#define JT1078FILE_UPLOAD_COUNT			1000

#define JT808_DATABASE_FILEUPLOAD			"/data/etc/jt1078_fileupload.db"

#define kSqlCreate1078FileUploadTable  	"CREATE TABLE IF NOT EXISTS JT1078FileUpload(_id INTEGER PRIMARY KEY, serialNum INT, url TEXT, port INT, userName TEXT, password TEXT,"\
										"remotePath TEXT, localPath TEXT, state INT, session TEXT);"

#define kSqlInsert1078FileUploadItem  	"INSERT OR REPLACE INTO JT1078FileUpload VALUES (NULL, %d, '%s', %d, '%s', '%s', '%s', '%s', '%d', '%s');"

#define kSqlDelete1078FileUploadItem  	"DELETE FROM JT1078FileUpload WHERE localpath='%s';"

#define kSqlDelete1078FileUploadByState	"DELETE FROM JT1078FileUpload WHERE state=%d;"

#define kSqlDelete1078FileUploadBySerialNum	"DELETE FROM JT1078FileUpload WHERE serialNum=%d;"

#define kUpdate1078FileUploadState 		"UPDATE JT1078FileUpload SET state=%d WHERE localPath='%s';"

#define kUpdate1078FileUploadStateBySerialNum	"UPDATE JT1078FileUpload SET state=%d WHERE serialNum=%d;"

#define kSql1078FileUploadCount 		"SELECT COUNT(*) FROM JT1078FileUpload;"

#define kSql1078FileUploadQuery 		"SELECT * FROM JT1078FileUpload;"

#define kSql1078FileUploadQueryBySerialNum 	"SELECT * FROM JT1078FileUpload WHERE serialNum=%d;"

#define kDeleteExpire1078FileUpload		"DELETE FROM JT1078FileUpload WHERE _id IN (SELECT _id FROM JT1078FileUpload ORDER BY _id ASC LIMIT %d);"

typedef enum
{

	JT1078FileUploadState_Pause,
	JT1078FileUploadState_Running,
	JT1078FileUploadState_Cancle,
	JT1078FileUploadState_Finish,

} JT1078FileUploadState_e;

typedef struct
{

	unsigned int serialNum;
	unsigned char url[255];
	unsigned int port;
	unsigned char userName[64];
	unsigned char password[64];
	unsigned char remotePath[255];
	unsigned char localPath[128];
	unsigned char state;
	unsigned char sessionStr[32];

} JT1078FileUploadItem_t;

typedef struct
{

	pthread_mutex_t			mMutex;
	sqlite3*				mDB;
	void*					mOwner;
	FtpUtil_t 				ftpUtil;

} JT1078FileUploadHandle_t;

void JT1078FileUpload_Init(JT1078FileUploadHandle_t* handle, void* owner);

void JT1078FileUpload_RequestProcess(unsigned short serialNum, JT808MsgBody_9206_t* req, JT1078FileUploadHandle_t* handle, JT808Session_t* session);

void JT1078FileUpload_RequestControl(JT1078FileUploadHandle_t* handle, JT808MsgBody_9207_t* ctrl);

#endif

