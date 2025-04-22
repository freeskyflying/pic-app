#ifndef _MP4V2_MUXER_H_
#define _MP4V2_MUXER_H_

#include "mp4v2/mp4v2.h"

#define MP4V2_TIMESCALE		(90000)

typedef struct
{

	MP4FileHandle mp4;
	MP4TrackId videoTrack;
	MP4TrackId audioTrack;

} mp4v2_muxer_t;

int mp4v2_muxer_create(mp4v2_muxer_t* muxer, char* filename);

void mp4v2_muxer_close(mp4v2_muxer_t* muxer);

int mp4v2_muxer_write264meta(mp4v2_muxer_t* muxer, int frameRate, int width, int height, unsigned char* sps, int spslen, unsigned char* pps, int ppslen);

void mp4v2_muxer_write264data(mp4v2_muxer_t* muxer, unsigned char* nalu, int len);

int mp4v2_muxer_writeAACmeta(mp4v2_muxer_t* muxer, int sample_rate, int sampleDuration);

void mp4v2_muxer_writeAACdata(mp4v2_muxer_t* muxer, unsigned char* frame, int len);

void mp4v2_muxer_setAACconfigs(mp4v2_muxer_t* muxer, unsigned short audioType, unsigned short sampleRateIndex, unsigned short channelNumber);

int mp4v2_muxer_write265meta(mp4v2_muxer_t* muxer, int frameRate, 
									int width, int height, 
									unsigned char* vps, int vpslen, 
									unsigned char* sps, int spslen, 
									unsigned char* pps, int ppslen);

void mp4v2_muxer_write265data(mp4v2_muxer_t* muxer, unsigned char* nalu, int len);

#endif

