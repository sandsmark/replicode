//	auto_focus.cpp
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

#include "auto_focus.h"

#include <r_code/atom.h>            // for Atom
#include <r_code/list.h>            // for list
#include <r_code/object.h>          // for Code, View::::SYNC_ONCE, etc
#include <r_code/replicode_defs.h>  // for VIEW_RES, I_HLP_WR_E, VIEW_SYNC, etc
#include <r_exec/ast_controller.h>  // for HASTController, PASTController
#include <r_exec/auto_focus.h>      // for AutoFocusController, etc
#include <r_exec/binding_map.h>     // for BindingMap
#include <r_exec/group.h>           // for Group
#include <r_exec/init.h>            // for Now
#include <r_exec/opcodes.h>         // for Opcodes, Opcodes::AntiFact, etc
#include <r_exec/view.h>            // for View
#include <mutex>                    // for lock_guard, mutex
#include <string>                   // for string, allocator


#include <r_exec/overlay.tpl.h>     // for __take_input


namespace r_exec
{

AutoFocusController::AutoFocusController(r_code::View *view): Controller(view)
{
    // Load arguments: pass_through, acquire_models, decompile_models, list of output groups: 1st must be the primary, 2nd the secondary, then other groups.
    Code *icpp_pgm = getObject();
    uint16_t arg_set_index = icpp_pgm->code(ICPP_PGM_ARGS).asIndex();
    uint16_t arg_count = icpp_pgm->code(arg_set_index).getAtomCount();
    uint8_t i = 1;
    _pass_through = icpp_pgm->code(arg_set_index + i++).asBoolean();
    _ctpx_on = icpp_pgm->code(arg_set_index + i++).asBoolean();
    _gtpx_on = icpp_pgm->code(arg_set_index + i++).asBoolean();
    _ptpx_on = icpp_pgm->code(arg_set_index + i++).asBoolean();
    _trace_injections = icpp_pgm->code(arg_set_index + i++).asBoolean();
    _decompile_models = icpp_pgm->code(arg_set_index + i).asBoolean();

    for (uint16_t j = i; j < arg_count; ++j) {
        output_groups.push_back((Group *)icpp_pgm->get_reference(j - i));
    }

    cross_buffer.set_thz(_Mem::Get()->get_tpx_time_horizon());
    cross_buffer.reserve(CrossBufferInitialSize);
    uint64_t thz = 2 * ((r_exec::View*)view)->get_host()->get_upr() * Utils::GetBasePeriod(); // thz==2*sampling period.
    cache.set_thz(thz);
    cache.reserve(CacheInitialSize);
}

AutoFocusController::~AutoFocusController()
{
}

Code *AutoFocusController::get_core_object() const
{
    return getObject(); // icpp_pgm.
}

inline void AutoFocusController::inject_input(View *input, uint64_t start)
{
    Group *origin = input->get_host();

    for (uint16_t i = start; i < output_groups.size(); ++i) {
        Group *output_group = output_groups[i];
        View *view = new View(input, true);
        view->references[0] = output_group;
        view->code(VIEW_RES) = Atom::Float(Utils::GetGroupResilience(view->code(VIEW_RES).asFloat(), origin->get_upr(), output_group->get_upr()));
        _Mem::Get()->inject(view);
    }
}

inline View *AutoFocusController::inject_input(View *input)
{
    _Fact *input_fact = (_Fact *)input->object;
    Group *origin = input->get_host();
    Group *ref_group = output_groups[0];
    uint64_t now = Now();
    View *primary_view;
    _Fact *copy;

    switch (input->get_sync()) {
    case View::SYNC_ONCE: // no copy, morph res; N.B.: cmds are sync_once.
        for (uint16_t i = 0; i < output_groups.size(); ++i) {
            Group *output_group = output_groups[i];
            View *view = new View(input, true);
            view->references[0] = output_group;
            view->references[1] = input->references[0];
            view->code(VIEW_RES) = Atom::Float(Utils::GetGroupResilience(view->code(VIEW_RES).asFloat(), origin->get_upr(), output_group->get_upr()));
            _Mem::Get()->inject(view);

            if (i == 0) {
                primary_view = view;
            }
        }

        break;

    case View::SYNC_PERIODIC: // inject a copy, morph res, add a controller.
        if (input_fact->is_anti_fact()) {
            copy = new AntiFact(input_fact->get_reference(0), ref_group->get_prev_upr_time(now), ref_group->get_next_upr_time(now), 1, 1);
        } else {
            copy = new Fact(input_fact->get_reference(0), ref_group->get_prev_upr_time(now), ref_group->get_next_upr_time(now), 1, 1);
        }

        for (uint16_t i = 0; i < output_groups.size(); ++i) {
            Group *output_group = output_groups[i];
            View *view = new View(input, true);
            view->references[0] = output_group;
            view->references[1] = input->references[0];
            view->code(VIEW_RES) = Atom::Float(Utils::GetGroupResilience(view->code(VIEW_RES).asFloat(), origin->get_upr(), output_group->get_upr()));
            view->object = copy;
            _Mem::Get()->inject(view);

            if (i == 0) {
                primary_view = view;

                if (_ctpx_on) {
                    _Mem::Get()->inject_null_program(new PASTController(this, view), output_group, output_group->get_upr()*Utils::GetBasePeriod(), true);
                }
            }
        }

        break;

    case View::SYNC_HOLD: { // inject a copy, add a controller, sync_once, morph res, after=now+time_tolerance (de-sync as it can have the same effect as a cmd), before=now+output_grp.upr+time_tolerance.
        uint64_t offset = 2 * Utils::GetTimeTolerance();

        if (input_fact->is_anti_fact()) {
            copy = new AntiFact(input_fact->get_reference(0), now + offset, now + offset + ref_group->get_upr()*Utils::GetBasePeriod(), 1, 1);
        } else {
            copy = new Fact(input_fact->get_reference(0), now + offset, now + offset + ref_group->get_upr()*Utils::GetBasePeriod(), 1, 1);
        }

        for (uint16_t i = 0; i < output_groups.size(); ++i) {
            Group *output_group = output_groups[i];
            View *view = new View(input, true);
            view->references[0] = output_group;
            view->references[1] = input->references[0];
            view->code(VIEW_SYNC) = Atom::Float(View::SYNC_ONCE);
            view->code(VIEW_RES) = Atom::Float(Utils::GetGroupResilience(view->code(VIEW_RES).asFloat(), origin->get_upr(), output_group->get_upr()));
            view->object = copy;
            _Mem::Get()->inject(view);

            if (i == 0) {
                primary_view = view;

                if (_ctpx_on) {
                    _Mem::Get()->inject_null_program(new HASTController(this, view, input_fact), output_group, output_group->get_upr()*Utils::GetBasePeriod(), true);
                }
            }
        }

        break;
    }

    case View::SYNC_AXIOM: // inject a copy, sync_once, res=1, fact.before=next output_grp upr.
        if (input_fact->is_anti_fact()) {
            copy = new AntiFact(input_fact->get_reference(0), ref_group->get_prev_upr_time(now), ref_group->get_next_upr_time(now), 1, 1);
        } else {
            copy = new Fact(input_fact->get_reference(0), ref_group->get_prev_upr_time(now), ref_group->get_next_upr_time(now), 1, 1);
        }

        for (uint16_t i = 0; i < output_groups.size(); ++i) {
            Group *output_group = output_groups[i];
            View *view = new View(input, true);
            view->references[0] = output_group;
            view->references[1] = input->references[0];
            view->code(VIEW_SYNC) = Atom::Float(View::SYNC_ONCE_AXIOM);
            view->code(VIEW_RES) = Atom::Float(1);
            view->object = copy;
            _Mem::Get()->inject(view);

            if (i == 0) {
                primary_view = view;
            }
        }

        break;
    case View::SYNC_ONCE_AXIOM:
        std::cerr << "Unhandled sync once axiom";
        exit(1);
        break;
    }

    if (_trace_injections) {
        std::string type;

        switch (input->get_sync()) {
        case View::SYNC_HOLD:
            type = "(hold)";
            break;

        case View::SYNC_ONCE:
            type = "(once)";
            break;

        case View::SYNC_PERIODIC:
            type = "(periodic)";
            break;

        case View::SYNC_AXIOM:
            type = "(axiom)";
            break;

        case View::SYNC_ONCE_AXIOM:
            type = "(once axiom)";
            break;
        }

        //    ::debug("auto_focus") << Utils::RelativeTime(Now()) << "A/F ->" << input->object->get_oid() << "|" << primary_view->object->get_oid() << type;
    }

    return primary_view;
}

inline void AutoFocusController::notify(_Fact *target, View *input, TPXMap &map)
{
    TPXMap::const_iterator m = map.find(target);

    if (m != map.end()) { // shall always be the case.
        m->second->signal(input); // will spawn a ReductionJob holding a P<> on m->second.
        map.erase(m);
    }
}

inline void AutoFocusController::dispatch_pred_success(_Fact *predicted_f, TPXMap &map)
{
    TPXMap::const_iterator m;

    for (m = map.begin(); m != map.end(); ++m) {
        m->second->ack_pred_success(predicted_f);
    }
}

inline void AutoFocusController::dispatch(View *input, _Fact *abstract_input, BindingMap *bm, bool &injected, TPXMap &map)
{
    TPXMap::const_iterator m;

    for (m = map.begin(); m != map.end(); ++m) {
        if (m->second->take_input(input, abstract_input, bm)) {
            if (!injected) {
                injected = true;
                inject_input(input, abstract_input, bm);
            }
        }
    }
}

inline void AutoFocusController::dispatch_no_inject(View *input, _Fact *abstract_input, BindingMap *bm, TPXMap &map)
{
    TPXMap::const_iterator m;

    for (m = map.begin(); m != map.end(); ++m) {
        m->second->take_input(input, abstract_input, bm);
    }
}

inline void AutoFocusController::rate(_Fact *target, bool success, TPXMap &map, RatingMap &ratings)
{
    /*
    TPXMap::iterator m=map.find(target);
    if(m!=map.end()){ // shall always be the case.

    _Fact *pattern=m->second->get_pattern();
    RatingMap::iterator r=ratings.find(pattern);
    if(r!=ratings.end()){ // shall always be the case.

    r->second.add_evidence(success);
    if(Rating::DSR(r->second.dSR)) // target for which we don't see much improvement over time.
    m->second=new TPX(m->second);
    }
    }*/
}

void AutoFocusController::take_input(r_exec::View *input)
{
    if (is_invalidated()) {
        return;
    }

    if (input->object->code(0).asOpcode() == Opcodes::Fact ||
        input->object->code(0).asOpcode() == Opcodes::AntiFact ||
        input->object->code(0).asOpcode() == Opcodes::MkRdx) { // discard everything but facts, |facts and mk.rdx.
        Controller::__take_input<AutoFocusController>(input);    // std::cout<<"A/F::TI: "<<get_host()->get_oid()<<" > "<<input->object->get_oid()<<std::endl;
    }
}

void AutoFocusController::reduce(r_exec::View *input)
{
    Code *input_object = input->object;
    uint16_t opcode = input_object->code(0).asOpcode();
    std::lock_guard<std::mutex> guard(m_reductionMutex);

    if (opcode == Opcodes::MkRdx) {
        Code *production = input_object->get_reference(MK_RDX_MDL_PRODUCTION_REF); // fact, if an ihlp was the producer.
        Fact *f_ihlp = (Fact *)input_object->get_reference(MK_RDX_IHLP_REF);
        BindingMap *bm = ((MkRdx *)input_object)->bindings;

        if (f_ihlp->get_reference(0)->code(0).asOpcode() == Opcodes::IMdl) { // handle new goals/predictions as new targets.
            Code *mdl = f_ihlp->get_reference(0)->get_reference(0);
            Code *unpacked_mdl = mdl->get_reference(mdl->references_size() - MDL_HIDDEN_REFS);
            uint16_t obj_set_index = unpacked_mdl->code(MDL_OBJS).asIndex();
            _Fact *pattern;
            TPX *tpx;
            Goal *goal = ((_Fact *)production)->get_goal();

            if (goal != nullptr) { // build a tpx to find models like M:[A -> B] where B is the goal target.
                pattern = (_Fact *)unpacked_mdl->get_reference(unpacked_mdl->code(obj_set_index + 1).asIndex()); // lhs.
                tpx = build_tpx<GTPX>((_Fact *)production, pattern, bm, goal_ratings, f_ihlp, f_ihlp->get_reference(0)->code(I_HLP_WR_E).asBoolean());
                goals.insert(std::pair<P<_Fact>, P<TPX> >((_Fact *)production, tpx));
                //std::cout<<Utils::RelativeTime(Now())<<" goal focus["<<production->get_oid()<<"]\n";
            } else {
                Pred *pred = ((_Fact *)production)->get_pred();

                if (pred != nullptr) { // build a tpx to find models like M:[A -> |imdl M0] where M0 is the model that produced the prediction.
                    pattern = (_Fact *)unpacked_mdl->get_reference(unpacked_mdl->code(obj_set_index + 2).asIndex()); // rhs.
                    tpx = build_tpx<PTPX>((_Fact *)production, pattern, bm, prediction_ratings, f_ihlp, f_ihlp->get_reference(0)->code(I_HLP_WR_E).asBoolean());
                    predictions.insert(std::pair<P<_Fact>, P<TPX> >((_Fact *)production, tpx));
                    //std::cout<<Utils::RelativeTime(Now())<<" pred focus["<<production->get_oid()<<"]\n";
                }
            }
        }
    } else {
        bool success = (opcode == Opcodes::Fact);

        if (success || opcode == Opcodes::AntiFact) { // discard everything but facts.
            Code *payload = input_object->get_reference(0);
            uint16_t opcode = payload->code(0).asOpcode();

            if (opcode == Opcodes::Success) { // input_object is f->success->payload, where payload is f->g or f->p; trim down the target list, rate targets, signal tpx.
                _Fact *target = (_Fact *)payload->get_reference(0);
                Goal *goal = target->get_goal();

                if (goal != nullptr) {
                    //rate(target,success,goals,goal_ratings);
                    notify(target, input, goals);
                } else { // prediction.
                    //rate(target,success,predictions,prediction_ratings);
                    notify(target, input, predictions);

                    if (success) { // a mdl has correctly predicted a GTPX's target: the GTPX shall not produce anything: we need to pass the prediction to all GTPX.
                        dispatch_pred_success((_Fact *)target->get_pred()->get_reference(0), goals);
                    }
                }
            } else if (opcode == Opcodes::Perf) {
                inject_input(input, 2);    // inject in all output groups but the primary and secondary.
            } else { // filter according to targets: inject (once) when possible and pass to TPX if any.
                if (_pass_through) {
                    if (opcode != Opcodes::ICst) { // don't inject again (since it comes from inside).
                        inject_input(input);
                    }
                } else {
                    P<BindingMap> bm = new BindingMap();

                    if (opcode == Opcodes::ICst) { // dispatch but don't inject again (since it comes from inside).
                        bm = ((ICST *)payload)->bindings;
                        _Fact *abstract_f_ihlp = bm->abstract_f_ihlp((_Fact *)input_object);
                        dispatch_no_inject(input, abstract_f_ihlp, bm, goals);
                        dispatch_no_inject(input, abstract_f_ihlp, bm, predictions);
                        cross_buffer.push_back(Input(input, abstract_f_ihlp, bm));
                    } else {
                        P<_Fact> abstract_input = (_Fact *)bm->abstract_object(input_object, false);
                        bool injected = false;
                        dispatch(input, abstract_input, bm, injected, goals);
                        dispatch(input, abstract_input, bm, injected, predictions);
                    }
                }
            }
        }
    }
}

void AutoFocusController::inject_hlps(const std::vector<P<Code> > &hlps) const
{
    std::vector<View *> views;
    uint64_t now = Now();
    std::vector<P<Code> >::const_iterator hlp;

    for (hlp = hlps.begin(); hlp != hlps.end(); ++hlp) {
        View *view = new View(View::SYNC_ONCE, now, 0, -1, output_groups[0], nullptr, *hlp, 1); // SYNC_ONCE,sln=0,res=forever,act=1.
        view->references[0] = output_groups[0];
        views.push_back(view);
    }

    _Mem::Get()->inject_hlps(views, output_groups[0]);
}

void AutoFocusController::copy_cross_buffer(r_code::list<Input> &destination)
{
    std::lock_guard<std::mutex> guard(m_reductionMutex);
    time_buffer<Input, Input::IsInvalidated>::iterator i;

    for (i = cross_buffer.begin(Now()); i != cross_buffer.end(); ++i) {
        destination.push_back(Input(*i));
    }
}
}
