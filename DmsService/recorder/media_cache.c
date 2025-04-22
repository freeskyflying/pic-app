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
#include "media_cache.h"

#include "mp4v2_muxer.h"

MediaCacheHandle_t gMediaCacheHandler[MAX_VIDEO_NUM] = { 0 };

static void FrameListContentFree(void* content)
{
	FrameInfo_t* frame = (FrameInfo_t*)content;

	if (NULL == frame)
	{
		return;
	}

	if (frame->frameBuffer)
		free(frame->frameBuffer);
	frame->frameBuffer = NULL;
	free(frame);
	content = NULL;
}

void MediaCache_InsertH264Frame(int channel, unsigned char* data, int len, unsigned char* data2, int len2, int type)
{
	MediaCacheHandle_t* handle = (MediaCacheHandle_t*)&gMediaCacheHandler[channel];

	if (!handle->frameH264.m_init)
	{
		return;
	}

	FrameInfo_t* frame = (FrameInfo_t*)malloc(sizeof(FrameInfo_t));
	frame->frameSize = len + len2;
	frame->timeStamp = currentTimeMillis();
	frame->frameType = type;
	frame->frameBuffer = (char*)malloc(frame->frameSize);
	if (NULL == frame->frameBuffer)
	{
		free(frame);
		return;
	}
	
	memcpy(frame->frameBuffer, data, len);
    if (data2 && len2 > 0)
	    memcpy(frame->frameBuffer+len, data2, len2);

    if (queue_list_push(&handle->frameH264, frame))
    {
    	FrameListContentFree(frame);
    }
}

void MediaCache_InsertAudioFrame(int channel, unsigned char* data, int len, unsigned char* data2, int len2, int type)
{
	MediaCacheHandle_t* handle = (MediaCacheHandle_t*)&gMediaCacheHandler[channel];

	if (!handle->frameAudio.m_init)
	{
		return;
	}

	FrameInfo_t* frame = (FrameInfo_t*)malloc(sizeof(FrameInfo_t));
	frame->frameSize = len + len2;
	frame->timeStamp = currentTimeMillis();
	frame->frameType = type;
	frame->frameBuffer = (char*)malloc(frame->frameSize);
	if (NULL == frame->frameBuffer)
	{
		free(frame);
		return;
	}
	
	memcpy(frame->frameBuffer, data, len);
    if (data2 && len2 > 0)
	    memcpy(frame->frameBuffer+len, data2, len2);

    if (queue_list_push(&handle->frameAudio, frame))
    {
    	FrameListContentFree(frame);
    }
}

