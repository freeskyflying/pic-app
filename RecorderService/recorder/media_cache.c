#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <errno.h>
#include <sys/prctl.h>
#include <linux/falloc.h>

#include "sample_comm.h"
#include "typedef.h"
#include "pnt_log.h"
#include "queue_list.h"
#include "utils.h"

#include "media_audio.h"
#include "media_video.h"
#include "media_cache.h"

#include "mp4v2_muxer.h"

void MediaCache_MakeVideoFile(MediaCacheHandle_t* handle, int duration, char* filename)
{
	char cachefile[128] = { 0 };

	MediaVideoStream_t* streamHandle = (MediaVideoStream_t*)handle->stream;

	sprintf(cachefile, CACHE_VIDEO_FORMAT, handle->chnNum+1);

	int fd = handle->writerVideo.cacheFd;
	if (fd < 0)
	{
		PNT_LOGE("make cache video %s error, %s not exist.", filename, cachefile);
		return;
	}

	MediaStreamWriter_t* cacheWriter = &handle->writerVideo;

	int start_sec_pos = 0;

	duration = (duration+streamHandle->param.minutePerFile-1)/streamHandle->param.minutePerFile*streamHandle->param.minutePerFile;

	if (cacheWriter->curSecond > duration/streamHandle->param.minutePerFile)
	{
		if (cacheWriter->curSecond%CHCHE_FRAME_SECOND >= duration/streamHandle->param.minutePerFile)
		{
			start_sec_pos = cacheWriter->curSecond%CHCHE_FRAME_SECOND - duration/streamHandle->param.minutePerFile;
		}
		else
		{
			start_sec_pos = (cacheWriter->curSecond%CHCHE_FRAME_SECOND + CHCHE_FRAME_SECOND) - duration/streamHandle->param.minutePerFile;
		}
	}
	else
	{
		duration = cacheWriter->curSecond*streamHandle->param.minutePerFile;
	}
	if (duration <= 2)
	{
		return;
	}

	PNT_LOGI("%s start_sec_pos:%d cacheSeconds:%d duration:%d.", filename, start_sec_pos, cacheWriter->curSecond, duration);

	mp4v2_muxer_t muxer = { 0 };
	
	if (RET_FAILED == mp4v2_muxer_create(&muxer, filename))
	{
		return;
	}

    CacheIndex_t* cacheIndex = NULL;
	unsigned char* readBuffer = (unsigned char*)malloc(CACHEVIDEO_CACHE_SIZE);
	if (NULL == readBuffer)
	{
    	PNT_LOGE("readBuffer malloc failed.");
		mp4v2_muxer_close(&muxer);
		return;
	}

	int sec = 0, dataLen = 0, idrOffset = 0, curSec = 0;
	long long start_pts = 0;
	unsigned char* data = NULL;
	
	while (sec < duration/streamHandle->param.minutePerFile)
	{
		curSec = (sec+start_sec_pos)%CHCHE_FRAME_SECOND;
		
		pread(fd, readBuffer, CACHEVIDEO_CACHE_SIZE, curSec * CACHEVIDEO_CACHE_SIZE);

		for (int idx = 0; idx<SDR_VENC_FRAMERATE_MAJOR; idx ++)
		{
			cacheIndex = &handle->writerVideo.indexCache[curSec*SDR_VENC_FRAMERATE_MAJOR + idx];
			
			data = readBuffer + cacheIndex->OFFSET;

		    if (cacheIndex->frameType == DATA_TYPE_I_FRAME || cacheIndex->frameType == DATA_TYPE_INITPACKET)
			{
				if (0 == idrOffset)
				{
					int vpsOffset = 0, vpsLen = 0, spsOffset = 0, ppsOffset = 0, spsLen = 0, ppsLen = 0, i = 0;

					i = 0;
					while (i<cacheIndex->frameSize)
					{
#if (defined VIDEO_PT_H265)
						if (0x00 == data[i] && 0x00 == data[i+1] && 0x00 == data[i+2] && 0x01 == data[i+3])
						{
							if (0x40 == data[i+4]) // vps
							{
								vpsOffset = i+4;
							}
							else if (0x42 == data[i+4]) // sps
							{
								spsOffset = i+4;
								vpsLen = i-vpsOffset;
							}
							else if (0x44 == data[i+4]) // pps
							{
								ppsOffset = i+4;
								spsLen = i-spsOffset;
							}
							else if (0x26 == data[i+4])
							{
								idrOffset = i;
								ppsLen = i-ppsOffset;
								break;
							}
						}
#else
						if (0x00 == data[i] && 0x00 == data[i+1] && 0x00 == data[i+2] && 0x01 == data[i+3])
						{
							if (0x67 == data[i+4])
							{
								spsOffset = i+4;
							}
							else if (0x68 == data[i+4])
							{
								ppsOffset = i+4;
								spsLen = i-spsOffset;
							}
							else if (0x65 == data[i+4])
							{
								idrOffset = i;
								ppsLen = i-ppsOffset;
								break;
							}
						}
#endif

						i++;
					}
					
#if (defined VIDEO_PT_H265)
					mp4v2_muxer_write265meta(&muxer, SDR_VENC_FRAMERATE_MAJOR, 
												streamHandle->param.width, streamHandle->param.height, 
												data+vpsOffset, vpsLen,
												data+spsOffset, spsLen, 
												data+ppsOffset, ppsLen);
#else
					mp4v2_muxer_write264meta(&muxer, SDR_VENC_FRAMERATE_MAJOR, 
												streamHandle->param.width, streamHandle->param.height, 
												data+spsOffset, spsLen, data+ppsOffset, ppsLen);
#endif
				}

				data = readBuffer + cacheIndex->OFFSET + idrOffset;
				dataLen = cacheIndex->frameSize - idrOffset;
		    }
		    else
		    {
				dataLen = cacheIndex->frameSize;
		    }
			
#if (defined VIDEO_PT_H265)
			mp4v2_muxer_write265data(&muxer, data, dataLen);
#else
			mp4v2_muxer_write264data(&muxer, data, dataLen);
#endif
		}
		
		sec ++;
	}

	if (SDR_VENC_FRAMERATE_MAJOR == streamHandle->param.framerate)
	{
		sprintf(cachefile, CACHE_AUDIO_FORMAT, handle->chnNum+1);
		fd = handle->writerAudio.cacheFd;
		if (fd >= 0)
		{
			mp4v2_muxer_writeAACmeta(&muxer, MEDIA_AUDIO_RATE, 1024);

			mp4v2_muxer_setAACconfigs(&muxer, 2/*AACLC*/, 11/*8K*/, 1);

			sec = 0;
			while (sec < duration)
			{
				curSec = (sec+start_sec_pos)%CHCHE_FRAME_SECOND;
				
				pread(fd, readBuffer, CACHEAUDIO_CACHE_SIZE, curSec * CACHEAUDIO_CACHE_SIZE);
				
				cacheIndex = &handle->writerAudio.indexCache[curSec];

				int offset = 0;
				while (1)
				{
					mp4v2_muxer_writeAACdata(&muxer, readBuffer+offset, 761);
					offset += 761;
					if (offset > cacheIndex->frameSize)
					{
						break;
					}
				}

				sec ++;
			}
		}
	}

	mp4v2_muxer_close(&muxer);

	free(readBuffer);

	PNT_LOGE("%s make success.", filename);
}

