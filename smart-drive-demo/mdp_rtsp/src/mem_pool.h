
    
#ifndef __MEM_POOL_H__
#define __MEM_POOL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "systype.h"

typedef struct 
{
	dword dwMemBlockSize;				
	dword dwMemBlockNum;				
	
}mem_block_dsc_s;


typedef struct 
{
	byte * pbyMemPoolBase;				
	dword dwMemPoolSize;				
	dword dwMemBlockType;				
	mem_block_dsc_s stMemBlockDsc[18];	
		
}mem_pool_dsc_s;



sdword mem_pool_init(mem_pool_dsc_s * pstMemPoolDsc);
svoid * mem_pool_malloc(mem_pool_dsc_s * pstMemPoolDsc, dword dwMemSize);
sdword mem_pool_free(svoid * pvBuffer);


#ifdef __cplusplus
}
#endif 

#endif



