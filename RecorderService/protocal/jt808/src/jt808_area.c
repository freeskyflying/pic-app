
#include "pnt_log.h"
#include "utils.h"
#include "jt808_utils.h"
#include "jt808_area.h"
#include "jt808_circlearea.h"
#include "jt808_polygonarea.h"
#include "jt808_rectangearea.h"
#include "jt808_roadarea.h"
#include "jt808_controller.h"

int JT808Area_IsInValidTime(int timeStart, int timeEnd, int timeCurrent, int timeOfDay)
{
	if (timeStart > timeCurrent)
	{
		return 0;
	}

	if (timeEnd <= 943977599) // 00-00-00-23-59-59
	{
		if (timeEnd < timeOfDay)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}

	if (timeEnd < timeCurrent)
	{
		return 0;
	}

	return 1;
}

int JT808Area_Query(void* owner, JT808MsgBody_8608_t* req, BYTE** out)
{
	unsigned int dataLen = 0, areaCount = 0;
	JT808Controller_t* controller = (JT808Controller_t*)owner;
	unsigned char* ack = NULL;

	dataLen += 5;

	if (1 == req->mQueryType)
	{
		if (0 == req->mQueryIDCount)
		{
			dataLen += JT808CircleArea_CalcDataLength(&controller->mCircleArea, 0, &areaCount);
		}
		else
		{
			for (int i=0; i<req->mQueryIDCount; i++)
			{
				int len = JT808CircleArea_CalcDataLength(&controller->mCircleArea, req->pQueryIDs[i], NULL);
				if (0 < len)
				{
					dataLen += len;
					areaCount ++;
				}
			}
		}
	}
	else if (2 == req->mQueryType)
	{
		if (0 == req->mQueryIDCount)
		{
			dataLen += JT808RectArea_CalcDataLength(&controller->mRectArea, 0, &areaCount);
		}
		else
		{
			for (int i=0; i<req->mQueryIDCount; i++)
			{
				int len = JT808RectArea_CalcDataLength(&controller->mRectArea, req->pQueryIDs[i], NULL);
				if (0 < len)
				{
					dataLen += len;
					areaCount ++;
				}
			}
		}
	}
	else if (3 == req->mQueryType)
	{
		if (0 == req->mQueryIDCount)
		{
			dataLen += JT808PolygonArea_CalcDataLength(&controller->mPolygonArea, 0, &areaCount);
		}
		else
		{
			for (int i=0; i<req->mQueryIDCount; i++)
			{
				int len = JT808PolygonArea_CalcDataLength(&controller->mPolygonArea, req->pQueryIDs[i], NULL);
				if (0 < len)
				{
					dataLen += len;
					areaCount ++;
				}
			}
		}
	}
	else if (4 == req->mQueryType)
	{
		if (0 == req->mQueryIDCount)
		{
			dataLen += JT808RoadArea_CalcDataLength(&controller->mRoadArea, 0, &areaCount);
		}
		else
		{
			for (int i=0; i<req->mQueryIDCount; i++)
			{
				int len = JT808RoadArea_CalcDataLength(&controller->mRoadArea, req->pQueryIDs[i], NULL);
				if (0 < len)
				{
					dataLen += len;
					areaCount ++;
				}
			}
		}
	}

	ack = (unsigned char*)malloc(dataLen);
	if (NULL == ack)
	{
		return 0;
	}

	int offset = 0;
	
	offset = NetbufPushBYTE(ack, req->mQueryType, offset);
	offset = NetbufPushDWORD(ack, areaCount, offset);

	if (1 == req->mQueryType)
	{
		if (0 == req->mQueryIDCount)
		{
			JT808CircleArea_QueryEncodeData(&controller->mCircleArea, 0, ack+offset);
		}
		else
		{
			for (int i=0; i<req->mQueryIDCount; i++)
			{
				offset += JT808CircleArea_QueryEncodeData(&controller->mCircleArea, req->pQueryIDs[i], ack+offset);
			}
		}
	}
	else if (2 == req->mQueryType)
	{
		if (0 == req->mQueryIDCount)
		{
			JT808RectArea_QueryEncodeData(&controller->mRectArea, 0, ack+offset);
		}
		else
		{
			for (int i=0; i<req->mQueryIDCount; i++)
			{
				offset += JT808RectArea_QueryEncodeData(&controller->mRectArea, req->pQueryIDs[i], ack+offset);
			}
		}
	}
	else if (3 == req->mQueryType)
	{
		if (0 == req->mQueryIDCount)
		{
			JT808PolygonArea_QueryEncodeData(&controller->mPolygonArea, 0, ack+offset);
		}
		else
		{
			for (int i=0; i<req->mQueryIDCount; i++)
			{
				offset += JT808PolygonArea_QueryEncodeData(&controller->mPolygonArea, req->pQueryIDs[i], ack+offset);
			}
		}
	}
	else if (4 == req->mQueryType)
	{
		if (0 == req->mQueryIDCount)
		{
			JT808RoadArea_QueryEncodeData(&controller->mRoadArea, 0, ack+offset);
		}
		else
		{
			for (int i=0; i<req->mQueryIDCount; i++)
			{
				offset += JT808RoadArea_QueryEncodeData(&controller->mRoadArea, req->pQueryIDs[i], ack+offset);
			}
		}
	}

	*out = ack;

	return dataLen;
}

