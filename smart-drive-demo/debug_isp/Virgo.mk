LOCAL_PATH := $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)
LOCAL_MODULE := debug_isp
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	../../../system/mdp/sample/common/sample_comm_doss.c \
	../../../system/mdp/sample/common/sample_comm_sys.c \
	../../../system/mdp/sample/common/sample_comm_viss.c \
	../../../system/mdp/sample/common/sample_comm_isp.c \
	../../../system/mdp/sample/common/sample_comm_ippu.c \
	../../../system/mdp/sample/common/sample_comm_venc.c \
	debug_isp.c

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../system/mdp/sample/common \
	$(MDP_INCLUDES)
LOCAL_CFLAGS :=
LOCAL_CPPFLAGS :=
LOCAL_LDFLAGS := -lpthread
LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := \
	$(MDP_SHARED_LIBRARIES) \
	$(MDP_CAMERA_LIBRARIES) \
	$(MDP_PANEL_LIBRARIES) \
	$(MDP_CAMERA_EXT_LIBRARIES) \
	liblog
include $(BUILD_EXECUTABLE)
