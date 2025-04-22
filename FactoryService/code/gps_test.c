
#include <termio.h>
#include "common_test.h"
#include "gpsReader.h"

gpsReader_t gGpsReader = { 0 };

static int uartOpen(char* name, int bdr)
{
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

    uart_cfg_opt.c_cc[VTIME] = 1;
    uart_cfg_opt.c_cc[VMIN] = 0;

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
}

int gpsUartOnEpollEvent(int fd, unsigned int event, void* arg)
{
    PNTEpoll_t* epoll = (PNTEpoll_t*)arg;
    gpsReader_t* gpsReader = (gpsReader_t*)epoll->mOwner;

    if (event & EPOLLIN)
    {
        for (;;)
        {
            ssize_t ret = read(fd, gpsReader->buffer + gpsReader->size, READBUFFER_SIZE - gpsReader->size - 1);
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
                break;
            }
            else
            {
                pthread_mutex_lock(&gpsReader->mutex);
                gpsReader->size += ret;
                if (gpsReader->size >= READBUFFER_SIZE)
                {
                    memset(gpsReader->buffer, 0, READBUFFER_SIZE);
                    gpsReader->size = 0;
                }
                pthread_mutex_unlock(&gpsReader->mutex);
            }
        }
    }

    return RET_SUCCESS;
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
		if (gpsLocation.valid != gpsReader->validFlag)
		{
			if (gpsLocation.valid)
			{
				gpsReader->validFlag = 1;
				setLedStatus(GPS_LED, 1);
        		gTestResults[ITEM_GPS].result = 1;
				gTestResults[ITEM_GPS].strInfo[0] = 0;
			}
			else
			{
				gpsReader->validFlag = 0;
				setLedStatus(GPS_LED, 0);
			}
		}

        gpsReader->abnormal = 0;
    }
    else
    {
        gpsReader->abnormal ++;
        if (0 == (gpsReader->abnormal % 20))
        {
            setLedStatus(GPS_LED, 0);

            if (gpsReader->abnormal == 20)
            {
	        	if (1 != getFlagStatus("restart"))
	        	{
	        		gTestResults[ITEM_GPS].result = 0;
					strcpy(gTestResults[ITEM_GPS].strInfo, "GPS模块异常");
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

void* GPSTestThread(void* arg)
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

    LibPnt_EpollCreate(&gGpsReader.epoll, 10, &gGpsReader);
    LibPnt_EpollSetTimeout(&gGpsReader.epoll, 100);
    LibPnt_EpollAddEvent(&gGpsReader.epoll, gGpsReader.fd, EPOLLIN | EPOLLET);
    LibPnt_EpollSetOnTimeout(&gGpsReader.epoll, gpsUartOnEpollTimeout);
    LibPnt_EpollSetOnEvent(&gGpsReader.epoll, gpsUartOnEpollEvent);
    LibPnt_EpollSetState(&gGpsReader.epoll, TRUE);

    while (gTestRunningFlag)
    {
        if (!gGpsReader.validFlag && gGpsReader.abnormal < 20)
        {
            led_status = !led_status;
            setLedStatus(GPS_LED, led_status);
	
			gTestResults[ITEM_GPS].result = 0;
			strcpy(gTestResults[ITEM_GPS].strInfo, "等待GPS模块定位");
        }
        SleepMs(1000);
    }

    return NULL;
}

void GPSTest_Start(void)
{
	pthread_t pid;

	pthread_create(&pid, 0, GPSTestThread, NULL);
}

