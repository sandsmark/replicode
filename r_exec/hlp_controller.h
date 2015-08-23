//	hlp_controller.h
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

#ifndef hlp_controller_h
#define hlp_controller_h


#include <r_code/list.h>            // for list
#include <r_code/object.h>          // for Code
#include <r_code/replicode_defs.h>  // for MDL_HIDDEN_REFS
#include <r_exec/binding_map.h>     // for MatchResult
#include <r_exec/factory.h>         // for _Fact
#include <r_exec/init.h>            // for Now
#include <r_exec/overlay.h>         // for OController
#include <r_exec/view.h>            // for View
#include <stdint.h>                 // for uint64_t, uint16_t
#include <mutex>                    // for mutex, lock_guard
#include <vector>                   // for vector

#include <replicode_common.h>       // for P

namespace r_exec {
class Group;
}  // namespace r_exec

namespace r_exec
{

typedef enum {
    WR_DISABLED = 0,
    SR_DISABLED_NO_WR = 1,
    SR_DISABLED_WR = 2,
    WR_ENABLED = 3,
    NO_R = 4
} ChainingStatus;

class HLPController:
    public OController
{
private:
    uint64_t strong_requirement_count; // number of active strong requirements in the same group; updated dynamically.
    uint64_t weak_requirement_count; // number of active weak requirements in the same group; updated dynamically.
    uint64_t requirement_count; // sum of the two above.
protected:
    class EEntry   // evidences.
    {
    private:
        void load_data(_Fact *evidence);
    public:
        P<_Fact> evidence;
        uint64_t after;
        uint64_t before;
        double confidence;

        EEntry();
        EEntry(_Fact *evidence);
        EEntry(_Fact *evidence, _Fact *payload);

        bool is_too_old(uint64_t now) const
        {
            return (evidence->is_invalidated() || before < now);
        }
    };

    class PEEntry: // predicted evidences.
        public EEntry
    {
    public:
        PEEntry();
        PEEntry(_Fact *evidence);
    };

    template<class E> class Cache
    {
    public:
        std::mutex mutex;
        r_code::list<E> evidences;
    };

    Cache<EEntry> evidences;
    Cache<PEEntry> predicted_evidences;

    template<class E> void _store_evidence(Cache<E> *cache, _Fact *evidence)
    {
        E e(evidence);
        std::lock_guard<std::mutex> guard(cache->mutex);
        uint64_t now = r_exec::Now();
        typename r_code::list<E>::const_iterator _e;

        for (_e = cache->evidences.begin(); _e != cache->evidences.end();) {
            if ((*_e).is_too_old(now)) { // garbage collection.
                _e = cache->evidences.erase(_e);
            } else {
                ++_e;
            }
        }

        cache->evidences.push_front(e);
    }

    P<HLPBindingMap> bindings;

    bool evaluate_bwd_guards(HLPBindingMap *bm);

    MatchResult check_evidences(_Fact *target, _Fact *&evidence); // evidence with the match (positive or negative), get_absentee(target) otherwise.
    MatchResult check_predicted_evidences(_Fact *target, _Fact *&evidence); // evidence with the match (positive or negative), NULL otherwise.

    bool _has_tpl_args;
    uint64_t ref_count; // used to detect _Object::refCount dropping down to 1 for hlp with tpl args.
    bool is_orphan(); // true when there are tpl args and no requirements: the controller cannot execute anymore.

    std::vector<P<HLPController> > controllers; // all controllers for models/states instantiated in the patterns; case of models: [0]==lhs, [1]==rhs.
    uint64_t last_match_time; // last time a match occurred (fwd), regardless of its outcome.
    bool become_invalidated(); // true if one controller is invalidated or if all controllers pointing to this are invalidated.
    virtual void kill_views() {}
    virtual void check_last_match_time(bool match) = 0;

    HLPController(r_code::View *view);
public:
    virtual ~HLPController();

    void invalidate();

    Code *get_core_object() const
    {
        return getObject(); // cst or mdl.
    }
    Code *get_unpacked_object() const   // the unpacked version of the core object.
    {
        Code *core_object = get_core_object();
        return core_object->get_reference(core_object->references_size() - MDL_HIDDEN_REFS);
    }

    void add_requirement(bool strong);
    void remove_requirement(bool strong);

    uint64_t get_requirement_count(uint64_t &weak_requirement_count, uint64_t &strong_requirement_count);
    uint64_t get_requirement_count();

    void store_evidence(_Fact *evidence)
    {
        _store_evidence<EEntry>(&evidences, evidence);
    }
    void store_predicted_evidence(_Fact *evidence)
    {
        _store_evidence <PEEntry>(&predicted_evidences, evidence);
    }

    virtual Fact *get_f_ihlp(HLPBindingMap *bindings, bool wr_enabled) const = 0;

    uint16_t get_out_group_count() const;
    Code *get_out_group(uint16_t i) const; // i starts at 1.
    inline Group *get_host() const
    {
        return (Group *)getView()->get_host();
    }
    bool has_tpl_args() const
    {
        return _has_tpl_args;
    }

    void inject_prediction(Fact *prediction, double confidence) const; // for simulated predictions.
};
}


#endif
