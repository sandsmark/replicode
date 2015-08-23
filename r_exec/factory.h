//	factory.h
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

#ifndef factory_h
#define factory_h

#include <r_code/atom.h>         // for Atom
#include <r_exec/binding_map.h>  // for MatchResult
#include <r_exec/object.h>       // for LObject
#include <stddef.h>              // for size_t
#include <stdint.h>              // for uint64_t, uint16_t, int64_t
#include <vector>                // for vector

#include <replicode_common.h>    // for P, _Object
#include <replicode_common.h>     // for REPLICODE_EXPORT

namespace r_code {
class Code;
class Mem;
class SysObject;
}  // namespace r_code
namespace r_exec {
class Controller;
class Fact;
}  // namespace r_exec


namespace r_exec
{

// No instances of the following classes can transmited; for now: facts and icst will be.

// Notification markers are not put in their references marker sets.
// They are not used to propagate saliency changes.
// They are encoded as Atom::Object instead of Atom::Marker.

class REPLICODE_EXPORT MkNew:
    public LObject
{
public:
    MkNew(r_code::Mem *m, Code *object);
};

class REPLICODE_EXPORT MkLowRes:
    public LObject
{
public:
    MkLowRes(r_code::Mem *m, Code *object);
};

class REPLICODE_EXPORT MkLowSln:
    public LObject
{
public:
    MkLowSln(r_code::Mem *m, Code *object);
};

class REPLICODE_EXPORT MkHighSln:
    public LObject
{
public:
    MkHighSln(r_code::Mem *m, Code *object);
};

class REPLICODE_EXPORT MkLowAct:
    public LObject
{
public:
    MkLowAct(r_code::Mem *m, Code *object);
};

class REPLICODE_EXPORT MkHighAct:
    public LObject
{
public:
    MkHighAct(r_code::Mem *m, Code *object);
};

class REPLICODE_EXPORT MkSlnChg:
    public LObject
{
public:
    MkSlnChg(r_code::Mem *m, Code *object, double value);
};

class REPLICODE_EXPORT MkActChg:
    public LObject
{
public:
    MkActChg(r_code::Mem *m, Code *object, double value);
};

class Goal;
class Pred;

class REPLICODE_EXPORT _Fact:
    public LObject
{
private:
    static bool MatchAtom(r_code::Atom lhs, r_code::Atom rhs);
    static bool MatchStructure(const Code *lhs, uint16_t lhs_base_index, uint16_t lhs_index, const Code *rhs, uint16_t rhs_index);
    static bool Match(const Code *lhs, uint16_t lhs_base_index, uint16_t lhs_index, const Code *rhs, uint16_t rhs_index, uint16_t lhs_arity);
    static bool CounterEvidence(const Code *lhs, const Code *rhs);
protected:
    _Fact();
    _Fact(r_code::SysObject *source);
    _Fact(_Fact *f);
    _Fact(uint16_t opcode, Code *object, uint64_t after, uint64_t before, double confidence, double psln_thr);
public:
    static bool MatchObject(const Code *lhs, const Code *rhs);

    virtual bool is_invalidated();

    bool is_fact() const;
    bool is_anti_fact() const;
    void set_opposite();
    _Fact *get_absentee() const;

    bool match_timings_sync(const _Fact *evidence) const;
    bool match_timings_overlap(const _Fact *evidence) const;
    bool match_timings_inclusive(const _Fact *evidence) const;

    MatchResult is_evidence(const _Fact *target) const;
    MatchResult is_timeless_evidence(const _Fact *target) const;

    uint64_t get_after() const;
    uint64_t get_before() const;
    float get_cfd() const;

    void set_cfd(double cfd);

    Pred *get_pred() const;
    Goal *get_goal() const;

    void trace() const;
};

typedef enum {
    SIM_ROOT = 0,
    SIM_OPTIONAL = 1,
    SIM_MANDATORY = 2
} SimMode;

class REPLICODE_EXPORT Sim:
    public _Object
{
private:
    uint64_t volatile invalidated; // 32 bits alignment.
public:
    Sim();
    Sim(Sim *s); // is_requirement=false (not copied).
    Sim(SimMode mode, uint64_t thz, Fact *super_goal, bool opposite, Controller *root); // use for SIM_ROOT.
    Sim(SimMode mode, uint64_t thz, Fact *super_goal, bool opposite, Controller *root, Controller *sol, double sol_cfd, uint64_t sol_deadline); // USE for SIM_MANDATORY or SIM_OPTIONAL.

    void invalidate();
    bool is_invalidated();

    bool is_requirement;

    bool opposite; // of the goal the sim is attached to, i.e. the result of the match during controller->reduce(); the confidence is in the goal target.

    SimMode mode; // if SIM_MANDATORY or SIM_OPTIONAL: qualifies a sub-goal of the branch's root.
    uint64_t thz; // simulation time allowance (this is not the goal deadline); 0 indicates no time for simulation.
    P<Fact> super_goal; // of the goal the sim is attached to.
    P<Controller> root; // controller that produced the simulation branch root (SIM_ROOT): identifies the branch.
    P<Controller> sol; // controller that produced a sub-goal of the branch's root: identifies the model that can be a solution for the super-goal.
    double sol_cfd; // confidence of the solution goal.
    uint64_t sol_before; // deadline of the solution goal.
};

// Caveat: instances of Fact can becone instances of AntiFact (set_opposite() upon MATCH_SUCCESS_NEGATIVE during backward chaining).
// In particular, is_fact() and is_anti_fact() are based on the opcode, not on the class.
// Do not store any data in this class.
class REPLICODE_EXPORT Fact:
    public _Fact
{
public:
    void *operator new(size_t s);
    Fact();
    Fact(r_code::SysObject *source);
    Fact(Fact *f);
    Fact(Code *object, uint64_t after, uint64_t before, double confidence, double psln_thr);
};

// Caveat: as for Fact.
class REPLICODE_EXPORT AntiFact:
    public _Fact
{
public:
    void *operator new(size_t s);
    AntiFact();
    AntiFact(r_code::SysObject *source);
    AntiFact(AntiFact *f);
    AntiFact(Code *object, uint64_t after, uint64_t before, double confidence, double psln_thr);
};

// Goals and predictions:
// When positive evidences are found for a goal/prediction, said object is invalidated: in g-monitors and p-monitors, respectively.
// Idem for negative evidences In such cases, an absentee (absence of the expected fact) is injected.
//
// A negative evidence is either:
// (a) a fact that is either the absence of the target (absentee) or a fact asserting a state for the target which is not the expected one (it is assumed that an object can be in only one state WRT a given attribute; and recurse in icst) or,
// (b) a prediction holding a fact that is a counter evidence for the target with a confidence higher than said target.
//
// When a decision ground is invalidated, the subsequent is also invalidated: said invalidation is performed in the p-monitors and g-monitors.
// When a super-goal is invalidated (simulated or actual), sub-goals are also invalidated: said invalidation is performed in the g-monitors of the sub-goals.
//
// Invalidation checks are performed at both _take_input() time and reduce() time as the invalidation may occur during the transit in the pipe.

class REPLICODE_EXPORT Pred:
    public LObject
{
public:
    Pred();
    Pred(r_code::SysObject *source);
    Pred(_Fact *target, double psln_thr);

    bool is_invalidated();
    bool grounds_invalidated(_Fact *evidence);

    _Fact *get_target() const;

    std::vector<P<_Fact> > grounds; // f1->obj; predictions that were used to build this predictions (i.e. antecedents); empty if simulated.
    std::vector<P<Sim> > simulations;

    bool is_simulation() const;
    Sim *get_simulation(Controller *root) const; // return true if there is a simulation for the goal.
};

class REPLICODE_EXPORT Goal:
    public LObject
{
public:
    Goal();
    Goal(r_code::SysObject *source);
    Goal(_Fact *target, Code *actor, double psln_thr);

    bool invalidate();
    bool is_invalidated();
    bool ground_invalidated(_Fact *evidence);

    bool is_requirement() const;

    bool is_self_goal() const;
    bool is_drive() const;

    _Fact *get_target() const;
    _Fact *get_super_goal() const;
    Code *get_actor() const;

    P<Sim> sim;
    P<_Fact> ground; // f->p->f->imdl (weak requirement) that allowed backward chaining, if any.

    double get_strength(uint64_t now) const; // goal->target->cfd/(before-now).
};

class REPLICODE_EXPORT MkRdx:
    public LObject
{
public:
    MkRdx();
    MkRdx(r_code::SysObject *source);
    MkRdx(Code *imdl_fact, Code *input, Code *output, double psln_thr, BindingMap *binding_map); // for mdl.

    P<BindingMap> bindings; // NULL when produced by programs.
};

class REPLICODE_EXPORT Success:
    public LObject
{
public:
    Success();
    Success(_Fact *object, _Fact *evidence, double psln_thr);
};

class REPLICODE_EXPORT Perf:
    public LObject
{
public:
    Perf();
    Perf(uint64_t reduction_job_avg_latency, int64_t d_reduction_job_avg_latency, uint64_t time_job_avg_latency, int64_t d_time_job_avg_latency);
};

class REPLICODE_EXPORT ICST:
    public LObject
{
public:
    ICST();
    ICST(r_code::SysObject *source);

    bool is_invalidated();

    bool contains(_Fact *component, uint16_t &component_index) const;

    P<BindingMap> bindings;
    std::vector<P<_Fact> > components; // the inputs that triggered the building of the icst.
};
}


#endif
