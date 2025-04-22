#include <sys/prctl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/statfs.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/mount.h>
#include <fcntl.h>

#include "utils.h"
#include "pnt_log.h"

#include "mp4v2_demuxer.h"
#include "media_storage.h"
#include "jt1078_media_files.h"

unsigned int mp4v2SeekIframe(MP4V2_Demuxer_t* demuxer, unsigned int frameIdxSeek)
{
	MP4V2_EsDemuxer_t* es = &demuxer->mVidHandle;

	unsigned long long seekTime = MP4GetSampleTime(demuxer->mp4Handle, es->trackid, frameIdxSeek);

	unsigned int frameIdx = MP4GetSampleIdFromTime(demuxer->mp4Handle, es->trackid, seekTime, true);

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

	int pps_offset = 0, iframe_offset = 0, sps_len = 0, pps_len, vps_offset = 0, vps_len = 0, sps_offset = 0;
	unsigned char lastDragOffset = 0;
	
	char filename[128] = { 0 };
	
	JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)demuxer->owner;
	get_videoname_by_time(playback->mLogicChannel, playback->filetime, filename);

	PNT_LOGE("%s frameCount:%d frameIntervel:%d frameIdx:%d gain:%d", filename, frameCount, frameNormalIntervel, frameIdx, demuxer->gain);
	
	unsigned char* pSample = (unsigned char*)malloc(800*1024);
	if (NULL == pSample)
	{
		PNT_LOGE("%s malloc failed.", filename);
		goto exit;
	}
	unsigned int sampleSize = 800*1024;

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
				frameIdxSeek = demuxer->dragOffset*1000*frameCount/duration;
				PNT_LOGI("%s drag position : %d s %d", filename, demuxer->dragOffset, frameIdxSeek);
				frameIdxSeek = mp4v2SeekIframe(demuxer, frameIdxSeek/*demuxer->dragOffset*1000*/);

				frameIdx = frameIdxSeek;
				PNT_LOGI("%s drag frameIdx : %d", filename, frameIdxSeek);
				lastDragOffset = demuxer->dragOffset;

				demuxer->mode = mp4v2_demuxer_ctrl_playing;
			}
			else
			{
				usleep(50*1000);
				continue;
			}
		}

		if (frameIdx == 0)
		{
			usleep(50*1000);
			frameIdx ++;
			continue;
		}

		if (frameIdx > frameCount)
		{
			break;
		}

		sampleSize = 800*1024;
		pthread_mutex_lock(&demuxer->mutex);
		int ret = MP4ReadSample(demuxer->mp4Handle, es->trackid, frameIdx, &pSample, &sampleSize, NULL, NULL, NULL, NULL);
		pthread_mutex_unlock(&demuxer->mutex);
		if (ret)
		{			
			sampleSize = 0;
			if (pSample[4] == 0x67)
			{
				if (0 == pps_offset)
				{
					sps_len = ((pSample[0] << 24)&0xff000000) | ((pSample[1] << 16)&0x00ff0000) | ((pSample[2] << 8)&0x0000ff00) | (pSample[3]&0x00ff);
					pps_offset = sps_len+4;
					pps_len = ((pSample[pps_offset+0] << 24)&0xff000000) | ((pSample[pps_offset+1] << 16)&0x00ff0000) | 
									((pSample[pps_offset+2] << 8)&0x0000ff00) | (pSample[pps_offset+3]&0x00ff);
					iframe_offset = sps_len+pps_len+8;

					sampleSize += sps_len + pps_len + 8;
				}
				pSample[0] = 0x00;
				pSample[1] = 0x00;
				pSample[2] = 0x00;
				pSample[3] = 0x01;
				
				pSample[pps_offset] = 0x00;
				pSample[pps_offset+1] = 0x00;
				pSample[pps_offset+2] = 0x00;
				pSample[pps_offset+3] = 0x01;
				
				sampleSize += pSample[iframe_offset+0] << 24;
				sampleSize += pSample[iframe_offset+1] << 16;
				sampleSize += pSample[iframe_offset+2] << 8;
				sampleSize += pSample[iframe_offset+3];
				sampleSize += 4;
				
				pSample[iframe_offset+0] = 0x00;
				pSample[iframe_offset+1] = 0x00;
				pSample[iframe_offset+2] = 0x00;
				pSample[iframe_offset+3] = 0x01;
			}
			else if (pSample[4] == 0x40)
			{
				if (0 == pps_offset)
				{
					vps_len = ((pSample[0] << 24)&0xff000000) | ((pSample[1] << 16)&0x00ff0000) | ((pSample[2] << 8)&0x0000ff00) | (pSample[3]&0x00ff);

					sps_offset = vps_len+4;					
					sps_len = ((pSample[sps_offset+0] << 24)&0xff000000) | ((pSample[sps_offset+1] << 16)&0x00ff0000) | 
									((pSample[sps_offset+2] << 8)&0x0000ff00) | (pSample[sps_offset+3]&0x00ff);
					
					pps_offset = vps_len + sps_len + 8;					
					pps_len = ((pSample[pps_offset+0] << 24)&0xff000000) | ((pSample[pps_offset+1] << 16)&0x00ff0000) | 
									((pSample[pps_offset+2] << 8)&0x0000ff00) | (pSample[pps_offset+3]&0x00ff);
					
					iframe_offset = vps_len + sps_len + pps_len + 12;

					sampleSize += sps_len + pps_len + vps_len + 12;
				}
				pSample[0] = 0x00;
				pSample[1] = 0x00;
				pSample[2] = 0x00;
				pSample[3] = 0x01;
				
				pSample[sps_offset] = 0x00;
				pSample[sps_offset+1] = 0x00;
				pSample[sps_offset+2] = 0x00;
				pSample[sps_offset+3] = 0x01;
				
				pSample[pps_offset] = 0x00;
				pSample[pps_offset+1] = 0x00;
				pSample[pps_offset+2] = 0x00;
				pSample[pps_offset+3] = 0x01;
				
				sampleSize += pSample[iframe_offset+0] << 24;
				sampleSize += pSample[iframe_offset+1] << 16;
				sampleSize += pSample[iframe_offset+2] << 8;
				sampleSize += pSample[iframe_offset+3];
				sampleSize += 4;
				
				pSample[iframe_offset+0] = 0x00;
				pSample[iframe_offset+1] = 0x00;
				pSample[iframe_offset+2] = 0x00;
				pSample[iframe_offset+3] = 0x01;
			}
			else if (0x41 == pSample[4])
			{
				sampleSize = pSample[0] << 24;
				sampleSize += pSample[1] << 16;
				sampleSize += pSample[2] << 8;
				sampleSize += pSample[3];
				
				pSample[0] = 0x00;
				pSample[1] = 0x00;
				pSample[2] = 0x00;
				pSample[3] = 0x01;
			}
			else
			{
				sampleSize = 0;

				PNT_LOGI("%s frame error [%d]->[%02X]", filename, frameIdx, pSample[4]);
				//continue;
			}

			if (demuxer->vidCb && 0 < sampleSize)
			{
				unsigned int constTime = demuxer->vidCb(pSample, sampleSize, demuxer->owner, frameNormalIntervel*frameIdx)*1000;
				if (constTime < frameIntervel)
				{
					usleep(frameIntervel-constTime);
				}
			}
			else
			{
				usleep(100);
			}
        }
		else
		{
			break;
		}
		
		if (demuxer->mode != mp4v2_demuxer_ctrl_drag)
		{
			if (frameIdx <= frameCount && frameIdx > 0)
			{
				frameIdx ++;
			}
		}
	}

