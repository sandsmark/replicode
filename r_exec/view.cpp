//	view.cpp
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

#include "view.h"
#include "CoreLibrary/utils.h"
#include "group.h"
#include "mem.h"

#include <limits>

const double PLUS_INFINITY = std::numeric_limits<float>::infinity();


namespace r_exec {

std::mutex oidMutex;

uint64_t View::LastOID = 0;

uint64_t View::GetOID()
{
    std::lock_guard<std::mutex> guard(oidMutex);
    uint64_t oid = LastOID++;
    return oid;
}

uint16_t View::ViewOpcode;

double View::MorphValue(double value, double source_thr, double destination_thr) {

    if (value == 0)
        return destination_thr;

    if (source_thr > 0) {

        if (destination_thr > 0) {

            double r = value * destination_thr / source_thr;
            if (r > 1) // handles precision errors.
                r = 1;
            return r;
        } else
            return value;
    }
    return destination_thr + value;
}

double View::MorphChange(double change, double source_thr, double destination_thr) { // change is always >0.

    if (source_thr > 0) {

        if (destination_thr > 0)
            return change * destination_thr / source_thr;
        else
            return change;
    }
    return destination_thr + change;
}

View::View(View *view, Group *group): r_code::View(), controller(NULL) {

    Group *source = view->get_host();
    object = view->object;
    memcpy(_code, view->_code, VIEW_CODE_MAX_SIZE * sizeof(Atom));
    _code[VIEW_OID].atom = GetOID();
    references[0] = group; // host.
    references[1] = source; // origin.

// morph ctrl values; NB: res is not morphed as it is expressed as a multiple of the upr.
    code(VIEW_SLN) = Atom::Float(MorphValue(view->code(VIEW_SLN).asDouble(), source->get_sln_thr(), group->get_sln_thr()));
    switch (object->code(0).getDescriptor()) {
    case Atom::GROUP:
        code(GRP_VIEW_VIS) = Atom::Float(MorphValue(view->code(GRP_VIEW_VIS).asDouble(), source->get_vis_thr(), group->get_vis_thr()));
        break;
    case Atom::NULL_PROGRAM:
    case Atom::INSTANTIATED_PROGRAM:
    case Atom::INSTANTIATED_CPP_PROGRAM:
    case Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
    case Atom::INSTANTIATED_ANTI_PROGRAM:
    case Atom::COMPOSITE_STATE:
    case Atom::MODEL:
        code(VIEW_ACT) = Atom::Float(MorphValue(view->code(VIEW_ACT).asDouble(), source->get_act_thr(), group->get_act_thr()));
        break;
    }

    reset();
}

void View::set_object(r_code::Code *object) {

    this->object = object;
    reset();
}

void View::reset_ctrl_values() {

    sln_changes = 0;
    acc_sln = 0;
    act_changes = 0;
    acc_act = 0;
    vis_changes = 0;
    acc_vis = 0;
    res_changes = 0;
    acc_res = 0;

    periods_at_low_sln = 0;
    periods_at_high_sln = 0;
    periods_at_low_act = 0;
    periods_at_high_act = 0;
}

void View::reset_init_sln() {

    initial_sln = get_sln();
}

void View::reset_init_act() {

    if (object != NULL)
        initial_act = get_act();
    else
        initial_act = 0;
}

double View::update_res() {

    double new_res = get_res();
    if (new_res == PLUS_INFINITY)
        return new_res;
    if (res_changes > 0 && acc_res != 0)
        new_res = get_res() + (double)acc_res / (double)res_changes;
    if (--new_res < 0) // decremented by one on behalf of the group (at upr).
        new_res = 0;
    code(VIEW_RES) = r_code::Atom::Float(new_res);
    acc_res = 0;
    res_changes = 0;
    return get_res();
}

double View::update_sln(double low, double high) {

    if (sln_changes > 0 && acc_sln != 0) {

        double new_sln = get_sln() + acc_sln / sln_changes;
        if (new_sln < 0)
            new_sln = 0;
        else if (new_sln > 1)
            new_sln = 1;
        code(VIEW_SLN) = r_code::Atom::Float(new_sln);
    }
    acc_sln = 0;
    sln_changes = 0;

    double sln = get_sln();
    if (sln < low)
        ++periods_at_low_sln;
    else {

        periods_at_low_sln = 0;
        if (sln > high)
            ++periods_at_high_sln;
        else
            periods_at_high_sln = 0;
    }
    return sln;
}

double View::update_act(double low, double high) {

    if (act_changes > 0 && acc_act != 0) {

        double new_act = get_act() + acc_act / act_changes;
        if (new_act < 0)
            new_act = 0;
        else if (new_act > 1)
            new_act = 1;
        code(VIEW_ACT) = r_code::Atom::Float(new_act);
    }
    acc_act = 0;
    act_changes = 0;

    double act = get_act();
    if (act < low)
        ++periods_at_low_act;
    else {

        periods_at_low_act = 0;
        if (act > high)
            ++periods_at_high_act;
        else
            periods_at_high_act = 0;
    }
    return act;
}

double View::update_vis() {

    if (vis_changes > 0 && acc_vis != 0) {

        double new_vis = get_vis() + acc_vis / vis_changes;
        if (new_vis < 0)
            new_vis = 0;
        else if (new_vis > 1)
            new_vis = 1;
        code(GRP_VIEW_VIS) = r_code::Atom::Float(new_vis);
    }
    acc_vis = 0;
    vis_changes = 0;
    return get_vis();
}

void View::delete_from_object() {

    object->acq_views();
    object->views.erase(this);
    if (object->views.size() == 0) {
        object->rel_views(); // avoid deadlock
        object->invalidate();
    } else {
        object->rel_views();
    }
}

void View::delete_from_group()
{
    Group *g = get_host();
    std::lock_guard<std::mutex> guard(g->mutex);
    g->delete_view(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

NotificationView::NotificationView(Code *origin, Code *destination, Code *marker): View() {

    code(VIEW_OPCODE) = r_code::Atom::SSet(ViewOpcode, VIEW_ARITY); // Structured Set.
    code(VIEW_SYNC) = r_code::Atom::Float(View::SYNC_ONCE); // sync once.
    code(VIEW_IJT) = r_code::Atom::IPointer(VIEW_ARITY + 1); // iptr to ijt.
    code(VIEW_SLN) = r_code::Atom::Float(1); // sln.
    code(VIEW_RES) = r_code::Atom::Float(_Mem::Get()->get_ntf_mk_res()); // res.
    code(VIEW_HOST) = r_code::Atom::RPointer(0); // destination.
    code(VIEW_ORG) = r_code::Atom::RPointer(1); // origin.
    code(VIEW_ARITY + 1) = r_code::Atom::Timestamp(); // ijt will be set at injection time.
    code(VIEW_ARITY + 2) = 0;
    code(VIEW_ARITY + 3) = 0;
    references[0] = destination;
    references[1] = origin;

    reset_init_sln();

    object = marker;
}
}
