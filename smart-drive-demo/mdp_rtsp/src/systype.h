    
#ifndef __SYS_TYPE_H__
#define __SYS_TYPE_H__

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef		char				sbyte;			
typedef		unsigned char  		byte;    		
typedef  	char          		schar;    		
typedef  	unsigned short  	word;    		
typedef  	signed short  		sword;   		
typedef  	signed int     		sdword;  		
typedef  	float          		sfloat;    		
typedef  	double	       	 	sdouble;	 		
typedef  	void          		svoid;          	
typedef  	unsigned int    	dword;   		
typedef  	uint64_t 			ddwrod;       	



#define		SYS_ERROR			-1
#define		SYS_OK				0
#define		SYS_NULL			NULL
#define		SYS_TRUE			1
#define		SYS_FAILS			0

typedef  unsigned char    	     sbool;    		





#if 1
#define TRACE_ERROR(fmt, args...) live_video_loge( fmt, ##args)
#define OS_ERROR(fmt, args...)    live_video_loge( fmt, ##args)
#define TRACE_DEBUG(fmt, args...) live_video_logd( fmt, ##args)
#define TRACE_DEBUG_RATE(rate,fmt, args...) do{ static int cnt_ = 0;\
												if ((rate) && cnt_++%(rate) ==0 )\
												live_video_logd( fmt, ##args);\
											}while(0)

#else
#define TRACE_ERROR(fmt, args...)
#define OS_ERROR(fmt, args...)
#define TRACE_DEBUG(fmt, args...)
#endif
#ifdef __cplusplus
}
#endif


#endif

