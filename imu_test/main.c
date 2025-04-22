#include <stdio.h>
#include <stdlib.h>
#include <libfdisk/libfdisk.h>

int main(int argc, char** argv)
{
#if 0
	inv_imu_start();
	usleep(500*1000);
	
	while (1)
	{
		imu_loop();
		
		usleep(10*1000);
	};
#else
	printf("start test.\n");

		alsa_playaudio("/etc/audio/starting.pcm", 8000, 1, "S16_LE", 0/* 1-block 0-nonblock */);
		
		alsa_playaudio("/etc/audio/no_tfcard.pcm", 8000, 1, "S16_LE", 0/* 1-block 0-nonblock */);
		
		alsa_playaudio("/etc/audio/start_format_tfcard.pcm", 8000, 1, "S16_LE", 0/* 1-block 0-nonblock */);
		
		alsa_playaudio("/etc/audio/start_update.pcm", 8000, 1, "S16_LE", 0/* 1-block 0-nonblock */);
	while (1)
	{
		alsa_playaudio("/etc/audio/wifi_start.pcm", 8000, 1, "S16_LE", 1/* 1-block 0-nonblock */);
		
		usleep(10*1000);
	}
#endif
	return 0;
}
