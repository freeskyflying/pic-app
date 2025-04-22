#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "pnt_log.h"

#include "mp4v2_demuxer.h"

unsigned int mp4v2SeekIframe(MP4V2_Demuxer_t* demuxer, unsigned int frameIdx, unsigned int totalFrame)
{
	MP4V2_EsDemuxer_t* es = &demuxer->mVidHandle;
	unsigned char* pSample = NULL;
	unsigned int sampleSize = 0;

	while (1)
	{
		if (MP4ReadSample(demuxer->mp4Handle, es->trackid, frameIdx, &pSample, &sampleSize, NULL, NULL, NULL, NULL))
		{
			if (pSample[4] == 0x67)
			{
				break;
			}
		}
		
		usleep(100);

		if (NULL != pSample)
		{
			free(pSample);
			pSample = NULL;
		}

		frameIdx ++;
		
		if (frameIdx > totalFrame)
		{
			return totalFrame;
		}
	}

	return frameIdx;
}

void* mp4v2DemuxerVideo(void* pArg)
{
	pthread_detach(pthread_self());
	MP4V2_Demuxer_t* demuxer = (MP4V2_Demuxer_t*)pArg;
	
	MP4V2_EsDemuxer_t* es = &demuxer->mVidHandle;
	
	unsigned int duration = MP4GetDuration(demuxer->mp4Handle);
	unsigned int frameCount = MP4GetTrackNumberOfSamples(demuxer->mp4Handle, es->trackid);
	unsigned int frameNormalIntervel = duration/frameCount;
	unsigned int frameIdx = demuxer->dragOffset*1000*frameCount/duration+1;
	unsigned int frameIdxSeek = frameIdx;
	unsigned int frameIntervel = frameNormalIntervel*1000;

	int pps_offset = 0, iframe_offset = 0;
	unsigned char lastDragOffset = 0;

	PNT_LOGE("frameCount:%d frameIntervel:%d", frameCount, frameNormalIntervel);
	
	unsigned char* pSample = NULL;
	unsigned int sampleSize = 0;

	while (demuxer->state != mp4v2_demuxer_ctrl_stop)
	{
		if (demuxer->gain > 0)
		{
			frameIntervel = frameNormalIntervel*1000/demuxer->gain;
		}

		if (demuxer->mode == mp4v2_demuxer_ctrl_pause)
		{
			usleep(50*1000);
			continue;
		}
		
		if (demuxer->mode == mp4v2_demuxer_ctrl_drag)
		{
			if (frameIdxSeek != frameIdx || lastDragOffset != demuxer->dragOffset)
			{
				frameIdxSeek = demuxer->dragOffset*1000*frameCount/duration+1;
				frameIdxSeek = mp4v2SeekIframe(demuxer, frameIdxSeek, frameCount);
				
				frameIdx = frameIdxSeek;
				lastDragOffset = demuxer->dragOffset;
			}
			else
			{
				usleep(50*1000);
				continue;
			}
		}

		if (frameIdx > frameCount || frameIdx == 0)
		{
			usleep(50*1000);
			continue;
		}
		
		if (MP4ReadSample(demuxer->mp4Handle, es->trackid, frameIdx, &pSample, &sampleSize, NULL, NULL, NULL, NULL))
		{
			if (pSample[4] == 0x67)
			{
				if (0 == pps_offset)
				{
					int sps_len = ((pSample[0] << 24)&0xff000000) | ((pSample[1] << 16)&0x00ff0000) | ((pSample[2] << 8)&0x0000ff00) | (pSample[3]&0x00ff);
					pps_offset = sps_len+4;
					int pps_len = ((pSample[pps_offset+0] << 24)&0xff000000) | ((pSample[pps_offset+1] << 16)&0x00ff0000) | 
									((pSample[pps_offset+2] << 8)&0x0000ff00) | (pSample[pps_offset+3]&0x00ff);
					iframe_offset = sps_len+pps_len+8;
				}
				pSample[0] = 0x00;
				pSample[1] = 0x00;
				pSample[2] = 0x00;
				pSample[3] = 0x01;
				
				pSample[pps_offset] = 0x00;
				pSample[pps_offset+1] = 0x00;
				pSample[pps_offset+2] = 0x00;
				pSample[pps_offset+3] = 0x01;
				
				pSample[iframe_offset+0] = 0x00;
				pSample[iframe_offset+1] = 0x00;
				pSample[iframe_offset+2] = 0x00;
				pSample[iframe_offset+3] = 0x01;
			}
			else
			{
				pSample[0] = 0x00;
				pSample[1] = 0x00;
				pSample[2] = 0x00;
				pSample[3] = 0x01;
			}

			if (demuxer->vidCb)
			{
				unsigned int constTime = demuxer->vidCb(pSample, sampleSize, demuxer->owner, frameNormalIntervel*frameIdx)*1000;
				if (constTime < frameIntervel)
				{
					usleep(frameIntervel-constTime);
				}
			}
        }
		else
		{
			break;
		}
		
		usleep(100);

		if (NULL != pSample)
		{
			free(pSample);
			pSample = NULL;
		}
		
		if (demuxer->mode != mp4v2_demuxer_ctrl_drag)
		{
			if (frameIdx <= frameCount && frameIdx > 0)
			{
				frameIdx ++;
			}
		}
	}

	if (demuxer->mRunningFlag > 0)
		demuxer->mRunningFlag --;

	return NULL;
}

