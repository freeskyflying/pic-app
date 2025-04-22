

LOCAL_PATH := $(call my-dir)
include system/mdp/Virgo.param
include apps/smart-drive-demo/uvc_demo/resource.mk
include $(CLEAR_VARS)

LOCAL_CFLAGS		:=
LOCAL_LDFLAGS		:=
LOCAL_C_INCLUDES	:=
LOCAL_SHARED_LIBRARIES	:=

## media platform
# mdp
# omx
# test
MEDIA_PLATFORM := test

LOCAL_MODULE := uvc_demo

SRC_LIST := $(wildcard $(LOCAL_PATH)/*.c)


LOCAL_SRC_FILES := $(SRC_LIST:$(LOCAL_PATH)/%=%)
LOCAL_SRC_FILES += \
	../../../system/mdp/sample/common/sample_comm_ippu.c \
	../../../system/mdp/sample/common/sample_comm_venc.c \
	../../../system/mdp/sample/common/sample_comm_sys.c \
	../../../system/mdp/sample/common/sample_comm_isp.c \
	../../../system/mdp/sample/common/sample_comm_viss.c

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/include/ \
	frameworks/protocols/libuvcg/include \
	frameworks/media/hal/include \
	frameworks/services/drive/include/osal \
	system/mdp/sample/common \
	$(MDP_INCLUDES)

LOCAL_SHARED_LIBRARIES += \
	liblog \
	libuvcg

LOCAL_CFLAGS += -std=gnu99
LOCAL_CFLAGS += -W -Wall

LOCAL_LDFLAGS += -lpthread

# for gdb debug
# LOCAL_CFLAGS += -g
# LOCAL_STRIP_MODULE = false

# LOCAL_LDFLAGS += -Wl,-Map,uvcgadget.map

LOCAL_SHARED_LIBRARIES += \
	$(MDP_SHARED_LIBRARIES) \
	$(MDP_CAMERA_LIBRARIES) \
	$(MDP_PANEL_LIBRARIES) \
	$(MDP_CAMERA_EXT_LIBRARIES) \
	libmuxer liblombo_malloc libalc
	
include $(BUILD_EXECUTABLE)

