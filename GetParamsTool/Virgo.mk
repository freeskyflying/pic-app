LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := GetParamsTool

CODE_PATH=RecorderService

LOCAL_SRC_FILES += \
	main.cpp \
	tinyxml2/tinyxml2.cpp

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
	../$(CODE_PATH)/algorithm/adas/adas_config.c \
	../$(CODE_PATH)/algorithm/dms/dms_config.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/utils \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/socket \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/epoll \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/ipc \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/mediadb \
	$(LOCAL_PATH)/../$(CODE_PATH)/common/inc/tts \
	$(LOCAL_PATH)/../$(CODE_PATH)/algorithm/adas \
	$(LOCAL_PATH)/../$(CODE_PATH)/algorithm/dms \
	$(LOCAL_PATH)/tinyxml2

# 注释此行如果你希望保留符合和调试信息
LOCAL_STRIP_MODULE := true

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
	LOCAL_CFLAGS += -DCAM2CH -DDINGDINGPAI
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
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_pdc2_losangle)
	LOCAL_CFLAGS += -DENGLISH -DCAM2CH -DDISABLECH3
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_pdc2_nigeria)
	LOCAL_CFLAGS += -DENGLISH -DCAM2CH -DDISABLECH3
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_pdc2_vietnam)
	LOCAL_CFLAGS += -DENGLISH -DCAM2CH -DDISABLECH3
else ifeq ($(TARGET_PRODUCT), n7v3_pnt_223_pdc2_3ch_vietnam)
	LOCAL_CFLAGS += -DENGLISH
endif

# C编译器选项
LOCAL_CFLAGS := -g -O0 -Wno-unused-variable -DANDROID_LOGCAT -Wno-unused-value -DNO_MEDIA

# 链接器选项
# 如果需要引用sysroot中的库，请在此指定，例如引用pthread和math
LOCAL_LDFLAGS += -lpthread -ldl -lsqlite3 -lcurl

#LOCAL_MODULE_TAGS := optional
# 注释此行如果你希望保留符合和调试信息
#LOCAL_STRIP_MODULE := false

LOCAL_CFLAGS += -g

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := 

LOCAL_SHARED_LIBRARIES += liblog liblombo_malloc libalc libdrm libosal

include $(BUILD_EXECUTABLE)