exit:

	PNT_LOGI("exit.");

	if (NULL != pSample)
	{
		free(pSample); pSample = NULL;
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
	
	char filename[128] = { 0 };
	
	JT1078MediaPlaybackCtrl_t* playback = (JT1078MediaPlaybackCtrl_t*)demuxer->owner;
	get_videoname_by_time(playback->mLogicChannel, playback->filetime, filename);
	
	MP4GetTrackESConfiguration(demuxer->mp4Handle, es->trackid, &aac_config, &aac_config_len);

	unsigned int frameIntervel = frameNormalIntervel*1000;
	unsigned char* tempBuf = (unsigned char*)malloc(10*1024);
	if (NULL == tempBuf)
	{
		PNT_LOGE("%s malloc failed.", filename);
		goto exit;
	}
	
	unsigned char* pSample = (unsigned char*)malloc(10*1024);
	if (NULL == pSample)
	{
		PNT_LOGE("%s malloc failed.", filename);
		goto exit;
	}
	unsigned int sampleSize = 0;

	PNT_LOGE("%s frameCount:%d frameIntervel:%d frameIdx:%d", filename, frameCount, frameNormalIntervel, frameIdx);

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

		if (frameIdx == 0)
		{
			usleep(50*1000);
			continue;
		}

		if (frameIdx > frameCount)
		{
			break;
		}
		
		sampleSize = 10*1024;
		pthread_mutex_lock(&demuxer->mutex);
		int ret = MP4ReadSample(demuxer->mp4Handle, es->trackid, frameIdx, &pSample, &sampleSize, NULL, NULL, NULL, NULL);
		pthread_mutex_unlock(&demuxer->mutex);
		if (ret)
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
			else
			{
				usleep(100);
			}
        }
		else
		{
			break;
		}
		
		if (demuxer->mode != mp4v2_demuxer_ctrl_drag)
		{
			if (frameIdx <= frameCount && frameIdx > 0)
			{
				frameIdx ++;
			}
		}
	}

