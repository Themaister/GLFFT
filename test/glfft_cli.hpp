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

#ifndef GLFFT_CLI_HPP__
#define GLFFT_CLI_HPP__

#include <functional>
#include "glfft_interface.hpp"

#ifdef GLFFT_CLI_ASYNC
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <queue>
#include <string>
#endif

namespace GLFFT
{
    namespace Internal
    {
        struct TestSuiteArguments
        {
            unsigned test_id_min = 0;
            unsigned test_id_max = 0;
            bool exhaustive = true;
            bool throw_on_fail = false;
            double min_snr_fp16 = 50.0;
            double min_snr_fp32 = 100.0;
            double epsilon_fp16 = 1e-3;
            double epsilon_fp32 = 1e-6;
        };

        void run_test_suite(Context *context, const TestSuiteArguments &args);
    }

#ifdef GLFFT_CLI_ASYNC
    // Throws this on cancellation.
    struct AsyncCancellation { int code; };

    // Glue code to fake cooperative threading which would be much nicer for this scenario ...
    class AsyncTask
    {
        public:
            AsyncTask(std::function<int ()> fun);

            void start();
            void end();

            // Called from auxillary thread or similar.
            bool pull(std::string &msg);
            bool is_completed() { return completed; }
            int get_exit_code() { return completed_status; }

            // Only called from task thread.
            bool is_cancelled() { return cancelled; }
            void push_message(const char *msg);

        private:
            std::function<int ()> fun;
            std::thread task;
            std::mutex mut;
            std::condition_variable cond;
            std::atomic_bool cancelled;
            std::atomic_bool completed;
            int completed_status = 0;

            std::queue<std::string> messages;
            void signal_completed(int status);
    };

    void set_async_task(std::function<int ()> fun);
    void end_async_task();
    void check_async_cancel();
    AsyncTask* get_async_task();
#endif

    int cli_main(
            Context *context,
            int argc, char *argv[])
#ifndef GLFFT_CLI_ASYNC
        noexcept
#endif
        ;
}

#endif