void MediaCache_MakeVideoFile(int chnNum, int duration, char* filename)
{
	char cachefile[128] = { 0 };

	MediaCacheHandle_t* handle = (MediaCacheHandle_t*)&gMediaCacheHandler[chnNum-1];

	sprintf(cachefile, CACHE_VIDEO_FORMAT, chnNum);

	int fd = open(cachefile, O_RDONLY);
	if (fd < 0)
	{
		PNT_LOGE("make cache video %s error, %s not exist.", filename, cachefile);
		return;
	}

	MediaStreamWriter_t* cacheWriter = &handle->writerVideo;

	int start_sec_pos = 0;

	if (cacheWriter->curSecond > duration)
	{
		if (cacheWriter->curSecond%CHCHE_FRAME_SECOND >= duration)
		{
			start_sec_pos = cacheWriter->curSecond%CHCHE_FRAME_SECOND - duration;
		}
		else
		{
			start_sec_pos = (cacheWriter->curSecond%CHCHE_FRAME_SECOND + CHCHE_FRAME_SECOND) - duration;
		}
	}
	else
	{
		duration = cacheWriter->curSecond;
	}
	if (duration <= 2)
	{
		return;
	}

	PNT_LOGI("%s start_sec_pos:%d cacheSeconds:%d duration:%d.", filename, start_sec_pos, cacheWriter->curSecond, duration);

	mp4v2_muxer_t muxer = { 0 };
	
	if (RET_FAILED == mp4v2_muxer_create(&muxer, filename))
	{
		close(fd);
		return;
	}

    CacheIndex_t* cacheIndex = NULL;
	unsigned char* readBuffer = (unsigned char*)malloc(CACHEVIDEO_CACHE_SIZE);
	if (NULL == readBuffer)
	{
    	PNT_LOGE("readBuffer malloc failed.");
		close(fd);
		mp4v2_muxer_close(&muxer);
		return;
	}

	int sec = 0, dataLen = 0, idrOffset = 0, curSec = 0;
	long long start_pts = 0;
	unsigned char* data = NULL;
	
	while (sec < duration)
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
					int spsOffset = 0, ppsOffset = 0, spsLen = 0, ppsLen = 0, i = 0;

					i = 0;
					while (i<cacheIndex->frameSize)
					{
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

						i++;
					}

					mp4v2_muxer_write264meta(&muxer, SDR_VENC_FRAMERATE_MAJOR, ((chnNum-1)==0)?CH1_VIDEO_WIDTH:CH2_VIDEO_WIDTH, 
											((chnNum-1)==0)?CH1_VIDEO_HEIGHT:CH2_VIDEO_HEIGHT, data+spsOffset, spsLen, data+ppsOffset, ppsLen);
				}

				data = readBuffer + cacheIndex->OFFSET + idrOffset;
				dataLen = cacheIndex->frameSize - idrOffset;
		    }
		    else
		    {
				dataLen = cacheIndex->frameSize;
		    }

			mp4v2_muxer_write264data(&muxer, data, dataLen);
		}
		
		sec ++;
	}
	close(fd);

	sprintf(cachefile, CACHE_AUDIO_FORMAT, chnNum);
	fd = open(cachefile, O_RDONLY);
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
		
		close(fd);
	}

	mp4v2_muxer_close(&muxer);

	free(readBuffer);

	PNT_LOGI("%s make success.", filename);
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

	int curIdx = 0;

	while (streamHandle->bThreadStart)
	{
		FrameInfo_t* frameH264 = queue_list_popup(&handle->frameH264);

		FrameInfo_t* frameAudio = NULL;
		if (handle->frameAudio.m_init)
		{
			frameAudio = queue_list_popup(&handle->frameAudio);
		}
	
		if (REC_STOP == streamHandle->recStatus)
		{
			handle->writerVideo.curSecond = 0;
			FrameListContentFree(frameH264);
			FrameListContentFree(frameAudio);
			usleep(1000/SDR_VENC_FRAMERATE_MAJOR*1000);
			continue;
		}
		
		cacheWriter = &handle->writerVideo;
		if (NULL != frameH264)
		{
	        if (frameH264->frameType == DATA_TYPE_I_FRAME && cacheWriter->cacheFd >= 0)
	        {
				pwrite(cacheWriter->cacheFd, cacheWriter->dataCache, cacheWriter->dataSize, CACHEVIDEO_CACHE_SIZE*(cacheWriter->curSecond%CHCHE_FRAME_SECOND));
				cacheWriter->dataSize = 0;
				cacheWriter->curSecond ++;
				curIdx = 0;
				memset(cacheWriter->dataCache, 0, CACHEVIDEO_CACHE_SIZE);
				if (cacheWriter->curSecond%CHCHE_FRAME_SECOND == 0)
				{
					close(cacheWriter->cacheFd);
					cacheWriter->cacheFd = -1;
				}
				
		        if (handle->writerAudio.cacheFd >= 0)
		        {
					pwrite(handle->writerAudio.cacheFd, handle->writerAudio.dataCache, handle->writerAudio.dataSize, CACHEAUDIO_CACHE_SIZE*(cacheWriter->curSecond%CHCHE_FRAME_SECOND));
					handle->writerAudio.dataSize = 0;
					memset(handle->writerAudio.dataCache, 0, CACHEAUDIO_CACHE_SIZE);
					memset(&handle->writerAudio.indexCache[(cacheWriter->curSecond%CHCHE_FRAME_SECOND)], 0, sizeof(CacheIndex_t));
					if (cacheWriter->curSecond%CHCHE_FRAME_SECOND == 0)
					{
						close(handle->writerAudio.cacheFd);
						handle->writerAudio.cacheFd = -1;
					}
		        }
	        }

	        if ((frameH264->frameType == DATA_TYPE_I_FRAME || frameH264->frameType == DATA_TYPE_INITPACKET) && cacheWriter->cacheFd < 0)
	        {
            	sprintf(filename, CACHE_VIDEO_FORMAT, handle->chnNum+1);
            	cacheWriter->cacheFd = open(filename, O_RDWR | O_CREAT);
				//curIdx = 0;
				//cacheWriter->curSecond = 0;
	        }

			if (REC_STOP != streamHandle->recStatus && cacheWriter->cacheFd >= 0 && curIdx < SDR_VENC_FRAMERATE_MAJOR)
			{
				if (cacheWriter->dataSize + frameH264->frameSize <= CACHEVIDEO_CACHE_SIZE)
				{
					cacheIndex = &cacheWriter->indexCache[(cacheWriter->curSecond%CHCHE_FRAME_SECOND)*SDR_VENC_FRAMERATE_MAJOR + curIdx];
					cacheIndex->frameSize = frameH264->frameSize;
					cacheIndex->frameType = frameH264->frameType;
					cacheIndex->OFFSET = cacheWriter->dataSize;
					
					memcpy(cacheWriter->dataCache+cacheWriter->dataSize, frameH264->frameBuffer, frameH264->frameSize);
					cacheWriter->dataSize += frameH264->frameSize;
				}
				
				curIdx ++;
			}
		}
		else
		{
			usleep(20*1000);
		}

		if (NULL != frameAudio)
		{
			if (handle->writerAudio.cacheFd < 0)
			{
            	sprintf(filename, CACHE_AUDIO_FORMAT, handle->chnNum+1);
            	handle->writerAudio.cacheFd = open(filename, O_RDWR | O_CREAT);
			}

			if (handle->writerAudio.cacheFd >= 0)
			{
				if (handle->writerAudio.dataSize + frameAudio->frameSize <= CACHEAUDIO_CACHE_SIZE)
				{
					cacheIndex = &handle->writerAudio.indexCache[(cacheWriter->curSecond%CHCHE_FRAME_SECOND)];
					
					memcpy(handle->writerAudio.dataCache+handle->writerAudio.dataSize, frameAudio->frameBuffer, frameAudio->frameSize);
					handle->writerAudio.dataSize += frameAudio->frameSize;
					
					cacheIndex->frameSize = handle->writerAudio.dataSize;
				}
			}
		}
	
		FrameListContentFree(frameH264);
		FrameListContentFree(frameAudio);
	}

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

		MediaCache_MakeVideoFile(handle->chnNum+1, req->beforeTime+req->afterTime, req->filename);

		free(req);
	}

	return NULL;
}

