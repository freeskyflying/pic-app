LOCAL_PATH:= $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)

LOCAL_MODULE := sdrive_sample_venc

#LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES += sdrive_sample_venc.c
LOCAL_SRC_FILES += sdrive_sample_muxer.c
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../system/mdp/sample/common \
	$(LOCAL_PATH)/../../../frameworks/nn/include \
	$(LOCAL_PATH)/omx_inc \
	$(MDP_INCLUDES)
LOCAL_C_INCLUDES += frameworks/media/hal/include

# 注释此行如果你希望保留符合和调试信息
LOCAL_STRIP_MODULE := false

# C编译器选项
# LOCAL_CFLAGS := -Wall -Werror -DWS_MG -g

# C++编译器选项
LOCAL_CPPFLAGS :=

# 链接器选项
# 如果需要引用sysroot中的库，请在此指定，例如引用pthread和math
LOCAL_LDFLAGS :=
LOCAL_LDFLAGS += -lpthread

# 依赖的动态库
LOCAL_SHARED_LIBRARIES := \
	$(MDP_SHARED_LIBRARIES) \
	$(MDP_CAMERA_LIBRARIES) \
	$(MDP_PANEL_LIBRARIES) \
	$(MDP_CAMERA_EXT_LIBRARIES)

LOCAL_SHARED_LIBRARIES += liblog libmdp_adaptor libmuxer liblombo_malloc libalc libdrm
include $(BUILD_EXECUTABLE)

