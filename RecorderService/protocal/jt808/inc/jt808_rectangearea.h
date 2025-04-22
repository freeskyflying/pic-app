#ifndef _JT808_RECTANGEAREA_H_
#define _JT808_RECTANGEAREA_H_

#include "jt808_area.h"
#include "jt808_data.h"


typedef struct
{

	unsigned int mAreaID;
	unsigned int mLatitudeTopLeft;
	unsigned int mLongitudeTopLeft;
	unsigned int mLatitudeBotRight;
	unsigned int mLongitudeBotRight;
	int mTimeStart;
	int mTimeStop;
	unsigned short mAttr;
	unsigned short mAreaNameLen;
	unsigned short mIsInArea;

} JT808RectAreaItem_t;

typedef struct
{

	queue_list_t mList;
	JT808AreaDb_t mDB;
	void* mOwner;

} JT808RectArea_t;

void JT808RectArea_DeleteItem(JT808RectArea_t* handle, unsigned int areaId);

void JT808RectArea_AddItem(JT808RectArea_t* handle, JT808_Rectangle_Area_t* areaInfo);

void JT808RectArea_DeleteAllItem(JT808RectArea_t* handle);

void JT808RectArea_Init(JT808RectArea_t* handle, void* owner);

int JT808RectArea_CalcDataLength(JT808RectArea_t* handle, unsigned int areaId, unsigned int* count);

int JT808RectArea_QueryEncodeData(JT808RectArea_t* handle, unsigned int areaId, unsigned char* buff);

#endif
