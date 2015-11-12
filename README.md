# GLFFT

GLFFT is a C++11/OpenGL library for doing the Fast Fourier Transform (FFT) on a GPU in one or two dimensions.
The target APIs are OpenGL 4.3 core profile and OpenGL ES 3.1.
GLFFT is implemented entirely with compute shaders.

The FFT has several uses in graphics. The two main ones are Tessendorf's FFT water simulation technique
as well as applying massive convolution filters to images more efficiently.

## Features

GLFFT is a well featured single-precision and half-precision (FP16) FFT library designed for
graphics use cases.

 - Power-of-two transforms
 - 1D/2D complex-to-complex transform
 - 1D/2D real-to-complex transform
 - 1D/2D complex-to-real transform
 - 1D/2D dual complex-to-complex transforms which pair two complex numbers into a vec4 (useful for working for RGBA data).
 - 1D/2D convolution support
 - Normalized and unnormalized FFT
 - Support for choosing if input and output to GLFFT is treated as packed FP16 or FP32.
 - Support for using Shader Storage Buffer Objects or Textures as inputs and outputs to GLFFT.
 - Applicable for many different GPUs with many performance oriented knobs to tweak.
 - Support for FFT "wisdom", a method where GLFFT will find optimal parameters for a particular GPU.
 - A serialization interface for storing GLFFT wisdom for later use.
 - A standalone CLI for verification and benchmarking.

### Platform support

Desktop Linux and Windows have been tested and are supported. Supports both OpenGL 4.3 core and OpenGL ES 3.1.

The GLSL code has been tested and verified on:

 - ARM Mali T-760 (Linux, Android 5.x)
 - nVidia GTX 760 (Linux)
 - Intel HD 4400 (Windows 7 x64)

Compilers tested:

 - GCC 4.8
 - GCC 4.9
 - GCC 5.2
 - Clang 3.6

## Performance

GLFFT is a performance oriented FFT library, and aims to reach optimal efficiency on both desktop and mobile GPUs.

 - Supports different vector sizes to match both scalar and vector-based GPU hardware.
 - Supports techniques to reduce bank conflicts which greatly improves performance of GPUs with banked shared memory such as nVidia and AMD.
 - Supports almost any workgroup size decomposition to better match ideal memory access patterns on various hardware.
 - Supports mediump precision to improve performance in FP16 FFTs on mobile GPUs, e.g. ARM Mali.
 - Supports packed FP16 input and FP16 output to greatly reduce bandwidth when FP16 is accurate enough for the particular application.

## Integrating GLFFT into a code base

GLFFT is an OpenGL oriented implementation, but all API specifics of GLFFT have been abstracted away to
make it suitable for integration into engines which might have abstracted interfaces to the underlying graphics APIs.
It is possible to implement GLFFT without OpenGL, as long as GLSL is supported as a shading language,
which is assumed to be feasible once SPIR-V becomes mainstream.
The abstracted interface is designed to match the spirit of next-generation APIs like Vulkan and D3D12.

As using GLFFT in a GL context is by far the most common use case, a default, ready-to-go OpenGL interface
is supplied in the API. Due to the nature of OpenGL, it is not always feasible to build GLFFT as a standalone library, as applications
might have their own ways of getting to OpenGL symbols and headers which is not easy for GLFFT to work with.
Instead, the GLFFT implementation will include a user-supplied header, `glfft_api_headers.hpp` which is responsible for including
the appropriate OpenGL or OpenGL ES 3.1 headers (or special headers like GLEW) the calling application uses,
as well as defining various constants, such as the GLSL language strings to use.
This header can also override various functions such as logging functions and time functions.
For a reference, see `test/glfw/glfft_api_headers.hpp` as to how the internal test/bench suite does it.

### Pre-baking GLSL shader source

To compile GLSL shader sources into header files, run:

    make -C glsl

This only needs to be run when the internal GLSL shaders are modified.

### Source files

To build source files, a `$(wildcard *.cpp)` in top level directory of GLFFT is sufficient to find the necessary files.
When compiling, C++11 must be enabled, and `glfft_api_headers.hpp` must be found in an include path.

## Snippets

### Do a 1024x256 Complex-To-Complex FFT.

```c++
#include "glfft.hpp"
#include "glfft_gl_interface.hpp"
#include <memory>

using namespace GLFFT;
using namespace std;

FFTOptions options; // Default initializes to something conservative in terms of performance.
options.type.fp16 = true; // Use mediump float (if GLES) in shaders.
options.type.input_fp16 = false; // Use FP32 input.
options.type.output_fp16 = true; // Use FP16 output.
options.type.normalize = true; // Normalized FFT.

GLContext context;

FFT fft(&context, 1024, 256, ComplexToComplex, Inverse, SSBO, SSBO, make_shared<ProgramCache>(), options);

GLuint output_ssbo, input_ssbo;
// Create GL_SHADER_STORAGE_BUFFERs and put some data in them.

// Adapt raw GL types to types which GLContext uses internally.
GLBuffer adaptor_output(output_ssbo);
GLBuffer adaptor_input(input_ssbo);

// Do the FFT
CommandBuffer *cmd = context.request_command_buffer();
fft.process(cmd, &adaptor_output, &adaptor_input);
context.submit_command_buffer(cmd);

glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
```

### Do a 1024x256 Complex-To-Complex FFT more optimally using wisdom.

