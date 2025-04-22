#ifndef _JT808_DATABASE_H_
#define _JT808_DATABASE_H_

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "utils.h"
#include "jt808_data.h"
#include "queue_list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JT808_DATABASE_SESSION			"/data/etc/pntsession.db"
#define JT808_DATABASE_SESSION_VER		(2)

#define kSqlCreateSessionTable  "CREATE TABLE IF NOT EXISTS SessionTable(_id INTEGER PRIMARY KEY, address TEXT, port INT, code TEXT, phone_num TEXT, session_type INT, vehicle_id INT,warningFilesEnable INT,realTimeVideoEnable INT);"
#define kSqlInsertSession  "INSERT OR REPLACE INTO SessionTable VALUES (NULL, ?, ?, ?, ?, ?, ?,?,?);"
#define kSqlDeleteSession  "DELETE FROM SessionTable WHERE address='%s' AND port=%d;"
#define kSqlDropSessionTable  "DROP TABLE SessionTable;"
#define kSqlQuerySession  "SELECT * FROM SessionTable;"
#define kSqlSelectOneSession  "SELECT * FROM SessionTable;"
#define kSqlUpdateSession  "UPDATE SessionTable SET code='%s'," \
										"phone_num='%s'," \
										"session_type=%d," \
										"vehicle_id=%d," \
										"warningFilesEnable=%d," \
										"realTimeVideoEnable=%d " \
										"WHERE address='%s' AND port=%d;"

#define kSqlCreateSessionParamTable  "CREATE TABLE IF NOT EXISTS SessionParam (_id INTEGER PRIMARY KEY AUTOINCREMENT, paramID INT UNIQUE ON CONFLICT REPLACE, paramLength INT, paramType INT, paramData TEXT);"
#define kSqlCreateSessionParamIndex  "CREATE INDEX IF NOT EXISTS SessionParamIndex ON SessionParam(paramID);"
#define kSqlInsertOrUpdateSessionParam  "INSERT OR REPLACE INTO SessionParam VALUES (NULL, %d, %d, %d, '%s');"
#define kSqlDropSessionParamTable  "DROP TABLE SessionParam;"
#define kSqlDropSessionParamIndex  "DROP INDEX SessionParamIndex;"
#define kSqlQuerySessionParam  "SELECT * FROM SessionParam WHERE paramID=%d;"

#define kSqlInitSessionParams  "INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0001, 4, 0x03, '10');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0002, 4, 0x03, '30');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0003, 4, 0x03, '3');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0004, 4, 0x03, '5');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0005, 4, 0x03, '3');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0006, 4, 0x03, '5');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0007, 4, 0x03, '3');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0010, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0011, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0012, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0013, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0014, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0015, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0016, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0017, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0018, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0019, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x001A, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x001B, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x001C, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x001D, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0020, 4, 0x03, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0021, 4, 0x03, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0022, 4, 0x03, '30');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0027, 4, 0x03, '300');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0028, 4, 0x03, '5');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0029, 4, 0x03, '10');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x002C, 4, 0x03, '100');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x002D, 4, 0x03, '500');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x002E, 4, 0x03, '500');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x002F, 4, 0x03, '200');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0030, 4, 0x03, '10');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0031, 2, 0x02, '500');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0040, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0041, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0042, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0043, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0044, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0045, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0046, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0047, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0048, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0049, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0050, 4, 0x03, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0051, 4, 0x03, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0052, 4, 0x03, '4294967295');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0053, 4, 0x03, '4294967295');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0054, 4, 0x03, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0055, 4, 0x03, '100');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0056, 4, 0x03, '10');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0057, 4, 0x03, '14400');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0058, 4, 0x03, '28800');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0059, 4, 0x03, '1200');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x005A, 4, 0x03, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x005B, 2, 0x02, '100');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x005C, 2, 0x02, '1800');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x005D, 2, 0x02, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x005E, 2, 0x02, '80');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0064, 4, 0x03, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0065, 4, 0x03, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0070, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0071, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0072, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0073, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0074, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0080, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0081, 2, 0x02, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0082, 2, 0x02, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0083, 0, 0x05, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0084, 1, 0x01, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0090, 1, 0x01, '1');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0091, 1, 0x01, '4');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0092, 1, 0x01, '1');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0093, 4, 0x03, '1');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0094, 1, 0x01, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0095, 4, 0x03, '1');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0100, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0101, 2, 0x02, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0102, 4, 0x03, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0103, 2, 0x02, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0110, 8, 0x04, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0111, 8, 0x04, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0112, 8, 0x04, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0113, 8, 0x04, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0114, 8, 0x04, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0115, 8, 0x04, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0116, 8, 0x04, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0117, 8, 0x04, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0075, 21, 0x06, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0076, 20, 0x06, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0077, 0, 0x06, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x0079, 0, 0x06, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x007A, 4, 0x03, '0');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x007B, 0, 0x06, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0x007C, 0, 0x06, '');" \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0xF364, 0, 0x06, '');"	\
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0xF365, 0, 0x06, '');"

