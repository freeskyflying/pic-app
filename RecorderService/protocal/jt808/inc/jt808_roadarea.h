#ifndef _JT808_ROADAREA_H_
#define _JT808_ROADAREA_H_

#include "jt808_area.h"
#include "jt808_data.h"
#include "utils.h"


typedef struct
{

	unsigned int mAreaID;
	unsigned short mAttr;
	unsigned short mAreaNameLen;
	unsigned short mIsInArea;
	
	unsigned short mAreaPointCount;
	JT808_InflectionPoint_t*	mPoints;

	int mTimeStart;
	int mTimeStop;

} JT808RoadAreaItem_t;

typedef struct
{

	queue_list_t mList;
	JT808AreaDb_t mDB;
	void* mOwner;

} JT808RoadArea_t;

void JT808RoadArea_DeleteItem(JT808RoadArea_t* handle, unsigned int areaId);

void JT808RoadArea_AddItem(JT808RoadArea_t* handle, JT808MsgBody_8606_t* areaInfo);

void JT808RoadArea_DeleteAllItem(JT808RoadArea_t* handle);

void JT808RoadArea_Init(JT808RoadArea_t* handle, void* owner);

int JT808RoadArea_CalcDataLength(JT808RoadArea_t* handle, unsigned int areaId, unsigned int* count);

int JT808RoadArea_QueryEncodeData(JT808RoadArea_t* handle, unsigned int areaId, unsigned char* buff);

#endif


