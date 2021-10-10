//	factory.cpp
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

#include "factory.h"

#include <r_code/object.h>          // for Code
#include <r_code/replicode_defs.h>  // for FACT_ARITY, MK_VAL_VALUE, etc
#include <r_code/utils.h>           // for Utils
#include <r_exec/factory.h>         // for _Fact, Goal, Pred, Sim, MkRdx, etc
#include <r_exec/mem.h>             // for _Mem
#include <r_exec/opcodes.h>         // for Opcodes, Opcodes::AntiFact, etc
#include <r_exec/overlay.h>         // for Controller
#include <iostream>                 // for operator<<, basic_ostream, etc
#include <string>                   // for operator<<, char_traits


namespace r_exec
{

MkNew::MkNew(r_code::Mem *m, Code *object): LObject(m)
{
    uint16_t write_index = 0;
    code(write_index++) = r_code::Atom::Marker(Opcodes::MkNew, 2);
    code(write_index++) = r_code::Atom::RPointer(0); // object.
    code(write_index++) = r_code::Atom::Float(0); // psln_thr.
    set_reference(0, object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MkLowRes::MkLowRes(r_code::Mem *m, Code *object): LObject(m)
{
    uint16_t write_index = 0;
    code(write_index++) = r_code::Atom::Marker(Opcodes::MkLowRes, 2);
    code(write_index++) = r_code::Atom::RPointer(0); // object.
    code(write_index++) = r_code::Atom::Float(0); // psln_thr.
    set_reference(0, object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MkLowSln::MkLowSln(r_code::Mem *m, Code *object): LObject(m)
{
    uint16_t write_index = 0;
    code(write_index++) = r_code::Atom::Marker(Opcodes::MkLowSln, 2);
    code(write_index++) = r_code::Atom::RPointer(0); // object.
    code(write_index++) = r_code::Atom::Float(0); // psln_thr.
    set_reference(0, object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MkHighSln::MkHighSln(r_code::Mem *m, Code *object): LObject(m)
{
    uint16_t write_index = 0;
    code(write_index++) = r_code::Atom::Marker(Opcodes::MkHighSln, 2);
    code(write_index++) = r_code::Atom::RPointer(0); // object.
    code(write_index++) = r_code::Atom::Float(0); // psln_thr.
    set_reference(0, object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MkLowAct::MkLowAct(r_code::Mem *m, Code *object): LObject(m)
{
    uint16_t write_index = 0;
    code(write_index++) = r_code::Atom::Marker(Opcodes::MkLowAct, 2);
    code(write_index++) = r_code::Atom::RPointer(0); // object.
    code(write_index++) = r_code::Atom::Float(0); // psln_thr.
    set_reference(0, object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MkHighAct::MkHighAct(r_code::Mem *m, Code *object): LObject(m)
{
    uint16_t write_index = 0;
    code(write_index++) = r_code::Atom::Marker(Opcodes::MkHighAct, 2);
    code(write_index++) = r_code::Atom::RPointer(0); // object.
    code(write_index++) = r_code::Atom::Float(0); // psln_thr.
    set_reference(0, object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MkSlnChg::MkSlnChg(r_code::Mem *m, Code *object, double value): LObject(m)
{
    uint16_t write_index = 0;
    code(write_index++) = r_code::Atom::Marker(Opcodes::MkSlnChg, 3);
    code(write_index++) = r_code::Atom::RPointer(0); // object.
    code(write_index++) = r_code::Atom::Float(value); // change.
    code(write_index++) = r_code::Atom::Float(0); // psln_thr.
    set_reference(0, object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MkActChg::MkActChg(r_code::Mem *m, Code *object, double value): LObject(m)
{
    uint16_t write_index = 0;
    code(write_index++) = r_code::Atom::Marker(Opcodes::MkActChg, 3);
    code(write_index++) = r_code::Atom::RPointer(0); // object.
    code(write_index++) = r_code::Atom::Float(value); // change.
    code(write_index++) = r_code::Atom::Float(0); // psln_thr.
    set_reference(0, object);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

_Fact::_Fact(): LObject()
{
}

_Fact::_Fact(SysObject *source): LObject(source)
{
}

_Fact::_Fact(_Fact *f)
{
    for (uint16_t i = 0; i < f->code_size(); ++i) {
        code(i) = f->code(i);
    }

    for (uint16_t i = 0; i < f->references_size(); ++i) {
        add_reference(f->get_reference(i));
    }
}

_Fact::_Fact(uint16_t opcode, Code *object, uint64_t after, uint64_t before, double confidence, double psln_thr): LObject()
{
    code(0) = Atom::Object(opcode, FACT_ARITY);
    code(FACT_OBJ) = Atom::RPointer(0);
    code(FACT_AFTER) = Atom::IPointer(FACT_ARITY + 1);
    code(FACT_BEFORE) = Atom::IPointer(FACT_ARITY + 4);
    code(FACT_CFD) = Atom::Float(confidence);
    code(FACT_ARITY) = Atom::Float(psln_thr);
    Utils::SetIndirectTimestamp<Code>(this, FACT_AFTER, after);
    Utils::SetIndirectTimestamp<Code>(this, FACT_BEFORE, before);
    add_reference(object);
}

bool _Fact::is_fact() const
{
    return (code(0).asOpcode() == Opcodes::Fact);
}

bool _Fact::is_anti_fact() const
{
    return (code(0).asOpcode() == Opcodes::AntiFact);
}

void _Fact::set_opposite()
{
    if (is_fact()) {
        code(0) = Atom::Object(Opcodes::AntiFact, FACT_ARITY);
    } else {
        code(0) = Atom::Object(Opcodes::Fact, FACT_ARITY);
    }
}

_Fact *_Fact::get_absentee() const
{
    _Fact *absentee;

    if (is_fact()) {
        absentee = new AntiFact(get_reference(0), get_after(), get_before(), 1, 1);
    } else {
        absentee = new Fact(get_reference(0), get_after(), get_before(), 1, 1);
    }

    return absentee;
}

bool _Fact::is_invalidated()
{
    if (LObject::is_invalidated()) {
        return true;
    }

    if (get_reference(0)->is_invalidated()) {
        invalidate();
        return true;
    }

    return false;
}

bool _Fact::match_timings_sync(const _Fact *evidence) const   // intervals of the form [after,before[.
{
    uint64_t after = get_after();
    uint64_t e_after = evidence->get_after();
    uint64_t e_before = evidence->get_before();
    return !(e_after > after + Utils::GetTimeTolerance() || e_before <= after);
}

bool _Fact::match_timings_overlap(const _Fact *evidence) const   // intervals of the form [after,before[.
{
    uint64_t after = get_after();
    uint64_t before = get_before();
    uint64_t e_after = evidence->get_after();
    uint64_t e_before = evidence->get_before();
    return !(e_after >= before || e_before <= after);
}

bool _Fact::match_timings_inclusive(const _Fact *evidence) const   // intervals of the form [after,before[.
{
    uint64_t after = get_after();
    uint64_t before = get_before();
    uint64_t e_after = evidence->get_after();
    uint64_t e_before = evidence->get_before();
    return (e_after <= after && e_before >= before);
}

bool _Fact::Match(const Code *lhs, uint16_t lhs_base_index, uint16_t lhs_index, const Code *rhs, uint16_t rhs_index, uint16_t lhs_arity)
{
    uint16_t lhs_full_index = lhs_base_index + lhs_index;
    Atom lhs_atom = lhs->code(lhs_full_index);
    Atom rhs_atom = rhs->code(rhs_index);

    switch (lhs_atom.getDescriptor()) {
    case Atom::T_WILDCARD:
    case Atom::WILDCARD:
    case Atom::VL_PTR:
        break;

    case Atom::I_PTR:
        switch (rhs_atom.getDescriptor()) {
        case Atom::I_PTR:
            if (!MatchStructure(lhs, lhs_atom.asIndex(), 0, rhs, rhs_atom.asIndex())) {
                return false;
            }

            break;

        case Atom::T_WILDCARD:
        case Atom::WILDCARD:
        case Atom::VL_PTR:
            break;

        default:
            return false;
        }

        break;

    case Atom::R_PTR:
        switch (rhs_atom.getDescriptor()) {
        case Atom::R_PTR:
            if (!MatchObject(lhs->get_reference(lhs_atom.asIndex()), rhs->get_reference(rhs_atom.asIndex()))) {
                return false;
            }

            break;

        case Atom::T_WILDCARD:
        case Atom::WILDCARD:
        case Atom::VL_PTR:
            break;

        default:
            return false;
        }

        break;

    default:
        switch (rhs_atom.getDescriptor()) {
        case Atom::T_WILDCARD:
        case Atom::WILDCARD:
        case Atom::VL_PTR:
            break;

        default:
            if (!MatchAtom(lhs_atom, rhs_atom)) {
                return false;
            }

            break;
        }

        break;
    }

    if (lhs_index == lhs_arity) {
        return true;
    }

    return Match(lhs, lhs_base_index, lhs_index + 1, rhs, rhs_index + 1, lhs_arity);
}

bool _Fact::MatchAtom(Atom lhs, Atom rhs)
{
    if (lhs == rhs) {
        return true;
    }

    if (lhs.isFloat() && rhs.isFloat()) {
        return Utils::Equal(lhs.asFloat(), rhs.asFloat());
    }

    return false;
}

bool _Fact::MatchStructure(const Code *lhs, uint16_t lhs_base_index, uint16_t lhs_index, const Code *rhs, uint16_t rhs_index)
{
    uint16_t lhs_full_index = lhs_base_index + lhs_index;
    Atom lhs_atom = lhs->code(lhs_full_index);
    Atom rhs_atom = rhs->code(rhs_index);

    if (lhs_atom != rhs_atom) {
        return false;
    }

    uint16_t arity = lhs_atom.getAtomCount();

    if (arity == 0) { // empty sets.
        return true;
    }

    if (lhs_atom.getDescriptor() == Atom::TIMESTAMP) {
        return Utils::Synchronous(Utils::GetTimestamp(&lhs->code(lhs_full_index)), Utils::GetTimestamp(&rhs->code(rhs_index)));
    }

    return Match(lhs, lhs_base_index, lhs_index + 1, rhs, rhs_index + 1, arity);
}

bool _Fact::MatchObject(const Code *lhs, const Code *rhs)
{
    if (lhs->code(0) != rhs->code(0)) {
        return false;
    }

    uint16_t lhs_opcode = lhs->code(0).asOpcode();

    if (lhs_opcode == Opcodes::Ent ||
        lhs_opcode == Opcodes::Ont ||
        lhs_opcode == Opcodes::Mdl ||
        lhs_opcode == Opcodes::Cst) {
        return lhs == rhs;
    }

    return Match(lhs, 0, 1, rhs, 1, lhs->code(0).getAtomCount());
}

bool _Fact::CounterEvidence(const Code *lhs, const Code *rhs)
{
    uint16_t opcode = lhs->code(0).asOpcode();

    if (opcode == Opcodes::Ent ||
        opcode == Opcodes::Ont ||
        opcode == Opcodes::IMdl) {
        return false;
    }

    if (lhs->code(0) != rhs->code(0)) {
        return false;
    }

    if (opcode == Opcodes::MkVal) {
        if (lhs->get_reference(lhs->code(MK_VAL_OBJ).asIndex()) == rhs->get_reference(rhs->code(MK_VAL_OBJ).asIndex()) &&
            lhs->get_reference(lhs->code(MK_VAL_ATTR).asIndex()) == rhs->get_reference(rhs->code(MK_VAL_ATTR).asIndex())) { // same attribute for the same object; value: r_ptr, atomic value or structure.
            Atom lhs_atom = lhs->code(MK_VAL_VALUE);
            Atom rhs_atom = rhs->code(MK_VAL_VALUE);

            if (lhs_atom.isFloat()) {
                if (rhs_atom.isFloat()) {
                    return !MatchAtom(lhs_atom, rhs_atom);
                } else {
                    return false;
                }
            } else if (rhs_atom.isFloat()) {
                return false;
            }

            uint16_t lhs_desc = lhs_atom.getDescriptor();

            if (lhs_desc != rhs_atom.getDescriptor()) { // values of different types.
                return false;
            }

            switch (lhs_desc) {
            case Atom::T_WILDCARD:
            case Atom::WILDCARD:
                return false;

            case Atom::R_PTR:
                return !MatchObject(lhs->get_reference(lhs->code(MK_VAL_VALUE).asIndex()), rhs->get_reference(rhs->code(MK_VAL_VALUE).asIndex()));

            case Atom::I_PTR:
                return !MatchStructure(lhs, MK_VAL_VALUE, lhs_atom.asIndex(), rhs, rhs_atom.asIndex());

            default:
                return !MatchAtom(lhs_atom, rhs_atom);
            }
        }
    } else if (opcode == Opcodes::ICst) {
        if (lhs->get_reference(0) != rhs->get_reference(0)) { // check if the icsts instantiate the same cst.
            return false;
        }

        for (uint64_t i = 0; i < ((ICST *)lhs)->components.size(); ++i) { // compare all components 2 by 2.
            if (CounterEvidence(((ICST *)lhs)->components[i], ((ICST *)rhs)->components[i])) {
                return true;
            }
        }
    }

    return false;
}

MatchResult _Fact::is_evidence(const _Fact *target) const
{
    if (MatchObject(get_reference(0), target->get_reference(0))) {
        MatchResult r;

        if (target->code(0) == code(0)) {
            r = MATCH_SUCCESS_POSITIVE;
        } else {
            r = MATCH_SUCCESS_NEGATIVE;
        }

        if (target->match_timings_overlap(this)) {
            return r;
        }
    } else if (target->code(0) == code(0)) { // check for a counter-evidence only if both the lhs and rhs are of the same kind of fact.
        if (target->match_timings_inclusive(this)) { // check timings first as this is less expensive than the counter-evidence check.
            if (CounterEvidence(get_reference(0), target->get_reference(0))) {
                return MATCH_SUCCESS_NEGATIVE;
            }
        }
    }

    return MATCH_FAILURE;
}

MatchResult _Fact::is_timeless_evidence(const _Fact *target) const
{
    if (MatchObject(get_reference(0), target->get_reference(0))) {
        MatchResult r;

        if (target->code(0) == code(0)) {
            r = MATCH_SUCCESS_POSITIVE;
        } else {
            r = MATCH_SUCCESS_NEGATIVE;
        }

        return r;
    } else if (target->code(0) == code(0)) { // check for a counter-evidence only if both the lhs and rhs are of the same kind of fact.
        if (CounterEvidence(get_reference(0), target->get_reference(0))) {
            return MATCH_SUCCESS_NEGATIVE;
        }
    }

    return MATCH_FAILURE;
}

float _Fact::get_cfd() const
{
    return code(FACT_CFD).asFloat();
}

void _Fact::set_cfd(double cfd)
{
    code(FACT_CFD) = Atom::Float(cfd);
}

Pred *_Fact::get_pred() const
{
    Code *pred = get_reference(0);

    if (pred->code(0).asOpcode() == Opcodes::Pred) {
        return (Pred *)pred;
    }

    return nullptr;
}

Goal *_Fact::get_goal() const
{
    Code *goal = get_reference(0);

    if (goal->code(0).asOpcode() == Opcodes::Goal) {
        return (Goal *)goal;
    }

    return nullptr;
}

uint64_t _Fact::get_after() const
{
    return Utils::GetTimestamp<Code>(this, FACT_AFTER);
}

uint64_t _Fact::get_before() const
{
    return Utils::GetTimestamp<Code>(this, FACT_BEFORE);
}

void _Fact::trace() const
{
    std::cout << "<" << get_oid() << " " << Utils::RelativeTime(get_after()) << " " << Utils::RelativeTime(get_before()) << ">" << std::endl;
}

////////////////////////////////////////////////////////////////

void *Fact::operator new(size_t s)
{
    return _Mem::Get()->allocate_object(Atom::Object(Opcodes::Fact, FACT_ARITY));
}

Fact::Fact(): _Fact()
{
    code(0) = Atom::Object(Opcodes::Fact, FACT_ARITY);
}

Fact::Fact(SysObject *source): _Fact(source)
{
}

Fact::Fact(Fact *f): _Fact(f)
{
}

Fact::Fact(Code *object, uint64_t after, uint64_t before, double confidence, double psln_thr): _Fact(Opcodes::Fact, object, after, before, confidence, psln_thr)
{
}

////////////////////////////////////////////////////////////////

void *AntiFact::operator new(size_t s)
{
    return _Mem::Get()->allocate_object(Atom::Object(Opcodes::AntiFact, FACT_ARITY));
}

AntiFact::AntiFact(): _Fact()
{
    code(0) = Atom::Object(Opcodes::AntiFact, FACT_ARITY);
}

AntiFact::AntiFact(SysObject *source): _Fact(source)
{
}

AntiFact::AntiFact(AntiFact *f): _Fact(f)
{
}

AntiFact::AntiFact(Code *object, uint64_t after, uint64_t before, double confidence, double psln_thr): _Fact(Opcodes::AntiFact, object, after, before, confidence, psln_thr)
{
}

////////////////////////////////////////////////////////////////

Pred::Pred(): LObject()
{
}

Pred::Pred(SysObject *source): LObject(source)
{
}

Pred::Pred(_Fact *target, double psln_thr): LObject()
{
    code(0) = Atom::Object(Opcodes::Pred, PRED_ARITY);
    code(PRED_TARGET) = Atom::RPointer(0);
    code(PRED_ARITY) = Atom::Float(psln_thr);
    add_reference(target);
}

bool Pred::is_invalidated()
{
    if (LObject::is_invalidated()) {
        return true;
    }

    for (P<Sim> simulation : simulations) {
        if (simulation->is_invalidated()) {
            invalidate();
            return true;
        }
    }

    for (P<_Fact> ground : grounds) {
        if (ground->is_invalidated()) {
            invalidate();
            return true;
        }
    }

    if (get_reference(0)->is_invalidated()) {
        invalidate();
        return true;
    }

    return false;
}

bool Pred::grounds_invalidated(_Fact *evidence)
{
    for (P<_Fact> ground : grounds) {
        if (evidence->is_evidence(ground) == MATCH_SUCCESS_NEGATIVE) {
            return true;
        }
    }

    return false;
}

_Fact *Pred::get_target() const
{
    return (_Fact *)get_reference(0);
}

bool Pred::is_simulation() const
{
    return simulations.size() > 0;
}

Sim *Pred::get_simulation(Controller *root) const
{
    for (P<Sim> simulation : simulations) {
        if (simulation->root == root) {
            return simulation;
        }
    }

    return nullptr;
}

////////////////////////////////////////////////////////////////

Goal::Goal(): LObject(), sim(nullptr), ground(nullptr)
{
}

Goal::Goal(SysObject *source): LObject(source), sim(nullptr), ground(nullptr)
{
}

Goal::Goal(_Fact *target, Code *actor, double psln_thr): LObject(), sim(nullptr), ground(nullptr)
{
    code(0) = Atom::Object(Opcodes::Goal, GOAL_ARITY);
    code(GOAL_TARGET) = Atom::RPointer(0);
    code(GOAL_ACTR) = Atom::RPointer(1);
    code(GOAL_ARITY) = Atom::Float(psln_thr);
    add_reference(target);
    add_reference(actor);
}

bool Goal::invalidate()   // return false when was not invalidated, true otherwise.
{
    if (sim != nullptr) {
        sim->invalidate();
    }

    return LObject::invalidate();
}

bool Goal::is_invalidated()
{
    if (LObject::is_invalidated()) {
        return true;
    }

    if (sim != nullptr && sim->super_goal != nullptr && sim->super_goal->is_invalidated()) {
        invalidate();
        return true;
    }

    return false;
}

bool Goal::ground_invalidated(_Fact *evidence)
{
    if (ground != nullptr) {
        return ground->get_pred()->grounds_invalidated(evidence);
    }

    return false;
}

bool Goal::is_requirement() const
{
    if (sim != nullptr && sim->is_requirement) {
        return true;
    }

    return false;
}

bool Goal::is_self_goal() const
{
    return (get_actor() == _Mem::Get()->get_self());
}

bool Goal::is_drive() const
{
    return (sim == nullptr && is_self_goal());
}

_Fact *Goal::get_target() const
{
    return (_Fact *)get_reference(0);
}

_Fact *Goal::get_super_goal() const
{
    return sim->super_goal;
}

Code *Goal::get_actor() const
{
    return get_reference(code(GOAL_ACTR).asIndex());
}

double Goal::get_strength(uint64_t now) const
{
    _Fact *target = get_target();
    return target->get_cfd() / (target->get_before() - now);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Sim::Sim(): _Object(), invalidated(0), is_requirement(false), opposite(false), super_goal(nullptr), root(nullptr), sol(nullptr), sol_cfd(0), sol_before(0)
{
}

Sim::Sim(Sim *s): _Object(), invalidated(0), is_requirement(false), opposite(s->opposite), mode(s->mode), thz(s->thz), super_goal(s->super_goal), root(s->root), sol(s->sol), sol_cfd(s->sol_cfd), sol_before(s->sol_before)
{
}

Sim::Sim(SimMode mode, uint64_t thz, Fact *super_goal, bool opposite, Controller *root): _Object(), invalidated(0), is_requirement(false), opposite(opposite), mode(mode), thz(thz), super_goal(super_goal), root(root), sol(nullptr), sol_cfd(0), sol_before(0)
{
}

Sim::Sim(SimMode mode, uint64_t thz, Fact *super_goal, bool opposite, Controller *root, Controller *sol, double sol_cfd, uint64_t sol_deadline): _Object(), invalidated(0), is_requirement(false), opposite(opposite), mode(mode), thz(thz), super_goal(super_goal), root(root), sol(sol), sol_cfd(sol_cfd), sol_before(0)
{
}

void Sim::invalidate()
{
    invalidated = 1;
}

bool Sim::is_invalidated()
{
    if (invalidated == 1) {
        return true;
    }

    if (super_goal != nullptr && super_goal->is_invalidated()) {
        invalidate();
        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

MkRdx::MkRdx(): LObject(), bindings(nullptr)
{
}

MkRdx::MkRdx(SysObject *source): LObject(source), bindings(nullptr)
{
}

MkRdx::MkRdx(Code *imdl_fact, Code *input, Code *output, double psln_thr, BindingMap *binding_map): LObject(), bindings(binding_map)
{
    uint16_t extent_index = MK_RDX_ARITY + 1;
    code(0) = Atom::Marker(Opcodes::MkRdx, MK_RDX_ARITY);
    code(MK_RDX_CODE) = Atom::RPointer(0); // code.
    add_reference(imdl_fact);
    code(MK_RDX_INPUTS) = Atom::IPointer(extent_index); // inputs.
    code(MK_RDX_ARITY) = Atom::Float(psln_thr);
    code(extent_index++) = Atom::Set(1); // set of one input.
    code(extent_index++) = Atom::RPointer(1);
    add_reference(input);
    code(MK_RDX_PRODS) = Atom::IPointer(extent_index); // set of one production.
    code(extent_index++) = Atom::Set(1);
    code(extent_index++) = Atom::RPointer(2);
    add_reference(output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Success::Success(): LObject()
{
}

Success::Success(_Fact *object, _Fact *evidence, double psln_thr): LObject()
{
    code(0) = Atom::Object(Opcodes::Success, SUCCESS_ARITY);
    code(SUCCESS_OBJ) = Atom::RPointer(0);

    if (evidence) {
        code(SUCCESS_EVD) = Atom::RPointer(1);
    } else {
        code(SUCCESS_EVD) = Atom::Nil();
    }

    code(SUCCESS_ARITY) = Atom::Float(psln_thr);
    add_reference(object);

    if (evidence) {
        add_reference(evidence);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Perf::Perf(): LObject()
{
}

Perf::Perf(uint64_t reduction_job_avg_latency, int64_t d_reduction_job_avg_latency, uint64_t time_job_avg_latency, int64_t d_time_job_avg_latency): LObject()
{
    code(0) = Atom::Object(Opcodes::Perf, PERF_ARITY);
    code(PERF_RDX_LTCY) = Atom::Float(reduction_job_avg_latency);
    code(PERF_D_RDX_LTCY) = Atom::Float(d_reduction_job_avg_latency);
    code(PERF_TIME_LTCY) = Atom::Float(time_job_avg_latency);
    code(PERF_D_TIME_LTCY) = Atom::Float(d_time_job_avg_latency);
    code(PERF_ARITY) = Atom::Float(1);
}

////////////////////////////////////////////////////////////////

ICST::ICST(): LObject()
{
}

ICST::ICST(SysObject *source): LObject(source)
{
}

bool ICST::is_invalidated()
{
    if (LObject::is_invalidated()) {
        //std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" "<<std::hex<<this<<std::dec<<" icst was invalidated"<<std::endl;
        return true;
    }

    for (P<_Fact> component : components) {
        if (component->is_invalidated()) {
            invalidate();
            //std::cout<<Time::ToString_seconds(Now()-Utils::GetTimeReference())<<" "<<std::hex<<this<<std::dec<<" icst invalidated"<<std::endl;
            return true;
        }
    }

    return false;
}

bool ICST::contains(_Fact *component, uint16_t &component_index) const
{
    for (uint64_t i = 0; i < components.size(); ++i) {
        if (components[i] == component) {
            component_index = i;
            return true;
        }
    }

    return false;
}
}
