LOCAL_PATH := $(call my-dir)

include system/mdp/Virgo.param

include $(CLEAR_VARS)

LOCAL_MODULE:= mdp_facerecg_nn
#LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES += mdp_facerecg_nn.c
LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../../system/mdp/sample/common \
	$(LOCAL_PATH)/../../../frameworks/nn/include \
	$(LOCAL_PATH)/../../../frameworks/media/hal/include \
	$(LOCAL_PATH)/../../../frameworks/services/drive/include/osal \
	$(MDP_INCLUDES)

# ע�ʹ��������ϣ���������Ϻ͵�����Ϣ
LOCAL_STRIP_MODULE := false

# C������ѡ��
LOCAL_CFLAGS := -g

# C++������ѡ��
LOCAL_CPPFLAGS :=

# ������ѡ��
# �����Ҫ����sysroot�еĿ⣬���ڴ�ָ������������pthread��math
LOCAL_LDFLAGS :=
LOCAL_LDFLAGS +=-lrt -lpthread -lm -lsqlite3

# �����Ķ�̬��
LOCAL_SHARED_LIBRARIES := \
	$(MDP_SHARED_LIBRARIES) \
	$(MDP_CAMERA_LIBRARIES) \
	$(MDP_PANEL_LIBRARIES) \
	$(MDP_CAMERA_EXT_LIBRARIES)

LOCAL_SHARED_LIBRARIES += \
        libdtbased_tk \
        libnn \
        libnn_base \
        libopenvx \
        libopenvx-nn \
        libnn_facebase \
        libdms_facerecg

LOCAL_SHARED_LIBRARIES += liblog libmdp_adaptor liblombo_malloc libomx libalc libdrm libosal
include $(BUILD_EXECUTABLE)