#define kSqlAddSessionParams  "INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0xF364, 128, 0x06, '1E0600003C00C803020503000F0FFF00000003001E320503023C0532050302320503021B320503021E320503020A320503020102320503023205030200000000');"  \
										"INSERT OR REPLACE INTO SessionParam VALUES (NULL, 0xF365, 132, 0x06, '1E06000E1000C803020503000FFFFF00000003012C012C00000032050302320503023205030232050302320503020132320503023205030232050302320503020000');"

#define kSqlGetVersion  "PRAGMA user_version;"
#define kSqlSetVersion  "PRAGMA user_version = %d;"

typedef JT808MsgBody_8103_Item_t	JT808Param_t;

/**
 * 上808平台参数.
 */
typedef struct 
{

	BYTE					mAuthenticode[64];				/* 鉴权码 */
	BYTE					mAddress[32];					/* ip或域名 */
	WORD					mPort;							/* 端口号 */
	char					mPhoneNum[21]; 					/* 每个平台自己的电话号码，BCD字符串，需要转换为BCD码给808Session使用 */
	BYTE					mSessionType; 					/* 协议类型: 0 JT808-2013; 1 天迈; 2 JT808-2019 */
	DWORD					mVehicleId; 					/* 车辆号(只是针对天迈的特殊必须字段) */
	DWORD					mWarningFileEnable; 			/* 报警附件不上传开关 */
	DWORD					mRealTimeVideo; 				/* 实时视频不播放的开关 */
	
}JT808SessionData_t;

#define LOAD_SESSIONDATA_TO_MEM					1

typedef struct
{

	pthread_mutex_t			mMutexSessionDB;
	sqlite3*				mSessionDB;
#if (LOAD_SESSIONDATA_TO_MEM == 1)
	queue_list_t			mSessionDataList;
#endif
	queue_list_t			mSessionParams;
	void*					mOwner;

} JT808Database_t;

int JT808Database_Init(JT808Database_t* jt808DB, void* owner);

int JT808Database_AddSessionData(JT808Database_t* jt808DB, JT808SessionData_t* data);

int JT808Database_DeleteSessionData(JT808Database_t* jt808DB, JT808SessionData_t* data);

int JT808Database_UpdateSessionData(JT808Database_t* jt808DB, JT808SessionData_t* data);

int JT808Database_SetJT808Param(JT808Database_t* jt808DB, JT808Param_t* param);

/* 获取参数值，返回指向参数值的指针，以便跟踪值的变化 */
void* JT808Database_GetJT808Param(JT808Database_t* jt808DB, DWORD paramID, JT808Param_t** paramOut);

void JT808DataBase_ClearSessionData(JT808Database_t* jt808DB);

int JT808Database_GetAllJT808Param(JT808Database_t* jt808DB, queue_list_t* plist);

#ifdef __cplusplus
}
#endif

#endif

