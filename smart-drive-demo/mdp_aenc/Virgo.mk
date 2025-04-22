LOCAL_PATH := $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)
LOCAL_MODULE := mdp_aenc
LOCAL_MODULE_TAGS := optional
LOCAL_SRC_FILES := \
	../../../system/mdp/sample/common/sample_comm_audio.c \
	../../../system/mdp/sample/common/sample_comm_sys.c \
	mdp_aenc.c
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../../../system/mdp/sample/common	\
	$(MDP_INCLUDES) 
	
LOCAL_CFLAGS :=
LOCAL_CPPFLAGS :=
LOCAL_LDFLAGS := -lpthread -ldl
LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := \
	$(MDP_SHARED_LIBRARIES) \
	libi2cops \
	liblog
include $(BUILD_EXECUTABLE)
