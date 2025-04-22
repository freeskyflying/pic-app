    
#ifndef __OS_ABSTRACT_H__
#define __OS_ABSTRACT_H__

#ifdef __cplusplus
extern "C" {
#endif


#include <pthread.h>
#include <sys/sem.h>


#include "systype.h"










sdword mutex_sem_init(key_t key);
sdword mutex_sem_get(key_t key);
sdword mutex_sem_put(key_t key);

sdword shm_init(key_t key, size_t size);
byte * shm_get(key_t key);

#ifdef __cplusplus
}
#endif 

#endif

