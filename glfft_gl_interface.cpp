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

#include "glfft_gl_interface.hpp"
#include <stdarg.h>
#include <vector>

using namespace GLFFT;
using namespace std;

static inline GLenum convert(WrapMode mode)
{
    switch (mode)
    {
        case WrapClamp: return GL_CLAMP_TO_EDGE;
        case WrapRepeat: return GL_REPEAT;
    }
    return 0;
}

static inline GLenum convert(Filter filter)
{
    switch (filter)
    {
        case FilterLinear: return GL_LINEAR;
        case FilterNearest: return GL_NEAREST;
    }
    return 0;
}

static inline GLenum convert(AccessMode mode)
{
    switch (mode)
    {
        case AccessStreamCopy: return GL_STREAM_COPY;
        case AccessStaticCopy: return GL_STATIC_COPY;
        case AccessStreamRead: return GL_STREAM_READ;
    }
    return 0;
}

static inline GLenum convert(Format format)
{
    switch (format)
    {
        case FormatR16G16B16A16Float: return GL_RGBA16F;
        case FormatR32G32B32A32Float: return GL_RGBA32F;
        case FormatR32Float: return GL_R32F;
        case FormatR16G16Float: return GL_RG16F;
        case FormatR32G32Float: return GL_RG32F;
        case FormatR32Uint: return GL_R32UI;
        case FormatUnknown: return 0;
    }
    return 0;
}

static inline GLenum convert_format(Format format)
{
    switch (format)
    {
        case FormatR16G16Float: return GL_RG;
        case FormatR32G32Float: return GL_RG;
        case FormatR16G16B16A16Float: return GL_RGBA;
        case FormatR32G32B32A32Float: return GL_RGBA;
        case FormatR32Float: return GL_RED;
        case FormatR32Uint: return GL_RED_INTEGER;
        case FormatUnknown: return 0;
    }
    return 0;
}

static inline GLenum convert_type(Format format)
{
    switch (format)
    {
        case FormatR16G16Float: return GL_HALF_FLOAT;
        case FormatR16G16B16A16Float: return GL_HALF_FLOAT;
        case FormatR32Float: return GL_FLOAT;
        case FormatR32G32Float: return GL_FLOAT;
        case FormatR32G32B32A32Float: return GL_FLOAT;
        case FormatR32Uint: return GL_UNSIGNED_INT;
        case FormatUnknown: return 0;
    }
    return 0;
}

GLCommandBuffer GLContext::static_command_buffer;

void GLCommandBuffer::bind_program(Program *program)
{
    glUseProgram(program ? static_cast<GLProgram*>(program)->name : 0);
}

void GLCommandBuffer::bind_storage_texture(unsigned binding, Texture *texture, Format format)
{
    glBindImageTexture(binding, static_cast<GLTexture*>(texture)->name,
            0, GL_FALSE, 0, GL_WRITE_ONLY, convert(format));
}

void GLCommandBuffer::bind_texture(unsigned binding, Texture *texture)
{
    glActiveTexture(GL_TEXTURE0 + binding);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLTexture*>(texture)->name);
}

void GLCommandBuffer::bind_sampler(unsigned binding, Sampler *sampler)
{
    glBindSampler(binding, sampler ? static_cast<GLSampler*>(sampler)->name : 0);
}

void GLCommandBuffer::bind_storage_buffer(unsigned binding, Buffer *buffer)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, static_cast<GLBuffer*>(buffer)->name);
}

void GLCommandBuffer::bind_storage_buffer_range(unsigned binding, size_t offset, size_t size, Buffer *buffer)
{
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, static_cast<GLBuffer*>(buffer)->name, offset, size);
}

void GLCommandBuffer::dispatch(unsigned x, unsigned y, unsigned z)
{
    glDispatchCompute(x, y, z);
}

