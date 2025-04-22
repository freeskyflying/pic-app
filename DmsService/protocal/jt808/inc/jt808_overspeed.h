#ifndef _JT808_OVERSPEED_H_
#define _JT808_OVERSPEED_H_

typedef struct
{

	unsigned int	mSpeedLimit;
	unsigned int	mTimeThreshold;
	unsigned short	mEalyWarnDelta;
	unsigned char	mAreaType;
	unsigned int 	mAreaID;

	void*	mOnwer;
	
} JT808Overspeed_t;

void JT808Overspeed_Init(JT808Overspeed_t* overspeed, void* controller);

#endif

