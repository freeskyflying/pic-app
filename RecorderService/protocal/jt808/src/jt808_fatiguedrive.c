#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <wait.h>
#include <pthread.h>

#include "pnt_log.h"
#include "jt808_fatiguedrive.h"
#include "jt808_controller.h"

extern volatile unsigned char gACCState;

void* JT808FatigueDrive_HandleTask(void* pArg)
{
	pthread_detach(pthread_self());

	JT808FatigueDrive_t* handle = (JT808FatigueDrive_t*)pArg;
	JT808Controller_t* controller = (JT808Controller_t*)handle->owner;

    while (controller->mStartFlag)
    {
        if (1690000000 < currentTimeMillis()/1000) //未校时不进行录像
        {
            break;
        }
        sleep(1);
    }

	DWORD* fatigueDriveTimeThr = NULL;
	DWORD* dayContinueDriveTimeThr = NULL;
	DWORD* minRestTimeThr = NULL;
	DWORD* longStopTimeThr = NULL;
	WORD* preFatigueDriveTimeThr = NULL;
	
	while (NULL == fatigueDriveTimeThr || NULL == dayContinueDriveTimeThr || NULL == minRestTimeThr ||
		NULL == longStopTimeThr || NULL == preFatigueDriveTimeThr)
	{
		fatigueDriveTimeThr = (unsigned int*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x0057, NULL);
		dayContinueDriveTimeThr = (unsigned int*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x0058, NULL);
		minRestTimeThr = (unsigned int*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x0059, NULL);
		longStopTimeThr = (unsigned int*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x005A, NULL);
		preFatigueDriveTimeThr = (unsigned short*)JT808Database_GetJT808Param(&controller->mJT808DB, 0x005C, NULL);
		sleep(1);
	}

	char bcdTime[6] = { 0 };

	unsigned int driveStartTime = currentTimeMillis()/1000;
	unsigned int restStartTime = currentTimeMillis()/1000;

	while (controller->mStartFlag)
	{
		getLocalBCDTime(bcdTime);
	
		if (strncmp((char*)handle->mDayTotalTimes->datetime, bcdTime, 3)) // 新的一天
		{
			memcpy(handle->mDayTotalTimes->datetime, bcdTime, 6);
			handle->mDayTotalTimes->mTotalDayDriveTime = 0;
			handle->mDayTotalTimes->mTotalDayRestTime = 0;
		}

		if (gACCState && controller->mLocation.speed > 10)
		{
			unsigned int driveTime = currentTimeMillis()/1000 - driveStartTime;
			
			if (driveTime > (*fatigueDriveTimeThr))
			{
				controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_FATIGUEDRIVE;
				controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_PRE_FATIGUEDRIVE);
			}
			else if (driveTime > ((*fatigueDriveTimeThr) - *(preFatigueDriveTimeThr)))
			{
				controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_PRE_FATIGUEDRIVE;
			}

			restStartTime = currentTimeMillis()/1000;

			handle->mDayTotalTimes->mTotalDayDriveTime += 5;
		}
		else
		{
			unsigned int restTime = currentTimeMillis()/1000 - restStartTime;
			
			if (restTime > (*minRestTimeThr))
			{
				controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_FATIGUEDRIVE);
				driveStartTime = currentTimeMillis()/1000;
			}
		}
		
		if (handle->mDayTotalTimes->mTotalDayDriveTime > (*dayContinueDriveTimeThr))
		{
			controller->mCurrentAlertFlag |= JT808_ALERT_FLAG_CURRENTDAYOVERDRIVE;
		}
		else
		{
			controller->mCurrentAlertFlag &= (~JT808_ALERT_FLAG_CURRENTDAYOVERDRIVE);
		}

		sleep(5);
	}

	return NULL;
}

void JT808FatigueDrive_Init(JT808FatigueDrive_t* handle, void* owner)
{
	DayDriveTimeRecord_t param;

	memset(handle, 0, sizeof(JT808FatigueDrive_t));

	memset(&param, 0, sizeof(DayDriveTimeRecord_t));

	int fd = open(DAY_DRIVE_TIME_RECORD_FILE, O_RDWR | O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("create %s failed.\n", DAY_DRIVE_TIME_RECORD_FILE);
		return;
	}
	
	int size = lseek(fd, 0, SEEK_END);

	if (size < sizeof(DayDriveTimeRecord_t))
	{
		PNT_LOGE("%s resize %d", DAY_DRIVE_TIME_RECORD_FILE, size);
		
		if (0 < size)
		{
			lseek(fd, 0, SEEK_SET);
			read(fd, &param, size);
		}
		ftruncate(fd, sizeof(DayDriveTimeRecord_t)-size);
		lseek(fd, 0, SEEK_SET);
		write(fd, &param, sizeof(DayDriveTimeRecord_t));
	}

	handle->mDayTotalTimes = mmap(NULL, sizeof(DayDriveTimeRecord_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (MAP_FAILED == handle->mDayTotalTimes)
	{
		PNT_LOGE("mmap file %s failed.\n", DAY_DRIVE_TIME_RECORD_FILE);
		close(fd);
		return;
	}

	handle->owner = owner;

	pthread_t pid;
	if (pthread_create(&pid, NULL, JT808FatigueDrive_HandleTask, handle))
	{
		PNT_LOGE("task create failed.")
	}
}


