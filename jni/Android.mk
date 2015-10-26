LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -std=c99 -Wall -Wextra
LOCAL_MODULE := muFFT
LOCAL_SRC_FILES := $(addprefix ../,muFFT/fft.c muFFT/kernel.c muFFT/cpu.c)
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES := muFFT
LOCAL_EXPORT_C_INCLUDES := muFFT

include $(BUILD_STATIC_LIBRARY)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -std=c++11 -Wall -Wextra -DGLFFT_CLI_ASYNC
LOCAL_MODULE := GLFFT
LOCAL_SRC_FILES := $(addprefix ../,$(wildcard *.cpp) test/android/jni.cpp test/glfft_cli.cpp test/glfft_test.cpp)
LOCAL_CPP_FEATURES := exceptions
LOCAL_ARM_MODE := arm
LOCAL_LDLIBS := -lGLESv3 -llog -lEGL -lm
LOCAL_C_INCLUDES := test/android test muFFT .

LOCAL_STATIC_LIBRARIES := muFFT

LOCAL_GLSL := $(wildcard glsl/*.comp)
LOCAL_GLSL_INC := $(patsubst %.comp,%.inc,$(LOCAL_GLSL))
$(LOCAL_PATH)/../glfft.cpp: $(LOCAL_GLSL_INC)

%.inc: %.comp
	glsl/shader_to_inc.sh $< $@

include $(BUILD_SHARED_LIBRARY)
