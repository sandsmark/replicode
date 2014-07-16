//	group.h
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

#ifndef group_h
#define group_h

#include <set>

#include "object.h"
#include "view.h"
#include <unordered_map>

namespace r_exec {

class _Mem;
class HLPController;

// Shared resources:
// all parameters: accessed by Mem::update and reduction cores (via overlay mod/set).
// all views: accessed by Mem::update and reduction cores.
// viewing_groups: accessed by Mem::injectNow and Mem::update.
class dll_export Group:
    public LObject {
private:
// Ctrl values.
    uint64_t sln_thr_changes;
    double acc_sln_thr;
    uint64_t act_thr_changes;
    double acc_act_thr;
    uint64_t vis_thr_changes;
    double acc_vis_thr;
    uint64_t c_sln_changes;
    double acc_c_sln;
    uint64_t c_act_changes;
    double acc_c_act;
    uint64_t c_sln_thr_changes;
    double acc_c_sln_thr;
    uint64_t c_act_thr_changes;
    double acc_c_act_thr;
    void reset_ctrl_values();

// Stats.
    double avg_sln;
    double high_sln;
    double low_sln;
    double avg_act;
    double high_act;
    double low_act;
    uint64_t sln_updates;
    uint64_t act_updates;

// Decay.
    double sln_decay;
    double sln_thr_decay;
    int64_t decay_periods_to_go;
    double decay_percentage_per_period;
    double decay_target; // -1: none, 0: sln, 1:sln_thr
    void reset_decay_values();

// Notifications.
    int64_t sln_change_monitoring_periods_to_go = 0;
    int64_t act_change_monitoring_periods_to_go = 0;

    void _mod_0_positive(uint16_t member_index, double value);
    void _mod_0_plus1(uint16_t member_index, double value);
    void _mod_minus1_plus1(uint16_t member_index, double value);
    void _set_0_positive(uint16_t member_index, double value);
    void _set_0_plus1(uint16_t member_index, double value);
    void _set_minus1_plus1(uint16_t member_index, double value);
    void _set_0_1(uint16_t member_index, double value);

    bool is_active_pgm(View *view);
    bool is_eligible_input(View *view);

    /// the view can hold anything but groups and notifications.
    void inject(View *view);

    void notifyNew(View *view);
    void cov(View *view);

    class GroupState {
    public:
        double former_sln_thr;
        bool was_c_active;
        bool is_c_active;
        bool was_c_salient;
        bool is_c_salient;
        GroupState(double former_sln_thr,
                   bool was_c_active,
                   bool is_c_active,
                   bool was_c_salient,
                   bool is_c_salient): former_sln_thr(former_sln_thr), was_c_active(was_c_active), is_c_active(is_c_active), was_c_salient(was_c_salient), is_c_salient(is_c_salient) {}
    };

    void _update_saliency(GroupState *state, View *view);
    void _update_activation(GroupState *state, View *view);
    void _update_visibility(GroupState *state, View *view);

    void _initiate_sln_propagation(Code *object, double change, double source_sln_thr) const;
    void _initiate_sln_propagation(Code *object, double change, double source_sln_thr, std::vector<Code *> &path) const;
    void _propagate_sln(Code *object, double change, double source_sln_thr, std::vector<Code *> &path) const;
public:
    std::mutex mutex;
// xxx_views are meant for erasing views with res==0. They are specialized by type to ease update operations.
// Active overlays are to be found in xxx_ipgm_views.
    std::unordered_map<uint64_t, P<View> > ipgm_views;
    std::unordered_map<uint64_t, P<View> > anti_ipgm_views;
    std::unordered_map<uint64_t, P<View> > input_less_ipgm_views;
    std::unordered_map<uint64_t, P<View> > notification_views;
    std::unordered_map<uint64_t, P<View> > group_views;
    std::unordered_map<uint64_t, P<View> > other_views;

// Defined to create reduction jobs in the viewing groups from the viewed group.
// Empty when the viewed group is invisible (this means that visible groups can be non c-active or non c-salient).
// Maintained by the viewing groups (at update time).
// Viewing groups are c-active and c-salient. the bool is the cov.
    std::unordered_map<Group *, bool> viewing_groups;

// Populated within update; ordered by increasing ijt; cleared at the beginning of update.
    std::multiset<P<View>, r_code::View::Less> newly_salient_views;

// Populated upon ipgm injection; used at update time; cleared afterward.
    std::vector<Controller *> new_controllers;

    class Operation {
    protected:
        Operation(uint64_t oid): oid(oid) {}
    public:
        virtual ~Operation() {};
        const uint64_t oid; // of the view.
        virtual void execute(Group *g) const = 0;
    };

    class ModSet:
        public Operation {
    protected:
        ModSet(uint64_t oid, uint16_t member_index, double value): Operation(oid), member_index(member_index), value(value) {}
        const uint16_t member_index;
        const double value;
    };

    class Mod:
        public ModSet {
    public:
        Mod(uint64_t oid, uint16_t member_index, double value): ModSet(oid, member_index, value) {}
        void execute(Group *g) const {

            View *v = g->get_view(oid);
            if (v)
                v->mod(member_index, value);
        }
    };

    class Set:
        public ModSet {
    public:
        Set(uint64_t oid, uint16_t member_index, double value): ModSet(oid, member_index, value) {}
        void execute(Group *g) const {

            View *v = g->get_view(oid);
            if (v)
                v->set(member_index, value);
        }
    };

// Pending mod/set operations on the group's view, exploited and cleared at update time.
    std::vector<Operation *> pending_operations;

    Group(r_code::Mem *m = NULL);
    Group(r_code::SysObject *source);
    virtual ~Group();

    bool invalidate(); // removes all views of itself and of any other object.

    bool all_views_cond(uint8_t &selector, std::unordered_map<uint64_t, P<View> >::const_iterator &it, std::unordered_map<uint64_t, P<View> >::const_iterator &end) {
        while (it == end) {
            switch (selector++) {
            case 0:
                it = anti_ipgm_views.begin();
                end = anti_ipgm_views.end();
                break;
            case 1:
                it = input_less_ipgm_views.begin();
                end = input_less_ipgm_views.end();
                break;
            case 2:
                it = notification_views.begin();
                end = notification_views.end();
                break;
            case 3:
                it = group_views.begin();
                end = group_views.end();
                break;
            case 4:
                it = other_views.begin();
                end = other_views.end();
                break;
            case 5:
                selector = 0;
                return false;
            }
        }
        return true;
    }

#define FOR_ALL_VIEWS_BEGIN(g,it) { \
        uint8_t selector; \
        std::unordered_map<uint64_t,P<View> >::const_iterator it=g->ipgm_views.begin(); \
        std::unordered_map<uint64_t,P<View> >::const_iterator end=g->ipgm_views.end(); \
        for(selector=0;g->all_views_cond(selector,it,end);++it){

#define FOR_ALL_VIEWS_BEGIN_NO_INC(g,it) { \
        uint8_t selector; \
        std::unordered_map<uint64_t,P<View> >::const_iterator it=g->ipgm_views.begin(); \
        std::unordered_map<uint64_t,P<View> >::const_iterator end=g->ipgm_views.end(); \
        for(selector=0;g->all_views_cond(selector,it,end);){

#define FOR_ALL_VIEWS_END } \
}

    bool views_with_inputs_cond(uint8_t &selector, std::unordered_map<uint64_t, P<View> >::const_iterator &it, std::unordered_map<uint64_t, P<View> >::const_iterator &end) {
        while (it == end) {
            switch (selector++) {
            case 0:
                it = anti_ipgm_views.begin();
                end = anti_ipgm_views.end();
                break;
            case 1:
                selector = 0;
                return false;
            }
        }
        return true;
    }

#define FOR_ALL_VIEWS_WITH_INPUTS_BEGIN(g,it) { \
        uint8_t selector; \
        std::unordered_map<uint64_t,P<View> >::const_iterator it=g->ipgm_views.begin(); \
        std::unordered_map<uint64_t,P<View> >::const_iterator end=g->ipgm_views.end(); \
        for(selector=0;g->views_with_inputs_cond(selector,it,end);++it){

#define FOR_ALL_VIEWS_WITH_INPUTS_END } \
}

    bool non_ntf_views_cond(uint8_t &selector, std::unordered_map<uint64_t, P<View> >::const_iterator &it, std::unordered_map<uint64_t, P<View> >::const_iterator &end) {
        while (it == end) {
            switch (selector++) {
            case 0:
                it = anti_ipgm_views.begin();
                end = anti_ipgm_views.end();
                break;
            case 1:
                it = input_less_ipgm_views.begin();
                end = input_less_ipgm_views.end();
                break;
            case 2:
                it = group_views.begin();
                end = group_views.end();
                break;
            case 3:
                it = other_views.begin();
                end = other_views.end();
                break;
            case 4:
                selector = 0;
                return false;
            }
        }
        return true;
    }

#define FOR_ALL_NON_NTF_VIEWS_BEGIN(g,it) { \
        uint8_t selector; \
        std::unordered_map<uint64_t,P<View> >::const_iterator it=g->ipgm_views.begin(); \
        std::unordered_map<uint64_t,P<View> >::const_iterator end=g->ipgm_views.end(); \
        for(selector=0;g->non_ntf_views_cond(selector,it,end);++it){

#define FOR_ALL_NON_NTF_VIEWS_END } \
}

    View *get_view(uint64_t OID);

    uint64_t get_upr() const;

    double get_sln_thr() const;
    double get_act_thr() const;
    double get_vis_thr() const;

    double get_c_sln() const;
    double get_c_act() const;

    double get_c_sln_thr() const;
    double get_c_act_thr() const;

    void mod_sln_thr(double value);
    void set_sln_thr(double value);
    void mod_act_thr(double value);
    void set_act_thr(double value);
    void mod_vis_thr(double value);
    void set_vis_thr(double value);
    void mod_c_sln(double value);
    void set_c_sln(double value);
    void mod_c_act(double value);
    void set_c_act(double value);
    void mod_c_sln_thr(double value);
    void set_c_sln_thr(double value);
    void mod_c_act_thr(double value);
    void set_c_act_thr(double value);

    double update_sln_thr(); // computes and applies decay on sln thr if any.
    double update_act_thr();
    double update_vis_thr();
    double update_c_sln();
    double update_c_act();
    double update_c_sln_thr();
    double update_c_act_thr();

    double get_sln_chg_thr();
    double get_sln_chg_prd();
    double get_act_chg_thr();
    double get_act_chg_prd();

    double get_avg_sln();
    double get_high_sln();
    double get_low_sln();
    double get_avg_act();
    double get_high_act();
    double get_low_act();

    double get_high_sln_thr();
    double get_low_sln_thr();
    double get_sln_ntf_prd();
    double get_high_act_thr();
    double get_low_act_thr();
    double get_act_ntf_prd();
    double get_low_res_thr();

    double get_ntf_new();

    uint16_t get_ntf_grp_count();
    Group *get_ntf_grp(uint16_t i); // i starts at 1.

// Delegate to views; update stats and notifies.
    double update_res(View *v);
    double update_sln(View *v); // applies decay if any.
    double update_act(View *v);

// Target upr, spr, c_sln, c_act, sln_thr, act_thr, vis_thr, c_sln_thr, c_act_thr, sln_chg_thr,
// sln_chg_prd, act_chg_thr, act_chg_prd, high_sln_thr, low_sln_thr, sln_ntf_prd, high_act_thr, low_act_thr, act_ntf_prd, low_res_thr, res_ntf_prd, ntf_new,
// dcy_per, dcy-tgt, dcy_prd.
    void mod(uint16_t member_index, double value);
    void set(uint16_t member_index, double value);

    void reset_stats(); // called at the begining of an update.
    void update_stats(); // at the end of an update; may produce notifcations.

    bool load(View *view, Code *object);

// Called at each update period.
// - set the final resilience value, if 0, delete.
// - set the final saliency.
// - set the final activation.
// - set the final visibility, cov.
// - propagate saliency changes.
// - inject next update job for the group.
// - inject new signaling jobs if act pgm with no input or act |pgm.
// - notify high and low values.
    void update(uint64_t planned_time);

    void inject_new_object(View *view);
    void inject_existing_object(View *view);

    /// the view holds a group.
    void inject_group(View *view);
    void inject_notification(View *view, bool lock);
    void inject_hlps(std::vector<View *> &views);
    /// group is assumed to be c-salient, already protected
    void inject_reduction_jobs(View *view);

    void cov();

    class Hash {
    public:
        size_t operator()(Group *g) const {
            return (size_t)g;
        }
    };

    class Equal {
    public:
        bool operator()(const Group *lhs, const Group *rhs) const {
            return lhs == rhs;
        }
    };

    void delete_view(View *v);
    void delete_view(std::unordered_map<uint64_t, P<View> >::const_iterator &v);

    Group *get_secondary_group();
    void load_secondary_mdl_controller(View *view);
    void inject_secondary_mdl_controller(View *view);

    uint64_t get_next_upr_time(uint64_t now) const;
    uint64_t get_prev_upr_time(uint64_t now) const;
};
}


#include "object.tpl.cpp"

#endif
