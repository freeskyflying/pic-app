#ifndef _NORMAL_RECORD_DB_H_
#define _NORMAL_RECORD_DB_H_

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "queue_list.h"
#include "utils.h"

#define MEDIARECORDDB  		"/data/etc/sMediaRecord.db"
#define MEDIARECORDDBTEMP	"/data/etc/sMediaRecordTemp.db"

#ifdef __cplusplus
extern "C" {
#endif

#define kSqlCreateMediaRecordTable "CREATE TABLE IF NOT EXISTS %s (_id INTEGER PRIMARY KEY AUTOINCREMENT, time INT UNIQUE ON CONFLICT REPLACE, channel INT, type INT, duration INT DEFAULT(0), name TEXT);"
#define kCreateMediaRecordTimeIndex "CREATE INDEX IF NOT EXISTS %sTimeIndex ON %s(time);"

#define kQueryAllNormalMediaRecordSql "SELECT * FROM %s;"

#define kInsertMediaRecordData "INSERT OR REPLACE INTO %s VALUES (NULL, %d, %d, %d, %d, '%s');" // ����һ��¼�Ƽ�¼
#define kSqlUpdateMediaRecord "UPDATE %s SET duration='%d' WHERE name='%s';" // ����ʱ��
#define kQueryMediaRecordByTime "SELECT* FROM %s WHERE time <= %d AND time >= %d;" // ��ȡʱ�������ڵ���Ƶ
#define kQueryOldestMediaRecord "SELECT name FROM %s ORDER BY _id ASC LIMIT 1;"// �ҳ��������Ƶ�ļ���
#define kDeleteMediaRecordByName "DELETE FROM %s WHERE name='%s';"

typedef struct
{

	uint32_t				mTimeStart;
	uint8_t					mChannelNum;
	uint8_t					mStreamType;			// 0-����Ƶ�� 1-��Ƶ��
	uint16_t				mDuration;
	char					mFileName[64];

} NormalRecordInfo_t;

typedef struct
{

	short					mVideoChnMax;			// ��Ƶͨ����
	short					mAudioChnMax;			// ��Ƶͨ����
	sqlite3*				mDB;
	pthread_mutex_t			mMutex;

} NormalRecordDB_t;

void NormalRecordDB_Init(NormalRecordDB_t* nRecDB, int videoChnMax, int audioChnMax);

void NormalRecordDB_InsertMediaInfo(NormalRecordDB_t* nRecDB, int32_t timestamp, int chnNum, uint8_t streamType, uint16_t duration, char* filename);

void NormalRecordDB_UpdateDuration(NormalRecordDB_t* nRecDB, int chnNum, uint8_t streamType, char* filename, uint16_t duration);

int NormalRecordDB_GetMediaInfoByTimeArea(NormalRecordDB_t* nRecDB, int chnNum, uint8_t streamType, int32_t timeStart, int32_t timeEnd, queue_list_t* plist);

void NormalRecordDB_GetAndDeleteOldestMediaInfo(NormalRecordDB_t* nRecDB, int chnNum, uint8_t streamType, char* fileName);

#ifdef __cplusplus
}
#endif

#endif