void* MediaCache_DataCacheTask(void* pArg)
{
	MediaCacheHandle_t* handle = (MediaCacheHandle_t*)pArg;
	MediaVideoStream_t* streamHandle = (MediaVideoStream_t*)handle->stream;

    CacheIndex_t* cacheIndex = NULL;
	MediaStreamWriter_t* cacheWriter = NULL;
	
    char thread_name[16];
    thread_name[15] = 0;
    snprintf(thread_name, 16, "streamcache%d", handle->chnNum);
    prctl(PR_SET_NAME, thread_name);

	char filename[64] = { 0 };

	handle->writerVideo.cacheFd = -1;
	handle->writerAudio.cacheFd = -1;
	handle->writerVideo.curSecond = 0;

	FrameInfo_t frameH264 = { 0 };
	FrameInfo_t frameAudio = { 0 };

	int curIdx = 0, wait = 2;

	while (streamHandle->bThreadStart)
	{
		if (REC_STOP == streamHandle->recStatus)
		{
			handle->writerVideo.curSecond = 0;
			usleep(1000/SDR_VENC_FRAMERATE_MAJOR*1000);
			continue;
		}

		wait = 2;
		cacheWriter = &handle->writerVideo;
		if (0 == get_video_stream(handle->chnNum, &frameH264))
		{
	        if (frameH264.frameType == DATA_TYPE_I_FRAME && cacheWriter->cacheFd >= 0)
	        {
				pwrite(cacheWriter->cacheFd, cacheWriter->dataCache, cacheWriter->dataSize, CACHEVIDEO_CACHE_SIZE*(cacheWriter->curSecond%CHCHE_FRAME_SECOND));
				cacheWriter->dataSize = 0;
				cacheWriter->curSecond ++;
				curIdx = 0;
				memset(cacheWriter->dataCache, 0, CACHEVIDEO_CACHE_SIZE);
				if (cacheWriter->curSecond%CHCHE_FRAME_SECOND == 0)
				{
					//close(cacheWriter->cacheFd);
					//cacheWriter->cacheFd = -1;
				}
				
		        if (handle->writerAudio.cacheFd >= 0)
		        {
					pwrite(handle->writerAudio.cacheFd, handle->writerAudio.dataCache, handle->writerAudio.dataSize, CACHEAUDIO_CACHE_SIZE*(cacheWriter->curSecond%CHCHE_FRAME_SECOND));
					handle->writerAudio.dataSize = 0;
					memset(handle->writerAudio.dataCache, 0, CACHEAUDIO_CACHE_SIZE);
					memset(&handle->writerAudio.indexCache[(cacheWriter->curSecond%CHCHE_FRAME_SECOND)], 0, sizeof(CacheIndex_t));
					if (cacheWriter->curSecond%CHCHE_FRAME_SECOND == 0)
					{
						//close(handle->writerAudio.cacheFd);
						//handle->writerAudio.cacheFd = -1;
					}
		        }
	        }

	        if ((frameH264.frameType == DATA_TYPE_I_FRAME || frameH264.frameType == DATA_TYPE_INITPACKET) && cacheWriter->cacheFd < 0)
	        {
            	sprintf(filename, CACHE_VIDEO_FORMAT, handle->chnNum+1);
            	cacheWriter->cacheFd = open(filename, O_RDWR | O_CREAT);
	        }

			if (REC_STOP != streamHandle->recStatus && cacheWriter->cacheFd >= 0 && curIdx < SDR_VENC_FRAMERATE_MAJOR)
			{
				if (cacheWriter->dataSize + frameH264.frameSize <= CACHEVIDEO_CACHE_SIZE)
				{
					cacheIndex = &cacheWriter->indexCache[(cacheWriter->curSecond%CHCHE_FRAME_SECOND)*SDR_VENC_FRAMERATE_MAJOR + curIdx];
					cacheIndex->frameSize = frameH264.frameSize;
					cacheIndex->frameType = frameH264.frameType;
					cacheIndex->OFFSET = cacheWriter->dataSize;
					
					memcpy(cacheWriter->dataCache+cacheWriter->dataSize, frameH264.frameBuffer, frameH264.frameSize);
					cacheWriter->dataSize += frameH264.frameSize;
				}
				
				curIdx ++;
			}

			wait --;
		}

		if (0 == get_audio_stream(handle->chnNum, &frameAudio))
		{
			if (handle->writerAudio.cacheFd < 0)
			{
            	sprintf(filename, CACHE_AUDIO_FORMAT, handle->chnNum+1);
            	handle->writerAudio.cacheFd = open(filename, O_RDWR | O_CREAT);
			}

			if (handle->writerAudio.cacheFd >= 0)
			{
				if (handle->writerAudio.dataSize + frameAudio.frameSize <= CACHEAUDIO_CACHE_SIZE)
				{
					cacheIndex = &handle->writerAudio.indexCache[(cacheWriter->curSecond%CHCHE_FRAME_SECOND)];

					if (frameAudio.frameType)
					{
						if (frameAudio.frameSize-7 > 0)
						{
							memcpy(handle->writerAudio.dataCache+handle->writerAudio.dataSize, frameAudio.frameBuffer+7, frameAudio.frameSize-7);
							handle->writerAudio.dataSize += (frameAudio.frameSize-7);
						}
					}
					else
					{
						memcpy(handle->writerAudio.dataCache+handle->writerAudio.dataSize, frameAudio.frameBuffer, frameAudio.frameSize);
						handle->writerAudio.dataSize += frameAudio.frameSize;
					}
					
					cacheIndex->frameSize = handle->writerAudio.dataSize;
				}
			}
			
			wait --;
		}

		if (wait < 2)
		{
			usleep(20*1000);
		}
	}

	if (frameAudio.frameBuffer)
		free(frameAudio.frameBuffer);
	if (frameH264.frameBuffer)
		free(frameH264.frameBuffer);

	return NULL;
}

