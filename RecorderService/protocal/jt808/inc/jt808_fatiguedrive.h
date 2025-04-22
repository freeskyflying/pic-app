#ifndef _JT808_FATIGUEDRIVE_H_
#define _JT808_FATIGUEDRIVE_H_

#define DAY_DRIVE_TIME_RECORD_FILE			"/data/etc/dayDriveTimeRecord"

typedef struct
{

	unsigned int mTotalDayDriveTime;
	unsigned int mTotalDayRestTime;

	unsigned char datetime[6];

} DayDriveTimeRecord_t;

typedef struct
{
	DayDriveTimeRecord_t* mDayTotalTimes; /* 使用文件映射方式获取 */

	void* owner;

} JT808FatigueDrive_t;

void JT808FatigueDrive_Init(JT808FatigueDrive_t* handle, void* owner);


#endif


