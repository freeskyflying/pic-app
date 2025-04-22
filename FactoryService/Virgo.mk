LOCAL_PATH := $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)
LOCAL_MODULE := FactoryService

CODE_PATH=RecorderService

LOCAL_SRC_FILES += \
	main.c \
	code/http_handle.c \
	code/tfcard_test.c \
	code/sos_test.c \
	code/wifikey_test.c \
	code/net4g_test.c \
	code/acc_test.c \
	code/gsensor_test.c \
	code/gps_test.c \
	code/rtc_test.c \
	code/mic_test.c \
	code/wifi_test.c \
	code/camera_test.c

LOCAL_SRC_FILES += \
	../$(CODE_PATH)/app/http/base64.c \
	../$(CODE_PATH)/app/http/cJSON.c \
	../$(CODE_PATH)/app/http/JSON_checker.c \
	../$(CODE_PATH)/app/http/mongoose.c \
	../$(CODE_PATH)/app/protocal/controller.c

LOCAL_SRC_FILES += \
	../$(CODE_PATH)/common/src/utils/ftpUtil.c \
	../$(CODE_PATH)/common/src/utils/pnt_log.c \
	../$(CODE_PATH)/common/src/utils/queue_list.c \
	../$(CODE_PATH)/common/src/utils/strnormalize.c \
	../$(CODE_PATH)/common/src/utils/utils.c \
	../$(CODE_PATH)/common/src/utils/md5.c \
	../$(CODE_PATH)/common/src/socket/pnt_socket.c \
	../$(CODE_PATH)/common/src/epoll/pnt_epoll.c \
	../$(CODE_PATH)/common/src/ipc/pnt_ipc.c \
	../$(CODE_PATH)/common/src/ipc/pnt_ipc_stream.c \
	../$(CODE_PATH)/common/src/mediadb/normal_record_db.c \
	../$(CODE_PATH)/common/src/tts/baidu/common.c \
	../$(CODE_PATH)/common/src/tts/baidu/token.c \
	../$(CODE_PATH)/common/src/tts/baidu/ttscurl.c \
	../$(CODE_PATH)/common/src/tts/baidu/ttsmain.c \
	../$(CODE_PATH)/common/src/utils/system_param.c \
	../$(CODE_PATH)/common/src/utils/alsa_utils.c \
	../$(CODE_PATH)/vehicle/key.c \
	../$(CODE_PATH)/vehicle/adc.c \
	../$(CODE_PATH)/vehicle/led/gpio_led.c \
	../$(CODE_PATH)/vehicle/4g/rild.c \
	../$(CODE_PATH)/vehicle/4g/rild_quectel_800G.c \
	../$(CODE_PATH)/vehicle/4g/rild_yuga_clm920.c \
	../$(CODE_PATH)/vehicle/4g/rild_quectel_EC200A.c \
	../$(CODE_PATH)/vehicle/gsensor/sc7a20.c \
	../$(CODE_PATH)/vehicle/gps/nmea.c \
	../$(CODE_PATH)/recorder/media_snap.c \
	../$(CODE_PATH)/recorder/media_osd.c \
	../$(CODE_PATH)/recorder/font_utils.c \
	../$(CODE_PATH)/recorder/media_audio.c \
	../$(CODE_PATH)/encrypt/encrypt.c \
	../../system/mdp/sample/common/sample_comm_audio.c \
	../../system/mdp/sample/common/sample_comm_sys.c \

LOCAL_SRC_FILES += \
	../$(CODE_PATH)/vehicle/gsensor/icm42607/inv_imu_apex.c \
	../$(CODE_PATH)/vehicle/gsensor/icm42607/inv_imu_driver.c \
	../$(CODE_PATH)/vehicle/gsensor/icm42607/inv_imu_main.c \
	../$(CODE_PATH)/vehicle/gsensor/icm42607/inv_imu_platform.c \
	../$(CODE_PATH)/vehicle/gsensor/icm42607/inv_imu_selftest.c \
	../$(CODE_PATH)/vehicle/gsensor/icm42607/inv_imu_transport.c \
	../$(CODE_PATH)/vehicle/gsensor/icm42607/Invn/EmbUtils/Message.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../frameworks/services/drive/include/osal \
	$(LOCAL_PATH)/../../system/mdp/sample/common \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/utils \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/socket \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/epoll \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/ipc \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/mediadb \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/tts \
	$(LOCAL_PATH)/../$(CODE_PATH)/algorithm/adas \
	$(LOCAL_PATH)/../$(CODE_PATH)/recorder \
	$(LOCAL_PATH)/../$(CODE_PATH)/algorithm/dms \
	$(LOCAL_PATH)/../$(CODE_PATH)/algorithm \
	$(LOCAL_PATH)/../$(CODE_PATH)/app/http \
	$(LOCAL_PATH)/../$(CODE_PATH)/app/protocal \
	$(LOCAL_PATH)/../$(CODE_PATH)/vehicle \
	$(LOCAL_PATH)/../$(CODE_PATH)/vehicle/led \
	$(LOCAL_PATH)/../$(CODE_PATH)/vehicle/4g \
	$(LOCAL_PATH)/../$(CODE_PATH)/vehicle/gsensor \
	$(LOCAL_PATH)/../$(CODE_PATH)/vehicle/gps \
	$(LOCAL_PATH)/../$(CODE_PATH)/vehicle/gsensor/icm42607 \
	$(LOCAL_PATH)/../$(CODE_PATH)/encrypt \
	$(LOCAL_PATH)/code \
	$(MDP_INCLUDES)

