//	mem.cpp
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

#include "mem.h"

#include <ext/alloc_traits.h>       // for __alloc_traits<>::value_type
#include <r_code/replicode_defs.h>  // for HLP_FWD_GUARDS, HLP_OUT_GRPS, etc
#include <r_comp/segments.h>        // for Image
#include <r_exec/factory.h>         // for Fact, Perf
#include <r_exec/init.h>            // for Now
#include <r_exec/mem.h>             // for _Mem, MemStatic, MemVolatile, etc
#include <r_exec/model_base.h>      // for ModelBase
#include <r_exec/object.h>          // for LObject
#include <r_exec/opcodes.h>         // for Opcodes, Opcodes::AntiFact, etc
#include <r_exec/overlay.h>         // for Controller
#include <r_exec/reduction_core.h>  // for runReductionCore
#include <r_exec/reduction_job.h>   // for AsyncInjectionJob, etc
#include <r_exec/time_core.h>       // for runTimeCore
#include <r_exec/time_job.h>        // for EInjectionJob, InjectionJob, etc
#include <r_exec/view.h>            // for View
#include <iostream>                 // for ostream, cout
#include <set>                      // for multiset
#include <unordered_map>            // for _Node_const_iterator, etc
#include <unordered_set>            // for unordered_set, etc
#include <utility>                  // for pair

#include "CoreLibrary/debug.h"      // for debug, DebugStream


