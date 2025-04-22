#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "pnt_log.h"
#include "adc.h"
#include "utils.h"

Adc_Result_t * gAdcResult;
extern BoardSysInfo_t gBoardInfo;

int getAdcValue(unsigned int channel)
{
	char valueStr[5] = { 0 };
	
	FILE* pf = fopen("/sys/class/gpadc/ctl_channel", "w");
	if (pf == NULL)
	{
		PNT_LOGE("open /sys/class/gpadc/ctl_channel failed.");
		return -1;
	}
	
	char chn[2] = { 0 };
	chn[0] = channel+'0';
	
	fwrite(chn, 1, sizeof(chn), pf);
	
	fclose(pf);
	
	pf = fopen("/sys/class/gpadc/ctl_channel", "r");
	if (pf == NULL)
	{
		PNT_LOGE("open /sys/class/gpadc/ctl_channel failed.");
		return -1;
	}

	fread(valueStr, 1, sizeof(valueStr), pf);
	
	fclose(pf);

	return atoi(valueStr);
}

#define ADC_REF_VOLT		1350
#define ACT_VOLT_RATIO		(11/(300+11))
void* adcDetectTask(void* pArg)
{
	pthread_detach(pthread_self());
	
	int sos_key_chn = 1, power_chn = -1;
	
	int adcValue = 0;
	
	if (0 == strcmp(gBoardInfo.mProductModel, "P-DC-2") || 0 == strcmp(gBoardInfo.mProductModel, "P-DC-3"))
	{
		sos_key_chn = 2;
		power_chn = 1;
	}

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

	int size = lseek(fd, 0, SEEK_END);
	if (size != sizeof(Adc_Result_t))
	{
		write(fd, &param, sizeof(Adc_Result_t));
		lseek(fd, 0, SEEK_SET);
	}

	gAdcResult = mmap(NULL, sizeof(Adc_Result_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);
	if (MAP_FAILED == gAdcResult)
	{
		PNT_LOGE("mmap /data/adcValue failed.\n");
		sleep(2);
		goto remmap;
	}
	
	while (1)
	{
		if (power_chn > 0)
		{
			adcValue = getAdcValue(power_chn);
			if (adcValue >= 0)
			{
				gAdcResult->powerVolt = ADC_REF_VOLT*adcValue*(300+11)/11/4096;
			}
		}
		else
		{
			gAdcResult->powerVolt = 24000;
		}
	
		usleep(50*1000);
		adcValue = getAdcValue(sos_key_chn);
		if (adcValue > 0)
		{
			if (adcValue < 1000) // SOS KEY
			{
				gAdcResult->sosKey = 1;
			}
			else if (adcValue > 3000)
			{
				gAdcResult->sosKey = 0;
			}
		}
	
		usleep(50*1000);
	}
	
	return NULL;
}

void adcDetectStart(void)
{
	pthread_t pid;
	pthread_create(&pid, NULL, adcDetectTask, NULL);
}

