#include "controller.h"
#include "6zhentan.h"

HttpController_t gHttpController = { 0 };

#define HTTPS_DBG         //printf
#define HTTPS_ERR         printf

void evHandler(struct mg_connection *nc, int ev, void *p, void *userData)
{
    HttpController_t* controller = (HttpController_t*)userData;

    struct http_message *hm = (struct http_message *) p;
    switch (ev)
    {
        case MG_EV_HTTP_REQUEST:
            if (!mg_vcmp(&hm->method, "POST"))
            {
            }
            else if (!mg_vcmp(&hm->method, "GET"))
            {
                HTTPS_DBG("RECV: %s\n", hm->uri.p);
                HttpGetRequestProcess(nc, hm);
            }
            break;
        default:
            break;
    }
}

int HttpController_Init(int port)
{
    HttpController_t* controller = &gHttpController;

    controller->s_http_server_opts.document_root = ".";
    controller->s_http_server_opts.enable_directory_listing = "yes";

    sprintf(controller->port, "%u", port);

    mg_mgr_init(&controller->mgr, NULL);
    struct mg_connection *nc;

    nc = mg_bind(&controller->mgr, controller->port, evHandler, controller);
    if (nc == NULL)
    {
        HTTPS_ERR("Failed to create listener");
        return 1;
    }

    mg_set_protocol_http_websocket(nc);

    return 0;
}

void* http_server_loop(void*arg)
{
    HttpController_t* controller = (HttpController_t*)arg;
    
    for (;;)
    {
        mg_mgr_poll(&controller->mgr, 1000);
    }
}

int HttpContorller_Start(void)
{
    HttpController_t* controller = &gHttpController;

    HttpController_Init(80);

    pthread_create(&controller->pid, 0, http_server_loop, controller);

    return 0;
}
