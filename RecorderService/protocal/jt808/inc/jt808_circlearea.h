#ifndef _JT808_CIRCLEAREA_H_
#define _JT808_CIRCLEAREA_H_


#include "jt808_area.h"
#include "jt808_data.h"


typedef struct
{

	unsigned int mAreaID;
	unsigned int mLatitude;
	unsigned int mLongitude;
	unsigned int mRadius;
	int mTimeStart;
	int mTimeStop;
	unsigned short mAttr;
	unsigned short mAreaNameLen;
	unsigned short mIsInArea;

} JT808CircleAreaItem_t;

typedef struct
{

	queue_list_t mList;
	JT808AreaDb_t mDB;
	void* mOwner;

} JT808CircleArea_t;

void JT808CircleArea_DeleteItem(JT808CircleArea_t* handle, unsigned int areaId);

void JT808CircleArea_AddItem(JT808CircleArea_t* handle, JT808_Circle_Area_t* areaInfo);

void JT808CircleArea_DeleteAllItem(JT808CircleArea_t* handle);

void JT808CircleArea_Init(JT808CircleArea_t* handle, void* owner);

int JT808CircleArea_CalcDataLength(JT808CircleArea_t* handle, unsigned int areaId, unsigned int* count);

int JT808CircleArea_QueryEncodeData(JT808CircleArea_t* handle, unsigned int areaId, unsigned char* buff);

#endif

