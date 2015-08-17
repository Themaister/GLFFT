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

#include "glfft_cli.hpp"
#include "glfft.hpp"
#include <cstdlib>
#include <stdexcept>
#include <functional>
#include <limits>
#include <memory>
#include <utility>

using namespace GLFFT;
using namespace GLFFT::Internal;
using namespace std;

struct CLIParser;
struct CLICallbacks
{
    void add(const char *cli, const function<void (CLIParser&)> &func)
    {
        callbacks[cli] = func;
    }
    unordered_map<string, function<void (CLIParser&)>> callbacks;
    function<void ()> error_handler;
};

struct CLIParser
{
    CLIParser(CLICallbacks cbs, int argc, char *argv[])
        : cbs(move(cbs)), argc(argc), argv(argv)
    {}

    bool parse()
    {
        try
        {
            while (argc && !ended_state)
            {
                const char *next = *argv++;
                argc--;

                auto itr = cbs.callbacks.find(next);
                if (itr == ::end(cbs.callbacks))
                {
                    throw logic_error("Invalid argument.\n");
                }

                itr->second(*this);
            }

            return true;
        }
        catch (...)
        {
            if (cbs.error_handler)
            {
                cbs.error_handler();
            }
            return false;
        }
    }

    void end()
    {
        ended_state = true;
    }

    unsigned next_uint()
    {
        if (!argc)
        {
            throw logic_error("Tried to parse uint, but nothing left in arguments.\n");
        }

        unsigned val = stoul(*argv);
        if (val > numeric_limits<unsigned>::max())
        {
            throw out_of_range("next_uint() out of range.\n");
        }

        argc--;
        argv++;

        return val;
    }

    double next_double()
    {
        if (!argc)
        {
            throw logic_error("Tried to parse double, but nothing left in arguments.\n");
        }

        double val = stod(*argv);

        argc--;
        argv++;

        return val;
    }

    const char *next_string()
    {
        if (!argc)
        {
            throw logic_error("Tried to parse string, but nothing left in arguments.\n");
        }

        const char *ret = *argv;
        argc--;
        argv++;
        return ret;
    }

    CLICallbacks cbs;
    int argc;
    char **argv;
    bool ended_state = false;
};

struct BenchArguments
{
    unsigned width = 0;
    unsigned height = 0;
    unsigned warmup = 2;
    unsigned iterations = 20;
    unsigned dispatches = 50;
    unsigned timeout = 1.0;
    Type type = ComplexToComplex;
    unsigned size_for_type = 2;
    const char *string_for_type = "C2C";
    bool fp16 = false;
    bool input_texture = false;
    bool output_texture = false;
};

