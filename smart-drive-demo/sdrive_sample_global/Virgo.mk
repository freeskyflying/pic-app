LOCAL_PATH:= $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)

LOCAL_MODULE := sdrive_sample_global

#LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES += sdrive_sample_global.c
LOCAL_SRC_FILES += sdrive_sample_muxer.c \
	mdp_dms_config.c
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../system/mdp/sample/common \
	$(LOCAL_PATH)/../../../frameworks/nn/include \
	$(LOCAL_PATH)/../../../frameworks/media/hal/include \
	$(LOCAL_PATH)/../../../frameworks/services/drive/include/osal \
	$(MDP_INCLUDES)

# 注释此行如果你希望保留符合和调试信息
LOCAL_STRIP_MODULE := false

# C编译器选项
# LOCAL_CFLAGS := -Wall -Werror -DWS_MG -g
LOCAL_CFLAGS := -Wall -Werror -g
LOCAL_CFLAGS := -DLINUX

# C++编译器选项
LOCAL_CPPFLAGS :=

LOCAL_CFLAGS += $(MEDIA_CFLAGS)
LOCAL_STRIP_MODULE := $(MEDIA_STRIP_MODULE)

# 链接器选项
# 如果需要引用sysroot中的库，请在此指定，例如引用pthread和math
LOCAL_LDFLAGS :=
LOCAL_LDFLAGS += -lpthread -ldl -lpng -lrt -lm

# 依赖的动态库
LOCAL_SHARED_LIBRARIES := \
	$(MDP_SHARED_LIBRARIES) \
	$(MDP_CAMERA_LIBRARIES) \
	$(MDP_PANEL_LIBRARIES) \
	$(MDP_CAMERA_EXT_LIBRARIES)
	
# 算法库依赖
LOCAL_SHARED_LIBRARIES += \
	libdtbased_tk \
    libnn \
	libextops	\
	libnn_base	\
    libopenvx \
    libopenvx-nn \
	libnn_facebase	\
	libvehicle_base	\
	libdms	\
	libadas_nn	\
	libbsd \
	libdms_facerecg

LOCAL_SHARED_LIBRARIES += liblog libmdp_adaptor libmuxer liblombo_malloc libomx libalc libdrm libosal
include $(BUILD_EXECUTABLE)

