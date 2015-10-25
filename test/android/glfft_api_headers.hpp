/* Copyright (C) 2015 Hans-Kristian Arntzen <maister@archlinux.us>
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef GLFFT_API_HEADERS_HPP__
#define GLFFT_API_HEADERS_HPP__

#include <GLES3/gl31.h>
#include <android/log.h>
#include <time.h>
#include <stdarg.h>

static inline void glfft_android_log(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, "GLFFT", fmt, va);
    va_end(va);
}

static inline double glfft_get_time()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + 1e-9 * ts.tv_nsec;
}

#define GLFFT_GLSL_LANG_STRING "#version 310 es\n"
#define GLFFT_LOG_OVERRIDE glfft_android_log 
#define GLFFT_TIME_OVERRIDE glfft_get_time 

#endif

