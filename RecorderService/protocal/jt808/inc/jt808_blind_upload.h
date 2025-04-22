#ifndef _JT808_BLIND_UPLOAD_H_
#define _JT808_BLIND_UPLOAD_H_

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define kDBPath "/data/etc/gpsdata_"
#define kUploadGpsCount  		3
#define kTotalGpsData 			10000
#define kGpsDataCountThreshold	1000

#define kCreateTable "CREATE TABLE IF NOT EXISTS %s (_id INTEGER PRIMARY KEY AUTOINCREMENT, serialNum INT, size INT, time INT, data BLOB, state INT DEFAULT(0), batchserialNum INT DEFAULT(0));"
#define kCreateSerialNumIndex "CREATE INDEX IF NOT EXISTS %sSerialNumIndex ON %s(serialNum);"
#define kCreateGpsTimeIndex "CREATE INDEX IF NOT EXISTS %sTimeIndex ON %s(time);"

#define kInsertGpsData "INSERT INTO %s(serialNum, size, time, data) VALUES (%d, %d, %d, ?);"
#define kUpdateGpsData "UPDATE %s SET state=1 WHERE serialNum=%d;"
#define kUpdateGpsDataByTime "UPDATE %s SET state=1 WHERE time=%ld;"
#define kDeleteGpsDataBySerialNum "DELETE FROM %s WHERE serialNum=%d;"
#define kDeleteGpsDataByTime "DELETE FROM %s WHERE time=%d;"
#define kDeleteGpsDataByState "DELETE FROM %s WHERE state=1;"

#define kQueryNewestGpsDataTime "SELECT time FROM %s WHERE state = 0 ORDER BY time DESC LIMIT 1;"
#define kQueryDataGpsByTime "SELECT* FROM %s WHERE time <= %d AND time >= %d AND state = 0 ORDER BY time ASC LIMIT %d;"

#define kDropTable "DROP TABLE %s;"
#define kGpsDataCount "SELECT COUNT(*) FROM %s;"
#define kDeleteExpireGpsData "DELETE FROM %s WHERE time IN (SELECT time FROM %s ORDER BY time ASC LIMIT %d);"
#define kDeleteGpsDataByBatchSerialNum "DELETE FROM %s WHERE batchserialNum=%d;"
#define kUpdateGpsDataBatchSerialNum "UPDATE %s SET batchserialNum=%d WHERE time = %d;"

typedef struct
{

	WORD			mSerialNum;
	WORD			mSize;
	unsigned long long	mTime;
	BYTE*			pData;

} JT808LocationData_t;

typedef struct
{

	pthread_mutex_t			mMutex;
	sqlite3*				mLocationDB;
	uint8_t					mTableName[16];
	void*					mOwner;

} JT808BlindUpload_t;

void JT808BlindUpload_InsertLocationData(JT808BlindUpload_t* blindUpload, WORD serialNum, WORD size, BYTE* data);

void JT808BlindUpload_DeleteLocationDataBySerialNum(JT808BlindUpload_t* blindUpload, WORD serialNum);

void JT808BlindUpload_DeleteLocationDataByBCDTime(JT808BlindUpload_t* blindUpload, uint8_t* bcdtime);

void JT808BlindUpload_DeleteLocationDataByBatchSerialNum(JT808BlindUpload_t* blindUpload, WORD batchSerialNum);

void JT808BlindUpload_DeleteLocationDataByState(JT808BlindUpload_t* blindUpload);

int JT808BlindUpload_GetDatasList(JT808BlindUpload_t * blindUpload, queue_list_t* list, int count, int time, int limitTime);

void JT808BlindUpload_SetBatchSerialNum(JT808BlindUpload_t* blindUpload, unsigned int time, WORD serialNum);

int JT808BlindUpload_GetCacheNewestGpsTime(JT808BlindUpload_t* blindUpload);

void JT808BlindUpload_DBInit(JT808BlindUpload_t* blindUpload, void* owner);

#endif

