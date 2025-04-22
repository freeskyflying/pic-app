#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "mp4v2_muxer.h"
#include "pnt_log.h"
#include "utils.h"

int mp4v2_muxer_create(mp4v2_muxer_t* muxer, char* filename)
{
	memset(muxer, 0, sizeof(mp4v2_muxer_t));	

	muxer->mp4 = MP4CreateEx(filename, MP4_CREATE_64BIT_DATA,1,1,0,0,0,0);
	
	if (muxer->mp4 == MP4_INVALID_FILE_HANDLE)
	{
		PNT_LOGE("mp4create failed %s.", filename);
		return RET_FAILED;
	}

	muxer->videoTrack = MP4_INVALID_TRACK_ID;
	muxer->audioTrack = MP4_INVALID_TRACK_ID;

	MP4SetTimeScale(muxer->mp4, MP4V2_TIMESCALE);

	return RET_SUCCESS;
}

void mp4v2_muxer_close(mp4v2_muxer_t* muxer)
{
	if (MP4_INVALID_FILE_HANDLE == muxer->mp4)
	{
		PNT_LOGE("mp4create failed.");
		return;
	}
	
	MP4Close(muxer->mp4, MP4_CLOSE_DO_NOT_COMPUTE_BITRATE);
	muxer->mp4 = NULL;
}

int mp4v2_muxer_write264meta(mp4v2_muxer_t* muxer, int frameRate, int width, int height, unsigned char* sps, int spslen, unsigned char* pps, int ppslen)
{
	if (MP4_INVALID_FILE_HANDLE == muxer->mp4)
	{
		PNT_LOGE("mp4create failed.");
		return RET_FAILED;
	}
	
	muxer->videoTrack = MP4AddH264VideoTrack
		(muxer->mp4, 
		MP4V2_TIMESCALE, 
		MP4V2_TIMESCALE / frameRate, 
		width, // width
		height,// height
		sps[1], // sps[1] AVCProfileIndication
		sps[2], // sps[2] profile_compat
		sps[3], // sps[3] AVCLevelIndication
		3);
	
	if (muxer->videoTrack == MP4_INVALID_TRACK_ID)
	{
		PNT_LOGE("cannot create videotrack.");
		return RET_FAILED;
	}
	
	MP4SetVideoProfileLevel(muxer->mp4, 0x7f);

	MP4AddH264SequenceParameterSet(muxer->mp4, muxer->videoTrack, sps, spslen);

	MP4AddH264PictureParameterSet(muxer->mp4, muxer->videoTrack, pps, ppslen);

	return RET_SUCCESS;
}

int mp4v2_muxer_writeAACmeta(mp4v2_muxer_t* muxer, int sample_rate, int sampleDuration)
{
	if (MP4_INVALID_FILE_HANDLE == muxer->mp4)
	{
		PNT_LOGE("mp4create failed.");
		return RET_FAILED;
	}
	
	muxer->audioTrack = MP4AddAudioTrack(muxer->mp4, sample_rate, sampleDuration, MP4_MPEG4_AUDIO_TYPE);
	
	if (muxer->audioTrack == MP4_INVALID_TRACK_ID)
	{
		PNT_LOGE("cannot create videotrack.");
		return RET_FAILED;
	}

	MP4SetAudioProfileLevel(muxer->mp4, 0x2);

	return RET_SUCCESS;
}

void mp4v2_muxer_write264data(mp4v2_muxer_t* muxer, unsigned char* nalu, int len)
{
	if (MP4_INVALID_FILE_HANDLE == muxer->mp4)
	{
		return;
	}
	
	if (muxer->videoTrack == MP4_INVALID_TRACK_ID)
	{
		return;
	}
	
	nalu[0] = (len-4)>>24;
	nalu[1] = (len-4)>>16;
	nalu[2] = (len-4)>>8;
	nalu[3] = (len-4)&0xff;

	int isIdr = (nalu[4]==0x65);

	MP4WriteSample(muxer->mp4, muxer->videoTrack, nalu, len, MP4_INVALID_DURATION, 0, 1);
}

void mp4v2_muxer_writeAACdata(mp4v2_muxer_t* muxer, unsigned char* frame, int len)
{
	if (MP4_INVALID_FILE_HANDLE == muxer->mp4)
	{
		return;
	}
	
	if (muxer->audioTrack == MP4_INVALID_TRACK_ID)
	{
		return;
	}

	MP4WriteSample(muxer->mp4, muxer->audioTrack, frame, len, MP4_INVALID_DURATION, 0, 1);
}

void mp4v2_muxer_setAACconfigs(mp4v2_muxer_t* muxer, unsigned short audioType, unsigned short sampleRateIndex, unsigned short channelNumber)
{
	if (MP4_INVALID_FILE_HANDLE == muxer->mp4)
	{
		return;
	}
	
	if (muxer->audioTrack == MP4_INVALID_TRACK_ID)
	{
		return;
	}

	unsigned char p[2];

	p[0] = ((audioType << 3) & 0xf8) | ((sampleRateIndex >> 1) & 0x07);
	p[1]=((sampleRateIndex << 7) & 0x80) | ((channelNumber <<3) & 0x78);
	
	MP4SetTrackESConfiguration(muxer->mp4, muxer->audioTrack, p, 2);
}

int mp4v2_muxer_write265meta(mp4v2_muxer_t* muxer, int frameRate, 
									int width, int height, 
									unsigned char* vps, int vpslen, 
									unsigned char* sps, int spslen, 
									unsigned char* pps, int ppslen)
{
	if (MP4_INVALID_FILE_HANDLE == muxer->mp4)
	{
		PNT_LOGE("mp4create failed.");
		return RET_FAILED;
	}
	
	muxer->videoTrack = MP4AddH265VideoTrack
		(muxer->mp4, 
		MP4V2_TIMESCALE, 
		MP4V2_TIMESCALE / frameRate, 
		width, // width
		height,// height
		1);
	
	if (muxer->videoTrack == MP4_INVALID_TRACK_ID)
	{
		PNT_LOGE("cannot create videotrack.");
		return RET_FAILED;
	}
	
	MP4SetVideoProfileLevel(muxer->mp4, 0x7f);

 	MP4AddH265VideoParameterSet (muxer->mp4, muxer->videoTrack, vps, vpslen);

	MP4AddH265SequenceParameterSet(muxer->mp4, muxer->videoTrack, sps, spslen);

	MP4AddH265PictureParameterSet(muxer->mp4, muxer->videoTrack, pps, ppslen);

	return RET_SUCCESS;
}

void mp4v2_muxer_write265data(mp4v2_muxer_t* muxer, unsigned char* nalu, int len)
{
	if (MP4_INVALID_FILE_HANDLE == muxer->mp4)
	{
		return;
	}
	
	if (muxer->videoTrack == MP4_INVALID_TRACK_ID)
	{
		return;
	}
	
	nalu[0] = (len-4)>>24;
	nalu[1] = (len-4)>>16;
	nalu[2] = (len-4)>>8;
	nalu[3] = (len-4)&0xff;

	int isIdr = ((nalu[4]&0x7E)==0x26);

	MP4WriteSample(muxer->mp4, muxer->videoTrack, nalu, len, MP4_INVALID_DURATION, 0, 1);
}


