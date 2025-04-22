LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := VehicleService

LOCAL_SRC_FILES += \
	main.c \
	key.c

LOCAL_SRC_FILES += \
	update/upgrade.c

LOCAL_SRC_FILES += \
	ftpUpdate/ftp_update.c \
	ftpUpdate/updateConfig.c \
	ftpUpdate/updateHandler.c \
	ftpUpdate/utils/base64.c \
	ftpUpdate/utils/cJSON.c \
	ftpUpdate/utils/JSON_checker.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/ftpUpdate \
	$(LOCAL_PATH)/ftpUpdate/utils

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
	../RecorderService/encrypt/encrypt.c \
	../RecorderService/common/src/utils/alsa_utils.c \
	../RecorderService/vehicle/adc.c
	
LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../frameworks/services/drive/include/osal \
	$(LOCAL_PATH)/../RecorderService/common/inc/utils \
	$(LOCAL_PATH)/../RecorderService/common/inc/socket \
	$(LOCAL_PATH)/../RecorderService/common/inc/epoll \
	$(LOCAL_PATH)/../RecorderService/common/inc/ipc \
	$(LOCAL_PATH)/../RecorderService/common/inc/mediadb \
	$(LOCAL_PATH)/../RecorderService/common/inc/tts \
	$(LOCAL_PATH)/../RecorderService/encrypt \
	$(LOCAL_PATH)/../RecorderService/vehicle \
	$(LOCAL_PATH)/update

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../frameworks/media/hal/include

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
LOCAL_SHARED_LIBRARIES := 

LOCAL_SHARED_LIBRARIES += liblog liblombo_malloc libalc libdrm libosal

include $(BUILD_EXECUTABLE)
