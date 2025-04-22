#include "common_test.h"

void* MICTestThread(void* pArg)
{
	pthread_detach(pthread_self());

	remove("/tmp/rec.pcm");
	system("arecord -r 8000 -f S16_LE -c 1 -d 3 /tmp/rec.pcm");
	system("amixer cset numid=7 84");
	system("aplay -r 8000 -c 1 -f S16_LE /tmp/rec.pcm");

	return NULL;
}

void MICTest_Start(void)
{
	pthread_t pid;

	pthread_create(&pid, 0, MICTestThread, NULL);
}

