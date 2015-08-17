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

#ifndef GLFFT_HPP__
#define GLFFT_HPP__

#include "glfft_common.hpp"
#include "glfft_wisdom.hpp"
#include <vector>
#include <unordered_map>
#include <limits>

namespace GLFFT
{

class FFT
{
    public:
        // Creates a full FFT.
        FFT(unsigned Nx, unsigned Ny,
                Type type, Direction direction, Target input_target, Target output_target,
                std::shared_ptr<ProgramCache> cache, const FFTOptions &options,
                const FFTWisdom &wisdom = FFTWisdom());

        // Creates a single stage FFT. Useful for benchmarking partial FFTs.
        FFT(unsigned Nx, unsigned Ny, unsigned radix, unsigned p,
                Mode mode, Target input_target, Target ouptut_target,
                std::shared_ptr<ProgramCache> cache, const FFTOptions &options);

        void process(GLuint output, GLuint input, GLuint input_aux = 0);
        double bench(GLuint output, GLuint input,
                unsigned warmup_iterations, unsigned iterations, unsigned dispatches_per_iteration,
                double max_time = std::numeric_limits<double>::max());

        double get_cost() const { return cost; }

        unsigned get_dimension_x() const { return size_x; }
        unsigned get_dimension_y() const { return size_y; }

        void set_texture_offset_scale(float offset_x, float offset_y, float scale_x, float scale_y)
        {
            texture.offset_x = offset_x;
            texture.offset_y = offset_y;
            texture.scale_x = scale_x;
            texture.scale_y = scale_y;
        }

        void set_input_buffer_range(GLintptr offset, GLsizei size)
        {
            ssbo.input.offset = offset;
            ssbo.input.size = size;
        }

        void set_input_aux_buffer_range(GLintptr offset, GLsizei size)
        {
            ssbo.input_aux.offset = offset;
            ssbo.input_aux.size = size;
        }

        void set_output_buffer_range(GLintptr offset, GLsizei size)
        {
            ssbo.output.offset = offset;
            ssbo.output.size = size;
        }

        void set_samplers(GLuint sampler0, GLuint sampler1 = 0)
        {
            texture.samplers[0] = sampler0;
            texture.samplers[1] = sampler1;
        }

    private:
        struct Pass
        {
            Parameters parameters;

            unsigned workgroups_x;
            unsigned workgroups_y;
            unsigned uv_scale_x;
            GLuint program;
            GLbitfield barriers;
        };

        double cost = 0.0;

        Buffer temp_buffer;
        Buffer temp_buffer_image;
        std::vector<Pass> passes;
        std::shared_ptr<ProgramCache> cache;

        GLuint build_program(const Parameters &params);
        GLuint compile_compute_shader(const char *src);
        static std::string load_shader_string(const char *path);
        static void store_shader_string(const char *path, const std::string &source);

        GLuint get_program(const Parameters &params);

        struct
        {
            float offset_x = 0.0f, offset_y = 0.0f, scale_x = 1.0f, scale_y = 1.0f;
            GLuint samplers[2] = { 0, 0 };
        } texture;

        struct
        {
            struct
            {
                GLintptr offset = 0;
                GLsizei size = 0;
            } input, input_aux, output;
        } ssbo;
        unsigned size_x, size_y;
};

}

#endif
