#include "common_test.h"
#include "sc7a20.h"
#include "imu/inv_imu_driver.h"
#include "Invn/EmbUtils/Message.h"

extern void inv_msg_logout(int level, const char * str, va_list ap);
extern int inv_imu_get_context(void);
extern int inv_imu_release_context(struct inv_imu_serif *serif);
extern int inv_imu_read(struct inv_imu_serif *serif, uint8_t reg, uint8_t *buf, uint32_t len);
extern int inv_imu_write(struct inv_imu_serif *serif, uint8_t reg, const uint8_t *buf, uint32_t len);

extern BoardSysInfo_t gBoardInfo;

void* GSensorTestThread(void* pArg)
{
	pthread_detach(pthread_self());

	int ret = -1;
	struct inv_imu_serif icm_serif;

	INV_MSG_SETUP(INV_MSG_LEVEL_DEBUG, inv_msg_logout);
	icm_serif.context    = inv_imu_get_context(); /* no need */
	icm_serif.read_reg   = inv_imu_read;
	icm_serif.write_reg  = inv_imu_write;
	icm_serif.max_read   = 1024 * 32; /* maximum number of bytes allowed per serial read */
	icm_serif.max_write  = 1024 * 32; /* maximum number of bytes allowed per serial write */
	
	while (gTestRunningFlag)
	{
		if (0 == strcmp(gBoardInfo.mProductModel, "P-DC-2") || 0 == strcmp(gBoardInfo.mProductModel, "P-DC-3"))
		{
			ret = setup_imu_device(&icm_serif);
		}
		else
		{
			ret = G_Sensor_SC7A20_Init(1);
		}
		if (RET_SUCCESS == ret/*G_Sensor_SC7A20_Init(1)*/)
		{
			gTestResults[ITEM_GSENSOR].result = 1;
			gTestResults[ITEM_GSENSOR].strInfo[0] = 0;
			break;
		}
		else
		{
			gTestResults[ITEM_GSENSOR].result = 0;
			strcpy(gTestResults[ITEM_GSENSOR].strInfo, "I2C通讯失败");
		}
		sleep(1);
	}

	return NULL;
}

void GSensorTest_Start(void)
{
	pthread_t pid;

	pthread_create(&pid, 0, GSensorTestThread, NULL);
}

