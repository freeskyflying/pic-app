#ifndef _HTTP_CONTROLLER_H_
#define _HTTP_CONTROLLER_H_

#include <pthread.h>
#include "base64.h"
#include "cJSON.h"
#include "JSON_checker.h"
#include "mongoose.h"

typedef struct
{

    pthread_t pid;
    char port[11];
    struct mg_mgr mgr;
    struct mg_serve_http_opts s_http_server_opts;

} HttpController_t;

int HttpController_Init(int port);

int HttpContorller_Start(void);

#endif
