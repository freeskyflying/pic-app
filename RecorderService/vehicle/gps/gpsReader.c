#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <termio.h>
#include <pthread.h>

#include "pnt_epoll.h"
#include "pnt_log.h"
#include "utils.h"
#include "gpsReader.h"
#include "gpio_led.h"

gpsReader_t gGpsReader = { 0 };

static int uartOpen(char* name, int bdr)
{
#if 0
    int fd = -1;
    
    fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0)
    {
        printf("Open %s uart fail:%s ", name, strerror(errno));
        return -1;
    }

    struct termios uart_cfg_opt;

	if (tcgetattr(fd, &uart_cfg_opt) == -1)
    {
		printf("error cannot get input description\n");
		return -1;
	}

    uart_cfg_opt.c_cc[VTIME] = 0;
    uart_cfg_opt.c_cc[VMIN] = 1;

    uart_cfg_opt.c_cflag|=(CLOCAL|CREAD );

	uart_cfg_opt.c_cflag = B115200;

    uart_cfg_opt.c_cflag &= ~CSIZE;
    uart_cfg_opt.c_cflag |= CS8;

    uart_cfg_opt.c_cflag &= ~PARENB;

    uart_cfg_opt.c_cflag &= ~CSTOPB;

    tcflush(fd ,TCIFLUSH);

	int err = tcsetattr(fd, TCSANOW, &uart_cfg_opt);
	if(err == -1 || err == EINTR)
    {
		printf("fialed to set attr to fd\n");
		return -1;
	}

    return fd;
#else
    int fd = -1;
    
    fd = open(name, O_RDWR | O_NOCTTY | O_NDELAY);
    if(fd < 0)
    {
        printf("Open %s uart fail:%s ", name, strerror(errno));
        return -1;
    }
	
	struct termios uart_cfg_opt;
	speed_t speed;
	
	if (-1 == tcgetattr(fd, &uart_cfg_opt))
		return -1;
	//Set speed.
	tcflush(fd, TCIOFLUSH);
	speed = bdr==115200?B115200:B9600;
	if (CBAUDEX != speed)
	{
		cfsetospeed(&uart_cfg_opt, speed);
		cfsetispeed(&uart_cfg_opt, speed);
	}
	if (-1 == tcsetattr(fd, TCSANOW, &uart_cfg_opt))
		return -1;
	
	tcflush(fd, TCIOFLUSH);
	
	uart_cfg_opt.c_cc[VTIME] = 30;
	uart_cfg_opt.c_cc[VMIN] = 0;
	
	/* Data length setting section */
	uart_cfg_opt.c_cflag &= ~CSIZE;
	uart_cfg_opt.c_cflag |= CS8;
	uart_cfg_opt.c_iflag &= ~INPCK;
	uart_cfg_opt.c_cflag &= ~PARODD;
	uart_cfg_opt.c_cflag &= ~CSTOPB;
	
	/* Using raw data mode */
	uart_cfg_opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	uart_cfg_opt.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
	uart_cfg_opt.c_oflag &= ~(INLCR | IGNCR | ICRNL);
	uart_cfg_opt.c_oflag &= ~(ONLCR | OCRNL);
	
	/* Apply new settings */
	if (-1 == tcsetattr(fd, TCSANOW, &uart_cfg_opt))
		return -1;
	
	tcflush(fd, TCIOFLUSH);

    return fd;
#endif
}

