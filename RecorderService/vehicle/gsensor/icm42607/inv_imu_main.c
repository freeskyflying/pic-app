#include <pthread.h>
#include <stdio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <math.h>


#include "imu/inv_imu_driver.h"
#include "Invn/EmbUtils/Message.h"

#include "pnt_log.h"

#define ICM_I2C_ADDR 0x68

typedef void(*imu_callback_f)(inv_imu_sensor_event_t *event);
int setup_imu_device(struct inv_imu_serif *icm_serif, void(*callback)(inv_imu_sensor_event_t *event));
int configure_imu_device();
int get_imu_data(void);

static imu_callback_f imu_data_callback = NULL;

void inv_msg_logout(int level, const char * str, va_list ap)
{
	PNT_LOGI(str, ap);
}

int inv_imu_get_context(void)
{
	int fd = 0;
	fd = open("/dev/i2c-1", O_RDWR);
	if (fd < 0)
	{
		PNT_LOGE("open /dev/i2c-1 failed.\n");
	}
	return fd;
}

int inv_imu_release_context(struct inv_imu_serif *serif)
{
	if (serif->context > 0)
	{
		close(serif->context);
	}
	return 0;
}

int inv_imu_read(struct inv_imu_serif *serif, uint8_t reg, uint8_t *buf, uint32_t len)
{
	unsigned char *inbuf, *buf_write;
	struct i2c_rdwr_ioctl_data packets;
	struct i2c_msg messages[2];
 
	/* messages[0] : write step */
	messages[0].addr  = ICM_I2C_ADDR;
	messages[0].flags = 0;
	messages[0].len   = 1;
	messages[0].buf   = &reg;
 
	/* messages[1] : read step */
	messages[1].addr  = ICM_I2C_ADDR;
	messages[1].flags = I2C_M_RD/* | I2C_M_NOSTART*/;
	messages[1].len   = len;
	messages[1].buf   = buf;
 
	packets.msgs      = messages;
	packets.nmsgs     = 2;
	if(ioctl(serif->context, I2C_RDWR, &packets) < 0) {
		perror("Unable to send data");
		return 1;
	}
 
	return 0;
}

int inv_imu_write(struct inv_imu_serif *serif, uint8_t reg, const uint8_t *buf, uint32_t len)
{
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

	uint8_t sdbuf[512] = { 0 };
	sdbuf[0] = reg;
	memcpy(sdbuf+1, buf, len);
 
    messages[0].addr  = ICM_I2C_ADDR;
    messages[0].flags = 0;
    messages[0].len   = len+1;
    messages[0].buf   = sdbuf;
 
    /* Transfer the i2c packets to the kernel and verify it worked */
    packets.msgs  = messages;
    packets.nmsgs = 1;
    if(ioctl(serif->context, I2C_RDWR, &packets) < 0) {
    	perror("Unable to send data");
    	return 1;
    }
 
    return 0;
}

struct inv_imu_serif icm_serif;
void* inv_imu_pthread(void* parg)
{
	INV_MSG_SETUP(INV_MSG_LEVEL_DEBUG, inv_msg_logout);
	icm_serif.context    = inv_imu_get_context(); /* no need */
	icm_serif.read_reg   = inv_imu_read;
	icm_serif.write_reg  = inv_imu_write;
	icm_serif.max_read   = 1024 * 32; /* maximum number of bytes allowed per serial read */
	icm_serif.max_write  = 1024 * 32; /* maximum number of bytes allowed per serial write */
	
	setup_imu_device(&icm_serif, imu_data_callback);
	configure_imu_device();

	while (1)
	{
		int ret = get_imu_data();
		if (0 != ret)
			PNT_LOGE("get_imu_data ret: %d", ret);
		usleep(1000*1000);
	}

	inv_imu_release_context(&icm_serif);

	return NULL;
}

void inv_imu_start(imu_callback_f callback)
{
	pthread_t pid;

	imu_data_callback = callback;

	pthread_create(&pid, NULL, inv_imu_pthread, NULL);
}

