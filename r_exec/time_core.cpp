//	time_core.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "time_core.h"

#include <r_exec/init.h>      // for Now
#include <r_exec/mem.h>       // for _Mem, _Mem::::RUNNING
#include <r_exec/time_job.h>  // for TimeJob
#include <stdint.h>           // for int64_t
#include <chrono>             // for duration, steady_clock, microseconds, etc
#include <thread>             // for thread, sleep_until
#include <atomic>

namespace r_exec
{

using namespace std::chrono;

void delegatedCoreWait(TimeJob *job)
{
    _Mem::Get()->start_core();
    bool run = true;

    static std::atomic<int64_t> last_lag;

    static const float smoothing_factor = 0.1;

    do {
        std::this_thread::sleep_until(steady_clock::time_point(microseconds(job->target_time - last_lag)));

        if (R_UNLIKELY(!job->is_alive())) {
            break;
        }

        if (R_UNLIKELY(_Mem::Get()->check_state() != _Mem::RUNNING)) {
            break;
        }

        int64_t lag = Now() - job->target_time;
        run = job->update();

        job->report(lag);

        last_lag = smoothing_factor * lag + (1.0 - smoothing_factor) * last_lag;
    } while(job->shouldRunAgain() && run);

    delete job->thread;
    delete job;
    _Mem::Get()->shutdown_core();
}

void runTimeCore()
{
    bool run = true;

    while (run) {
        TimeJob *job = _Mem::Get()->popTimeJob();

        if (job == nullptr) {
            break;
        }

        do {
            if (!job->is_alive()) {
                break;
            }

            if (_Mem::Get()->check_state() != _Mem::RUNNING) {
                break;
            }

            if (job->target_time == 0) {// means ASAP. Control jobs (shutdown) are caught here.
                run = job->update();
                if (run) {
                    continue;
                } else {
                    break;
                }
            }

            int64_t waitTime = job->target_time - Now();

            if (waitTime > 0) { // early: spawn a delegate to wait for the due time; delegate will die when done.
                job->thread = new std::thread(delegatedCoreWait, job);
                job->thread->detach();
                _Mem::Get()->register_time_job_latency(waitTime);
                job = nullptr;
                break; // get a new job
            }

            run = job->update();

            if (waitTime < 0) {
                job->report(-waitTime);
            }
        } while(job->shouldRunAgain() && run);

        delete job;
    }
}

}
