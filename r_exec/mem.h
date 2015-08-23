//	mem.h
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

#ifndef mem_h
#define mem_h

#include <r_code/atom.h>       // for Atom
#include <r_code/list.h>       // for list
#include <r_code/object.h>     // for Mem
#include <r_code/utils.h>      // for Utils
#include <r_exec/group.h>      // for Group
#include <stddef.h>            // for NULL
#include <stdint.h>            // for uint64_t, uint16_t, int64_t
#include <atomic>              // for atomic, atomic_int_fast64_t
#include <condition_variable>  // for condition_variable
#include <iosfwd>              // for ostream
#include <mutex>               // for mutex, unique_lock
#include <queue>               // for queue
#include <thread>              // for thread
#include <vector>              // for vector

#include <replicode_common.h>  // for P
#include <replicode_common.h>   // for REPLICODE_EXPORT

namespace r_comp {
class Image;
class Metadata;
}  // namespace r_comp
namespace r_exec {
class Controller;
class TimeJob;
class View;
class _ReductionJob;
}  // namespace r_exec

namespace r_exec
{

// The rMem.
// Maintains 2 pipes of jobs (injection, update, etc.). each job is processed asynchronously by instances of ReductionCore and TimeCore.
// Pipes and threads are created at starting time and deleted at stopping time.
// Groups and IPGMControllers are cleared up when only held by jobs;
// - when a group is not projected anywhere anymore, it is invalidated (it releases all its views) and when a job attempts an update, the latter is cancelled.
// - when a reduction core attempts to perform a reduction for an ipgm-controller that is not projected anywhere anymore, the reduction is cancelled.
// In addition:
// - when an object is scheduled for injection and the target group does not exist anymore (or is invalidated), the injection is cancelled.
// - when an object is scheduled for propagation of sln changes and has no view anymore, the operation is cancelled.
// Main processing in _Mem::update().
class REPLICODE_EXPORT _Mem:
    public r_code::Mem
{
public:
    typedef enum {
        NOT_STARTED = 0,
        RUNNING = 1,
        STOPPED = 2
    } State;
protected:
    // Parameters::Init.
    uint64_t base_period;
    uint64_t reduction_core_count;
    uint64_t time_core_count;

    // Parameters::System.
    double mdl_inertia_sr_thr;
    uint64_t mdl_inertia_cnt_thr;
    double tpx_dsr_thr;
    uint64_t min_sim_time_horizon;
    uint64_t max_sim_time_horizon;
    double sim_time_horizon;
    uint64_t tpx_time_horizon;
    uint64_t perf_sampling_period;
    double float_tolerance;
    uint64_t time_tolerance;
    uint64_t primary_thz;
    uint64_t secondary_thz;

    // Parameters::Debug.
    bool debug;
    uint64_t ntf_mk_res;
    uint64_t goal_pred_success_res;

    // Parameters::Run.
    uint64_t probe_level;

    template <class Type> struct JobQueue {
        void pushJob(Type *job)
        {
            std::unique_lock<std::mutex> lock(m_pushMutex);
            m_mutex.lock();

            while (m_jobs.size() > 1024) { // while, because spurious wakeups
                m_mutex.unlock();
                m_canPushCondition.wait(lock);
                m_mutex.lock();
            }

            m_jobs.push(job);

            if (m_jobs.size() == 1) {
                m_canPopCondition.notify_all();
            }

            m_mutex.unlock();
        }

        Type *popJob()
        {
            std::unique_lock<std::mutex> lock(m_popMutex);
            m_mutex.lock();

            while (m_jobs.size() < 1) { // while, because of spurious wakeups
                m_mutex.unlock();
                m_canPopCondition.wait(lock);
                m_mutex.lock();
            }

            Type *r = m_jobs.front();
            m_jobs.pop();

            if (m_jobs.size() < 1024) {
                m_canPushCondition.notify_all();
            }

            m_mutex.unlock();
            return r;
        }

    private:
        std::mutex m_mutex;
        std::queue<Type*> m_jobs;

        std::mutex m_pushMutex;
        std::condition_variable m_canPushCondition;
        std::mutex m_popMutex;
        std::condition_variable m_canPopCondition;
    };

    JobQueue<_ReductionJob> m_reductionJobQueue;
    JobQueue<TimeJob> m_timeJobQueue;
    std::mutex m_timeJobMutex;
    std::mutex m_reductionJobMutex;

    std::vector<std::thread> m_coreThreads;

    // Performance stats.
    uint64_t reduction_job_count;
    uint64_t reduction_job_avg_latency; // latency: popping time.-pushing time; the lower the better.
    uint64_t _reduction_job_avg_latency; // previous value.
    uint64_t time_job_count;
    uint64_t time_job_avg_latency; // latency: deadline-the time the job is popped from the pipe; if <0, not registered (as it is too late for action); the higher the better.
    uint64_t _time_job_avg_latency; // previous value.

    std::atomic<uint64_t> m_coreCount;
    std::condition_variable m_coresRunning;
    std::mutex m_coreCountMutex; // blocks the rMem until all cores terminate.

    State state;
    std::mutex m_stateMutex;


    r_code::list<core::P<r_code::Code> > objects; // store objects in order of injection: holds the initial objects (and dynamically created ones if MemStatic is used).

    P<Group> _root; // holds everything.
    r_code::Code *_stdin;
    r_code::Code *_stdout;
    r_code::Code *_self;

    std::vector<Group *> initial_groups; // convenience; cleared after start();

    void init_timings(uint64_t now) const;

    void store(r_code::Code *object);
    virtual void set_last_oid(int64_t oid) = 0;
    virtual void bind(View *view) = 0;

    bool deleted;

    static const uint64_t DebugStreamCount = 8;
    std::ostream *debug_streams[8];

    _Mem();

    void _unpack_code(r_code::Code *hlp, uint16_t fact_object_index, r_code::Code *fact_object, uint16_t read_index) const;
public:
    static _Mem *Get()
    {
        return (_Mem *)Mem::Get();
    }

    typedef enum {
        STDIN = 0,
        STDOUT = 1
    } STDGroupID;

    virtual ~_Mem();

    void init(uint64_t base_period,
              uint64_t reduction_core_count,
              uint64_t time_core_count,
              double mdl_inertia_sr_thr,
              uint64_t mdl_inertia_cnt_thr,
              double tpx_dsr_thr,
              uint64_t min_sim_time_horizon,
              uint64_t max_sim_time_horizon,
              double sim_time_horizon,
              uint64_t tpx_time_horizon,
              uint64_t perf_sampling_period,
              double float_tolerance,
              uint64_t time_tolerance,
              uint64_t primary_thz,
              uint64_t secondary_thz,
              bool debug,
              uint64_t ntf_mk_res,
              uint64_t goal_pred_success_res,
              uint64_t probe_level,
              uint64_t traces);

    uint64_t get_probe_level() const
    {
        return probe_level;
    }
    double get_mdl_inertia_sr_thr() const
    {
        return mdl_inertia_sr_thr;
    }
    uint64_t get_mdl_inertia_cnt_thr() const
    {
        return mdl_inertia_cnt_thr;
    }
    double get_tpx_dsr_thr() const
    {
        return tpx_dsr_thr;
    }
    uint64_t get_min_sim_time_horizon() const
    {
        return min_sim_time_horizon;
    }
    uint64_t get_max_sim_time_horizon() const
    {
        return max_sim_time_horizon;
    }
    uint64_t get_sim_time_horizon(uint64_t horizon) const
    {
        return horizon * sim_time_horizon;
    }
    uint64_t get_tpx_time_horizon() const
    {
        return tpx_time_horizon;
    }
    uint64_t get_primary_thz() const
    {
        return primary_thz;
    }
    uint64_t get_secondary_thz() const
    {
        return secondary_thz;
    }

    bool get_debug() const
    {
        return debug;
    }
    uint64_t get_ntf_mk_res() const
    {
        return ntf_mk_res;
    }
    uint64_t get_goal_pred_success_res(Group *host, uint64_t now, uint64_t time_to_live) const
    {
        if (debug) {
            return goal_pred_success_res;
        }

        if (time_to_live == 0) {
            return 1;
        }

        return r_code::Utils::GetResilience(now, time_to_live, host->get_upr());
    }

    r_code::Code *get_root() const;
    r_code::Code *get_stdin() const;
    r_code::Code *get_stdout() const;
    r_code::Code *get_self() const;

    State check_state(); // called by delegates after waiting in case stop() is called in the meantime.
    void start_core(); // called upon creation of a delegate.
    void shutdown_core(); // called upon completion of a delegate's task.

    /// call before start; no mod/set/eje will be executed (only inj);
    /// no cov at init time.
    bool load(std::vector<r_code::Code *> *objects, uint64_t stdin_oid, uint64_t stdout_oid, uint64_t self_oid);
    // return false on error.
    uint64_t start(); // return the starting time.
    void stop(); // after stop() the content is cleared and one has to call load() and start() again.

    // Internal core processing ////////////////////////////////////////////////////////////////

    _ReductionJob *popReductionJob();
    void pushReductionJob(_ReductionJob *j);
    TimeJob *popTimeJob();
    void pushTimeJob(TimeJob *j);

    // Called upon successful reduction.
    void inject(View *view);
    void inject_async(View *view);
    void inject_new_object(View *view);
    void inject_existing_object(View *view, r_code::Code *object, Group *host);
    void inject_null_program(Controller *c, Group *group, uint64_t time_to_live, bool take_past_inputs); // build a view v (ijt=now, act=1, sln=0, res according to time_to_live in the group), attach c to v, inject v in the group.
    void inject_hlps(std::vector<View *> views, Group *destination);
    void inject_notification(View *view, bool lock);
    virtual r_code::Code *check_existence(r_code::Code *object) = 0; // returns the existing object if any, or object otherwise: in the latter case, packing may occur.

    void propagate_sln(r_code::Code *object, double change, double source_sln_thr);

    // Called by groups.
    void inject_copy(View *view, Group *destination); // for cov; NB: no cov for groups, r-groups, models, pgm or notifications.

    // Called by cores.
    void register_reduction_job_latency(uint64_t latency);
    void register_time_job_latency(uint64_t latency);
    void inject_perf_stats();

    // rMem to rMem.
    // The view must contain the destination group (either stdin or stdout) as its grp member.
    // To be redefined by object transport aware subcalsses.
    virtual void eject(View *view, uint16_t nodeID);

    // From rMem to I/O device.
    // To be redefined by object transport aware subcalsses.
    virtual void eject(r_code::Code *command);

    virtual r_code::Code *_build_object(r_code::Atom head) const = 0;
    virtual r_code::Code *build_object(r_code::Atom head) const = 0;

    // unpacking of high-level patterns: upon loading or reception.
    void unpack_hlp(r_code::Code *hlp) const;
    r_code::Code *unpack_fact(r_code::Code *hlp, uint16_t fact_index) const;
    r_code::Code *unpack_fact_object(r_code::Code *hlp, uint16_t fact_object_index) const;

    // packing of high-level patterns: upon dynamic generation or transmission.
    void pack_hlp(r_code::Code *hlp) const;
    void pack_fact(r_code::Code *fact, r_code::Code *hlp, uint16_t &write_index, std::vector<core::P<r_code::Code> > *references) const;
    void pack_fact_object(r_code::Code *fact_object, r_code::Code *hlp, uint16_t &write_index, std::vector<core::P<r_code::Code> > *references) const;

    r_code::Code *clone(r_code::Code *original) const; // shallow copy.

    // External device I/O ////////////////////////////////////////////////////////////////
    virtual r_comp::Image *get_objects() = 0; // create an image; fill with all objects; call only when stopped.
    r_comp::Image *get_models(); // create an image; fill with all models; call only when stopped.

    //std::vector<uint64> timings_report; // debug facility.
    r_comp::Metadata *metadata = nullptr;
    typedef enum {
        CST_IN = 0,
        CST_OUT = 1,
        MDL_IN = 2,
        MDL_OUT = 3,
        PRED_MON = 4,
        GOAL_MON = 5,
        MDL_REV = 6,
        HLP_INJ = 7
    } TraceLevel;
    static std::ostream &Output(TraceLevel l);
};


#define OUTPUT(c) _Mem::Output(_Mem::c)

// _Mem that stores the objects as long as they are not invalidated.
class REPLICODE_EXPORT MemStatic:
    public _Mem
{
private:
    std::mutex m_objectsMutex; // protects last_oid and objects.
    uint64_t last_oid;
    void bind(View *view); // assigns an oid, stores view->object in objects if needed.
    void set_last_oid(int64_t oid);
protected:
    MemStatic();
public:
    virtual ~MemStatic();

    void delete_object(r_code::Code *object); // erase the object from objects if needed.

    r_comp::Image *get_objects(); // return an image containing valid objects.
};

// _Mem that does not store objects.
class REPLICODE_EXPORT MemVolatile:
    public _Mem
{
private:
    std::atomic_int_fast64_t last_oid;
    uint64_t get_oid();
    void bind(View *view); // assigns an oid (atomic operation).
    void set_last_oid(int64_t oid);
protected:
    MemVolatile();
public:
    virtual ~MemVolatile();

    void delete_object(r_code::Code *object) {}

    r_comp::Image *get_objects()
    {
        return NULL;
    }
};

// O is the class of the objects held by the rMem (except groups and notifications):
// r_exec::LObject if non distributed, or
// RObject (see the integration project) when network-aware.
// Notification objects and groups are instances of r_exec::LObject (they are not network-aware).
// Objects are built at reduction time as r_exec:LObjects and packed into instances of O when O is network-aware.
// S is the super-class.
template<class O, class S> class Mem:
    public S
{
public:
    Mem();
    virtual ~Mem();

    // Called at load time.
    r_code::Code *build_object(r_code::SysObject *source) const;

    // Called at runtime.
    r_code::Code *_build_object(r_code::Atom head) const;
    r_code::Code *build_object(r_code::Atom head) const;

    // Executive device functions ////////////////////////////////////////////////////////

    r_code::Code *check_existence(r_code::Code *object);

    // Called by the communication device (I/O).
    void inject(O *object, View *view);
};

}

#include <r_exec/mem.tpl.h>

#endif
