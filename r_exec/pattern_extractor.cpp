//	pattern_extractor.cpp
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

#include <r_code/atom.h>               // for Atom
#include <r_code/list.h>               // for list<>::const_iterator, list, etc
#include <r_code/object.h>             // for Code, View::::SYNC_AXIOM, etc
#include <r_code/replicode_defs.h>     // for FACT_AFTER, MK_VAL_VALUE, etc
#include <r_code/time_buffer.h>        // for time_buffer<>::iterator, etc
#include <r_code/utils.h>              // for Utils
#include <r_exec/auto_focus.h>         // for AutoFocusController
#include <r_exec/binding_map.h>        // for BindingMap, HLPBindingMap
#include <r_exec/cst_controller.h>     // for CSTController
#include <r_exec/factory.h>            // for _Fact, ICST, Fact
#include <r_exec/group.h>              // for Group
#include <r_exec/guard_builder.h>      // for GuardBuilder, etc
#include <r_exec/init.h>               // for TDecompiler, Now
#include <r_exec/mem.h>                // for _Mem
#include <r_exec/model_base.h>         // for ModelBase
#include <r_exec/opcodes.h>            // for Opcodes, Opcodes::ICst, etc
#include <r_exec/pattern_extractor.h>  // for CTPX, Input, GTPX, PTPX, _TPX, etc
#include <r_exec/reduction_job.h>      // for ReductionJob
#include <r_exec/view.h>               // for View
#include <stdio.h>                     // for snprintf
#include <cstdint>                     // for uint64_t, uint16_t
#include <string>                      // for allocator, string, operator+, etc
#include <vector>                      // for vector, etc

#include <replicode_common.h>          // for P, _Object
#include <replicode_common.h>         // for DebugStream

