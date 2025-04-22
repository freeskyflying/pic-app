#include "xop/RtspServer.h"
#include "net/Timer.h"
#include <thread>
#include <memory>
#include <iostream>
#include <string>

#include "queue_list.h"
#include "pnt_ipc_stream.h"
#include "media_utils.h"
#include "media_video.h"

#define RTSP_CHN_MAX        1

int gCurrentRtspChannel = 0;
static int gRtspChannel = 0;
static volatile int gRtspStatus = 0;
static queue_list_t gQueueList = { 0 };
static queue_list_t gAudioQueueList = { 0 };

void AvFrameListFree(void* content)
{
    xop::AVFrame* avFrame = (xop::AVFrame*)content;

	if (avFrame->buffer)
		free(avFrame->buffer);
	avFrame->buffer = NULL;
	free(avFrame);
}

extern "C" void RtspFramePushQueue(int channel, char* data, int len, char * data2, int len2)
{
    if (channel != gCurrentRtspChannel || 0 == gRtspChannel)
    {
        return;
    }
    
    xop::AVFrame* avFrame = (xop::AVFrame*)malloc(sizeof(xop::AVFrame));
    if (NULL != avFrame)
    {
		avFrame->type = 0; 
		avFrame->size = len+len2;
		avFrame->timestamp = xop::H264Source::GetTimestamp();
        avFrame->buffer = (uint8_t*)malloc(avFrame->size);
        if (NULL == avFrame->buffer)
        {
            free(avFrame);
            return;
        }
		memcpy(avFrame->buffer, data, len);
        if (data2 && len2 > 0)
		    memcpy(avFrame->buffer+len, data2, len2);

        queue_list_push(&gQueueList, avFrame);
    }
}

extern "C" void RtspAudioPushQueue(int channel, char* data, int len)
{
    if (channel >= RTSP_CHN_MAX || 0 == gRtspChannel)
    {
        return;
    }
    
    xop::AVFrame* avFrame = (xop::AVFrame*)malloc(sizeof(xop::AVFrame));
    if (NULL != avFrame)
    {
		avFrame->type = 0; 
		avFrame->size = len;
		avFrame->timestamp = xop::H264Source::GetTimestamp();
        avFrame->buffer = (uint8_t*)malloc(avFrame->size);
        if (NULL == avFrame->buffer)
        {
            free(avFrame);
            return;
        }
		memcpy(avFrame->buffer, data, len);

        queue_list_push(&gAudioQueueList, avFrame);
    }
}

void* RtspServerSingleChnTask(void* arg)
{
	std::string ip = "127.0.0.1";
	std::string port = "554";

    std::shared_ptr<xop::EventLoop> event_loop(new xop::EventLoop());
    std::shared_ptr<xop::RtspServer> rtsp_server = xop::RtspServer::Create(event_loop.get());

    if (!rtsp_server->Start("0.0.0.0", atoi(port.c_str()))) {
        printf("RTSP Server listen on %s failed.\n", port.c_str());
        return NULL;
    }

    xop::MediaSessionId session_ids = { 0 };

    std::string suffix = "live";
    std::string rtsp_url = "rtsp://" + ip + ":" + port + "/" + suffix;

    xop::MediaSession *session = xop::MediaSession::CreateNew(suffix);
    
    session->AddSource(xop::channel_0, xop::H264Source::CreateNew());

    session->AddSource(xop::channel_1, xop::G711ASource::CreateNew());

    session->AddNotifyConnectedCallback([] (xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port){
        gRtspChannel += 1;
        printf("RTSP client connect, ip=%s, port=%hu sessionId=%d cam=%d\n", peer_ip.c_str(), peer_port, sessionId, gCurrentRtspChannel);
    });

    session->AddNotifyDisconnectedCallback([](xop::MediaSessionId sessionId, std::string peer_ip, uint16_t peer_port) {
        if (gRtspChannel > 0) gRtspChannel -= 1;
        printf("RTSP client disconnect, ip=%s, port=%hu sessionId=%d \n", peer_ip.c_str(), peer_port, sessionId);
    });

    session_ids = rtsp_server->AddSession(session);

    printf("session_id: %d.\n", session_ids);

    gRtspChannel = 0;

    queue_list_create(&gQueueList, 5);
    queue_list_set_free(&gQueueList, AvFrameListFree);
    queue_list_create(&gAudioQueueList, 5);
    queue_list_set_free(&gAudioQueueList, AvFrameListFree);

    std::cout << "Play URL: " << rtsp_url << std::endl;

	while (1) {
        if (gRtspChannel)
        {
            xop::AVFrame* avFrame = (xop::AVFrame*)queue_list_popup_last(&gQueueList);
            if (NULL == avFrame)
            {
                xop::Timer::Sleep(25);
                continue;
            } 
            rtsp_server->PushFrame(session_ids, xop::channel_0, *avFrame);
            free(avFrame->buffer);
            free(avFrame);

            avFrame = (xop::AVFrame*)queue_list_popup(&gAudioQueueList);
            if (NULL == avFrame)
            {
                continue;
            }
            rtsp_server->PushFrame(session_ids, xop::channel_1, *avFrame);
            free(avFrame->buffer);
            free(avFrame);
        }
	}

    return NULL;
}

extern "C" int RtspServerInit(void)
{
    pthread_t pid;

    pthread_create(&pid, 0, RtspServerSingleChnTask, NULL);

    return 0;
}
