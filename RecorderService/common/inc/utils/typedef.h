#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#include <stdint.h>

#define FALSE               0
#define TRUE                1

#define RET_FAILED          -1
#define RET_PARAMERR		-2
#define RET_SUCCESS         0

#ifdef __cplusplus
extern "C" {
#endif

typedef int                 bool_t;

#ifdef __cplusplus
}
#endif

#endif
