LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := VehicleService

LOCAL_SRC_FILES += \
	main.c

LOCAL_SRC_FILES += \
	gps/gpsReader.c \
	gps/nmea.c

LOCAL_SRC_FILES += \
	4g/rild.c

LOCAL_SRC_FILES += \
	led/gpio_led.c

LOCAL_SRC_FILES += \
	update/upgrade.c

LOCAL_SRC_FILES += \
	wifi/hostap.c \
	wifi/station.c

LOCAL_C_INCLUDES += $(LOCAL_PATH)

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../frameworks/services/drive/include/osal \
	$(LOCAL_PATH)/../../frameworks/libpnt/inc/utils \
	$(LOCAL_PATH)/../../frameworks/libpnt/inc/socket \
	$(LOCAL_PATH)/../../frameworks/libpnt/inc/epoll \
	$(LOCAL_PATH)/../../frameworks/libpnt/inc/ipc \
	$(LOCAL_PATH)/../../frameworks/libpnt/inc/mediadb \
	$(LOCAL_PATH)/../../frameworks/libpnt/inc/tts \
	$(LOCAL_PATH)/gps \
	$(LOCAL_PATH)/4g \
	$(LOCAL_PATH)/led \
	$(LOCAL_PATH)/update \
	$(LOCAL_PATH)/wifi

LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../frameworks/media/hal/include

# ע�ʹ��������ϣ���������Ϻ͵�����Ϣ
LOCAL_STRIP_MODULE := true

# C������ѡ��
LOCAL_CFLAGS := -g -O0 -Wno-unused-variable -DANDROID_LOGCAT -Wno-unused-value

# ������ѡ��
# �����Ҫ����sysroot�еĿ⣬���ڴ�ָ������������pthread��math
LOCAL_LDFLAGS += -lpthread -ldl -lsqlite3 -lcurl

#LOCAL_MODULE_TAGS := optional
# ע�ʹ��������ϣ���������Ϻ͵�����Ϣ
#LOCAL_STRIP_MODULE := false

LOCAL_CFLAGS += -g

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := 

LOCAL_SHARED_LIBRARIES += liblog libpnt liblombo_malloc libalc libdrm libosal

include $(BUILD_EXECUTABLE)
