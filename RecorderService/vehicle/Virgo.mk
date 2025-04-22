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

# 注释此行如果你希望保留符合和调试信息
LOCAL_STRIP_MODULE := true

# C编译器选项
LOCAL_CFLAGS := -g -O0 -Wno-unused-variable -DANDROID_LOGCAT -Wno-unused-value

# 链接器选项
# 如果需要引用sysroot中的库，请在此指定，例如引用pthread和math
LOCAL_LDFLAGS += -lpthread -ldl -lsqlite3 -lcurl

#LOCAL_MODULE_TAGS := optional
# 注释此行如果你希望保留符合和调试信息
#LOCAL_STRIP_MODULE := false

LOCAL_CFLAGS += -g

LOCAL_STATIC_LIBRARIES :=
LOCAL_SHARED_LIBRARIES := 

LOCAL_SHARED_LIBRARIES += liblog libpnt liblombo_malloc libalc libdrm libosal

include $(BUILD_EXECUTABLE)
