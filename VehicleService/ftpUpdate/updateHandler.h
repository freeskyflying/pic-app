#ifndef _UPDATEHANDLER_H_
#define _UPDATEHANDLER_H_

#if __cplusplus
extern "C" {
#endif

int updateHandler_MCUUpdate(char* filename);

int updateHandler_SOCUpdate(char* filename);

int updateHandler_ParamUpdate(char* filename);

#if __cplusplus
} // extern "C"
#endif

#endif