static char buffer[1024] = { 0 };
int gpsUartOnEpollEvent(int fd, unsigned int event, void* arg)
{
    PNTEpoll_t* epoll = (PNTEpoll_t*)arg;
    gpsReader_t* gpsReader = (gpsReader_t*)epoll->mOwner;

    if (event & EPOLLIN)
    {
        for (;;)
        {
            ssize_t ret = read(fd, buffer, sizeof(buffer));
            if (ret == -1)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    break;
                }
                printf("Uart Client close. %s", strerror(errno));
                break;
            }
            else if (ret == 0)
            {
                //printf("Uart Client close. %s", strerror(errno));
                break;
            }
            else
            {
                pthread_mutex_lock(&gpsReader->mutex);
                if (gpsReader->size + ret >= READBUFFER_SIZE)
                {
                    memset(gpsReader->buffer, 0, READBUFFER_SIZE);
                    gpsReader->size = 0;
                }
	            memcpy(gpsReader->buffer + gpsReader->size, buffer, ret);
	            gpsReader->size += ret;
                pthread_mutex_unlock(&gpsReader->mutex);
            }
        }
    }

    return RET_SUCCESS;
}

extern volatile unsigned char gACCState;

void gpsLocationHandler(gpsReader_t* gpsReader, GpsLocation_t* gpsLocation)
{
	if (gpsReader->validFlag != gpsLocation->valid)
	{
		if (gpsLocation->valid)
		{
			setLedStatus(GPS_LED, 1);
		}
		else
		{
			setLedStatus(GPS_LED, 0);				 
		}
	}
	
	if(gpsLocation->valid && gpsLocation->satellites >= 4)//(连续3s卫星数量大于4颗)
	{
		if(gpsReader->validIndex++ >= 3)
		{
			gpsReader->validFlag = 1;
		}
		else
		{
			gpsLocation->valid = 0;
		}
	}
	else
	{
		gpsReader->validIndex = 0;
		gpsReader->validFlag = 0;
		gpsLocation->valid = 0;
		gpsLocation->speed = 0;
	}

	if (gpsReader->validFlag)
	{
		if(!gpsReader->hasLocation || gpsLocation->satellites - gpsReader->mLocation.satellites >= 3)
		{
			gpsReader->mLocation.latitude = gpsLocation->latitude;
			gpsReader->mLocation.longitude = gpsLocation->longitude;
			gpsReader->mLocation.satellites = gpsLocation->satellites;
            gpsReader->mLocation.timestamp = gpsLocation->timestamp;
			gpsReader->mLocation.valid = gpsLocation->valid;
			gpsReader->hasLocation = 1;
		}
		else
		{
			char gpsfly = 0;
			if(gpsLocation->speed < 30)
			{
				char runState = (gpsLocation->speed >= 4) && gACCState && gpsLocation->satellites > 8;
				
				if(gACCState == 0)//STOP STATE.
				{
					gpsfly = 1;
					gpsReader->mLocation.speed = 0;
				}
			}
		
			if(gpsReader->mLocation.latitude != 0 && gpsReader->mLocation.longitude != 0 && 
				computeDistanceAndBearing(gpsLocation->latitude, gpsLocation->longitude, gpsReader->mLocation.latitude, gpsReader->mLocation.longitude) > 1000.0f)
			{
				PNT_LOGE("GPS fly...[%.6f, %.6f][%.6f, %.6f]", gpsLocation->latitude, gpsLocation->longitude, gpsReader->mLocation.latitude, gpsReader->mLocation.longitude);
				gpsfly = 1;
			}
		
			if(!gpsfly ||(gpsLocation->timestamp - gpsReader->mLocation.timestamp > 60 && gpsLocation->satellites >= 8))
			{
				gpsReader->mLocation.longitude = gpsLocation->longitude;
				gpsReader->mLocation.latitude = gpsLocation->latitude;
				gpsReader->mLocation.satellites = gpsLocation->satellites;
                gpsReader->mLocation.timestamp = gpsLocation->timestamp;
				gpsReader->mLocation.speed = gpsLocation->speed;
			}
			else
			{
			#if 0
				gpsLocation->longitude = gpsReader->mLocation.longitude;
				gpsLocation->latitude = gpsReader->mLocation.latitude;
				gpsLocation->satellites = gpsReader->mLocation.satellites;
                gpsLocation->timestamp = gpsReader->mLocation.timestamp;
				gpsLocation->speed = gpsReader->mLocation.speed;
			#endif
			}
		}
	}
	else
	{
		gpsReader->mLocation.speed = 0;
		gpsReader->hasLocation = 0;
	}
	
    gpsReader->mLocation.altitude = gpsLocation->altitude;
    gpsReader->mLocation.antenna = gpsLocation->antenna;
    gpsReader->mLocation.bearing = gpsLocation->bearing;
    gpsReader->mLocation.lat = gpsLocation->lat;
    gpsReader->mLocation.lon = gpsLocation->lon;
    gpsReader->mLocation.quality = gpsLocation->quality;
    gpsReader->mLocation.satellites = gpsLocation->satellites;
    gpsReader->mLocation.pdop = gpsLocation->pdop;
    gpsReader->mLocation.vdop = gpsLocation->vdop;
    gpsReader->mLocation.hdop = gpsLocation->hdop;
    gpsReader->mLocation.locationtype = gpsLocation->locationtype;

	extern int gSimulateSpeed;
	if (0 != gSimulateSpeed)
	{
		gpsLocation->speed = gSimulateSpeed;
	}
	
	LibPnt_IPCSend(&gpsReader->ipcServer, (char*)gpsLocation, sizeof(GpsLocation_t), 0);
}