```c++
#include "glfft.hpp"
#include "glfft_wisdom.hpp"
#include "glfft_gl_interface.hpp"
#include <memory>

using namespace GLFFT;
using namespace std;

FFTOptions options; // Default initializes to something conservative in terms of performance.
options.type.fp16 = true; // Use mediump float (if GLES) in shaders.
options.type.input_fp16 = false; // Use FP32 input.
options.type.output_fp16 = true; // Use FP16 output.
options.type.normalize = true; // Normalized FFT.

GLContext context;

FFTWisdom wisdom;
// Use some static wisdom to make the learning step faster.
// Avoids searching for options which are known to be bogus for a particular vendor.
wisdom.set_static_wisdom(FFTWisdom::get_static_wisdom_from_renderer(reinterpret_cast<const char*>(glGetString(GL_RENDERER))));
// Learn how to do 1024x256 much faster!
wisdom.learn_optimal_options_exhaustive(&context, 1024, 256, ComplexToComplex, SSBO, SSBO, options.type);

GLContext context;

FFT fft(&context, 1024, 256, ComplexToComplex, Inverse, SSBO, SSBO, make_shared<ProgramCache>(), options);

GLuint output_ssbo, input_ssbo;
// Create GL_SHADER_STORAGE_BUFFERs and put some data in them.

// Adapt raw GL types to types which GLContext uses internally.
GLBuffer adaptor_output(output_ssbo);
GLBuffer adaptor_input(input_ssbo);

// Do the FFT
CommandBuffer *cmd = context.request_command_buffer();
fft.process(cmd, &adaptor_output, &adaptor_input);
context.submit_command_buffer(cmd);

glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
```

### Serializing wisdom to a string

```c++
GLContext context;

FFTWisdom wisdom;
// Use some static wisdom to make the learning step faster.
// Avoids searching for options which are known to be bogus for a particular vendor.
wisdom.set_static_wisdom(FFTWisdom::get_static_wisdom_from_renderer(reinterpret_cast<const char*>(glGetString(GL_RENDERER))));
// Learn how to do 1024x256 much faster!
wisdom.learn_optimal_options_exhaustive(&context, 1024, 256, ComplexToComplex, SSBO, SSBO, options.type);

// Serialize to string.
string wisdom_json = wisdom.archive();

// Unserialize wisdom.
wisdom.extract(wisdom_json.c_str());
```

### Documentation

Proper documentation is still TODO. However, `test/glfft_test.cpp` and `test/glfft_cli.cpp` should give a good idea for how to use the API.

## Verification and Benchmarking

For verification and benchmarking, GLFFT has a standalone executable which uses the [GLFW3](http://glfw.org) library to create a context.

### Building

Building requires GLFW3 to be installed, visible from pkg-config,
although for Windows 64-bit a precompiled library with headers is included for convenience.

    make
    ./glfft_cli help

#### Cross compilation for e.g. Windows from Linux

    make PLATFORM=win TOOLCHAIN_PREFIX=x86_64-w64-mingw32-
    wine64 ./glfft_cli help

#### Dependencies

Other than GLFW, muFFT and rapidjson are needed, which are included as git submodules.

    git submodule init
    git submodule update
    # Or git clone --recursive when checking out GLFFT.

### Tests

To verify GLFFT correctness, GLFFT has a large amount of tests which exhaustively tests the GLFFT API with almost any thinkable
input and output parameters.

    ./glfft_cli test --test-all # Exhaustively tests everything (over 3000 tests currently), so will take some time.

Verification is based on SNR compared to [muFFT](https://github.com/Themaister/muFFT) as a reference and maximum allowed delta from reference value.
The default precision requirements are fairly stringent, so particular GPUs might not support high enough precision.
Precision requirements can be overridden in such scenarios on the command line (see help).

### Benchmarking

To evaluate GLFFT performance, GLFFT can be benchmarked with various parameters.
The benchmarking interface will run a full wisdom search first to find optimal parameters before running the actual benchmark.

    ./glfft_cli bench --width 1024 --height 1024 --type ComplexToComplex --input-texture # Benchmark a 1024x1024 C2C FFT with texture as input and SSBO as output. See ./glfft_cli bench help for more.

## FFT method

GLFFT implements radix-4, radix-8, radix-16 (radix-4 two times in single pass) and radix-64 (radix-8 two times in single pass) FFT kernels.
GLFFT will automatically find the optimal subdivision of a larger FFT problem based on either wisdom knowledge or estimations.
Radix-16 and Radix-64 kernels are implemented by using shared memory to perform multiple passes without going to global memory between
the two passes.

In order to support a vast number of options, GLFFT will compile shaders on-demand during initialization
and store them in a user-provided cache which can be shared between GLFFT instantiations.

GLFFT is out-of-place using the Stockham auto-sort algorithm to avoid explicit reorder passes which is common for in-place algorithms.

GLFFT's implementation is overall very similar to [muFFT](https://github.com/Themaister/muFFT),
and more information about the algorithms can be found [here](https://github.com/Themaister/muFFT/blob/master/doxygen/fft.md).

## License

The GLFFT library is licensed under the permissive MIT license, see COPYING and preambles in source files for more detail.
The verification and benchmarking library links against GLFW, whose license is [here](https://github.com/glfw/glfw/blob/master/COPYING.txt).

