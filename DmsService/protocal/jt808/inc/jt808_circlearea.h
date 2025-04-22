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

} JT808CircleAreaItem_t;

typedef struct
{

	queue_list_t mList;
	jt808AreaDb_t mDB;
	void* mOwner;

} JT808CircleArea_t;

void JT808CircleArea_Init(JT808CircleArea_t* handle, void* owner);

#endif