static void run_benchmark(const BenchArguments &args)
{
    auto cache = make_shared<ProgramCache>();

    FFTOptions options;
    options.type.input_fp16 = args.fp16;
    options.type.output_fp16 = args.fp16;
    options.type.fp16 = args.fp16;

    Buffer input;
    Buffer output;
    Texture input_tex;
    Texture output_tex;
    GLuint input_name = 0;
    GLuint output_name = 0;

    Target input_target = SSBO;
    Target output_target = SSBO;

    size_t buffer_size = sizeof(float) * (args.fp16 ? 1 : 2) * args.size_for_type * args.width * args.height;

    if (args.input_texture)
    {
        GLenum internal_format = 0;

        switch (args.type)
        {
            case ComplexToComplexDual:
                internal_format = GL_RGBA32F;
                input_target = Image;
                break;

            case ComplexToComplex:
            case ComplexToReal:
                internal_format = GL_RG32F;
                input_target = Image;
                break;

            case RealToComplex:
                internal_format = GL_R32F;
                input_target = ImageReal;
                break;
        }

        input_tex.init(args.width, args.height, 1, internal_format);
        input_name = input_tex.get();
    }
    else
    {
        vector<float> tmp(buffer_size / sizeof(float));
        input.init(tmp.data(), buffer_size, GL_STATIC_COPY);
        input_name = input.get();
    }

    if (args.output_texture)
    {
        GLenum internal_format = 0;

        switch (args.type)
        {
            case ComplexToComplexDual:
                internal_format = GL_RGBA16F;
                output_target = Image;
                break;

            case ComplexToComplex:
            case RealToComplex:
                internal_format = GL_RG16F;
                output_target = Image;
                break;

            case ComplexToReal:
                internal_format = GL_R32F;
                output_target = ImageReal;
                break;
        }
        output_tex.init(args.width, args.height, 1, internal_format);
        output_name = output_tex.get();
    }
    else
    {
        output.init(nullptr, buffer_size, GL_STREAM_COPY);
        output_name = output.get();
    }

    FFTWisdom wisdom;
    wisdom.set_static_wisdom(FFTWisdom::get_static_wisdom_from_renderer(reinterpret_cast<const char*>(glGetString(GL_RENDERER))));
    wisdom.set_bench_params(args.warmup, args.iterations, args.dispatches, args.timeout);
    wisdom.learn_optimal_options_exhaustive(args.width, args.height, args.type, input_target, output_target, options.type);

    FFT fft(args.width, args.height, args.type, args.type == ComplexToReal ? Inverse : Forward, input_target, output_target, cache, options, wisdom);

    glfft_log("Test:\n");
    glfft_log("  %s -> %s\n", input_target == SSBO ? "SSBO" : "Texture", output_target == SSBO ? "SSBO" : "Image");
    glfft_log("  Size: %u x %u %s %s\n", args.width, args.height, args.string_for_type, args.fp16 ? "FP16" : "FP32");
    glfft_log("  %8.3f ms\n\n", 1000.0 * fft.bench(output_name, input_name, 5, 100, 100, 5.0));
}

static void cli_help(char *argv[])
{
    glfft_log("Usage: %s [test | bench | help] (args...)\n", argv[0]);
    glfft_log("       For help on various subsystems, e.g. %s test help\n", argv[0]);
}

static void cli_test_help()
{
    glfft_log("Usage: test [--test testid] [--test-all] [--test-range testidmin testidmax] [--exit-on-fail] [--minimum-snr-fp16 value-db] [--maximum-snr-fp32 value-db] [--epsilon-fp16 value] [--epsilon-fp32 value]\n"
              "       --test testid: Run a specific test, indexed by number.\n"
              "       --test-all: Run all tests.\n"
              "       --test-range testidmin testidmax: Run specific tests between testidmin and testidmax, indexed by number.\n"
              "       --exit-on-fail: Exit immediately when a test does not pass.\n");
}

static int cli_test(int argc, char *argv[])
{
    if (argc < 1)
    {
        cli_test_help();
        return EXIT_FAILURE;
    }

    TestSuiteArguments args;

    CLICallbacks cbs;
    cbs.add("help",               [](CLIParser &parser)      { cli_test_help(); parser.end(); });
    cbs.add("--test",             [&args](CLIParser &parser) { args.test_id_min = args.test_id_max = parser.next_uint(); args.exhaustive = false; });
    cbs.add("--test-range",       [&args](CLIParser &parser) { args.test_id_min = parser.next_uint(); args.test_id_max = parser.next_uint(); args.exhaustive = false; });
    cbs.add("--test-all",         [&args](CLIParser&)        { args.exhaustive = true; });
    cbs.add("--exit-on-fail",     [&args](CLIParser&)        { args.throw_on_fail = true; });
    cbs.add("--minimum-snr-fp16", [&args](CLIParser &parser) { args.min_snr_fp16 = parser.next_double(); });
    cbs.add("--minimum-snr-fp32", [&args](CLIParser &parser) { args.min_snr_fp32 = parser.next_double(); });
    cbs.add("--epsilon-fp16",     [&args](CLIParser &parser) { args.epsilon_fp16 = parser.next_double(); });
    cbs.add("--epsilon-fp32",     [&args](CLIParser &parser) { args.epsilon_fp32 = parser.next_double(); });

    cbs.error_handler = []{ cli_test_help(); };
    CLIParser parser(move(cbs), argc, argv);

    if (!parser.parse())
    {
        return EXIT_FAILURE;
    }
    else if (parser.ended_state)
    {
        return EXIT_SUCCESS;
    }

    run_test_suite(args);
    return EXIT_SUCCESS;
}