int gpsUartOnEpollTimeout(void* arg)
{
    PNTEpoll_t* epoll = (PNTEpoll_t*)arg;
    gpsReader_t* gpsReader = (gpsReader_t*)epoll->mOwner;
    GpsLocation_t gpsLocation = { 0 };

    if (gpsReader->size > 0)
    {
        pthread_mutex_lock(&gpsReader->mutex);

        memset(&gpsLocation, 0, sizeof(GpsLocation_t));

        char *saveptr = NULL;
        const char *p = strtok_r((char*)gpsReader->buffer, "\r\n",&saveptr);
        int index = 0;
        while (p != NULL) {
            uint8_t type = nmea_get_message_type(p);
            if (type == NMEA_GPGGA) {
                gpgga_t gpgga;
                memset(&gpgga, 0, sizeof(gpgga_t));
                nmea_parse_gpgga((char *) p, &gpgga);
                gpsLocation.quality = gpgga.quality;
                gpsLocation.latitude = gpgga.latitude;
                gpsLocation.lat = gpgga.lat;
                gpsLocation.longitude = gpgga.longitude;
                gpsLocation.lon = gpgga.lon;
                gpsLocation.satellites = gpgga.satellites;
                gpsLocation.altitude = gpgga.altitude;
                index++;
            } else if (type == NMEA_GPRMC) {
                gprmc_t gprmc;
                memset(&gprmc, 0, sizeof(gprmc_t));
                nmea_parse_gprmc((char *) p, &gprmc);
                gpsLocation.valid = gprmc.valid;
                gpsLocation.speed = gprmc.speed;
                gpsLocation.bearing = gprmc.course;
                gpsLocation.timestamp = timegm(&gprmc.time);
                index++;
            } if(type == NMEA_GPGSA||type == NMEA_GNGSA){
                gngsa_t gngsa = {0};
                nmea_parse_gpgsa((char*)p, &gngsa);
                gpsLocation.locationtype = gngsa.type;
                gpsLocation.pdop = gngsa.pdop;
                gpsLocation.hdop = gngsa.hdop;
                gpsLocation.vdop = gngsa.vdop;
                index++;
            }else if(type == NMEA_GPTXT){
                index++;
                uint8_t status = -1;
                nmea_parse_gntxt((char*)p, &status);
                gpsLocation.antenna = status;
            }
            if(p == NULL)
            {
                break;
            }
            p = strtok_r(NULL, "\r\n",&saveptr);
        }

        gpsLocation.latitude		= GPS_MINUTE_TO_DEC(gpsLocation.latitude);
        gpsLocation.longitude		= GPS_MINUTE_TO_DEC(gpsLocation.longitude);
        gpsLocation.speed			= GPS_SPEED_KNOT_TO_KM(gpsLocation.speed);

        gpsReader->size = 0;
        pthread_mutex_unlock(&gpsReader->mutex);
		
		if (gpsLocation.valid && 0 == gpsReader->timeCaliFlag)
		{
			struct timeval tv;
			tv.tv_sec = gpsLocation.timestamp;
			tv.tv_usec = 0;
			settimeofday(&tv, NULL);
			system("hwclock -w");
		
			gettimeofday(&tv, NULL);
			if (tv.tv_sec <= gpsLocation.timestamp+1 && tv.tv_sec >= gpsLocation.timestamp-1)
			{
				gpsReader->timeCaliFlag = 1;
			}
		}

		gpsLocationHandler(gpsReader, &gpsLocation);

        gpsReader->abnormal = 0;
    }
    else
    {
        gpsReader->abnormal ++;
        if (0 == (gpsReader->abnormal % 50))
        {
            setLedStatus(GPS_LED, 0);

            extern int gSimulateSpeed;
            if (0 != gSimulateSpeed)
            {
                gpsLocation.speed = gSimulateSpeed;
            }

            LibPnt_IPCSend(&gpsReader->ipcServer, (char*)&gpsLocation, sizeof(gpsLocation), 0);
            if (gpsReader->abnormal == 50)
            {
	        	if (1 != getFlagStatus("restart"))
	        	{
               	 	playAudio("gps_abnormal.pcm");
	        	}
			
				setLedStatus("/sys/class/leds/gpspwr", 0);
				sleep(1);
				setLedStatus("/sys/class/leds/gpspwr", 1);
            }
        }
    }

    return RET_SUCCESS;
}

