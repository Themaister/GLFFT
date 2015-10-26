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

#include "glfft_interface.hpp"
#include "glfft_context.hpp"
#include "glfft_cli.hpp"
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "net_themaister_glfft_Native.h"

#include <memory>
#include <vector>
using namespace std;

struct egl_ctx
{
    EGLContext ctx = EGL_NO_CONTEXT;
    EGLSurface surf = EGL_NO_SURFACE;
    EGLDisplay dpy = EGL_NO_SURFACE;
    EGLConfig conf = 0;

    ~egl_ctx()
    {
        if (dpy)
        {
            eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (ctx)
                eglDestroyContext(dpy, ctx);
            if (surf)
                eglDestroySurface(dpy, surf);
            eglTerminate(dpy);
        }
    }
};

void *GLFFT::Context::create()
{
    auto egl = unique_ptr<egl_ctx>(new egl_ctx);

    static const EGLint attr[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 1,
        EGL_GREEN_SIZE, 1,
        EGL_BLUE_SIZE, 1,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_NONE,
    };

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE,
    };

    static const EGLint surface_attr[] = {
        EGL_WIDTH, 64,
        EGL_HEIGHT, 64,
        EGL_NONE,
    };

    egl->dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl->dpy == EGL_NO_DISPLAY)
    {
        glfft_log("Failed to create display.\n");
        return nullptr;
    }
    eglInitialize(egl->dpy, nullptr, nullptr);

    EGLint num_configs = 0;
    eglChooseConfig(egl->dpy, attr, &egl->conf, 1, &num_configs);
    if (num_configs != 1)
    {
        glfft_log("Failed to get EGL config.\n");
        return nullptr;
    }

    egl->ctx = eglCreateContext(egl->dpy, egl->conf, EGL_NO_CONTEXT, context_attr);
    if (egl->ctx == EGL_NO_CONTEXT)
    {
        glfft_log("Failed to create GLES context.\n");
        return nullptr;
    }

    egl->surf = eglCreatePbufferSurface(egl->dpy, egl->conf, surface_attr);
    if (egl->surf == EGL_NO_SURFACE)
    {
        glfft_log("Failed to create Pbuffer surface.\n");
        return nullptr;
    }

    if (!eglMakeCurrent(egl->dpy, egl->surf, egl->surf, egl->ctx))
    {
        glfft_log("Failed to make EGL context current.\n");
        return nullptr;
    }

    const char *version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    unsigned major = 0, minor = 0;
    sscanf(version, "OpenGL ES %u.%u", &major, &minor);

    unsigned ctx_version = major * 1000 + minor;
    if (ctx_version < 3001)
    {
        glfft_log("OpenGL ES 3.1 not supported (got %u.%u context).\n",
                major, minor);
        return nullptr;
    }

    return egl.release();
}

void GLFFT::Context::destroy(void *ptr)
{
    delete static_cast<egl_ctx*>(ptr);
}

void glfft_log(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    __android_log_vprint(ANDROID_LOG_INFO, "GLFFT", fmt, va);
    va_end(va);

#ifdef GLFFT_CLI_ASYNC
    char buffer[16 * 1024];
    va_start(va, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, va);
    GLFFT::get_async_task()->push_message(buffer);
#endif
}

static int start_task(const vector<const char*> &argv)
{
    GLFFT::set_async_task([argv] {
            return GLFFT::cli_main(
                GLFFT::Context::create,
                GLFFT::Context::destroy,
                argv.size() - 1, (char**)argv.data());
            });

    GLFFT::get_async_task()->start();
    return 0;
}

JNIEXPORT jint JNICALL Java_net_themaister_glfft_Native_beginRunTestSuiteTask(JNIEnv *, jclass)
{
    vector<const char*> argv = {
        "glfft_cli",
        "test",
        "--test-all",
        nullptr,
    };

    return start_task(argv);
}

JNIEXPORT jint JNICALL Java_net_themaister_glfft_Native_beginBenchTask
  (JNIEnv *, jclass)
{
    vector<const char*> argv = {
        "glfft_cli",
        "bench",
        "--width",
        "1024",
        "--height",
        "1024",
        "--fp16",
        nullptr,
    };

    return start_task(argv);
}

JNIEXPORT jstring JNICALL Java_net_themaister_glfft_Native_pull
  (JNIEnv *env, jclass)
{
    string str;
    auto *task = GLFFT::get_async_task();
    bool ret = task->pull(str);
    return ret ? env->NewStringUTF(str.c_str()) : nullptr;
}

JNIEXPORT jint JNICALL Java_net_themaister_glfft_Native_getExitCode
  (JNIEnv *, jclass)
{
    auto *task = GLFFT::get_async_task();
    return task->get_exit_code();
}

JNIEXPORT jint JNICALL Java_net_themaister_glfft_Native_isComplete
  (JNIEnv *, jclass)
{
    auto *task = GLFFT::get_async_task();
    return task->is_completed();
}

JNIEXPORT void JNICALL Java_net_themaister_glfft_Native_endTask
  (JNIEnv *, jclass)
{
    GLFFT::end_async_task();
}

