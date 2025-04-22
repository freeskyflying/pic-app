#include "common_test.h"
#include <sys/mman.h>
#include <fcntl.h>
#include "adc.h"

void* SOSTestThread(void* pArg)
{
	pthread_detach(pthread_self());

	int keystate = 0;
	
	strcpy(gTestResults[ITEM_SOS].strInfo, "等待SOS键按下再松开");

	sleep(5);
	
	int fd = 0;
	Adc_Result_t param = { 0 };
remmap:

	fd = open("/tmp/adcValue", O_RDWR | O_CREAT);
	if (fd < 0)
	{
		PNT_LOGE("create /data/adcValue failed.\n");
		sleep(2);
		goto remmap;
	}
	
	write(fd, &param, sizeof(Adc_Result_t));
	lseek(fd, 0, SEEK_SET);

	gAdcResult = mmap(NULL, sizeof(Adc_Result_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (MAP_FAILED == gAdcResult)
	{
		PNT_LOGE("mmap /data/adcValue failed.\n");
		sleep(2);
		goto remmap;
	}

	while (gTestRunningFlag)
	{
		sprintf(gTestResults[ITEM_PWRADC].strInfo, "%d.%d V", gAdcResult->powerVolt/1000, gAdcResult->powerVolt%1000);
		
		if (keystate != gAdcResult->sosKey)
		{
			if (0 == gAdcResult->sosKey)
			{
				gTestResults[ITEM_SOS].result = 1;
				gTestResults[ITEM_SOS].strInfo[0] = 0;
			}
			keystate = gAdcResult->sosKey;
		}
		
		usleep(100*1000);
	}

	return NULL;
}

void SOSTest_Start(void)
{
	pthread_t pid;

	pthread_create(&pid, 0, SOSTestThread, NULL);
}