void getAACHeader(unsigned char* header, unsigned char* aac_config, int frameSize)
{
	frameSize = frameSize + 7;

	// aac_config   2byte
	// 5 bits | 4 bits | 4 bits | 3 bits
	// AAC Object Type | Sample Rate Index | Channel Number | dont care
	header[0] = 0xFF;
	header[1] = 0xF1;

	char aacObjectType = 1;
	char sampleRateIndex = ((aac_config[0] & 0x07) << 1) + ((aac_config[1] & 0x80) >> 7);
	char channelNumber = (aac_config[1] >> 3) & 0x03;

	header[2] = ((aacObjectType << 6) & 0xC0) + ((sampleRateIndex << 2) & 0x3C) + ((channelNumber >> 2) & 0x01);
	header[3] = ((channelNumber << 6) & 0xC0) + ((frameSize >> 11) & 0x03);
	header[4] = ((frameSize >> 3) & 0xFF);
	header[5] = ((frameSize << 5) & 0xE0) + 0x1F/*(0x7F >> 2)*/;
	header[6] = 0xFC;
}

void formatAACFrame(unsigned char* aac_config, unsigned char* org, unsigned int frameSize, unsigned char* out)
{
	if (NULL != aac_config)
		getAACHeader(out, aac_config, frameSize);

	memcpy(out+7, org, frameSize);
}

void* mp4v2DemuxerAudio(void* pArg)
{
	pthread_detach(pthread_self());
	MP4V2_Demuxer_t* demuxer = (MP4V2_Demuxer_t*)pArg;
	
	MP4V2_EsDemuxer_t* es = &demuxer->mAudHandle;

	unsigned char* aac_config = NULL;
	unsigned int aac_config_len = 0;

	unsigned int duration = MP4GetDuration(demuxer->mp4Handle);
	unsigned int frameCount = MP4GetTrackNumberOfSamples(demuxer->mp4Handle, es->trackid);
	unsigned int frameNormalIntervel = duration/frameCount;
	unsigned int frameIdx = demuxer->dragOffset*1000*frameCount/duration+1;
	unsigned int frameIdxSeek = frameIdx;
	
	MP4GetTrackESConfiguration(demuxer->mp4Handle, es->trackid, &aac_config, &aac_config_len);

	unsigned int frameIntervel = frameNormalIntervel*1000;
	unsigned char tempBuf[2048] = { 0 };
	
	unsigned char* pSample = NULL;
	unsigned int sampleSize = 0;

	while (demuxer->state != mp4v2_demuxer_ctrl_stop)
	{
		if (demuxer->gain > 0)
		{
			frameIntervel = (frameNormalIntervel*1000)/demuxer->gain;
		}

		if (demuxer->mode == mp4v2_demuxer_ctrl_pause)
		{
			usleep(50*1000);
			continue;
		}
		
		if (demuxer->mode == mp4v2_demuxer_ctrl_drag)
		{
			if (frameIdxSeek != frameIdx)
			{
				frameIdxSeek = demuxer->dragOffset*1000*frameCount/duration+1;
				frameIdx = frameIdxSeek;
			}
			else
			{
				usleep(50*1000);
				continue;
			}
		}

		if (frameIdx > frameCount || frameIdx == 0)
		{
			usleep(50*1000);
			continue;
		}
		
		if (MP4ReadSample(demuxer->mp4Handle, es->trackid, frameIdx, &pSample, &sampleSize, NULL, NULL, NULL, NULL))
		{			
			formatAACFrame(aac_config, pSample, sampleSize, tempBuf);
			
			if (demuxer->audCb)
			{
				unsigned int constTime = demuxer->audCb(tempBuf, sampleSize+7, demuxer->owner, frameNormalIntervel*frameIdx)*1000;
				if (constTime < frameIntervel)
				{
					usleep(frameIntervel-constTime);
				}
			}
        }
		else
		{
			break;
		}

		usleep(100);

		if (NULL != pSample)
		{
			free(pSample);
			pSample = NULL;
		}
		
		if (demuxer->mode != mp4v2_demuxer_ctrl_drag)
		{
			if (frameIdx <= frameCount && frameIdx > 0)
			{
				frameIdx ++;
			}
		}
	}

	if (NULL != aac_config)
		free(aac_config);
	
	if (demuxer->mRunningFlag > 0)
		demuxer->mRunningFlag --;

	return NULL;
}

