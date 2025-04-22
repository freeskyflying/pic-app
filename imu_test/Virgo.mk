LOCAL_PATH := $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)
LOCAL_MODULE := imu_test
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	main.c \
#	IMU/imu.c \
#	IMU/kalman.c \
#	IMU/filter.c \
#	IMU/myMath.c
#	angle_kalmen_filter.c

LOCAL_SRC_FILES += \
	../RecorderService/common/src/utils/ftpUtil.c \
	../RecorderService/common/src/utils/pnt_log.c \
	../RecorderService/common/src/utils/queue_list.c \
	../RecorderService/common/src/utils/strnormalize.c \
	../RecorderService/common/src/utils/utils.c \
	../RecorderService/common/src/utils/md5.c \
	../RecorderService/common/src/socket/pnt_socket.c \
	../RecorderService/common/src/epoll/pnt_epoll.c \
	../RecorderService/common/src/ipc/pnt_ipc.c \
	../RecorderService/common/src/ipc/pnt_ipc_stream.c \
	../RecorderService/common/src/mediadb/normal_record_db.c \
	../RecorderService/common/src/tts/baidu/common.c \
	../RecorderService/common/src/tts/baidu/token.c \
	../RecorderService/common/src/tts/baidu/ttscurl.c \
	../RecorderService/common/src/tts/baidu/ttsmain.c \
	../RecorderService/common/src/utils/system_param.c \
	../RecorderService/common/src/utils/alsa_utils.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../RecorderService/algorithm \
	$(LOCAL_PATH)/../RecorderService/algorithm/adas \
	$(LOCAL_PATH)/../RecorderService/algorithm/dms \
	$(LOCAL_PATH)/../RecorderService/algorithm/bsd

LOCAL_SRC_FILES += \
	../RecorderService/vehicle/gsensor/icm42607/inv_imu_apex.c \
	../RecorderService/vehicle/gsensor/icm42607/inv_imu_driver.c \
	../RecorderService/vehicle/gsensor/icm42607/inv_imu_main.c \
	../RecorderService/vehicle/gsensor/icm42607/inv_imu_platform.c \
	../RecorderService/vehicle/gsensor/icm42607/inv_imu_selftest.c \
	../RecorderService/vehicle/gsensor/icm42607/inv_imu_transport.c \
	../RecorderService/vehicle/gsensor/icm42607/Invn/EmbUtils/Message.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../RecorderService/vehicle/gsensor/icm42607

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../frameworks/services/drive/include/osal \
	$(LOCAL_PATH)/../../system/mdp/sample/common \
	$(LOCAL_PATH)/../RecorderService/app/http \
	$(LOCAL_PATH)/../RecorderService/app/protocal \
	$(LOCAL_PATH)/../RecorderService/recorder \
	$(LOCAL_PATH)/../RecorderService/vehicle \
	$(LOCAL_PATH)/../RecorderService/vehicle/gps \
	$(LOCAL_PATH)/../RecorderService/vehicle/4g \
	$(LOCAL_PATH)/../RecorderService/vehicle/led \
	$(LOCAL_PATH)/../RecorderService/vehicle/update \
	$(LOCAL_PATH)/../RecorderService/vehicle/wifi \
	$(LOCAL_PATH)/../RecorderService/common/inc/utils \
	$(LOCAL_PATH)/../RecorderService/common/inc/socket \
	$(LOCAL_PATH)/../RecorderService/common/inc/epoll \
	$(LOCAL_PATH)/../RecorderService/common/inc/ipc \
	$(LOCAL_PATH)/../RecorderService/common/inc/mediadb \
	$(LOCAL_PATH)/../RecorderService/common/inc/tts \
	$(LOCAL_PATH)/../RecorderService/protocal/jt808/inc \
	$(LOCAL_PATH)/../RecorderService/protocal/mqtt_ftp \
	$(LOCAL_PATH)/../RecorderService/protocal \
	$(LOCAL_PATH)/../RecorderService/mp4v2 \
	$(MDP_INCLUDES)

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../frameworks/media/hal/include

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../frameworks/nn/include
	
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../RecorderService/encrypt

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../RecorderService/rtsp \
	$(LOCAL_PATH)/../RecorderService/rtsp/3rdpart \
	$(LOCAL_PATH)/../RecorderService/rtsp/net \
	$(LOCAL_PATH)/../RecorderService/rtsp/xop

# 注释此行如果你希望保留符合和调试信息
LOCAL_STRIP_MODULE := true

# C编译器选项
LOCAL_CFLAGS := -g -O0 -Wno-unused-variable -DANDROID_LOGCAT -Wno-unused-value -rdynamic

ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_india)
	LOCAL_CFLAGS += -DINDIA -DINDIA_2CH -DDISABLECH3 -DENGLISH
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_dmsmodule)
	LOCAL_CFLAGS += -DONLY_3rdCH -DDISABLEADAS -DDISABLECH1 -DDISABLECH2
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_3ch_india)
	LOCAL_CFLAGS += -DINDIA -DENGLISH
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_ch13_india)
	LOCAL_CFLAGS += -DINDIA -DINDIA_2CH -DDISABLEDMS -DDISABLECH2 -DENGLISH
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_spanish)
	LOCAL_CFLAGS += -DSPANISH
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_pdc2_iran)
	LOCAL_CFLAGS += -DENGLISH
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_pdc2_urugay)
	LOCAL_CFLAGS += -DSPANISH -DCH3MIRRO
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_london)
	LOCAL_CFLAGS += -DENGLISH -DCAM2CH -DDISABLECH3 -DVDENCRYPT
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_ddp)
	LOCAL_CFLAGS += -DCAM2CH -DDISABLECH3 -DDINGDINGPAI
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_losangle)
	LOCAL_CFLAGS += -DENGLISH
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_pdc2_3ch_india)
	LOCAL_CFLAGS += -DINDIA -DENGLISH
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_indiaT)
	LOCAL_CFLAGS += -DINDIA -DINDIA_2CH -DDISABLECH3 -DENGLISH
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_3ch_indiaT)
	LOCAL_CFLAGS += -DINDIA -DENGLISH
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_ch13_indiaT)
	LOCAL_CFLAGS += -DINDIA -DINDIA_2CH -DDISABLEDMS -DDISABLECH2 -DENGLISH
endif

# C++编译器选项
LOCAL_CPPFLAGS :=

# 链接器选项
# 如果需要引用sysroot中的库，请在此指定，例如引用pthread和math
LOCAL_LDFLAGS += -lpthread -ldl -lsqlite3 -lcurl -lasound -lmp4v2 -lpaho-mqtt3a

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
