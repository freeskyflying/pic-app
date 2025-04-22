LOCAL_PATH:= $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)

LOCAL_MODULE := mdp_vdec

#LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES += mdp_vdec.c
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../system/mdp/sample/common \
	$(LOCAL_PATH)/../../../frameworks/media/hal/include \
	$(LOCAL_PATH)/../../../frameworks/services/drive/include/osal \
	$(MDP_INCLUDES)

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

LOCAL_SHARED_LIBRARIES += liblog libmdp_adaptor liblombo_malloc libomx libalc libdrm libosal
include $(BUILD_EXECUTABLE)

