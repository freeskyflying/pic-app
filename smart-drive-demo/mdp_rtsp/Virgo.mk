LOCAL_PATH:= $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)

LOCAL_MODULE := mdp_rtsp


#LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES += mdp_rtsp.c \
	src/ref_queue.cpp \
	src/live_server.cpp \
	src/mem_pool.c \
	src/osa.c \
	src/VideoMediaSubsession.cpp \
	src/live_video.cpp \
	../../../system/mdp/sample/common/sample_comm_isp.c \
	../../../system/mdp/sample/common/sample_comm_sys.c \
	../../../system/mdp/sample/common/sample_comm_video.c \
	../../../system/mdp/sample/common/sample_comm_viss.c \
	../../../system/mdp/sample/common/sample_comm_ippu.c \
	../../../system/mdp/sample/common/sample_comm_venc.c \
	../../../system/mdp/sample/common/sample_comm_vdec.c \
	../../../system/mdp/sample/common/sample_comm_doss.c

LOCAL_C_INCLUDES := \
	external/live555/BasicUsageEnvironment/include \
	external/live555/groupsock/include \
	external/live555/liveMedia/include \
	external/live555/mediaServer \
	external/live555/UsageEnvironment/include \
	frameworks/nn/include \
	frameworks/services/drive/include/osal \
	frameworks/media/hal/include \
	$(MDP_INCLUDES) \
	
LOCAL_C_INCLUDES += \
	system/mdp/sample/common \
	system/madp/code/common \
	system/madp/code/include \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/src \
	$(MDP_INCLUDES) \
	

# 注释此行如果你希望保留符合和调试信息
LOCAL_STRIP_MODULE := true

# C编译器选项
LOCAL_CFLAGS :=  -D_LIVE_BUFFER_LOG_SUPPORT_ -DLIVE_TEST -g -O0

# C++编译器选项
LOCAL_CPPFLAGS :=

# 链接器选项
# 如果需要引用sysroot中的库，请在此指定，例如引用pthread和math
LOCAL_LDFLAGS :=
LOCAL_LDFLAGS += -lrt -lpthread -lm


LOCAL_STATIC_LIBRARIES := \
	libliveMedia \
	libgroupsock \
	libBasicUsageEnvironment \
	libUsageEnvironment \
	$(MDP_STATIC_LIBRARIES) \
	$(MDP_CAMERA_LIBRARIES) \
	$(MDP_CAMERA_EXT_LIBRARIES) \
	$(MDP_PANEL_LIBRARIES) \
	libaudiocommon \
	libupvqe \
# 依赖的动态库
LOCAL_SHARED_LIBRARIES := \
	$(MDP_SHARED_LIBRARIES) \
	$(MDP_CAMERA_LIBRARIES) \
	$(MDP_PANEL_LIBRARIES) \
	$(MDP_CAMERA_EXT_LIBRARIES)

LOCAL_SHARED_LIBRARIES += \
	libdtbased_tk \
	libnn \
	libnn_base	\
	libopenvx \
	libopenvx-nn \
	libnn_facebase \
	libnn_gesture

LOCAL_SHARED_LIBRARIES += liblog libmdp_adaptor liblombo_malloc libomx libalc libdrm libosal
include $(BUILD_EXECUTABLE)

