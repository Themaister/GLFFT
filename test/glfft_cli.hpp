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
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

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

        void run_test_suite(const TestSuiteArguments &args);
    }

    class AsyncTask
    {
        public:
            AsyncTask(std::function<int ()> fun);
            ~AsyncTask();

            int wait_next_update();
            int wait_initialized();
            void start();

            unsigned get_current_progress() { return observed_progress; }
            unsigned get_target_progress() { return target_progress; }
            bool is_completed() { return completed; }
            int get_exit_code() { return completed_status; }

            // Only called from task thread.
            void set_current_progress(unsigned progress);
            void set_target_progress(unsigned progress);
            void signal_initialized();
            void signal_completed(int status);
            bool is_cancelled() { return cancelled; }

        private:
            std::function<int ()> fun;
            std::thread task;
            std::mutex mut;
            std::condition_variable cond;
            bool initialized = false;
            std::atomic_bool cancelled;
            std::atomic_bool completed;
            int completed_status = 0;

            unsigned observed_progress = 0;
            unsigned current_progress = 0;
            unsigned target_progress = 0;
    };

    void set_async_task(std::function<int ()> fun);
    void end_async_task();
    AsyncTask* get_async_task();

    int cli_main(
            const std::function<void* ()> &create_context,
            const std::function<void (void*)> &destroy_context,
            int argc, char *argv[]) noexcept;
}

#endif

