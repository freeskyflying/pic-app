#ifndef _JT808_AREA_H_
#define _JT808_AREA_H_

#include <pthread.h>
#include "jt808_data.h"
#include "utils.h"
#include "jt808_overspeed.h"


#define JT808_DATABASE_CIRCLEAREA		"/data/etc/jt808_circlearea.db"

#define JT808_DATABASE_RECTAREA			"/data/etc/jt808_rectarea.db"

#define JT808_DATABASE_POLYGAREA		"/data/etc/jt808_polygarea.db"

#define JT808_DATABASE_ROADAREA			"/data/etc/jt808_roadarea.db"

#define AREA_COUNT					1000

#define kSqlCreateAreaTable			"CREATE TABLE IF NOT EXISTS jt808Area(_id INTEGER PRIMARY KEY, areaId INT UNIQUE ON CONFLICT REPLACE, data BLOB)"
#define kSqlCreateAreaIndex  		"CREATE INDEX IF NOT EXISTS jt808AreaIndex ON jt808Area(areaId);"
#define kSqlInsertOrUpdateArea  	"INSERT OR REPLACE INTO jt808Area VALUES (NULL, %d, ?);"
#define kSqlDeleteOneArea  			"DELETE FROM jt808Area WHERE areaId=%d;"
#define kSqlSelectOneArea  			"SELECT * FROM jt808Area WHERE areaId=%d;"
#define kSqlQueryAllArea  			"SELECT * FROM jt808Area"
#define kSqlQueryAreaCount 			"SELECT COUNT(*) FROM jt808Area"
#define kSqlDeleteAllArea  			"DELETE FROM jt808Area"
#define kDeleteExpireAreaItem 		"DELETE FROM jt808Area WHERE _id IN (SELECT _id FROM jt808Area ORDER BY _id ASC LIMIT %d);"

typedef struct
{

	pthread_mutex_t			mMutex;
	sqlite3*				mDB;

} JT808AreaDb_t;

int JT808Area_IsInValidTime(int timeStart, int timeEnd, int timeCurrent, int timeOfDay);

int JT808Area_Query(void* owner, JT808MsgBody_8608_t* req, BYTE** out);

#endif

