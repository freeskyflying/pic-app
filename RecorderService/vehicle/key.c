#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <pthread.h>

#include "key.h"
#include "utils.h"
#include "pnt_log.h"

GpioKey_t gGpioKey = { 0 };

#define GPIO_DET_KEYS		2

int GPIOKeyDetectInit(int gpioIdx, int out_in/*0-out 1-in*/)
{
	char cmd[128] = { 0 };

	sprintf(cmd, "echo %d > /sys/class/gpio/export", gpioIdx);
	system(cmd);

	sprintf(cmd, "echo %s > /sys/class/gpio/gpio%d/direction", (out_in)?"in":"out", gpioIdx);
	system(cmd);

	return 0;
}

int GPIOKeyReadGpioState(int gpioIdx)
{
	char path[128] = { 0 };
	
	sprintf(path, "/sys/class/gpio/gpio%d/value", gpioIdx);

	FILE* pf = fopen(path, "rb");
	if (pf == NULL)
	{
		return -1;
	}

	char value[2] = { 0 };
	fread(value, 1, 1, pf);
	fclose(pf);

	pf = NULL;

	return atoi(value);
}

extern BoardSysInfo_t gBoardInfo;
void* KeyDetectListen(void* pArg)
{
	pthread_detach(pthread_self());

	GpioKey_t* key = (GpioKey_t*)pArg;
	struct input_event dev;

	int gpios[GPIO_DET_KEYS] = {83, 116}; // wifi key gpiof12 83 V1.2
	
	const int gpios_valid_state[GPIO_DET_KEYS] = {3, 3};
	const int gpios_key_code[GPIO_DET_KEYS] = {103, 105};
	int gpios_last_state[GPIO_DET_KEYS] = { -1, -1 };

	sleep(2);

	for (int i=0; i<GPIO_DET_KEYS; i++)
	{
		GPIOKeyDetectInit(gpios[i], 1);
	}

	while (1)
	{
		for (int i=0; i<GPIO_DET_KEYS; i++)
		{
			int state = GPIOKeyReadGpioState(gpios[i]);
			if (-1 == state)
			{
				usleep(100*1000);
				continue;
			}
			if (state != gpios_last_state[i])
			{
				usleep(50*1000);
				state = GPIOKeyReadGpioState(gpios[i]);
				
				if (state != gpios_last_state[i])
				{
					if (gpios_valid_state[i] & (1<<state))
					{
						dev.code = gpios_key_code[i];
						dev.type = 1;
						dev.value = state;
						
						KeyHandle_t* node = key->handles;
						while (node)
						{
							if (node->keyCode == dev.code)
							{
								node->cb(&dev);
							}
							node = node->next;
						}
					}
					gpios_last_state[i] = state;
				}
			}
		}
		
		usleep(50*1000);
	}
	
	return NULL;
}

int KeyHandle_Register(key_event_handler cb, unsigned char keycode)
{
	KeyHandle_t* handle = (KeyHandle_t*)malloc(sizeof(KeyHandle_t));
	if (NULL == handle)
	{
		PNT_LOGE("gpiokey [%d] register failed.", keycode);
		return RET_FAILED;
	}

	GpioKey_t* key = &gGpioKey;

	handle->keyCode = keycode;
	handle->cb = cb;
	handle->next = NULL;

	KeyHandle_t* node = key->handles;

	while (node)
	{
		if (NULL == node->next)
		{
			break;
		}
		node = node->next;
	}

	if (NULL == node)
	{
		key->handles = handle;
	}
	else
	{
		node->next = handle;
	}

	return RET_SUCCESS;
}

void GpioKey_Init(void)
{
	GpioKey_t* key = &gGpioKey;

	memset(key, 0, sizeof(GpioKey_t));

	pthread_t pid;
	
	pthread_create(&pid, NULL, KeyDetectListen, key);
}

