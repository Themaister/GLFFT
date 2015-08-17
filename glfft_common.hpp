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

#ifndef GLFFT_COMMON_HPP__
#define GLFFT_COMMON_HPP__

#include "glfft_interface.hpp"
#include <functional>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <cstring>
#include <memory>
#include <unordered_map>

namespace GLFFT
{

enum Direction
{
    Forward = -1,
    InverseConvolve = 0,
    Inverse = 1
};

enum Mode
{
    Horizontal,
    HorizontalDual,
    Vertical,
    VerticalDual,

    ResolveRealToComplex,
    ResolveComplexToReal,
};

enum Type
{
    ComplexToComplex,
    ComplexToComplexDual,
    ComplexToReal,
    RealToComplex
};

enum Target
{
    SSBO,
    Image,
    ImageReal
};

struct Parameters
{
    unsigned workgroup_size_x;
    unsigned workgroup_size_y;
    unsigned workgroup_size_z;
    unsigned radix;
    unsigned vector_size;
    Direction direction;
    Mode mode;
    Target input_target;
    Target output_target;
    bool p1;
    bool pow2_stride;
    bool shared_banked;
    bool fft_fp16, input_fp16, output_fp16;
    bool fft_normalize;

    bool operator==(const Parameters &other) const
    {
        return std::memcmp(this, &other, sizeof(Parameters)) == 0;
    }
};

struct FFTOptions
{
    // Conservative defaults.
    struct Performance
    {
        unsigned workgroup_size_x = 4;
        unsigned workgroup_size_y = 1;
        unsigned vector_size = 2;
        bool shared_banked = false;
    } performance;

    struct Type
    {
        bool fp16 = false;
        bool input_fp16 = false;
        bool output_fp16 = false;
        bool normalize = false;
    } type;
};

}

namespace std
{
    template<>
    struct hash<GLFFT::Parameters>
    {
        std::size_t operator()(const GLFFT::Parameters &params) const
        {
            std::size_t h = 0;
            hash<uint8_t> hasher;
            for (std::size_t i = 0; i < sizeof(GLFFT::Parameters); i++)
            {
                h ^= hasher(reinterpret_cast<const uint8_t*>(&params)[i]);
            }

            return h;
        }
    };
}

namespace GLFFT
{

class Buffer
{
    public:
        Buffer() = default;
        ~Buffer();

        Buffer(GLuint buffer);
        Buffer& operator=(Buffer &&buffer) noexcept;
        Buffer(Buffer &&buffer) noexcept;

        void init(const void *data, size_t size, GLenum access);

        inline GLuint get() const { return name; }

    private:
        GLuint name = 0;
};

class Texture
{
    public:
        Texture() = default;
        ~Texture();

        Texture(GLuint tex);
        Texture& operator=(Texture &&tex) noexcept;
        Texture(Texture &&tex) noexcept;

        void init(unsigned width, unsigned height, unsigned levels, GLenum internal_format,
                GLenum wrap_s = GL_REPEAT, GLenum wrap_t = GL_REPEAT,
                GLenum min_filter = GL_NEAREST, GLenum mag_filter = GL_NEAREST);
        void upload(const void *data, GLenum format, GLenum type,
                unsigned x_off, unsigned y_off, unsigned width, unsigned height);

        inline GLuint get() const { return name; }

    private:
        GLuint name = 0;
};

class Program
{
    public:
        Program() = default;
        ~Program();

        Program(GLuint prog);
        Program& operator=(Program &&prog) noexcept;
        Program(Program &&prog) noexcept;

        inline GLuint get() const { return name; }

    private:
        GLuint name = 0;
};

class ProgramCache
{
    public:
        GLuint find_program(const Parameters &parameters) const;
        void insert_program(const Parameters &parameters, GLuint program);

        size_t cache_size() const { return programs.size(); }

    private:
        std::unordered_map<Parameters, Program> programs;
};

}

#endif