void* mp4v2DemuxerProcessor(void* pArg)
{
	pthread_detach(pthread_self());
	MP4V2_Demuxer_t* demuxer = (MP4V2_Demuxer_t*)pArg;

    char thread_name[16];
    thread_name[15] = 0;
    snprintf(thread_name, 16, "demux%ld", demuxer->mPid);
    prctl(PR_SET_NAME, thread_name);

	unsigned int video_trackid = 0, autio_trackid = 0;
	
	unsigned int tracks_num = MP4GetNumberOfTracks(demuxer->mp4Handle, NULL, 0);
	
	unsigned int mp4_duration = MP4GetDuration(demuxer->mp4Handle);

    for (uint32_t i = 0; i < tracks_num; i++)
	{
        int track_id = MP4FindTrackId( demuxer->mp4Handle, i, NULL, 0);
		
		const char* track_type = MP4GetTrackType(demuxer->mp4Handle, track_id);
		
		if (MP4_IS_VIDEO_TRACK_TYPE(track_type))
		{
			demuxer->mVidHandle.trackid = track_id;
			pthread_create(&demuxer->mVidHandle.mPid, NULL, mp4v2DemuxerVideo, demuxer);
			demuxer->mRunningFlag ++;
		}
		else if (MP4_IS_AUDIO_TRACK_TYPE(track_type))
		{
			demuxer->mAudHandle.trackid = track_id;
			pthread_create(&demuxer->mAudHandle.mPid, NULL, mp4v2DemuxerAudio, demuxer);
			demuxer->mRunningFlag ++;
		}
    }

	while (demuxer->mRunningFlag)
	{
		usleep(80*1000);
	}

	PNT_LOGE("%s exit.", thread_name);
	return NULL;
}

int mp4v2_Demuxer_Start(MP4V2_Demuxer_t* demuxer, char* filename)
{
	if (NULL == demuxer)
	{
		return RET_FAILED;
	}

	if (demuxer->mRunningFlag)
	{
		mp4v2_Demuxer_Stop(demuxer);
	}

	memset(&demuxer->mAudHandle, 0, sizeof(MP4V2_EsDemuxer_t));
	memset(&demuxer->mVidHandle, 0, sizeof(MP4V2_EsDemuxer_t));

	demuxer->mp4Handle = MP4Read(filename);

	if (MP4_INVALID_FILE_HANDLE == demuxer->mp4Handle)
	{
		PNT_LOGE("play %s failed.", filename);
		return RET_FAILED;
	}

	demuxer->state = mp4v2_demuxer_ctrl_start;

	pthread_create(&demuxer->mPid, NULL, mp4v2DemuxerProcessor, demuxer);
	
	PNT_LOGE("start to play %s %ld.", filename, demuxer->mPid);

	return RET_SUCCESS;
}

void mp4v2_Demuxer_Stop(MP4V2_Demuxer_t* demuxer)
{
	if (!demuxer->mRunningFlag)
	{
		return;
	}

	demuxer->state = mp4v2_demuxer_ctrl_stop;

	while (demuxer->mRunningFlag)
	{
		usleep(80*1000);
	}

	if (MP4_INVALID_FILE_HANDLE != demuxer->mp4Handle)
	{
		MP4Close(demuxer->mp4Handle, 0);
		demuxer->mp4Handle = MP4_INVALID_FILE_HANDLE;
	}
}

void mp4v2_Demuxer_SetCtrl(MP4V2_Demuxer_t* demuxer, unsigned char ctrl, unsigned char gain, unsigned char* timeNodeBCD)
{
	if (ctrl == mp4v2_demuxer_ctrl_stop)
	{
		mp4v2_Demuxer_Stop(demuxer);
	}
	else
	{
		demuxer->mode = ctrl;
		demuxer->gain = gain;

		if (NULL != timeNodeBCD)
		{
			unsigned int dragPos = convertBCDToSysTime(timeNodeBCD);
			demuxer->dragOffset = dragPos - demuxer->startTimestamp;
			if (demuxer->dragOffset >= 60)
			{
				demuxer->dragOffset = 0;
			}
		}
	}
}