void* gpsReaderThread(void* arg)
{
    int led_status = 0;
    int fd = -1;
	gGpsReader.fd = -1;
	
open_uart:
    fd = uartOpen("/dev/ttySLB1", 115200);
    if (fd < 0)
    {
        printf("open /dev/ttySLB1 failed.\n");
        SleepMs(1000);
        goto open_uart;
    }

    gGpsReader.fd = fd;
    gGpsReader.size = 0;

    pthread_mutex_init(&gGpsReader.mutex, NULL);

    LibPnt_IPCCreate(&gGpsReader.ipcServer, PNT_IPC_GPS_NAME, NULL, NULL, &gGpsReader);

    LibPnt_EpollCreate(&gGpsReader.epoll, 10, &gGpsReader);
    LibPnt_EpollSetTimeout(&gGpsReader.epoll, 100);
    LibPnt_EpollAddEvent(&gGpsReader.epoll, gGpsReader.fd, EPOLLIN | EPOLLET);
    LibPnt_EpollSetOnTimeout(&gGpsReader.epoll, gpsUartOnEpollTimeout);
   	LibPnt_EpollSetOnEvent(&gGpsReader.epoll, gpsUartOnEpollEvent);
    LibPnt_EpollSetState(&gGpsReader.epoll, TRUE);

    while (1)
    {
        if (!gGpsReader.validFlag && gGpsReader.abnormal < 50)
        {
            led_status = !led_status;
            setLedStatus(GPS_LED, led_status);
        }
        SleepMs(1000);
    }

    return NULL;
}

int gpsReaderStart(void)
{
    pthread_t pid;

    memset(&gGpsReader, 0, sizeof(gGpsReader));
	
	gGpsReader.fd = -1;

    int ret = pthread_create(&pid, 0, gpsReaderThread, NULL);
    if (ret)
    {
        printf("errno=%d, reason(%s)\n", errno, strerror(errno));
    }

    extern int gSimulateSpeed;
	int fd = open("/data/simSpeed", O_WRONLY | O_CREAT);
	if (0 < fd)
	{
		read(fd, &gSimulateSpeed, sizeof(gSimulateSpeed));
		close(fd);
        remove("/data/simSpeed");
	}

    return 0;
}

void gpsReaderStop(void)
{
	LibPnt_IPCServerRelease(&gGpsReader.ipcServer);
}

