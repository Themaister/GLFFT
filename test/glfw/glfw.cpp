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
#include "glfft_context.hpp"

using namespace GLFFT;
using namespace std;

#ifdef GLFFT_GL_DEBUG
static void APIENTRY gl_debug_cb(GLenum, GLenum, GLuint, GLenum, GLsizei,
        const GLchar *message, void*)
{
    glfft_log("GLDEBUG: %s.\n", message);
}
#endif

struct GLFWContext : GLContext
{
    ~GLFWContext()
    {
        if (window)
        {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
    GLFWwindow *window = nullptr;

    bool supports_texture_readback() override { return true; }
    void read_texture(void *buffer, Texture *texture, Format format) override
    {
        glBindTexture(GL_TEXTURE_2D, static_cast<GLTexture*>(texture)->get());
        glGetTexImage(GL_TEXTURE_2D, 0, convert_format(format), convert_type(format), buffer);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

unique_ptr<Context> GLFFT::create_cli_context()
{
    if (!glfwInit())
    {
        return nullptr;
    }

    glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    unique_ptr<GLFWContext> context(new GLFWContext);

    context->window = glfwCreateWindow(128, 128, "GLFFT Test", nullptr, nullptr);
    if (!context->window)
    {
        glfwTerminate();
        return nullptr;
    }

    glfwMakeContextCurrent(context->window);
    rglgen_resolve_symbols(glfwGetProcAddress);

#ifdef GLFFT_GL_DEBUG
    glDebugMessageCallback(gl_debug_cb, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    return unique_ptr<Context>(move(context));
}