LOCAL_SRC_FILES += \
	../$(CODE_PATH)/rtsp/net/Acceptor.cpp \
	../$(CODE_PATH)/rtsp/net/BufferReader.cpp \
	../$(CODE_PATH)/rtsp/net/BufferWriter.cpp \
	../$(CODE_PATH)/rtsp/net/EpollTaskScheduler.cpp \
	../$(CODE_PATH)/rtsp/net/EventLoop.cpp \
	../$(CODE_PATH)/rtsp/net/Logger.cpp \
	../$(CODE_PATH)/rtsp/net/MemoryManager.cpp \
	../$(CODE_PATH)/rtsp/net/NetInterface.cpp \
	../$(CODE_PATH)/rtsp/net/Pipe.cpp \
	../$(CODE_PATH)/rtsp/net/SelectTaskScheduler.cpp \
	../$(CODE_PATH)/rtsp/net/SocketUtil.cpp \
	../$(CODE_PATH)/rtsp/net/TaskScheduler.cpp \
	../$(CODE_PATH)/rtsp/net/TcpConnection.cpp \
	../$(CODE_PATH)/rtsp/net/TcpServer.cpp \
	../$(CODE_PATH)/rtsp/net/TcpSocket.cpp \
	../$(CODE_PATH)/rtsp/net/Timer.cpp \
	../$(CODE_PATH)/rtsp/net/Timestamp.cpp

LOCAL_SRC_FILES += \
	../$(CODE_PATH)/rtsp/xop/AACSource.cpp \
	../$(CODE_PATH)/rtsp/xop/DigestAuthentication.cpp \
	../$(CODE_PATH)/rtsp/xop/G711ASource.cpp \
	../$(CODE_PATH)/rtsp/xop/H264Parser.cpp \
	../$(CODE_PATH)/rtsp/xop/H264Source.cpp \
	../$(CODE_PATH)/rtsp/xop/H265Source.cpp \
	../$(CODE_PATH)/rtsp/xop/MediaSession.cpp \
	../$(CODE_PATH)/rtsp/xop/RtpConnection.cpp \
	../$(CODE_PATH)/rtsp/xop/RtspConnection.cpp \
	../$(CODE_PATH)/rtsp/xop/RtspMessage.cpp \
	../$(CODE_PATH)/rtsp/xop/RtspPusher.cpp \
	../$(CODE_PATH)/rtsp/xop/RtspServer.cpp

LOCAL_SRC_FILES += \
	../$(CODE_PATH)/rtsp/rtsp_main_2ch.cpp
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../$(CODE_PATH)/rtsp \
	$(LOCAL_PATH)/../$(CODE_PATH)/rtsp/3rdpart \
	$(LOCAL_PATH)/../$(CODE_PATH)/rtsp/net \
	$(LOCAL_PATH)/../$(CODE_PATH)/rtsp/xop
	
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../frameworks/media/hal/include

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../frameworks/nn/include

# 注释此行如果你希望保留符合和调试信息
LOCAL_STRIP_MODULE := true

# C编译器选项
include apps/VirgoCommon.param

# 链接器选项
# 如果需要引用sysroot中的库，请在此指定，例如引用pthread和math
LOCAL_LDFLAGS += -lpthread -ldl -lsqlite3 -lcurl -lasound

#LOCAL_MODULE_TAGS := optional
# 注释此行如果你希望保留符合和调试信息
#LOCAL_STRIP_MODULE := false

LOCAL_CFLAGS += -g

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