void GLCommandBuffer::barrier(Buffer*)
{
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GLCommandBuffer::barrier(Texture*)
{
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
}

void GLCommandBuffer::barrier()
{
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void GLCommandBuffer::uniform1ui(unsigned location, unsigned value)
{
    glUniform1ui(location, value);
}

void GLCommandBuffer::uniform2f(unsigned location, float v0, float v1)
{
    glUniform2f(location, v0, v1);
}

CommandBuffer* GLContext::request_command_buffer()
{
    return &static_command_buffer;
}

void GLContext::submit_command_buffer(CommandBuffer*)
{}

void GLContext::wait_idle()
{
    glFinish();
}

unique_ptr<Texture> GLContext::create_texture(const void *initial_data,
        unsigned width, unsigned height, unsigned levels,
        Format format, WrapMode wrap_s, WrapMode wrap_t,
        Filter min_filter, Filter mag_filter)
{
    return unique_ptr<Texture>(new GLTexture(initial_data, width, height, levels, format, wrap_s, wrap_t, min_filter, mag_filter));
}

unique_ptr<Buffer> GLContext::create_buffer(const void *initial_data, size_t size, AccessMode access)
{
    return unique_ptr<Buffer>(new GLBuffer(initial_data, size, access));
}

unique_ptr<Program> GLContext::compile_compute_shader(const char *source)
{
    GLuint program = glCreateProgram();
    if (!program)
    {
        return nullptr;
    }

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);

    const char *sources[] = { GLFFT_GLSL_LANG_STRING, source };
    glShaderSource(shader, 2, sources, NULL);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint len;
        GLsizei out_len;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        vector<char> buf(len);
        glGetShaderInfoLog(shader, len, &out_len, buf.data());
        log("GLFFT: Shader log:\n%s\n\n", buf.data());

        glDeleteShader(shader);
        glDeleteProgram(program);
        return 0;
    }

    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE)
    {
        GLint len;
        GLsizei out_len;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        vector<char> buf(len);
        glGetProgramInfoLog(program, len, &out_len, buf.data());
        log("Program log:\n%s\n\n", buf.data());

        glDeleteProgram(program);
        glDeleteShader(shader);
        return nullptr;
    }

    return unique_ptr<Program>(new GLProgram(program));
}

void GLContext::log(const char *fmt, ...)
{
    char buffer[4 * 1024];

    va_list va;
    va_start(va, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, va);
    va_end(va);
    glfft_log("%s", buffer);
}

double GLContext::get_time()
{
    return glfft_time();
}

unsigned GLContext::get_max_work_group_threads()
{
    GLint value;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &value);
    return value;
}

const char* GLContext::get_renderer_string()
{
    return reinterpret_cast<const char*>(glGetString(GL_RENDERER));
}

const void* GLContext::map(Buffer *buffer, size_t offset, size_t size)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, static_cast<GLBuffer*>(buffer)->name);
    const void *ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, offset, size, GL_MAP_READ_BIT);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    return ptr;
}

void GLContext::unmap(Buffer *buffer)
{
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, static_cast<GLBuffer*>(buffer)->name);
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

GLTexture::GLTexture(const void *initial_data,
        unsigned width, unsigned height, unsigned levels,
        Format format, WrapMode wrap_s, WrapMode wrap_t,
        Filter min_filter, Filter mag_filter)
{
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexStorage2D(GL_TEXTURE_2D, levels, convert(format), width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, convert(wrap_s));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, convert(wrap_t));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, convert(min_filter));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, convert(mag_filter));

    if (initial_data)
    {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                convert_format(format), convert_type(format), initial_data);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

GLTexture::~GLTexture()
{
    glDeleteTextures(1, &name);
}

GLBuffer::GLBuffer(const void *initial_data, size_t size, AccessMode access)
{
    glGenBuffers(1, &name);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, name);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, initial_data, convert(access));
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

GLBuffer::~GLBuffer()
{
    glDeleteBuffers(1, &name);
}

GLProgram::GLProgram(GLuint name)
    : name(name)
{}

GLProgram::~GLProgram()
{
    if (name != 0)
    {
        glDeleteProgram(name);
    }
}

GLSampler::GLSampler(GLuint name)
    : name(name)
{}

GLSampler::~GLSampler()
{
    if (name != 0)
    {
        glDeleteSamplers(1, &name);
    }
}