void* MediaCache_VideoProc(void* pArg)
{
	MediaCacheHandle_t* handle = (MediaCacheHandle_t*)pArg;
	MediaVideoStream_t* streamHandle = (MediaVideoStream_t*)handle->stream;
	
    char thread_name[16];
    thread_name[15] = 0;
    snprintf(thread_name, 16, "videocache%d", handle->chnNum);
    prctl(PR_SET_NAME, thread_name);

	while (streamHandle->bThreadStart)
	{
		CacheVideoReq_t* req = (CacheVideoReq_t*)queue_list_popup(&handle->videoReqs);
		if (NULL == req)
		{
			usleep(100*1000);
			continue;
		}

		if (req->startTime > currentTimeMillis()/1000)
		{
			queue_list_push(&handle->videoReqs, req);
			usleep(100*1000);
			continue;
		}

		MediaCache_MakeVideoFile(handle, req->beforeTime+req->afterTime, req->filename);

		free(req);
	}

	return NULL;
}

int MediaCache_VideoCreate(unsigned char before_time, unsigned char after_time, int chnNum, char* filename)
{
	if (RET_FAILED == checkTFCard(NULL))
	{
		PNT_LOGE("create %s failed, sdcard not prepare.");
		return RET_FAILED;
	}

	CacheVideoReq_t* req = (CacheVideoReq_t*)malloc(sizeof(CacheVideoReq_t));
	if (NULL == req)
	{
		PNT_LOGE("create %s failed, no buffer", filename);
		return RET_FAILED;
	}
	
	MediaCacheHandle_t* handle = (MediaCacheHandle_t*)&gMediaStream[chnNum].cache;
	
	if ((after_time+before_time) >= CHCHE_FRAME_SECOND)
	{
		after_time = before_time = (CHCHE_FRAME_SECOND-2)/2;
	}
	req->afterTime = after_time;
	req->beforeTime = before_time;
	strcpy(req->filename, filename);
	req->startTime = after_time + currentTimeMillis()/1000;

	if (queue_list_push(&handle->videoReqs, req))
	{
		PNT_LOGE("create %s failed, no buffer", filename);
		free(req);
		return RET_FAILED;
	}

	return RET_SUCCESS;
}

