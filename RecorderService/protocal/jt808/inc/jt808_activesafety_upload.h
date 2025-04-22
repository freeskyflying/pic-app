#ifndef _JT808_ACTIVESAFETY_UPLOAD_H_
#define _JT808_ACTIVESAFETY_UPLOAD_H_

#include "jt808_data.h"
#include "queue_list.h"

#define FILE_PACK_SIZE		(64*1024)

#define JT808ACTIVE_DATABASE_UPLOAD			"/data/etc/jt808active_upload_%s.db"

#define JT808ACTIVE_UPLOAD_DB_VERSION		1

#define kSqlCreateUploadTable			"CREATE TABLE IF NOT EXISTS jt808ActiveUpload(id INTEGER PRIMARY KEY AUTOINCREMENT, ip TEXT, port INT, alertID TEXT, upload INT, alertFlag BLOB)"
#define kSqlInsertOneUpload		  		"INSERT INTO jt808ActiveUpload VALUES (NULL, '%s', %d, '%s', %d, ?);"
#define kSqlDeleteOneUpload 			"DELETE FROM jt808ActiveUpload WHERE id=%d;"
#define kSqlDropUploadTable  			"DROP TABLE jt808ActiveUpload;"
#define kQueryUploadBySeq	 			"SELECT* FROM jt808ActiveUpload WHERE upload = 0 ORDER BY id ASC LIMIT %d;"

#define kSqlUploadGetVersion 		 	"PRAGMA user_version;"
#define kSqlUploadSetVersion  			"PRAGMA user_version = %d;"

typedef struct
{

	char mIp[64];
	WORD mTCPPort;
	BYTE mAlertFlag[16];
	BYTE mAlertID[33];
	BYTE mRetryCount;
	DWORD mID;

} JT808ActivesafetyRequest_t;

typedef enum
{

	upload_state_connect_server,
	upload_state_report_attinfo,
	upload_state_report_fileinfo,
	upload_state_running,
	upload_state_complete,
	upload_state_exit,

} ActivesafetyUploadState_e;

typedef struct
{

	queue_list_t reqList;
	int socketFd;

	int msgSerialNum;

	volatile int mState;

	JT808MsgBody_1210_File_t* fileList;
	int currenFileIdx;
	int currentFileCount;

	pthread_mutex_t			mMutex;
	sqlite3*				mDB;

	void* owner;

} JT808ActivesafetyUploadHandle_t;

void JT808ActiveSafety_UploadRequest(JT808ActivesafetyUploadHandle_t* handle, JT808MsgBody_9208_t* req);

void JT808ActiveSafety_UploadInit(JT808ActivesafetyUploadHandle_t * handle, void* owner);

#endif