namespace r_exec
{

_Mem::_Mem(): r_code::Mem(), state(NOT_STARTED), deleted(false)
{
    new ModelBase();
    objects.reserve(1024);
}

_Mem::~_Mem()
{
    for (std::ostream *stream : debug_streams) {
        delete stream;
    }
}

void _Mem::init(uint64_t base_period,
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
                uint64_t traces)
{
    this->base_period = base_period;
    this->reduction_core_count = reduction_core_count;
    this->time_core_count = time_core_count;
    this->mdl_inertia_sr_thr = mdl_inertia_sr_thr;
    this->mdl_inertia_cnt_thr = mdl_inertia_cnt_thr;
    this->tpx_dsr_thr = tpx_dsr_thr;
    this->min_sim_time_horizon = min_sim_time_horizon;
    this->max_sim_time_horizon = max_sim_time_horizon;
    this->sim_time_horizon = sim_time_horizon;
    this->tpx_time_horizon = tpx_time_horizon;
    this->perf_sampling_period = perf_sampling_period;
    this->float_tolerance = float_tolerance;
    this->time_tolerance = time_tolerance;
    this->primary_thz = primary_thz * 1000000;
    this->secondary_thz = secondary_thz * 1000000;
    this->debug = debug;

    if (debug) {
        this->ntf_mk_res = ntf_mk_res;
    } else {
        this->ntf_mk_res = 1;
    }

    this->goal_pred_success_res = goal_pred_success_res;
    this->probe_level = probe_level;
    reduction_job_count = time_job_count = 0;
    reduction_job_avg_latency = _reduction_job_avg_latency = 0;
    time_job_avg_latency = _time_job_avg_latency = 0;
    uint64_t mask = 1;

    for (auto & stream : debug_streams) {
        if (traces & mask) {
            stream = nullptr;
        } else {
            stream = new NullOStream();
        }

        mask <<= 1;
    }
}

std::ostream &_Mem::Output(TraceLevel l)
{
    return (_Mem::Get()->debug_streams[l] == nullptr ? std::cout : * (_Mem::Get()->debug_streams[l]));
}

////////////////////////////////////////////////////////////////

Code *_Mem::get_root() const
{
    return _root;
}

Code *_Mem::get_stdin() const
{
    return _stdin;
}

Code *_Mem::get_stdout() const
{
    return _stdout;
}

Code *_Mem::get_self() const
{
    return _self;
}

////////////////////////////////////////////////////////////////

_Mem::State _Mem::check_state()
{
    State s;
    std::lock_guard<std::mutex> guard(m_stateMutex);
    s = state;
    return s;
}

void _Mem::start_core()
{
    std::unique_lock<std::mutex> guard(m_coreCountMutex);
    m_coreCount++;
}

void _Mem::shutdown_core()
{
    std::unique_lock<std::mutex> guard(m_coreCountMutex);
    m_coreCount--;
    m_coresRunning.notify_all();
}

////////////////////////////////////////////////////////////////

void _Mem::store(Code *object)
{
    int64_t location;
    objects.push_back(object, location);
    object->set_stroage_index(location);
}

bool _Mem::load(std::vector<r_code::Code *> *objects, uint64_t stdin_oid, uint64_t stdout_oid, uint64_t self_oid)
{
    Utils::SetReferenceValues(base_period, float_tolerance, time_tolerance);
    // load root (always comes first).
    _root = (Group *)(*objects)[0];
    store((Code *)_root);
    initial_groups.push_back(_root);
    set_last_oid(objects->size() - 1);

    for (uint64_t i = 1; i < objects->size(); ++i) { // skip root as it has no initial views.
        Code *object = (*objects)[i];
        store(object);

        if (object->get_oid() == stdin_oid) {
            _stdin = (Group *)(*objects)[i];
        } else if (object->get_oid() == stdout_oid) {
            _stdout = (Group *)(*objects)[i];
        } else if (object->get_oid() == self_oid) {
            _self = (*objects)[i];
        }

        switch (object->code(0).getDescriptor()) {
        case Atom::MODEL:
            unpack_hlp(object);
            //object->add_reference(NULL); // classifier.
            ModelBase::Get()->load(object);
            break;

        case Atom::COMPOSITE_STATE:
            unpack_hlp(object);
            break;

        case Atom::INSTANTIATED_PROGRAM: // refine the opcode depending on the inputs and the program type.
            if (object->get_reference(0)->code(0).asOpcode() == Opcodes::Pgm) {
                if (object->get_reference(0)->code(object->get_reference(0)->code(PGM_INPUTS).asIndex()).getAtomCount() == 0) {
                    object->code(0) = Atom::InstantiatedInputLessProgram(object->code(0).asOpcode(), object->code(0).getAtomCount());
                }
            } else {
                object->code(0) = Atom::InstantiatedAntiProgram(object->code(0).asOpcode(), object->code(0).getAtomCount());
            }

            break;
        }

        std::unordered_set<r_code::View *, r_code::View::Hash, r_code::View::Equal>::const_iterator v;

        for (v = object->views.begin(); v != object->views.end(); ++v) {
            // init hosts' member_set.
            View *view = (r_exec::View *)*v;
            view->set_object(object);
            Group *host = view->get_host();

            if (!host->load(view, object)) {
                return false;
            }
        }

        if (object->code(0).getDescriptor() == Atom::GROUP) {
            initial_groups.push_back((Group *)object);    // convenience to create initial update jobs - see start().
        }
    }

    return true;
}

void _Mem::init_timings(uint64_t now) const   // called at the beginning of _Mem::start(); use initial user-supplied facts' times as offsets from now.
{
    uint64_t time_tolerance = Utils::GetTimeTolerance() * 2;
    r_code::list<P<Code> >::const_iterator o;

    for (o = objects.begin(); o != objects.end(); ++o) {
        uint16_t opcode = (*o)->code(0).asOpcode();

        if (opcode == Opcodes::Fact || opcode == Opcodes::AntiFact) {
            uint64_t after = Utils::GetTimestamp<Code>(*o, FACT_AFTER);
            uint64_t before = Utils::GetTimestamp<Code>(*o, FACT_BEFORE);
            (*o)->trace();

            if (after < Utils::MaxTime - now) {
                Utils::SetIndirectTimestamp<Code>(*o, FACT_AFTER, after + now);
            }

            if (before < Utils::MaxTime - now - time_tolerance) {
                Utils::SetIndirectTimestamp<Code>(*o, FACT_BEFORE, before + now + time_tolerance);
            } else {
                Utils::SetIndirectTimestamp<Code>(*o, FACT_BEFORE, Utils::MaxTime);
            }
        }
    }
}

uint64_t _Mem::start()
{
    if (state != STOPPED && state != NOT_STARTED) {
        return 0;
    }

    m_coreCount = 0;
    std::vector<std::pair<View *, Group *> > initial_reduction_jobs;
    uint64_t i;
    uint64_t now = Now();
    Utils::SetTimeReference(now);
    ModelBase::Get()->set_thz(secondary_thz);
    init_timings(now);

    for (i = 0; i < initial_groups.size(); ++i) {
        Group *g = initial_groups[i];
        bool c_active = g->get_c_act() > g->get_c_act_thr();
        bool c_salient = g->get_c_sln() > g->get_c_sln_thr();
        FOR_ALL_VIEWS_BEGIN(g, v)
        Utils::SetIndirectTimestamp<View>(v->second, VIEW_IJT, now); // init injection time for the view.
        FOR_ALL_VIEWS_END

        if (c_active) {
            std::unordered_map<uint64_t, P<View> >::const_iterator v;

            // build signaling jobs for active input-less overlays.
            for (v = g->input_less_ipgm_views.begin(); v != g->input_less_ipgm_views.end(); ++v) {
                if (v->second->controller != nullptr && v->second->controller->is_activated()) {
                    pushTimeJob(new InputLessPGMSignalingJob(v->second, now + Utils::GetTimestamp<Code>(v->second->object, IPGM_TSC)));
                }
            }

            // build signaling jobs for active anti-pgm overlays.
            for (v = g->anti_ipgm_views.begin(); v != g->anti_ipgm_views.end(); ++v) {
                if (v->second->controller != nullptr && v->second->controller->is_activated()) {
                    pushTimeJob(new AntiPGMSignalingJob(v->second, now + Utils::GetTimestamp<Code>(v->second->object, IPGM_TSC)));
                }
            }
        }

        if (c_salient) {
            // build reduction jobs for each salient view and each active overlay - regardless of the view's sync mode.
            FOR_ALL_VIEWS_BEGIN(g, v)

            if (v->second->get_sln() > g->get_sln_thr()) { // salient view.
                g->newly_salient_views.insert(v->second);
                initial_reduction_jobs.push_back(std::pair<View *, Group *>(v->second, g));
            }

            FOR_ALL_VIEWS_END
        }

        if (g->get_upr() > 0) { // inject the next update job for the group.
            pushTimeJob(new UpdateJob(g, g->get_next_upr_time(now)));
        }
    }

    initial_groups.clear();
    state = RUNNING;
    pushTimeJob(new PerfSamplingJob(now + perf_sampling_period, perf_sampling_period));

    for (i = 0; i < reduction_core_count; ++i) {
        m_coreThreads.push_back(std::thread(&r_exec::runReductionCore));
    }

    for (i = 0; i < time_core_count; ++i) {
        m_coreThreads.push_back(std::thread(&r_exec::runTimeCore));
    }

    for (auto & initial_reduction_job : initial_reduction_jobs) {
        initial_reduction_job.second->inject_reduction_jobs(initial_reduction_job.first);
    }

    return now;
}

void _Mem::stop()
{
    m_stateMutex.lock();

    if (state != RUNNING) {
        ::debug("mem") << "memory asked to stop while not running";
        return;
    }

    // We need to do this because things are wait()ing
    uint64_t i;

    for (i = 0; i < reduction_core_count; ++i) {
        pushReductionJob(new ShutdownReductionCore());
    }

    for (i = 0; i < time_core_count; ++i) {
        pushTimeJob(new ShutdownTimeCore());
    }

    state = STOPPED;
    m_stateMutex.unlock();

    for (i = 0; i < m_coreThreads.size(); ++i) {
        m_coreThreads[i].join();
    }

    std::unique_lock<std::mutex> lock(m_coreCountMutex);

    if (m_coreCount > 0) {
        m_coresRunning.wait(lock);
    }

    m_coreThreads.clear();
}

////////////////////////////////////////////////////////////////

_ReductionJob *_Mem::popReductionJob()
{
    if (state == STOPPED) {
        return nullptr;
    }

    return m_reductionJobQueue.popJob();
}

void _Mem::pushReductionJob(_ReductionJob *j)
{
    if (state == STOPPED) {
        return;
    }

    j->ijt = Now();
    m_reductionJobQueue.pushJob(j);
}

TimeJob *_Mem::popTimeJob()
{
    if (state == STOPPED) {
        return nullptr;
    }

    return m_timeJobQueue.popJob();
}

void _Mem::pushTimeJob(r_exec::TimeJob *j)
{
    if (state == STOPPED) {
        return;
    }

    m_timeJobQueue.pushJob(j);
}

////////////////////////////////////////////////////////////////

void _Mem::eject(View *view, uint16_t nodeID)
{
}

void _Mem::eject(Code *command)
{
}

////////////////////////////////////////////////////////////////

void _Mem::inject_copy(View *view, Group *destination)
{
    View *copied_view = new View(view, destination); // ctrl values are morphed.
    inject_existing_object(copied_view, view->object, destination);
}

void _Mem::inject_existing_object(View *view, Code *object, Group *host)
{
    view->set_object(object); // the object already exists (content-wise): have the view point to the existing one.
    host->inject_existing_object(view);
}

void _Mem::inject_null_program(Controller *c, Group *group, uint64_t time_to_live, bool take_past_inputs)
{
    uint64_t now = Now();
    Code *null_pgm = new LObject();
    null_pgm->code(0) = Atom::NullProgram(take_past_inputs);
    uint64_t res = Utils::GetResilience(now, time_to_live, group->get_upr() * Utils::GetBasePeriod());
    View *view = new View(View::SYNC_ONCE, now, 0, res, group, nullptr, null_pgm, 1);
    view->controller = c;
    c->set_view(view);
    inject(view);
}

void _Mem::inject_new_object(View *view)
{
    Group *host = view->get_host();

    //uint64_t t0,t1,t2;
    switch (view->object->code(0).getDescriptor()) {
    case Atom::GROUP:
        bind(view);
        host->inject_group(view);
        break;

    default:
        //t0=Now();
        bind(view);
        //t1=Now();
        host->inject_new_object(view);
        //t2=Now();
        //timings_report.push_back(t2-t0);
        break;
    }
}

void _Mem::inject(View *view)
{
    if (view->object->is_invalidated()) {
        return;
    }

    Group *host = view->get_host();

    if (host->is_invalidated()) {
        return;
    }

    uint64_t now = Now();
    uint64_t ijt = view->get_ijt();

    if (view->object->is_registered()) { // existing object.
        if (ijt <= now) {
            inject_existing_object(view, view->object, host);
        } else {
            pushTimeJob(new EInjectionJob(view, ijt));
        }
    } else { // new object.
        if (ijt <= now) {
            inject_new_object(view);
        } else {
            pushTimeJob(new InjectionJob(view, ijt));
        }
    }
}

void _Mem::inject_async(View *view)
{
    if (view->object->is_invalidated()) {
        return;
    }

    Group *host = view->get_host();

    if (host->is_invalidated()) {
        return;
    }

    uint64_t now = Now();
    uint64_t ijt = view->get_ijt();

    if (ijt <= now) {
        P<_ReductionJob> j = new AsyncInjectionJob(view);
        pushReductionJob(j);
    } else {
        if (view->object->is_registered()) { // existing object.
            pushTimeJob(new EInjectionJob(view, ijt));
        } else {
            pushTimeJob(new InjectionJob(view, ijt));
        }
    }
}

void _Mem::inject_hlps(std::vector<View *> views, Group *destination)
{
    std::vector<View *>::const_iterator view;

    for (view = views.begin(); view != views.end(); ++view) {
        bind(*view);
    }

    destination->inject_hlps(views);
}

void _Mem::inject_notification(View *view, bool lock)   // no notification for notifications; no cov.
{
    // notifications are ephemeral: they are not held by the marker sets of the object they refer to; this implies no propagation of saliency changes trough notifications.
    Group *host = view->get_host();
    bind(view);
    host->inject_notification(view, lock);
}

////////////////////////////////////////////////////////////////

void _Mem::register_reduction_job_latency(uint64_t latency)
{
    std::lock_guard<std::mutex> guard(m_reductionJobMutex);
    ++reduction_job_count;
    reduction_job_avg_latency += latency;
}

void _Mem::register_time_job_latency(uint64_t latency)
{
    std::lock_guard<std::mutex> guard(m_timeJobMutex);
    ++time_job_count;
    time_job_avg_latency += latency;
}

void _Mem::inject_perf_stats()
{
    m_reductionJobMutex.lock();
    m_timeJobMutex.lock();
    int64_t d_reduction_job_avg_latency;

    if (reduction_job_count > 0) {
        reduction_job_avg_latency /= reduction_job_count;
        d_reduction_job_avg_latency = reduction_job_avg_latency - _reduction_job_avg_latency;
    } else {
        reduction_job_avg_latency = d_reduction_job_avg_latency = 0;
    }

    int64_t d_time_job_avg_latency;

    if (time_job_count > 0) {
        time_job_avg_latency /= time_job_count;
        d_time_job_avg_latency = time_job_avg_latency - _time_job_avg_latency;
    } else {
        time_job_avg_latency = d_time_job_avg_latency = 0;
    }

    Code *perf = new Perf(reduction_job_avg_latency, d_reduction_job_avg_latency, time_job_avg_latency, d_time_job_avg_latency);
    // reset stats.
    reduction_job_count = time_job_count = 0;
    _reduction_job_avg_latency = reduction_job_avg_latency;
    _time_job_avg_latency = time_job_avg_latency;
    m_timeJobMutex.unlock();
    m_reductionJobMutex.unlock();
    // inject f->perf in stdin.
    uint64_t now = Now();
    Code *f_perf = new Fact(perf, now, now + perf_sampling_period, 1, 1);
    View *view = new View(View::SYNC_ONCE, now, 1, 1, _stdin, nullptr, f_perf); // sync front, sln=1, res=1.
    inject(view);
}

////////////////////////////////////////////////////////////////

void _Mem::propagate_sln(Code *object, double change, double source_sln_thr)
{
    // apply morphed change to views.
    // loops are prevented within one call, but not accross several upr:
    // - feedback can happen, i.e. m:(mk o1 o2); o1.vw.g propag -> o1 propag ->m propag -> o2 propag o2.vw.g, next upr in g, o2 propag -> m propag -> o1 propag -> o1,vw.g: loop spreading accross several upr.
    // - to avoid this, have the psln_thr set to 1 in o2: this is applicaton-dependent.
    object->acq_views();

    if (object->views.size() == 0) {
        object->invalidate();
        object->rel_views();
        return;
    }

    std::unordered_set<r_code::View *, r_code::View::Hash, r_code::View::Equal>::const_iterator it;

    for (it = object->views.begin(); it != object->views.end(); ++it) {
        double morphed_sln_change = View::MorphChange(change, source_sln_thr, ((r_exec::View*)*it)->get_host()->get_sln_thr());

        if (morphed_sln_change != 0) {
            ((r_exec::View*)*it)->get_host()->pending_operations.push_back(new Group::Mod(((r_exec::View*)*it)->get_oid(), VIEW_SLN, morphed_sln_change));
        }
    }

    object->rel_views();
}

////////////////////////////////////////////////////////////////

void _Mem::unpack_hlp(Code *hlp) const   // produces a new object (featuring a set of pattern objects instread of a set of embedded pattern expressions) and add it as a hidden reference to the original (still packed) hlp.
{
    Code *unpacked_hlp = new LObject(); // will not be transmitted nor decompiled.

    for (uint16_t i = 0; i < hlp->code_size(); ++i) {
        unpacked_hlp->code(i) = hlp->code(i);
    }

    uint16_t pattern_set_index = hlp->code(HLP_OBJS).asIndex();
    uint16_t pattern_count = hlp->code(pattern_set_index).getAtomCount();

    for (uint16_t i = 1; i <= pattern_count; ++i) { // init the new references with the facts; turn the exisitng i-ptrs into r-ptrs.
        Code *fact = unpack_fact(hlp, hlp->code(pattern_set_index + i).asIndex());
        unpacked_hlp->add_reference(fact);
        unpacked_hlp->code(pattern_set_index + i) = Atom::RPointer(unpacked_hlp->references_size() - 1);
    }

    uint16_t group_set_index = hlp->code(HLP_OUT_GRPS).asIndex();
    uint16_t group_count = hlp->code(group_set_index++).getAtomCount();

    for (uint16_t i = 0; i < group_count; ++i) { // append the out_groups to the new references; adjust the exisitng r-ptrs.
        unpacked_hlp->add_reference(hlp->get_reference(hlp->code(group_set_index + i).asIndex()));
        unpacked_hlp->code(group_set_index + i) = Atom::RPointer(unpacked_hlp->references_size() - 1);
    }

    uint16_t invalid_point = pattern_set_index + pattern_count + 1; // index of what is after set of the patterns.
    uint16_t valid_point = hlp->code(HLP_FWD_GUARDS).asIndex(); // index of the first atom that does not belong to the patterns.
    uint16_t invalid_zone_length = valid_point - invalid_point;

    for (uint16_t i = valid_point; i < hlp->code_size(); ++i) { // shift the valid code upward; adjust i-ptrs.
        Atom h_atom = hlp->code(i);

        switch (h_atom.getDescriptor()) {
        case Atom::I_PTR:
            unpacked_hlp->code(i - invalid_zone_length) = Atom::IPointer(h_atom.asIndex() - invalid_zone_length);
            break;

        case Atom::ASSIGN_PTR:
            unpacked_hlp->code(i - invalid_zone_length) = Atom::AssignmentPointer(h_atom.asAssignmentIndex(), h_atom.asIndex() - invalid_zone_length);
            break;

        default:
            unpacked_hlp->code(i - invalid_zone_length) = h_atom;
            break;
        }
    }

    // adjust set indices.
    unpacked_hlp->code(HLP_FWD_GUARDS) = Atom::IPointer(hlp->code(CST_FWD_GUARDS).asIndex() - invalid_zone_length);
    unpacked_hlp->code(HLP_BWD_GUARDS) = Atom::IPointer(hlp->code(CST_BWD_GUARDS).asIndex() - invalid_zone_length);
    unpacked_hlp->code(HLP_OUT_GRPS) = Atom::IPointer(hlp->code(CST_OUT_GRPS).asIndex() - invalid_zone_length);
    uint16_t unpacked_code_length = hlp->code_size() - invalid_zone_length;
    unpacked_hlp->resize_code(unpacked_code_length);
    hlp->add_reference(unpacked_hlp);
}

Code *_Mem::unpack_fact(Code *hlp, uint16_t fact_index) const
{
    Code *fact = new LObject();
    Code *fact_object;
    uint16_t fact_size = hlp->code(fact_index).getAtomCount() + 1;

    for (uint16_t i = 0; i < fact_size; ++i) {
        Atom h_atom = hlp->code(fact_index + i);

        switch (h_atom.getDescriptor()) {
        case Atom::I_PTR:
            fact->code(i) = Atom::RPointer(fact->references_size());
            fact_object = unpack_fact_object(hlp, h_atom.asIndex());
            fact->add_reference(fact_object);
            break;

        case Atom::R_PTR: // case of a reference to an exisitng object.
            fact->code(i) = Atom::RPointer(fact->references_size());
            fact->add_reference(hlp->get_reference(h_atom.asIndex()));
            break;

        default:
            fact->code(i) = h_atom;
            break;
        }
    }

    return fact;
}

Code *_Mem::unpack_fact_object(Code *hlp, uint16_t fact_object_index) const
{
    Code *fact_object = new LObject();
    _unpack_code(hlp, fact_object_index, fact_object, fact_object_index);
    return fact_object;
}

void _Mem::_unpack_code(Code *hlp, uint16_t fact_object_index, Code *fact_object, uint16_t read_index) const
{
    Atom h_atom = hlp->code(read_index);
    uint16_t code_size = h_atom.getAtomCount() + 1;
    uint16_t write_index = read_index - fact_object_index;

    for (uint16_t i = 0; i < code_size; ++i) {
        switch (h_atom.getDescriptor()) {
        case Atom::R_PTR:
            fact_object->code(write_index + i) = Atom::RPointer(fact_object->references_size());
            fact_object->add_reference(hlp->get_reference(h_atom.asIndex()));
            break;

        case Atom::I_PTR:
            fact_object->code(write_index + i) = Atom::IPointer(h_atom.asIndex() - fact_object_index);
            _unpack_code(hlp, fact_object_index, fact_object, h_atom.asIndex());
            break;

        default:
            fact_object->code(write_index + i) = h_atom;
            break;
        }

        h_atom = hlp->code(read_index + i + 1);
    }
}

void _Mem::pack_hlp(Code *hlp) const   // produces a new object where a set of pattern objects is transformed into a packed set of pattern code.
{
    Code *unpacked_hlp = clone(hlp);
    std::vector<Atom> trailing_code; // copy of the original code (the latter will be overwritten by packed facts).
    uint16_t trailing_code_index = hlp->code(HLP_FWD_GUARDS).asIndex();

    for (uint16_t i = trailing_code_index; i < hlp->code_size(); ++i) {
        trailing_code.push_back(hlp->code(i));
    }

    uint16_t group_set_index = hlp->code(HLP_OUT_GRPS).asIndex();
    uint16_t group_count = hlp->code(group_set_index).getAtomCount();
    std::vector<P<Code> > references;
    uint16_t pattern_set_index = hlp->code(HLP_OBJS).asIndex();
    uint16_t pattern_count = hlp->code(pattern_set_index).getAtomCount();
    uint16_t insertion_point = pattern_set_index + pattern_count + 1; // point from where compacted code is to be inserted.
    uint16_t extent_index = insertion_point;

    for (uint16_t i = 0; i < pattern_count; ++i) {
        Code *pattern_object = hlp->get_reference(i);
        hlp->code(pattern_set_index + i + 1) = Atom::IPointer(extent_index);
        pack_fact(pattern_object, hlp, extent_index, &references);
    }

    uint16_t inserted_zone_length = extent_index - insertion_point;

    for (uint16_t i = 0; i < trailing_code.size(); ++i) { // shift the trailing code downward; adjust i-ptrs.
        Atom t_atom = trailing_code[i];

        switch (t_atom.getDescriptor()) {
        case Atom::I_PTR:
            hlp->code(i + extent_index) = Atom::IPointer(t_atom.asIndex() + inserted_zone_length);
            break;

        case Atom::ASSIGN_PTR:
            hlp->code(i + extent_index) = Atom::AssignmentPointer(t_atom.asAssignmentIndex(), t_atom.asIndex() + inserted_zone_length);
            break;

        default:
            hlp->code(i + extent_index) = t_atom;
            break;
        }
    }

    // adjust set indices.
    hlp->code(CST_FWD_GUARDS) = Atom::IPointer(hlp->code(HLP_FWD_GUARDS).asIndex() + inserted_zone_length);
    hlp->code(CST_BWD_GUARDS) = Atom::IPointer(hlp->code(HLP_BWD_GUARDS).asIndex() + inserted_zone_length);
    hlp->code(CST_OUT_GRPS) = Atom::IPointer(hlp->code(HLP_OUT_GRPS).asIndex() + inserted_zone_length);
    group_set_index += inserted_zone_length;

    for (uint16_t i = 1; i <= group_count; ++i) { // append the out_groups to the new references; adjust the exisitng r-ptrs.
        references.push_back(hlp->get_reference(hlp->code(group_set_index + i).asIndex()));
        hlp->code(group_set_index + i) = Atom::RPointer(references.size() - 1);
    }

    hlp->set_references(references);
    hlp->add_reference(unpacked_hlp); // hidden reference.
}

void _Mem::pack_fact(Code *fact, Code *hlp, uint16_t &write_index, std::vector<P<Code> > *references) const
{
    uint16_t extent_index = write_index + fact->code_size();

    for (uint16_t i = 0; i < fact->code_size(); ++i) {
        Atom p_atom = fact->code(i);

        switch (p_atom.getDescriptor()) {
        case Atom::R_PTR: // transform into a i_ptr and pack the pointed object.
            hlp->code(write_index) = Atom::IPointer(extent_index);
            pack_fact_object(fact->get_reference(p_atom.asIndex()), hlp, extent_index, references);
            ++write_index;
            break;

        default:
            hlp->code(write_index) = p_atom;
            ++write_index;
            break;
        }
    }

    write_index = extent_index;
}

void _Mem::pack_fact_object(Code *fact_object, Code *hlp, uint16_t &write_index, std::vector<P<Code> > *references) const
{
    uint16_t offset = write_index;

    for (uint16_t i = 0; i < fact_object->code_size(); ++i) {
        Atom p_atom = fact_object->code(i);

        switch (p_atom.getDescriptor()) {
        case Atom::R_PTR: { // append this reference to the hlp's if not already there.
            Code *reference = fact_object->get_reference(p_atom.asIndex());
            bool found = false;

            for (uint16_t i = 0; i < references->size(); ++i) {
                if ((*references)[i] == reference) {
                    hlp->code(write_index) = Atom::RPointer(i);
                    found = true;
                    break;
                }
            }

            if (!found) {
                hlp->code(write_index) = Atom::RPointer(references->size());
                references->push_back(reference);
            }

            ++write_index;
            break;
        }

        case Atom::I_PTR: // offset the ptr by write_index. PB HERE.
            hlp->code(write_index) = Atom::IPointer(offset + p_atom.asIndex());
            ++write_index;
            break;

        default:
            hlp->code(write_index) = p_atom;
            ++write_index;
            break;
        }
    }
}

Code *_Mem::clone(Code *original) const   // shallow copy; oid not copied.
{
    Code *_clone = build_object(original->code(0));
    uint16_t opcode = original->code(0).asOpcode();

    if (opcode == Opcodes::Ont || opcode == Opcodes::Ent) {
        return original;
    }

    for (uint16_t i = 0; i < original->code_size(); ++i) {
        _clone->code(i) = original->code(i);
    }

    for (uint16_t i = 0; i < original->references_size(); ++i) {
        _clone->add_reference(original->get_reference(i));
    }

    return _clone;
}

////////////////////////////////////////////////////////////////

r_comp::Image *_Mem::get_models()
{
    r_comp::Image *image = new r_comp::Image();
    image->timestamp = Now();
    r_code::list<P<Code> > models;
    ModelBase::Get()->get_models(models); // protected by ModelBase.
    image->add_objects_full(models);
    return image;
}

////////////////////////////////////////////////////////////////

MemStatic::MemStatic(): _Mem(), last_oid(-1)
{
}

MemStatic::~MemStatic()
{
}

void MemStatic::bind(View *view)
{
    Code *object = view->object;
    object->views.insert(view);
    std::lock_guard<std::mutex> guard(m_objectsMutex);
    object->set_oid(++last_oid);

    if (object->code(0).getDescriptor() == Atom::NULL_PROGRAM) {
        return;
    }

    int64_t location;
    objects.push_back(object, location);
    object->set_stroage_index(location);
}
void MemStatic::set_last_oid(int64_t oid)
{
    last_oid = oid;
}

void MemStatic::delete_object(r_code::Code *object)   // called only if the object is registered, i.e. has a valid storage index.
{
    if (deleted) {
        return;
    }

    std::lock_guard<std::mutex> guard(m_objectsMutex);
    objects.erase(object->get_storage_index());
}

r_comp::Image *MemStatic::get_objects()
{
    r_comp::Image *image = new r_comp::Image();
    image->timestamp = Now();
    std::lock_guard<std::mutex> guard(m_objectsMutex);
    image->add_objects(objects);
    return image;
}

////////////////////////////////////////////////////////////////

MemVolatile::MemVolatile(): _Mem(), last_oid(-1)
{
}

MemVolatile::~MemVolatile()
{
}

uint64_t MemVolatile::get_oid()
{
    return ++last_oid;
}

void MemVolatile::set_last_oid(int64_t oid)
{
    last_oid = oid;
}

void MemVolatile::bind(View *view)
{
    Code *object = view->object;
    object->views.insert(view);
    object->set_oid(get_oid());
}

} //namespace r_core
