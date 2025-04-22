LOCAL_PATH := $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)
LOCAL_MODULE := DmsService
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	main.c

LOCAL_SRC_FILES += \
	common/src/utils/ftpUtil.c \
	common/src/utils/pnt_log.c \
	common/src/utils/queue_list.c \
	common/src/utils/strnormalize.c \
	common/src/utils/utils.c \
	common/src/utils/md5.c \
	common/src/socket/pnt_socket.c \
	common/src/epoll/pnt_epoll.c \
	common/src/ipc/pnt_ipc.c \
	common/src/ipc/pnt_ipc_stream.c \
	common/src/mediadb/normal_record_db.c \
	common/src/tts/baidu/common.c \
	common/src/tts/baidu/token.c \
	common/src/tts/baidu/ttscurl.c \
	common/src/tts/baidu/ttsmain.c \
	common/src/utils/system_param.c

LOCAL_SRC_FILES += \
	protocal/jt808/src/jt808_blind_upload.c \
	protocal/jt808/src/jt808_cmd.c \
	protocal/jt808/src/jt808_controller.c \
	protocal/jt808/src/jt808_database.c \
	protocal/jt808/src/jt808_session.c \
	protocal/jt808/src/jt808_utils.c \
	protocal/jt808/src/jt808_activesafety.c \
	protocal/jt808/src/jt808_activesafety_upload.c \
	protocal/jt808/src/jt1078_controller.c \
	protocal/jt808/src/jt1078_rtsp.c \
	protocal/jt808/src/jt1078_media_files.c \
	protocal/jt808/src/jt1078_fileupload.c \
	protocal/jt808/src/jt808_overspeed.c \
	protocal/protocal_main.c
	
LOCAL_SRC_FILES += \
	recorder/media_video.c \
	recorder/media_osd.c \
	recorder/media_snap.c \
	recorder/media_storage.c \
	recorder/media_audio.c \
	recorder/font_utils.c \
	recorder/media_cache.c \
	../../system/mdp/sample/common/sample_comm_audio.c \
	../../system/mdp/sample/common/sample_comm_sys.c \
	mp4v2/mp4v2_muxer.c \
	mp4v2/mp4v2_demuxer.c

LOCAL_SRC_FILES += \
	dms/dms_config.c \
	dms/dms_nn.c

LOCAL_SRC_FILES += \
	adas/adas_config.c \
	adas/adas_nn.c

LOCAL_SRC_FILES += \
	warning/al_warning.c

LOCAL_SRC_FILES += \
	app/http/base64.c \
	app/http/cJSON.c \
	app/http/JSON_checker.c \
	app/http/mongoose.c
	
LOCAL_SRC_FILES += \
	app/protocal/controller.c \
	app/protocal/6zhentan.c

LOCAL_SRC_FILES += \
	vehicle/vehicle_main.c \
	vehicle/gps/gpsReader.c \
	vehicle/gps/nmea.c \
	vehicle/4g/rild.c \
	vehicle/led/gpio_led.c  \
	vehicle/update/upgrade.c  \
	vehicle/wifi/hostap.c \
	vehicle/wifi/station.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../frameworks/services/drive/include/osal \
	$(LOCAL_PATH)/../../system/mdp/sample/common \
	$(LOCAL_PATH)/app/http \
	$(LOCAL_PATH)/app/protocal \
	$(LOCAL_PATH)/recorder \
	$(LOCAL_PATH)/vehicle \
	$(LOCAL_PATH)/vehicle/gps \
	$(LOCAL_PATH)/vehicle/4g \
	$(LOCAL_PATH)/vehicle/led \
	$(LOCAL_PATH)/vehicle/update \
	$(LOCAL_PATH)/vehicle/wifi \
	$(LOCAL_PATH)/common/inc/utils \
	$(LOCAL_PATH)/common/inc/socket \
	$(LOCAL_PATH)/common/inc/epoll \
	$(LOCAL_PATH)/common/inc/ipc \
	$(LOCAL_PATH)/common/inc/mediadb \
	$(LOCAL_PATH)/common/inc/tts \
	$(LOCAL_PATH)/protocal/jt808/inc \
	$(LOCAL_PATH)/mp4v2 \
	$(MDP_INCLUDES)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../frameworks/media/hal/include

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../frameworks/nn/include

LOCAL_SRC_FILES += \
	rtsp/net/Acceptor.cpp \
	rtsp/net/BufferReader.cpp \
	rtsp/net/BufferWriter.cpp \
	rtsp/net/EpollTaskScheduler.cpp \
	rtsp/net/EventLoop.cpp \
	rtsp/net/Logger.cpp \
	rtsp/net/MemoryManager.cpp \
	rtsp/net/NetInterface.cpp \
	rtsp/net/Pipe.cpp \
	rtsp/net/SelectTaskScheduler.cpp \
	rtsp/net/SocketUtil.cpp \
	rtsp/net/TaskScheduler.cpp \
	rtsp/net/TcpConnection.cpp \
	rtsp/net/TcpServer.cpp \
	rtsp/net/TcpSocket.cpp \
	rtsp/net/Timer.cpp \
	rtsp/net/Timestamp.cpp

LOCAL_SRC_FILES += \
	rtsp/xop/AACSource.cpp \
	rtsp/xop/DigestAuthentication.cpp \
	rtsp/xop/G711ASource.cpp \
	rtsp/xop/H264Parser.cpp \
	rtsp/xop/H264Source.cpp \
	rtsp/xop/H265Source.cpp \
	rtsp/xop/MediaSession.cpp \
	rtsp/xop/RtpConnection.cpp \
	rtsp/xop/RtspConnection.cpp \
	rtsp/xop/RtspMessage.cpp \
	rtsp/xop/RtspPusher.cpp \
	rtsp/xop/RtspServer.cpp

LOCAL_SRC_FILES += \
	rtsp/rtsp_main.cpp
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/rtsp \
	$(LOCAL_PATH)/rtsp/3rdpart \
	$(LOCAL_PATH)/rtsp/net \
	$(LOCAL_PATH)/rtsp/xop

# 注释此行如果你希望保留符合和调试信息
LOCAL_STRIP_MODULE := true

# C编译器选项
LOCAL_CFLAGS := -g -O0 -Wno-unused-variable -DANDROID_LOGCAT -Wno-unused-value -rdynamic

# C++编译器选项
LOCAL_CPPFLAGS :=

# 链接器选项
# 如果需要引用sysroot中的库，请在此指定，例如引用pthread和math
LOCAL_LDFLAGS += -lpthread -ldl -lsqlite3 -lcurl -lasound -lmp4v2

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := \
	$(MDP_SHARED_LIBRARIES) \
	$(MDP_CAMERA_LIBRARIES)

LOCAL_SHARED_LIBRARIES += \
	libvehicle_base	\
	libdtbased_tk \
	libnn \
	libnn_base	\
	libopenvx \
	libopenvx-nn \
	libnn_facebase \
	libdms \
	libnn_liveness \
	libadas_nn

LOCAL_SHARED_LIBRARIES += liblog libmdp_adaptor libmuxer libsys_sound liblombo_malloc libalc libdrm libosal

include $(BUILD_EXECUTABLE)
