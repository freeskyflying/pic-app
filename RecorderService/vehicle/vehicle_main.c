#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <strings.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <pthread.h>

#include "gpsReader.h"
#include "utils.h"
#include "rild.h"
#include "gpio_led.h"
#include "adc.h"
#include "pnt_log.h"
#include "wifi.h"

extern BoardSysInfo_t gBoardInfo;
volatile unsigned char gSOSKeyState = 0;

void* sosDetectTask(void* pArg)
{
	pthread_detach(pthread_self());

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

	while (1)
	{
		gSOSKeyState = gAdcResult->sosKey;
		if (gSOSKeyState)
		{
			sleep(15);
		}
		else
		{
			usleep(50*1000);
		}
	}

	return NULL;
}

int vehicle_main()
{
    setLedStatus(NET4G_LED, 0);
    setLedStatus(GPS_LED, 0);
    setLedStatus(REC_LED1, 0);
    setLedStatus(REC_LED2, 0);

	net4GReboot();

	pthread_t pid;
	pthread_create(&pid, NULL, sosDetectTask, NULL);

	gpsReaderStart();
    net4gRildStart();
    wifiStart();

	return 0;
}