void MediaCache_FileInit(void)
{
	char filename[128] = { 0 };

	for (int i=0; i<MAX_VIDEO_NUM; i++)
	{
		MediaVideoStream_t* handle = &gMediaStream[i];
		
		if (handle->param.needCache)
		{
			sprintf(filename, CACHE_VIDEO_FORMAT, i+1);
			
			do_fallocate(filename, CACHEVIDEO_CACHE_SIZE*CHCHE_FRAME_SECOND + CACHEVIDEO_CACHE_SIZE);
		}
	}
	
	sprintf(filename, CACHE_AUDIO_FORMAT, AUDIO_BIND_CHN+1);
	do_fallocate(filename, CACHEAUDIO_CACHE_SIZE*CHCHE_FRAME_SECOND);
}

void MediaCache_Create(MediaCacheHandle_t* handle, int chnNum, void* onwer)
{
	memset(handle, 0, sizeof(MediaCacheHandle_t));

	handle->chnNum = chnNum;
	handle->stream = onwer;

	handle->writerVideo.dataCache = (char*)malloc(CACHEVIDEO_CACHE_SIZE);
	handle->writerVideo.indexCache = (CacheIndex_t*)malloc(CACHE_FRAME_COUNT*sizeof(CacheIndex_t));
	memset(handle->writerVideo.indexCache, 0, CACHE_FRAME_COUNT*sizeof(CacheIndex_t));
	
	queue_list_create(&handle->videoReqs, 10);

	if (chnNum == AUDIO_BIND_CHN)
	{
		handle->writerAudio.dataCache = (char*)malloc(CACHEAUDIO_CACHE_SIZE);
		handle->writerAudio.indexCache = (CacheIndex_t*)malloc(CHCHE_FRAME_SECOND*sizeof(CacheIndex_t));
		memset(handle->writerAudio.indexCache, 0, CHCHE_FRAME_SECOND*sizeof(CacheIndex_t));
	}

	pthread_create(&handle->pid, 0, MediaCache_DataCacheTask, (EI_VOID *)handle);

	pthread_t pid;
	pthread_create(&pid, 0, MediaCache_VideoProc, (EI_VOID *)handle);
}

