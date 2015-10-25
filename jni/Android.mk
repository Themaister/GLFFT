LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -std=c++11 -Wall -Wextra
LOCAL_MODULE := GLFFT
LOCAL_SRC_FILES := $(addprefix ../,$(wildcard *.cpp))
LOCAL_CPP_FEATURES := exceptions
LOCAL_ARM_MODE := arm
LOCAL_LDLIBS := -lGLESv3 -llog
LOCAL_C_INCLUDES := test/android

include $(BUILD_SHARED_LIBRARY)
