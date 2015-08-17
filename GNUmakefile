TARGET := glfft_cli

BACKEND := glfw

EXTERNAL_INCLUDE_DIRS := -ImuFFT -Irapidjson/include -I. -Itest -Itest/$(BACKEND)
EXTERNAL_LIB_DIRS := -LmuFFT
EXTERNAL_LIBS := -lmufft
MUFFT_LIB := muFFT/libmufft.a

ifeq ($(PLATFORM),)
	PLATFORM = unix
	ifeq ($(shell uname -a),)
		PLATFORM = win
	else ifneq ($(findstring MINGW,$(shell uname -a)),)
		PLATFORM = win
	else ifneq ($(findstring Darwin,$(shell uname -a)),)
		PLATFORM = osx
	else ifneq ($(findstring win,$(shell uname -a)),)
		PLATFORM = win
	endif
endif

ifeq ($(PLATFORM),win)
	CC = gcc
	CXX = g++
endif

ifeq ($(BACKEND), glfw)
	ifeq ($(PLATFORM),win)
		LDFLAGS += -Lexternal/lib/win-x64 -lglfw3 -lopengl32 -lgdi32
		EXTERNAL_INCLUDE_DIRS += -Iexternal/include
	else
		LDFLAGS += -lmufft $(shell pkg-config glfw3 --libs) -lGL
	endif
	GLSYM := test/glfw/glsym/rglgen.c test/glfw/glsym/glsym_gl.c

	CC = clang
	CXX = clang++
endif

ifeq ($(DEBUG_SANITIZE), 1)
	CXXFLAGS += -O0 -g -fsanitize=memory -DGLFFT_GL_DEBUG
	CFLAGS += -O0 -g -fsanitize=memory -DGLFFT_GL_DEBUG
	LDFLAGS += -fsanitize=memory
else ifeq ($(DEBUG), 1)
	CXXFLAGS += -O0 -g -DGLFFT_GL_DEBUG
	CFLAGS += -O0 -g -DGLFFT_GL_DEBUG
else
	CXXFLAGS += -Ofast -g
	CFLAGS += -Ofast -g
endif

ifneq ($(TOOLCHAIN_PREFIX),)
	CC = $(TOOLCHAIN_PREFIX)gcc
	CXX = $(TOOLCHAIN_PREFIX)g++
endif

CXX_SOURCES := $(wildcard *.cpp) $(wildcard test/*.cpp) $(wildcard test/$(BACKEND)/*.cpp)
C_SOURCES := $(GLSYM)
OBJDIR := obj
OBJECTS := $(addprefix $(OBJDIR)/,$(CXX_SOURCES:.cpp=.o)) $(addprefix $(OBJDIR)/,$(C_SOURCES:.c=.o))
DEPS := $(OBJECTS:.o=.d)

CXXFLAGS += -Wall -Wextra -pedantic -std=gnu++11 $(EXTERNAL_INCLUDE_DIRS)
CFLAGS += -Wall -Wextra -std=gnu99 $(EXTERNAL_INCLUDE_DIRS)
LDFLAGS += $(EXTERNAL_LIB_DIRS)


all: $(TARGET)

glfft.cpp: build_fft_inc

build_fft_inc:
	$(MAKE) -C glsl

-include $(DEPS)

muFFT/libmufft.a:
	$(MAKE) -C muFFT static

$(TARGET): $(OBJECTS) $(MUFFT_LIB)
	$(CXX) -o $@ $(OBJECTS) $(LDFLAGS) $(EXTERNAL_LIBS)

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $< $(CXXFLAGS) -MMD

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS) -MMD

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: clean
