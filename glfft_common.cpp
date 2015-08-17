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

#include "glfft_common.hpp"

using namespace std;
using namespace GLFFT;

Texture::~Texture()
{
    if (name)
    {
        glDeleteTextures(1, &name);
    }
}

Texture::Texture(GLuint tex)
    : name(tex)
{}

void Texture::init(unsigned width, unsigned height, unsigned levels, GLenum internal_format,
        GLenum wrap_s, GLenum wrap_t, GLenum min_filter, GLenum mag_filter)
{
    if (name)
    {
        glDeleteTextures(1, &name);
    }
    glGenTextures(1, &name);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexStorage2D(GL_TEXTURE_2D, levels, internal_format, width, height);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::upload(const void *data, GLenum format, GLenum type,
        unsigned x_off, unsigned y_off, unsigned width, unsigned height)
{
    if (!name)
    {
        throw logic_error("Cannot upload to null-texture.");
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, name);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x_off, y_off, width, height, format, type, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture& Texture::operator=(Texture &&texture) noexcept
{
    if (this != &texture)
    {
        if (name)
        {
            glDeleteTextures(1, &name);
        }
        name = texture.name;
        texture.name = 0;
    }
    return *this;
}

Texture::Texture(Texture &&texture) noexcept
{
    *this = move(texture);
}

Buffer::~Buffer()
{
    if (name)
    {
        glDeleteBuffers(1, &name);
    }
}

Buffer::Buffer(GLuint buffer)
    : name(buffer)
{}

void Buffer::init(const void *data, size_t size, GLenum access)
{
    if (name)
    {
        glDeleteBuffers(1, &name);
    }
    glGenBuffers(1, &name);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, name);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, access);
}

Buffer& Buffer::operator=(Buffer &&buffer) noexcept
{
    if (this != &buffer)
    {
        if (name)
        {
            glDeleteBuffers(1, &name);
        }
        name = buffer.name;
        buffer.name = 0;
    }
    return *this;
}

Buffer::Buffer(Buffer &&buffer) noexcept
{
    *this = move(buffer);
}

Program::Program(GLuint prog)
    : name(prog)
{}

Program::~Program()
{
    if (name)
    {
        glDeleteProgram(name);
    }
}

Program& Program::operator=(Program &&prog) noexcept
{
    if (this != &prog)
    {
        if (name)
        {
            glDeleteProgram(name);
        }
        name = prog.name;
        prog.name = 0;
    }
    return *this;
}

Program::Program(Program &&prog) noexcept
{
    *this = move(prog);
}

