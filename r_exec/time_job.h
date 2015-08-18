//	time_job.h
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

#ifndef time_job_h
#define time_job_h

#include <stdint.h>             // for uint64_t, int64_t

#include "CoreLibrary/base.h"   // for P, _Object
#include "CoreLibrary/debug.h"  // for DebugStream, debug
#include "CoreLibrary/dll.h"    // for dll_export

namespace r_code {
class Code;
}  // namespace r_code
namespace r_exec {
class Group;
class View;
}  // namespace r_exec

namespace r_exec
{

class dll_export TimeJob:
    public core::_Object
{
protected:
    TimeJob(uint64_t target_time);
public:
    uint64_t target_time; // absolute deadline; 0 means ASAP.
    virtual bool update() = 0; // next_target: absolute deadline; 0 means no more waiting; return false to shutdown the time core.
    virtual bool is_alive() const
    {
        return true;
    }
    virtual bool shouldRunAgain() const
    {
        return false;
    }
    virtual void report(int64_t lag) const;
};

class dll_export UpdateJob:
    public TimeJob
{
public:
    core::P<Group> group;
    UpdateJob(Group *g, uint64_t ijt);
    bool update();
    void report(int64_t lag) const;
};

class dll_export SignalingJob:
    public TimeJob
{
protected:
    SignalingJob(View *v, uint64_t ijt);
public:
    core::P<View> view;
    bool is_alive() const;
};

class dll_export AntiPGMSignalingJob:
    public SignalingJob
{
public:
    AntiPGMSignalingJob(View *v, uint64_t ijt);
    bool update();
    void report(int64_t lag) const;
};

class dll_export InputLessPGMSignalingJob:
    public SignalingJob
{
public:
    InputLessPGMSignalingJob(View *v, uint64_t ijt);
    bool update();
    void report(int64_t lag) const;
};

class dll_export InjectionJob:
    public TimeJob
{
public:
    core::P<View> view;
    InjectionJob(View *v, uint64_t ijt);
    bool update();
    void report(int64_t lag) const;
};

class dll_export EInjectionJob:
    public TimeJob
{
public:
    core::P<View> view;
    EInjectionJob(View *v, uint64_t ijt);
    bool update();
    void report(int64_t lag) const;
};

class dll_export SaliencyPropagationJob:
    public TimeJob
{
public:
    core::P<r_code::Code> object;
    double sln_change;
    double source_sln_thr;
    SaliencyPropagationJob(r_code::Code *o, double sln_change, double source_sln_thr, uint64_t ijt);
    bool update();
    void report(int64_t lag) const;
};

class dll_export ShutdownTimeCore:
    public TimeJob
{
public:
    ShutdownTimeCore();
    bool update();
};

template<class M> class MonitoringJob:
    public TimeJob
{
public:
    core::P<M> monitor;
    MonitoringJob(M *monitor, uint64_t deadline): TimeJob(deadline), monitor(monitor) {}
    bool update()
    {
        monitor->update(target_time);
        return true;
    }
    virtual bool shouldRunAgain() const
    {
        return (target_time != 0);
    }
    bool is_alive() const
    {
        return monitor->is_alive();
    }
    void report(int64_t lag) const
    {
        debug("monitoring job") << "late:" << lag << "us behind.";
    }
};

class dll_export PerfSamplingJob:
    public TimeJob
{
public:
    uint64_t period;
    PerfSamplingJob(uint64_t start, uint64_t period);
    bool update();
    bool shouldRunAgain() const
    {
        return true;
    }
};
}


#endif
