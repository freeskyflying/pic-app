#ifndef _JT808_POLYGONAREA_H_
#define _JT808_POLYGONAREA_H_

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
	LatLng_t*	mPoints;
	
	int mTimeStart;
	int mTimeStop;

} JT808PolygonAreaItem_t;

typedef struct
{

	queue_list_t mList;
	JT808AreaDb_t mDB;
	void* mOwner;

} JT808PolygonArea_t;

void JT808PolygonArea_DeleteItem(JT808PolygonArea_t* handle, unsigned int areaId);

void JT808PolygonArea_AddItem(JT808PolygonArea_t* handle, JT808MsgBody_8604_t* areaInfo);

void JT808PolygonArea_DeleteAllItem(JT808PolygonArea_t* handle);

void JT808PolygonArea_Init(JT808PolygonArea_t* handle, void* owner);

int JT808PolygonArea_CalcDataLength(JT808PolygonArea_t* handle, unsigned int areaId, unsigned int* count);

int JT808PolygonArea_QueryEncodeData(JT808PolygonArea_t* handle, unsigned int areaId, unsigned char* buff);

#endif