exit:

	PNT_LOGI("exit.");

	if (NULL != pSample)
	{
		free(pSample); pSample = NULL;
	}
	if (NULL != tempBuf)
	{
		free(tempBuf); tempBuf = NULL;
	}
	
	if (NULL != aac_config)
		free(aac_config);
	
	if (demuxer->mRunningFlag > 0)
		demuxer->mRunningFlag --;

	return NULL;
}

int mp4v2_Demuxer_Start(MP4V2_Demuxer_t* demuxer, char* filename)
{
	if (NULL == demuxer)
	{
		return RET_FAILED;
	}

	PNT_LOGI("demuxer stop and start");

	demuxer->state = mp4v2_demuxer_ctrl_init;

	memset(&demuxer->mAudHandle, 0, sizeof(MP4V2_EsDemuxer_t));
	memset(&demuxer->mVidHandle, 0, sizeof(MP4V2_EsDemuxer_t));
	
	// free 节点重新处理
	int offset = 0, len = 0;
	char data[9] = { 0 };
	int fd = open(filename, O_RDWR);
	
	struct stat buf = { 0 };
	memset(&buf, 0, sizeof(struct stat));
	stat(filename, &buf);

	while (offset < buf.st_size-8)
	{
		pread(fd, data, 8, offset);
		
		if (data[4] == 'f' && data[5] == 'r' && data[6] == 'e' && data[7] == 'e')
		{
			len = buf.st_size - offset;
			PNT_LOGI("%s file free size:[%d]-[%d] %d", filename, buf.st_size, offset, len);
			
			data[0] = len >> 24;
			data[1] = len >> 16;
			data[2] = len >> 8;
			data[3] = len;
			
			pwrite(fd, data, 8, offset);

			close(fd);
			break;
		}
		
		len = ((data[0]<<24)&0xff000000) + ((data[1]<<16)&0xff0000) + ((data[2]<<8)&0xff00) + ((data[3])&0xff);	
		PNT_LOGI("%s file type[%s] offset[%d] len[%d]", filename, &data[4], offset, len);	
		offset += len;
	}
	
#if (defined VDENCRYPT)
	
	char decrypt[128] = { 0 };
	strcpy(decrypt, VIDEO_PATH"/tmp.mp4");
	remove(decrypt);
	if (0 == copyFile(decrypt, filename))
	{
		if (RET_SUCCESS == decrypt_video_file(decrypt))
		{
			demuxer->mp4Handle = MP4Read(decrypt);
		}
		else
		{
			PNT_LOGE("play %s failed.", filename);
			return RET_FAILED;
		}
	}
	else
	{
		PNT_LOGE("play %s failed,decrypt failed.", filename);
		return RET_FAILED;
	}
	
#else
	
	demuxer->mp4Handle = MP4Read(filename);
	
#endif

	if (MP4_INVALID_FILE_HANDLE == demuxer->mp4Handle)
	{
		PNT_LOGE("play %s failed.", filename);
		return RET_FAILED;
	}

	PNT_LOGE("start to play %s.", filename);
	
	unsigned int video_trackid = 0, autio_trackid = 0;
	
	unsigned int tracks_num = MP4GetNumberOfTracks(demuxer->mp4Handle, NULL, 0);
	
	unsigned int mp4_duration = MP4GetDuration(demuxer->mp4Handle);

	demuxer->state = mp4v2_demuxer_ctrl_playing;

	pthread_mutex_init(&demuxer->mutex, NULL);

	demuxer->mRunningFlag = 0;

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

	demuxer->mRunningFlag ++;

	while (demuxer->mRunningFlag > 1)
	{
		usleep(80*1000);
	}

	MP4Close(demuxer->mp4Handle, MP4_CLOSE_DO_NOT_COMPUTE_BITRATE);
	demuxer->mp4Handle = MP4_INVALID_FILE_HANDLE;

	demuxer->state = mp4v2_demuxer_ctrl_stop;

	pthread_mutex_destroy(&demuxer->mutex);

	return RET_SUCCESS;
}

void mp4v2_Demuxer_SetCtrl(MP4V2_Demuxer_t* demuxer, unsigned char ctrl, unsigned char gain, unsigned char* timeNodeBCD)
{
	PNT_LOGI("control type %d [%d].", ctrl, demuxer->state);

	if (ctrl == mp4v2_demuxer_ctrl_stop)
	{
		demuxer->state = mp4v2_demuxer_ctrl_stop;
	}
	else
	{
		demuxer->mode = ctrl;
		demuxer->gain = gain;

		if (NULL != timeNodeBCD && !(0 == ctrl && mp4v2_demuxer_ctrl_playing == demuxer->state))
		{
			unsigned int dragPos = convertBCDToSysTime(timeNodeBCD);
			demuxer->dragOffset = dragPos - demuxer->startTimestamp;
			if (demuxer->dragOffset >= 60)
			{
//				demuxer->state = mp4v2_demuxer_ctrl_stop;
			}
		}
	}
}