static void cli_bench_help()
{
    glfft_log("Usage: bench [--width value] [--height value] [--warmup arg] [--iterations arg] [--dispatches arg] [--timeout arg] [--type type] [--input-texture] [--output-texture]\n"
              "--type type: ComplexToComplex, ComplexToComplexDual, ComplexToReal, RealToComplex\n");
}

static Type parse_type(const char *arg, BenchArguments &args)
{
    if (!strcmp(arg, "ComplexToComplex"))
    {
        args.size_for_type = 2;
        return ComplexToComplex;
    }
    else if (!strcmp(arg, "ComplexToComplexDual"))
    {
        args.size_for_type = 4;
        args.string_for_type = "C2C dual";
        return ComplexToComplexDual;
    }
    else if (!strcmp(arg, "RealToComplex"))
    {
        args.size_for_type = 2;
        args.string_for_type = "R2C";
        return RealToComplex;
    }
    else if (!strcmp(arg, "ComplexToReal"))
    {
        args.size_for_type = 2;
        args.string_for_type = "C2R";
        return ComplexToReal;
    }
    else
    {
        throw logic_error("Invalid argument to parse_type().\n");
    }
}

static int cli_bench(int argc, char *argv[])
{
    if (argc < 1)
    {
        cli_bench_help();
        return EXIT_FAILURE;
    }

    BenchArguments args;

    CLICallbacks cbs;
    cbs.add("help",             [](CLIParser &parser) { cli_bench_help(); parser.end(); });
    cbs.add("--width",          [&args](CLIParser &parser) { args.width = parser.next_uint(); });
    cbs.add("--height",         [&args](CLIParser &parser) { args.height = parser.next_uint(); });
    cbs.add("--warmup",         [&args](CLIParser &parser) { args.warmup = parser.next_uint(); });
    cbs.add("--iterations",     [&args](CLIParser &parser) { args.iterations = parser.next_uint(); });
    cbs.add("--dispatches",     [&args](CLIParser &parser) { args.dispatches = parser.next_uint(); });
    cbs.add("--timeout",        [&args](CLIParser &parser) { args.timeout = parser.next_double(); });
    cbs.add("--fp16",           [&args](CLIParser&)        { args.fp16 = true; });
    cbs.add("--type",           [&args](CLIParser &parser) { args.type = parse_type(parser.next_string(), args); });
    cbs.add("--input-texture",  [&args](CLIParser&)        { args.input_texture = true; });
    cbs.add("--output-texture", [&args](CLIParser&)        { args.output_texture = true; });

    cbs.error_handler = []{ cli_bench_help(); };

    CLIParser parser(move(cbs), argc, argv);

    if (!parser.parse())
    {
        return EXIT_FAILURE;
    }
    else if (parser.ended_state)
    {
        return EXIT_SUCCESS;
    }

    run_benchmark(args);
    return EXIT_SUCCESS;
}

struct Context
{
    Context(void *ctx, const function<void (void*)> &cb) : ctx(ctx), cb(cb) {}
    ~Context() { cb(ctx); }

    void *ctx;
    const function<void (void*)> &cb;
};

int GLFFT::cli_main(
        const function<void* ()> &create_context,
        const function<void (void*)> &destroy_context,
        int argc, char *argv[]) noexcept
{
    auto ctx = unique_ptr<Context>(new Context(create_context(), destroy_context));
    if (!ctx)
    {
        return EXIT_FAILURE;
    }

    // Do not leak exceptions beyond this function.
    try
    {
        if (argc < 2)
        {
            cli_help(argv);
            return EXIT_FAILURE;
        }

        if (!strcmp(argv[1], "test"))
        {
            return cli_test(argc - 2, argv + 2);
        }
        else if (!strcmp(argv[1], "bench"))
        {
            return cli_bench(argc - 2, argv + 2);
        }
        else if (!strcmp(argv[1], "help"))
        {
            cli_help(argv);
            return EXIT_SUCCESS;
        }
        else
        {
            cli_help(argv);
            return EXIT_FAILURE;
        }
    }
    catch (const std::exception &e)
    {
        glfft_log("Caught exception \"%s\" ...\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

