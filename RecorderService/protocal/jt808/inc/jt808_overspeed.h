#ifndef _JT808_OVERSPEED_H_
#define _JT808_OVERSPEED_H_

typedef struct
{

	unsigned int	mSpeedLimit;
	unsigned int	mTimeThreshold;
	unsigned short	mEalyWarnDelta;
	unsigned char	mAreaType;
	unsigned int 	mAreaID;

	unsigned int 	mParamUpdate;

	void*	mOnwer;
	
} JT808Overspeed_t;

void JT808Overspeed_SetSpeedLimit(JT808Overspeed_t* overspeed, unsigned int speedLimit, unsigned char areaType, unsigned int areaId, unsigned int limitThreashold);

void JT808Overspeed_Init(JT808Overspeed_t* overspeed, void* controller);

void JT808Overspeed_ParamUpdate(JT808Overspeed_t* handle, unsigned short paramID);

#endif

