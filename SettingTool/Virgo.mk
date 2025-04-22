LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := SettingTool

CODE_PATH=RecorderService

LOCAL_SRC_FILES += \
	main.cpp \
	tinyxml2/tinyxml2.cpp \
	systemNodeParser.cpp \
	ftpUpdateNodeParser.cpp \
	algoNodeParser.cpp \
	serversNodeParser.cpp

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

# ע�ʹ��������ϣ���������Ϻ͵�����Ϣ
LOCAL_STRIP_MODULE := true

# C������ѡ��
include apps/VirgoCommon.param


# ������ѡ��
# �����Ҫ����sysroot�еĿ⣬���ڴ�ָ������������pthread��math
LOCAL_LDFLAGS += -lpthread -ldl -lsqlite3 -lcurl -lasound

#LOCAL_MODULE_TAGS := optional
# ע�ʹ��������ϣ���������Ϻ͵�����Ϣ
#LOCAL_STRIP_MODULE := false

LOCAL_CFLAGS += -g

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := 

LOCAL_SHARED_LIBRARIES += liblog liblombo_malloc libalc libdrm libosal

include $(BUILD_EXECUTABLE)
