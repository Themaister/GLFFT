LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -std=c++11 -Wall -Wextra
LOCAL_MODULE := GLFFT
LOCAL_SRC_FILES := $(addprefix ../,$(wildcard *.cpp))
LOCAL_CPP_FEATURES := exceptions
LOCAL_ARM_MODE := arm
LOCAL_LDLIBS := -lGLESv3 -llog
LOCAL_C_INCLUDES := test/android

LOCAL_GLSL := $(wildcard glsl/*.comp)
LOCAL_GLSL_INC := $(patsubst %.comp,%.inc,$(LOCAL_GLSL))
$(LOCAL_PATH)/../glfft.cpp: $(LOCAL_GLSL_INC)

%.inc: %.comp
	glsl/shader_to_inc.sh $< $@

include $(BUILD_SHARED_LIBRARY)