namespace r_exec
{

bool Input::IsEligibleCause(r_exec::View *view)
{
    switch (view->get_sync()) {
    case View::SYNC_AXIOM:
    case View::SYNC_ONCE_AXIOM:
        return false;

    default:
        return true;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TPX::TPX(AutoFocusController *auto_focus, _Fact *target, _Fact *pattern, BindingMap *bindings): _Object(), auto_focus(auto_focus), target(target), target_bindings(bindings), abstracted_target(pattern), cst_hook(nullptr)   // called by GTPX and PTPX's ctor.
{
    if (bindings->is_fully_specified()) { // get a hook on a cst controller so we get icsts from it: this is needed if the target is an underspecified icst.
        Code *target_payload = target->get_reference(0)->get_reference(0)->get_reference(0);

        if (target_payload->code(0).asOpcode() == Opcodes::ICst) {
            Code *cst = target_payload->get_reference(0);
            cst_hook = (CSTController *)((r_exec::View *)cst->get_view(auto_focus->get_primary_group(), true))->controller;
        }
    }
}

TPX::TPX(AutoFocusController *auto_focus, _Fact *target): _Object(), auto_focus(auto_focus)   // called by CTPX's ctor.
{
    P<BindingMap> bm = new BindingMap();
    abstracted_target = (_Fact *)bm->abstract_object(target, false);
    this->target = target;
    target_bindings = bm;
}

TPX::~TPX()
{
}

bool TPX::take_input(View *input, _Fact *abstracted_input, BindingMap *bm)
{
    return filter(input, abstracted_input, bm);
}

void TPX::signal(View *input)   // input->object is f->success or|f->success.
{
    //std::cout<<Utils::RelativeTime(Now())<<" "<<input->object->get_reference(0)->get_reference(1)->get_oid()<<": end of focus["<<target->get_oid()<<"]\n";
}

void TPX::ack_pred_success(_Fact *predicted_f)
{
}

bool TPX::filter(View *input, _Fact *abstracted_input, BindingMap *bm)
{
    if (input->object->get_reference(0)->code(0).asOpcode() == Opcodes::ICst) { // if we get an icst we are called by auto_focus::dispatch_no_inject: the input is irrelevant.
        return false;
    }

    //std::cout<<Utils::RelativeTime(Now())<<" tpx ["<<target->get_oid()<<"] <- "<<input->object->get_oid();
    if (target_bindings->intersect(bm)) { //std::cout<<" lvl0"<<std::endl;
        return true;
    }

    if (target_bindings->is_fully_specified()) {
        return false;
    }

    for (P<BindingMap> new_map : new_maps)
        if (new_map->intersect(bm)) { //std::cout<<" lvl1"<<std::endl;
            return true;
        }

    P<BindingMap> _bm = new BindingMap(target_bindings);
    _bm->reset_fwd_timings(input->object);
    time_buffer<CInput, CInput::IsInvalidated> &cache = auto_focus->get_cache();

    if (_bm->match_fwd_strict(input->object, (_Fact *)target->get_reference(0)->get_reference(0))) { // both GTPX and PTPX' target are f0->g/p->f1: we need to match on f1.
        //std::cout<<" match";
        new_maps.push_back(_bm);
        time_buffer<CInput, CInput::IsInvalidated>::iterator i;
        uint64_t now = Now();

        for (i = cache.begin(now); i != cache.end(); ++i) {
            if (i->injected) { //std::cout<<" ?"<<(*i).input->object->get_oid()<<" ("<<Utils::RelativeTime(i->ijt)<<") skip\n";
                continue;
            }

            if (_bm->intersect(i->bindings)) {
                i->injected = true;
                auto_focus->inject_input(i->input, i->abstraction, i->bindings);
                //std::cout<<" ?"<<(*i).input->object->get_oid()<<" ("<<Utils::RelativeTime(i->ijt)<<") success\n";
            }//else std::cout<<" ?"<<(*i).input->object->get_oid()<<" ("<<Utils::RelativeTime(i->ijt)<<") failure\n";
        }

        return true;
    } else {
        if (cst_hook != nullptr) {
            cst_hook->take_input(input);
        }

        CInput ci(input, abstracted_input, bm);
        time_buffer<CInput, CInput::IsInvalidated>::iterator i = cache.find(Now(), ci);

        if (i != cache.end()) { // input already cached.
            if (i->ijt < ci.ijt) { // older view (happens for sync_axiom and sync_old).
                cache.erase(i);
            } else {
                return false;
            }
        }

        cache.push_back(ci);
        //std::cout<<" cached"<<" ("<<Utils::RelativeTime(input->get_ijt())<<")\n";
        return false;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_TPX::_TPX(AutoFocusController *auto_focus, _Fact *target, _Fact *pattern, BindingMap *bindings): TPX(auto_focus, target, pattern, bindings)
{
    inputs.reserve(InputsInitialSize);
}

_TPX::_TPX(AutoFocusController *auto_focus, _Fact *target): TPX(auto_focus, target)
{
}

_TPX::~_TPX()
{
}

void _TPX::filter_icst_components(ICST *icst, uint64_t icst_index, std::vector<Component> &components)
{
    uint64_t found_component_count = 0;
    uint64_t *found = new uint64_t[icst->components.size()];

    for (uint64_t j = 0; j < components.size(); ++j) {
        for (P<_Fact> component : icst->components) {
            if (components[j].discarded) {
                continue;
            }

            if (components[j].object == component) {
                found[found_component_count] = j;
                ++found_component_count;
            }
        }
    }

    if (found_component_count > 0) { // some of the icst components are already in the inputs: discard said components, keep the icst.
        for (uint64_t i = 0; i < found_component_count; ++i) {
            components[found[i]].discarded = true;
        }
    } else { // none of the icst components are in the inputs; this can only happen because the icst shares one timestamp with the TPX's target: discard the icst.
        components[icst_index].discarded = true;
    }

    delete[] found;
}

_Fact *_TPX::_find_f_icst(_Fact *component, uint16_t &component_index)
{
    r_code::list<Input>::const_iterator i;

    for (i = inputs.begin(); i != inputs.end(); ++i) {
        Code *candidate = i->input->get_reference(0);

        if (candidate->code(0).asOpcode() == Opcodes::ICst) {
            ICST *icst = (ICST *)candidate;

            if (icst->contains(component, component_index)) {
                return i->input;
            }
        }
    }

    std::vector<P<_Fact> >::const_iterator f_icst;

    for (f_icst = icsts.begin(); f_icst != icsts.end(); ++f_icst) {
        ICST *icst = (ICST *)(*f_icst)->get_reference(0);

        if (icst->contains(component, component_index)) {
            return (*f_icst);
        }
    }

    return nullptr;
}

_Fact *_TPX::find_f_icst(_Fact *component, uint16_t &component_index)
{
    uint16_t opcode = component->get_reference(0)->code(0).asOpcode();

    if (opcode == Opcodes::Cmd || opcode == Opcodes::IMdl) { // cmds/imdls cannot be components of a cst.
        return nullptr;
    }

    return _find_f_icst(component, component_index);
}

_Fact *_TPX::find_f_icst(_Fact *component, uint16_t &component_index, Code *&cst)
{
    uint16_t opcode = component->get_reference(0)->code(0).asOpcode();

    if (opcode == Opcodes::Cmd || opcode == Opcodes::IMdl) { // cmds/imdls cannot be components of a cst.
        cst = nullptr;
        return nullptr;
    }

    _Fact *f_icst = _find_f_icst(component, component_index);

    if (f_icst != nullptr) {
        cst = nullptr;
        return f_icst;
    }

    std::vector<Component> components; // no icst found, try to identify components to assemble a cst.
    std::vector<uint64_t> icst_components;
    r_code::list<Input>::const_iterator i;

    for (i = inputs.begin(); i != inputs.end(); ++i) {
        if (component == i->input) {
            component_index = components.size();
            components.push_back(Component(component));
        } else if (component->match_timings_sync(i->input)) {
            Code *icst = i->input->get_reference(0);

            if (icst->code(0).asOpcode() == Opcodes::ICst) {
                icst_components.push_back(components.size());
            }

            components.push_back(Component(i->input));
        }
    }

    for (uint64_t j = 0; j < icst_components.size(); ++j) {
        ICST *icst = (ICST *)components[icst_components[j]].object->get_reference(0);
        filter_icst_components(icst, j, components);
    }

    uint64_t actual_size = 0;

    for (const Component &component : components) {
        if (component.discarded) {
            continue;
        } else {
            ++actual_size;
        }
    }

    if (actual_size <= 1) { // contains at most only the provided component.
        cst = nullptr;
        return nullptr;
    }

    r_code::list<Input>::iterator _i;

    for (_i = inputs.begin(); _i != inputs.end(); ++_i) { // flag the components so the tpx does not try them again.
        for (const Component &component : components) {
            if (_i->input == component.object) {
                _i->eligible_cause = false;
            }
        }
    }

    P<HLPBindingMap> bm = new HLPBindingMap();
    cst = build_cst(components, bm, component);
    f_icst = bm->build_f_ihlp(cst, Opcodes::ICst, false);
    icsts.push_back(f_icst); // the f_icst can be reused in subsequent model building attempts.
    return f_icst;
}

Code *_TPX::build_cst(const std::vector<Component> &components, BindingMap *bm, _Fact *main_component)
{
    _Fact *abstracted_component = (_Fact *)bm->abstract_object(main_component, false);
    Code *cst = _Mem::Get()->build_object(Atom::CompositeState(Opcodes::Cst, CST_ARITY));
    uint16_t actual_component_count = 0;

    for (const Component &component : components) { // reference patterns;
        if (component.discarded) {
            continue;
        }

        if (component.object == main_component) {
            cst->add_reference(abstracted_component);
        } else {
            cst->add_reference(bm->abstract_object(component.object, true));
        }

        ++actual_component_count;
    }

    uint16_t extent_index = CST_ARITY;
    cst->code(CST_TPL_ARGS) = Atom::IPointer(++extent_index);
    cst->code(extent_index) = Atom::Set(0); // no tpl args.
    cst->code(CST_OBJS) = Atom::IPointer(++extent_index);
    cst->code(extent_index) = Atom::Set(actual_component_count);

    for (uint16_t i = 0; i < actual_component_count; ++i) {
        cst->code(++extent_index) = Atom::RPointer(i);
    }

    cst->code(CST_FWD_GUARDS) = Atom::IPointer(++extent_index);
    cst->code(extent_index) = Atom::Set(0); // no fwd guards.
    cst->code(CST_BWD_GUARDS) = Atom::IPointer(++extent_index);
    cst->code(extent_index) = Atom::Set(0); // no bwd guards.
    cst->code(CST_OUT_GRPS) = Atom::IPointer(++extent_index);
    cst->code(extent_index) = Atom::Set(1); // only one output group: the one the tpx lives in.
    cst->code(++extent_index) = Atom::RPointer(cst->references_size());
    cst->code(CST_ARITY) = Atom::Float(1); // psln_thr.
    cst->add_reference(auto_focus->getView()->get_host()); // reference the output group.
    return cst;
}

Code *_TPX::build_mdl_head(HLPBindingMap *bm, uint16_t tpl_arg_count, _Fact *lhs, _Fact *rhs, uint16_t &write_index)
{
    Code *mdl = _Mem::Get()->build_object(Atom::Model(Opcodes::Mdl, MDL_ARITY));
    mdl->add_reference(bm->abstract_object(lhs, false)); // reference lhs.
    mdl->add_reference(bm->abstract_object(rhs, false)); // reference rhs.
    write_index = MDL_ARITY;
    mdl->code(MDL_TPL_ARGS) = Atom::IPointer(++write_index);
    mdl->code(write_index) = Atom::Set(tpl_arg_count);

    for (uint16_t i = 0; i < tpl_arg_count; ++i) {
        mdl->code(++write_index) = Atom::VLPointer(i);
    }

    mdl->code(MDL_OBJS) = Atom::IPointer(++write_index);
    mdl->code(write_index) = Atom::Set(2);
    mdl->code(++write_index) = Atom::RPointer(0);
    mdl->code(++write_index) = Atom::RPointer(1);
    return mdl;
}

void _TPX::build_mdl_tail(Code *mdl, uint16_t write_index)
{
    mdl->code(MDL_OUT_GRPS) = Atom::IPointer(++write_index);
    mdl->code(write_index) = Atom::Set(1); // only one group: the one the tpx lives in.
    mdl->code(++write_index) = Atom::RPointer(2);
    mdl->code(MDL_STRENGTH) = Atom::Float(0);
    mdl->code(MDL_CNT) = Atom::Float(1);
    mdl->code(MDL_SR) = Atom::Float(1);
    mdl->code(MDL_DSR) = Atom::Float(1);
    mdl->code(MDL_ARITY) = Atom::Float(1); // psln_thr.
    mdl->add_reference(auto_focus->getView()->get_host()); // reference the output group.
}

void _TPX::inject_hlps() const
{
    std::vector<P<Code> >::const_iterator c;

    for (c = csts.begin(); c != csts.end(); ++c) {
        _Mem::Get()->pack_hlp(*c);
    }

    auto_focus->inject_hlps(csts);
    auto_focus->inject_hlps(mdls);
}

void _TPX::inject_hlps(uint64_t analysis_starting_time)
{
    if (auto_focus->decompile_models()) {
        r_code::list<P<Code> > tmp;
        r_code::list<Input>::const_iterator i;

        for (i = inputs.begin(); i != inputs.end(); ++i) {
            tmp.push_back((Code *)i->input);
        }

        std::string header("> from buffer -------------------\n\n");
        P<TDecompiler> td = new TDecompiler(1, header, _Mem::Get()->metadata);
        td->add_objects(tmp);
        td->runDecompiler();
        uint64_t analysis_end = Now();
        uint64_t d = analysis_end - analysis_starting_time;
        char _timing[255];
        snprintf(_timing, 255, "%lu", d);
        header = DebugStream::timestamp(Now() - Utils::GetTimeReference());
        std::string s0 = (" > ");
        s0 += get_header() + std::string(":production [");
        std::string timing(_timing);
        std::string s1("us] -------------------\n\n");
        header += s0 + timing + s1;
        td = new TDecompiler(1, header, _Mem::Get()->metadata);
        td->add_objects(mdls);
        inject_hlps();
        td->runDecompiler();
    } else {
        inject_hlps();
    }

    csts.clear();
    mdls.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

GTPX::GTPX(AutoFocusController *auto_focus, _Fact *target, _Fact *pattern, BindingMap *bindings, Fact *f_imdl): _TPX(auto_focus, target, pattern, bindings), f_imdl(f_imdl)
{
}

GTPX::~GTPX()
{
}

bool GTPX::take_input(View *input, _Fact *abstracted_input, BindingMap *bm)   // push new input in the time-controlled buffer; old inputs are in front.
{
    if (!filter(input, abstracted_input, bm)) {
        return false;
    }

    inputs.push_back(Input(input, abstracted_input, bm));
    return true;
}

void GTPX::signal(View *input)   // will be erased from the AF map upon return. P<> kept in reduction job.
{
    if (!auto_focus->gtpx_on()) {
        return;
    }

    if (((_Fact *)input->object)->is_fact()) { // goal success.
        ReductionJob<GTPX> *j = new ReductionJob<GTPX>(new View(input), (GTPX *)this);
        _Mem::Get()->pushReductionJob(j);
    }
}

void GTPX::ack_pred_success(_Fact *predicted_f)   // successful prediction: store; at reduce() time, check if the target was successfully predicted and if so, abort mdl building.
{
    predictions.push_back(predicted_f);
}

void GTPX::reduce(r_exec::View *input)   // input->object: f->success.
{
    _Fact *consequent = (_Fact *)input->object->get_reference(0)->get_reference(1);
    P<BindingMap> consequent_bm = new BindingMap();

    for (P<_Fact> prediction : predictions) { // check if some models have successfully predicted the target: if so, abort.
        P<BindingMap> bm = new BindingMap(consequent_bm);
        bm->reset_fwd_timings(prediction);

        if (bm->match_fwd_strict(prediction, consequent)) {
            return;
        }
    }

    uint64_t analysis_starting_time = Now();

    if (target->get_reference(0)->code(0).asOpcode() == Opcodes::MkVal) {
        return;    // this case will be handled by CTPXs.
    }

    P<GuardBuilder> guard_builder;
    uint64_t period;
    r_code::list<Input>::const_iterator i;

    for (i = inputs.begin(); i != inputs.end();) {
        if (i->input->get_after() >= consequent->get_after()) { // discard inputs younger than the consequent.
            i = inputs.erase(i);
            continue;
        }

        if (i->input->get_reference(0)->code(0).asOpcode() == Opcodes::ICst) {
            ++i;
            continue; // components will be evaluated first, then the icst will be identified.
        }

        Input cause = *i;

        if (!cause.eligible_cause) {
            ++i;
            continue;
        }

        if (Utils::Synchronous(cause.input->get_after(), target->get_after())) { // cause in sync with the premise: ignore.
            ++i;
            continue;
        }

        period = consequent->get_after() - cause.input->get_after();
        guard_builder = new TimingGuardBuilder(period); // TODO: use the durations.
        uint16_t cause_index;
        Code *new_cst;
        _Fact *f_icst = find_f_icst(cause.input, cause_index, new_cst);

        if (f_icst == nullptr) {
            if (build_mdl(cause.input, consequent, guard_builder, period)) {
                inject_hlps(analysis_starting_time);
            }
        } else {
            Code *unpacked_cst;

            if (new_cst == nullptr) {
                Code *cst = f_icst->get_reference(0)->get_reference(0);
                unpacked_cst = cst->get_reference(cst->references_size() - CST_HIDDEN_REFS); // the cst is packed, retrieve the pattern from the unpacked code.
            } else {
                unpacked_cst = new_cst;
            }

            _Fact *cause_pattern = (_Fact *)unpacked_cst->get_reference(cause_index);

            if (build_mdl(f_icst, cause_pattern, consequent, guard_builder, period, new_cst)) {
                inject_hlps(analysis_starting_time);
            }
        }

        ++i;
    }
}

bool GTPX::build_mdl(_Fact *cause, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period)
{
    P<BindingMap> bm = new BindingMap();
    uint16_t write_index;
    P<Code> m0 = build_mdl_head(bm, 0, cause, consequent, write_index);
    guard_builder->build(m0, nullptr, cause, write_index);
    build_mdl_tail(m0, write_index);
    Code *_m0 = ModelBase::Get()->check_existence(m0);

    if (_m0 == nullptr) {
        return false;
    } else if (_m0 == m0) {
        mdls.push_back(m0);
        return true;
    } else {
        return false;
    }
}

bool GTPX::build_mdl(_Fact *f_icst, _Fact *cause_pattern, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period, Code *new_cst)
{
    P<BindingMap> bm = new BindingMap();
    uint16_t write_index;
    P<Code> m0 = build_mdl_head(bm, 0, f_icst, consequent, write_index);
    guard_builder->build(m0, nullptr, cause_pattern, write_index);
    build_mdl_tail(m0, write_index);
    Code *_m0 = ModelBase::Get()->check_existence(m0);

    if (_m0 == nullptr) {
        return false;
    } else if (_m0 == m0) {
        if (new_cst) {
            csts.push_back(new_cst);
        }

        mdls.push_back(m0);
        return true;
    } else { // if m0 already exist, new_cst==NULL.
        return false;
    }
}

std::string GTPX::get_header() const
{
    return std::string("GTPX");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PTPX::PTPX(AutoFocusController *auto_focus, _Fact *target, _Fact *pattern, BindingMap *bindings, Fact *f_imdl): _TPX(auto_focus, target, pattern, bindings), f_imdl(f_imdl)
{
}

PTPX::~PTPX()
{
}

void PTPX::signal(View *input)   // will be erased from the AF map upon return. P<> kept in reduction job.
{
    if (!auto_focus->ptpx_on()) {
        return;
    }

    if (((_Fact *)input->object)->is_anti_fact()) { // prediction failure.
        ReductionJob<PTPX> *j = new ReductionJob<PTPX>(new View(input), (PTPX *)this);
        _Mem::Get()->pushReductionJob(j);
    }
}

void PTPX::reduce(r_exec::View *input)
{
    auto_focus->copy_cross_buffer(inputs); // the cause of the prediction failure comes before the prediction.
    uint64_t analysis_starting_time = Now();
    _Fact *consequent = new Fact((Fact *)f_imdl); // input->object is the prediction failure: ignore and consider |f->imdl instead.
    consequent->set_opposite();
    P<BindingMap> end_bm = new BindingMap();
    P<_Fact> abstract_input = (_Fact *)end_bm->abstract_object(consequent, false);
    r_code::list<Input>::const_iterator i;

    for (i = inputs.begin(); i != inputs.end();) { // filter out inputs irrelevant for the prediction.
        if (i->input->code(0).asOpcode() == Opcodes::Cmd) { // no cmds as req lhs (because no bwd-operational); prefer: cmd->effect, effect->imdl.
            i = inputs.erase(i);
        } else if (!end_bm->intersect(i->bindings) || // discard inputs that do not share values with the consequent.
                   i->input->get_after() >= consequent->get_after()) { // discard inputs younger than the consequent.
            i = inputs.erase(i);
        } else {
            ++i;
        }
    }

    P<GuardBuilder> guard_builder;
    uint64_t period;

    for (i = inputs.begin(); i != inputs.end(); ++i) {
        if (i->input->get_reference(0)->code(0).asOpcode() == Opcodes::ICst) {
            continue;    // components will be evaluated first, then the icst will be identified.
        }

        Input cause = *i;

        if (!cause.eligible_cause) {
            continue;
        }

        period = consequent->get_after() - cause.input->get_after();
        guard_builder = new TimingGuardBuilder(period); // TODO: use the durations.
        uint16_t cause_index;
        Code *new_cst;
        _Fact *f_icst = find_f_icst(cause.input, cause_index, new_cst);

        if (f_icst == nullptr) {
            if (build_mdl(cause.input, consequent, guard_builder, period)) {
                inject_hlps(analysis_starting_time);
            }
        } else {
            Code *unpacked_cst;

            if (new_cst == nullptr) {
                Code *cst = f_icst->get_reference(0)->get_reference(0);
                unpacked_cst = cst->get_reference(cst->references_size() - CST_HIDDEN_REFS); // the cst is packed, retrieve the pattern from the unpacked code.
            } else {
                unpacked_cst = new_cst;
            }

            _Fact *cause_pattern = (_Fact *)unpacked_cst->get_reference(cause_index);

            if (build_mdl(f_icst, cause_pattern, consequent, guard_builder, period, new_cst)) {
                inject_hlps(analysis_starting_time);
            }
        }
    }
}

bool PTPX::build_mdl(_Fact *cause, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period)
{
    P<BindingMap> bm = new BindingMap();
    uint16_t write_index;
    P<Code> m0 = build_mdl_head(bm, 0, cause, consequent, write_index);
    guard_builder->build(m0, nullptr, cause, write_index);
    build_mdl_tail(m0, write_index);
    Code *_m0 = ModelBase::Get()->check_existence(m0);

    if (_m0 == nullptr) {
        return false;
    } else if (_m0 == m0) {
        mdls.push_back(m0);
        return true;
    } else {
        return false;
    }
}

bool PTPX::build_mdl(_Fact *f_icst, _Fact *cause_pattern, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period, Code *new_cst)
{
    P<BindingMap> bm = new BindingMap();
    uint16_t write_index;
    P<Code> m0 = build_mdl_head(bm, 0, f_icst, consequent, write_index);
    guard_builder->build(m0, nullptr, cause_pattern, write_index);
    build_mdl_tail(m0, write_index);
    Code *_m0 = ModelBase::Get()->check_existence(m0);

    if (_m0 == nullptr) {
        return false;
    } else if (_m0 == m0) {
        if (new_cst) {
            csts.push_back(new_cst);
        }

        mdls.push_back(m0);
        return true;
    } else { // if m0 already exist, new_cst==NULL.
        return false;
    }
}

std::string PTPX::get_header() const
{
    return std::string("PTPX");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CTPX::CTPX(AutoFocusController *auto_focus, View *premise): _TPX(auto_focus, premise->object), stored_premise(false), premise(premise)
{
}

CTPX::~CTPX()
{
}

void CTPX::store_input(r_exec::View *input)
{
    _Fact *input_object = (_Fact *)input->object;
    P<BindingMap> bm = new BindingMap();
    _Fact *abstracted_input = (_Fact *)bm->abstract_object(input_object, false);
    Input i(input, abstracted_input, bm);
    inputs.push_front(i);

    if (input_object == target) {
        stored_premise = true;
    }
}

void CTPX::signal(r_exec::View *input)
{
    View *_view = new View(input); // controller not copied.
    ReductionJob<CTPX> *j = new ReductionJob<CTPX>(_view, this); // holds a reference to this.
    _Mem::Get()->pushReductionJob(j);
}

void CTPX::reduce(r_exec::View *input)
{
    uint64_t analysis_starting_time = Now();

    if (!stored_premise) {
        inputs.push_back(Input(premise, abstracted_target, target_bindings));
    }

    _Fact *consequent = (_Fact *)input->object; // counter-evidence for the premise.
    P<BindingMap> end_bm = new BindingMap();
    P<_Fact> abstract_input = (_Fact *)end_bm->abstract_object(consequent, false);
    r_code::list<Input>::const_iterator i;

    for (i = inputs.begin(); i != inputs.end();) {
        if (!end_bm->intersect(i->bindings) || // discard inputs that do not share values with the consequent.
            i->input->get_after() >= consequent->get_after()) { // discard inputs younger than the consequent.
            i = inputs.erase(i);
        } else {
            ++i;
        }
    }

    bool need_guard;

    if (target->get_reference(0)->code(0).asOpcode() == Opcodes::MkVal) {
        need_guard = target->get_reference(0)->code(MK_VAL_VALUE).isFloat();
    } else {
        need_guard = false;
    }

    uint64_t period = Utils::GetTimestamp<Code>(consequent, FACT_AFTER) - Utils::GetTimestamp<Code>(target, FACT_AFTER); // sampling period.
    P<GuardBuilder> guard_builder;

    for (i = inputs.begin(); i != inputs.end(); ++i) {
        if (target == i->input) {
            continue;
        }

        if (i->input->get_reference(0)->code(0).asOpcode() == Opcodes::ICst) {
            continue;    // components will be evaluated first, then the icst will be identified.
        }

        Input cause = *i;

        if (!cause.eligible_cause) {
            continue;
        }

        if (Utils::Synchronous(cause.input->get_after(), target->get_after())) { // cause in sync with the premise: ignore.
            continue;
        }

        if (need_guard) {
            if ((guard_builder = find_guard_builder(cause.input, consequent, period)) == nullptr) {
                continue;
            }
        } else {
            guard_builder = get_default_guard_builder(cause.input, consequent, period);
        }

        uint16_t cause_index;
        _Fact *f_icst = find_f_icst(cause.input, cause_index);

        if (f_icst == nullptr) { // m0:[premise.value premise.after premise.before][cause->consequent] and m1:[lhs1->imdl m0[...][...]] with lhs1 either the premise or an icst containing the premise.
            if (build_mdl(cause.input, consequent, guard_builder, period)) {
                inject_hlps(analysis_starting_time);
            }
        } else {
            Code *cst = f_icst->get_reference(0)->get_reference(0); // cst is packed.
            _Fact *cause_pattern = (_Fact *)cst->get_reference(cause_index);

            if (build_mdl(f_icst, cause_pattern, consequent, guard_builder, period)) { // m0:[premise.value premise.after premise.before][icst->consequent] and m1:[lhs1->imdl m0[...][...]] with lhs1 either the premise or an icst containing the premise.
                inject_hlps(analysis_starting_time);
            }
        }
    }
}

GuardBuilder *CTPX::get_default_guard_builder(_Fact *cause, _Fact *consequent, uint64_t period)
{
    Code *cause_payload = cause->get_reference(0);
    uint16_t opcode = cause_payload->code(0).asOpcode();

    if (opcode == Opcodes::Cmd) {
        uint64_t offset = consequent->get_after() - cause->get_after();
        uint64_t cmd_duration = cause->get_before() - cause->get_after();
        return new NoArgCmdGuardBuilder(period, offset, cmd_duration);
    }

    return new TimingGuardBuilder(period);
}

// 5 forms:
// 0 - q1=q0+cmd_arg (if the cause is a cmd) or q1=q0*cmd_arg with q0!=0.
// 1 - q1=q0+speed*period, with q1=consequent.value, q0=premise.value, speed=cause.value,
// 3 - q1=q0+constant or q1=q0*constant with q0!=0.
GuardBuilder *CTPX::find_guard_builder(_Fact *cause, _Fact *consequent, uint64_t period)
{
    Code *cause_payload = cause->get_reference(0);
    uint16_t opcode = cause_payload->code(0).asOpcode();

    if (opcode == Opcodes::Cmd) { // form 0.
        float q0 = target->get_reference(0)->code(MK_VAL_VALUE).asFloat();
        float q1 = consequent->get_reference(0)->code(MK_VAL_VALUE).asFloat();
        float searched_for = q1 - q0;
        uint16_t cmd_arg_set_index = cause_payload->code(CMD_ARGS).asIndex();
        uint16_t cmd_arg_count = cause_payload->code(cmd_arg_set_index).getAtomCount();

        for (uint16_t i = 1; i <= cmd_arg_count; ++i) {
            Atom s = cause_payload->code(cmd_arg_set_index + i);

            if (!s.isFloat()) {
                continue;
            }

            float _s = s.asFloat();

            if (Utils::Equal(_s, searched_for)) {
                return new ACGuardBuilder(period, cmd_arg_set_index + i);
            }
        }

        if (q0 != 0) {
            searched_for = q1 / q0;

            for (uint16_t i = cmd_arg_set_index + 1; i <= cmd_arg_count; ++i) {
                Atom s = cause_payload->code(i);

                if (!s.isFloat()) {
                    continue;
                }

                float _s = s.asFloat();

                if (Utils::Equal(_s, searched_for)) {
                    return new MCGuardBuilder(period, i);
                }
            }
        }
    } else if (opcode == Opcodes::MkVal) {
        Atom s = cause_payload->code(MK_VAL_VALUE);

        if (s.isFloat()) {
            float _s = s.asFloat();
            float q0 = target->get_reference(0)->code(MK_VAL_VALUE).asFloat();
            float q1 = consequent->get_reference(0)->code(MK_VAL_VALUE).asFloat();
            float searched_for = (q1 - q0) / period;

            if (Utils::Equal(_s, searched_for)) { // form 1.
                uint64_t offset = Utils::GetTimestamp<Code>(cause, FACT_AFTER) - Utils::GetTimestamp<Code>(target, FACT_AFTER);
                return new SGuardBuilder(period, period - offset);
            }

            if (q0 != 0) { // form 2.
                uint64_t offset = Utils::GetTimestamp<Code>(cause, FACT_AFTER) - Utils::GetTimestamp<Code>(target, FACT_AFTER);
                return new MGuardBuilder(period, q1 / q0, offset);
            }

            uint64_t offset = Utils::GetTimestamp<Code>(cause, FACT_AFTER) - Utils::GetTimestamp<Code>(target, FACT_AFTER);
            return new AGuardBuilder(period, q1 - q0, offset);
        }
    }

    return nullptr;
}

// m0:[premise.value premise.after premise.before][cause->consequent].
// m1:[icst->imdl m0[...][...]] with icst containing the premise.
bool CTPX::build_mdl(_Fact *cause, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period)
{
    P<HLPBindingMap> bm = new HLPBindingMap();
    bm->init(target->get_reference(0), MK_VAL_VALUE);
    bm->init(target, FACT_AFTER);
    bm->init(target, FACT_BEFORE);
    uint16_t write_index;
    P<Code> m0 = build_mdl_head(bm, 3, cause, consequent, write_index);
    guard_builder->build(m0, nullptr, cause, write_index);
    build_mdl_tail(m0, write_index);
    //std::cout<<Utils::RelativeTime(Now())<<" found --------------------- M0\n";
    return build_requirement(bm, m0, period); // existence checks performed there.
}

// m0:[premise.value premise.after premise.before][icst->consequent] with icst containing the cause.
// m1:[icst->imdl m0[...][...]] with icst containing the premise.
bool CTPX::build_mdl(_Fact *f_icst, _Fact *cause_pattern, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period)
{
    P<BindingMap> bm = new BindingMap();
    bm->init(target->get_reference(0), MK_VAL_VALUE);
    bm->init(target, FACT_AFTER);
    bm->init(target, FACT_BEFORE);
    uint16_t write_index;
    Code *m0 = build_mdl_head(bm, 3, f_icst, consequent, write_index);
    guard_builder->build(m0, nullptr, cause_pattern, write_index);
    build_mdl_tail(m0, write_index);
    return build_requirement(bm, m0, period); // existence checks performed there.
}

bool CTPX::build_requirement(HLPBindingMap *bm, Code *m0, uint64_t period)   // check for mdl existence at the same time (ModelBase::mdlCS-wise).
{
    uint16_t premise_index;
    Code *new_cst;
    _Fact *f_icst = find_f_icst(target, premise_index, new_cst);

    if (f_icst == nullptr) { //std::cout<<Utils::RelativeTime(Now())<<" failed xxxxxxxxx M1 / 0\n";
        return false;
    }

    P<Fact> f_im0 = bm->build_f_ihlp(m0, Opcodes::IMdl, false);
    Utils::SetIndirectTimestamp<Code>(f_im0, FACT_AFTER, f_icst->get_after());
    Utils::SetIndirectTimestamp<Code>(f_im0, FACT_BEFORE, f_icst->get_before());
    Code *unpacked_cst;

    if (new_cst == nullptr) {
        Code *cst = f_icst->get_reference(0)->get_reference(0);
        unpacked_cst = cst->get_reference(cst->references_size() - CST_HIDDEN_REFS); // the cst is packed, retrieve the pattern from the unpacked code.
    } else {
        unpacked_cst = new_cst;
    }

    _Fact *premise_pattern = (_Fact *)unpacked_cst->get_reference(premise_index);
    P<HLPBindingMap> _bm = new HLPBindingMap();
    uint16_t write_index;
    P<Code> m1 = build_mdl_head(_bm, 0, f_icst, f_im0, write_index);
    P<GuardBuilder> guard_builder = new GuardBuilder();
    guard_builder->build(m1, premise_pattern, nullptr, write_index);
    build_mdl_tail(m1, write_index);
    Code *_m0;
    Code *_m1;
    ModelBase::Get()->check_existence(m0, m1, _m0, _m1);

    if (_m1 == nullptr) {
        return false;
    } else if (_m1 == m1) {
        if (_m0 == nullptr) {
            return false;
        } else if (_m0 == m0) {
            mdls.push_back(m0);
        }

        if (new_cst != nullptr) {
            csts.push_back(new_cst);
        }

        mdls.push_back(m1);
    } // if m1 alrady exists, new_cst==NULL.

    //std::cout<<Utils::RelativeTime(Now()<<" found --------------------- M1\n";
    return true;
}

std::string CTPX::get_header() const
{
    return std::string("CTPX");
}
}
