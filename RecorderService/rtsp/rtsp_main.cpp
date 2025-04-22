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

#define RTSP_CHN_MAX        2

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

extern "C" int get_video_stream(int chn, FrameInfo_t* frame);

extern "C" long long currentTimeMillis();

extern "C" uint64_t get_curr_framepts(int chn);

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

    session->AddSource(xop::channel_1, xop::AACSource::CreateNew(8000, 1, true));

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

    std::cout << "Play URL: " << rtsp_url << std::endl;

	FrameInfo_t frame = { 0 };
	FrameInfo_t frameAudio = { 0 };
    xop::AVFrame avFrame;

	while (1) {
        if (gRtspChannel)
        {
            if (0 == get_video_stream(gCurrentRtspChannel+MAX_VIDEO_NUM, &frame))
            {
				avFrame.size = frame.frameSize;
				avFrame.timestamp = xop::H264Source::GetTimestamp();
				avFrame.type = 0;
				avFrame.buffer = (uint8_t*)frame.frameBuffer;
				
	            rtsp_server->PushFrame(session_ids, xop::channel_0, avFrame);
            }
			if (0 == get_audio_stream(AUDIO_BIND_CHN, &frameAudio))
			{
				avFrame.size = frameAudio.frameSize;
				avFrame.timestamp = xop::AACSource::GetTimestamp(8000);
				avFrame.type = 0;
				avFrame.buffer = (uint8_t*)frameAudio.frameBuffer;
				
	            rtsp_server->PushFrame(session_ids, xop::channel_1, avFrame);
			}

			usleep(30*1000);
        }
		else
		{
			frame.timeStamp = get_curr_framepts(gCurrentRtspChannel+MAX_VIDEO_NUM);
		}
	}

	if (frameAudio.frameBuffer)
		free(frameAudio.frameBuffer);
	if (frame.frameBuffer)
		free(frame.frameBuffer);

    return NULL;
}

extern "C" int RtspServerInit(void)
{
    pthread_t pid;

    pthread_create(&pid, 0, RtspServerSingleChnTask, NULL);

    return 0;
}