int MediaCache_VideoCreate(unsigned char before_time, unsigned char after_time, int chnNum, char* filename)
{
	CacheVideoReq_t* req = (CacheVideoReq_t*)malloc(sizeof(CacheVideoReq_t));
	if (NULL == req)
	{
		PNT_LOGE("create %s failed, no buffer", filename);
		return RET_FAILED;
	}
	
	MediaCacheHandle_t* handle = (MediaCacheHandle_t*)&gMediaCacheHandler[chnNum];
	
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

	sprintf(filename, CACHE_VIDEO_FORMAT, 1);
	do_fallocate(filename, CACHEVIDEO_CACHE_SIZE*CHCHE_FRAME_SECOND + CACHEVIDEO_CACHE_SIZE);

	sprintf(filename, CACHE_AUDIO_FORMAT, AUDIO_BIND_CHN+1);
	do_fallocate(filename, CACHEAUDIO_CACHE_SIZE*CHCHE_FRAME_SECOND);
}

void MediaCache_Start(MediaVideoStream_t* streams)
{
	for (int i=0; i<MAX_VIDEO_NUM; i++)
	{
		MediaCacheHandle_t* handle = (MediaCacheHandle_t*)&gMediaCacheHandler[i];
	
		memset(handle, 0, sizeof(MediaCacheHandle_t));

		handle->chnNum = i;
		handle->stream = &streams[i];

		handle->writerVideo.dataCache = (char*)malloc(CACHEVIDEO_CACHE_SIZE);
		handle->writerVideo.indexCache = (CacheIndex_t*)malloc(CACHE_FRAME_COUNT*sizeof(CacheIndex_t));
		memset(handle->writerVideo.indexCache, 0, CACHE_FRAME_COUNT*sizeof(CacheIndex_t));
		
		queue_list_create(&handle->frameH264, 4);
		queue_list_set_free(&handle->frameH264, FrameListContentFree);
		
		queue_list_create(&handle->videoReqs, 10);

		if (i == AUDIO_BIND_CHN)
		{
			handle->writerAudio.dataCache = (char*)malloc(CACHEAUDIO_CACHE_SIZE);
			handle->writerAudio.indexCache = (CacheIndex_t*)malloc(CHCHE_FRAME_SECOND*sizeof(CacheIndex_t));
			memset(handle->writerAudio.indexCache, 0, CHCHE_FRAME_SECOND*sizeof(CacheIndex_t));
		
			queue_list_create(&handle->frameAudio, 4);
			queue_list_set_free(&handle->frameAudio, FrameListContentFree);
		}

		pthread_create(&handle->pid, 0, MediaCache_DataCacheTask, (EI_VOID *)handle);

		pthread_t pid;
		pthread_create(&pid, 0, MediaCache_VideoProc, (EI_VOID *)handle);
	}
}

