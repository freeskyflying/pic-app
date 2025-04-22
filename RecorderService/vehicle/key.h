#ifndef _KEY_H_
#define _KEY_H_

#include <linux/input.h>

typedef int(*key_event_handler)(struct input_event* dev);

 typedef struct
 {

	unsigned char keyCode;
	key_event_handler cb;
	void* next;
 
 } KeyHandle_t;

typedef struct
{

	int fd;
	KeyHandle_t* handles;

} GpioKey_t;

int KeyHandle_Register(key_event_handler cb, unsigned char keycode);

void GpioKey_Init(void);

#endif

