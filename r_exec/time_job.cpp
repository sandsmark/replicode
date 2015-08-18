//	time_job.cpp
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

#include "time_job.h"

#include <r_code/object.h>          // for Code
#include <r_exec/group.h>           // for Group
#include <r_exec/mem.h>             // for _Mem
#include <r_exec/overlay.h>         // for Controller
#include <r_exec/pgm_controller.h>  // for AntiPGMController, etc
#include <r_exec/time_job.h>        // for TimeJob, SaliencyPropagationJob, etc
#include <r_exec/view.h>            // for View


namespace r_exec
{

TimeJob::TimeJob(uint64_t target_time): _Object(), target_time(target_time)
{
}

void TimeJob::report(int64_t lag) const
{
    debug("time job") << "late generic:" << lag << "us behind.";
}

////////////////////////////////////////////////////////////

UpdateJob::UpdateJob(Group *g, uint64_t ijt): TimeJob(ijt)
{
    group = g;
}

bool UpdateJob::update()
{
    group->update(target_time);
    return true;
}

void UpdateJob::report(int64_t lag) const
{
    //debug("update job") << "job" << this << "is late:" << lag << "us behind.";
}

////////////////////////////////////////////////////////////

SignalingJob::SignalingJob(View *v, uint64_t ijt): TimeJob(ijt)
{
    view = v;
}

bool SignalingJob::is_alive() const
{
    return view->controller->is_alive();
}

////////////////////////////////////////////////////////////

AntiPGMSignalingJob::AntiPGMSignalingJob(View *v, uint64_t ijt): SignalingJob(v, ijt)
{
}

bool AntiPGMSignalingJob::update()
{
    if (is_alive()) {
        ((AntiPGMController *)view->controller)->signal_anti_pgm();
    }

    return true;
}

void AntiPGMSignalingJob::report(int64_t lag) const
{
    debug("anti-program signaling job") << "signaling:" << lag << "us behind.";
}

////////////////////////////////////////////////////////////

InputLessPGMSignalingJob::InputLessPGMSignalingJob(View *v, uint64_t ijt): SignalingJob(v, ijt)
{
}

bool InputLessPGMSignalingJob::update()
{
    if (is_alive()) {
        ((InputLessPGMController *)view->controller)->signal_input_less_pgm();
    }

    return true;
}

void InputLessPGMSignalingJob::report(int64_t lag) const
{
    debug("input-less program signaling job") << "late:" << lag << "us behind.";
}

////////////////////////////////////////////////////////////

InjectionJob::InjectionJob(View *v, uint64_t ijt): TimeJob(ijt)
{
    view = v;
}

bool InjectionJob::update()
{
    _Mem::Get()->inject(view);
    return true;
}

void InjectionJob::report(int64_t lag) const
{
    //debug("injection job") << "late:" << lag << "us behind.";
}

////////////////////////////////////////////////////////////

EInjectionJob::EInjectionJob(View *v, uint64_t ijt): TimeJob(ijt)
{
    view = v;
}

bool EInjectionJob::update()
{
    _Mem::Get()->inject_existing_object(view, view->object, view->get_host());
    return true;
}

void EInjectionJob::report(int64_t lag) const
{
    debug("einjection job") << "late:" << lag << "us behind.";
}

////////////////////////////////////////////////////////////

SaliencyPropagationJob::SaliencyPropagationJob(Code *o, double sln_change, double source_sln_thr, uint64_t ijt) :
    TimeJob(ijt), sln_change(sln_change), source_sln_thr(source_sln_thr)
{
    object = o;
}

bool SaliencyPropagationJob::update()
{
    if (!object->is_invalidated()) {
        _Mem::Get()->propagate_sln(object, sln_change, source_sln_thr);
    }

    return true;
}

void SaliencyPropagationJob::report(int64_t lag) const
{
    debug("saliency propagation job") << "late:" << lag << "us behind.";
}

////////////////////////////////////////////////////////////

ShutdownTimeCore::ShutdownTimeCore(): TimeJob(0)
{
}

bool ShutdownTimeCore::update()
{
    return false;
}

////////////////////////////////////////////////////////////

PerfSamplingJob::PerfSamplingJob(uint64_t start, uint64_t period): TimeJob(start), period(period)
{
}

bool PerfSamplingJob::update()
{
    _Mem::Get()->inject_perf_stats();
    target_time += period;
    return true;
}
}
