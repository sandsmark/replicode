//	pattern_extractor.h
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

#ifndef pattern_extractor_h
#define pattern_extractor_h




#include <r_code/list.h>         // for list
#include <r_exec/binding_map.h>  // for BindingMap
#include <r_exec/factory.h>      // for _Fact
#include <r_exec/view.h>         // for View
#include <stddef.h>              // for NULL
#include <stdint.h>              // for uint64_t, uint16_t
#include <string>                // for string
#include <vector>                // for vector

#include <replicode_common.h>    // for P, _Object
#include <replicode_common.h>     // for REPLICODE_EXPORT

namespace r_code {
class Code;
}  // namespace r_code
namespace r_exec {
class CSTController;
class GuardBuilder;
}  // namespace r_exec

namespace r_exec
{

class AutoFocusController;

class Input
{
public:
    P<BindingMap> bindings; // contains the values for the abstraction.
    P<_Fact> abstraction;
    P<_Fact> input;
    bool eligible_cause;
    uint64_t ijt; // injection time.

    Input(View *input, _Fact *abstraction, BindingMap *bindings): bindings(bindings), abstraction(abstraction), input(input->object), eligible_cause(IsEligibleCause(input)), ijt(input->get_ijt()) {}
    Input(): bindings(NULL), abstraction(NULL), input(NULL), eligible_cause(false), ijt(0) {}
    Input(const Input &original): bindings(original.bindings), abstraction(original.abstraction), input(original.input), eligible_cause(original.eligible_cause), ijt(original.ijt) {}

    static bool IsEligibleCause(r_exec::View *view);

    class IsInvalidated   // for storage in time_buffers.
    {
    public:
        bool operator()(Input &i, uint64_t time_reference, uint64_t thz) const
        {
            return (time_reference - i.ijt > thz);
        }
    };
};

class CInput   // cached inputs.
{
public:
    P<BindingMap> bindings; // contains the values for the abstraction.
    P<_Fact> abstraction;
    P<View> input;
    bool injected;
    uint64_t ijt; // injection time.
    CInput(View *input, _Fact *abstraction, BindingMap *bindings): bindings(bindings), abstraction(abstraction), input(input), injected(false), ijt(input->get_ijt()) {}
    CInput(): bindings(NULL), abstraction(NULL), input(NULL), injected(false), ijt(0) {}

    bool operator ==(const CInput &i) const
    {
        return input == i.input;
    }

    class IsInvalidated   // for storage in time_buffers.
    {
    public:
        bool operator()(CInput &i, uint64_t time_reference, uint64_t thz) const
        {
            return (time_reference - i.ijt > thz);
        }
    };
};

// Targeted Pattern eXtractor.
// Does nothing.
// Used for wr_enabled productions, for well-rated productions or when model acqusiiton is disabled.
class REPLICODE_EXPORT TPX:
    public _Object
{
protected:
    AutoFocusController *auto_focus;
    P<_Fact> target; // goal or prediction target, or premise (CTPX); abstraction: lhs of a mdl for goals, rhs for predictions, premise for CTPX.
    P<BindingMap> target_bindings;
    P<_Fact> abstracted_target;

    P<CSTController> cst_hook; // in case the target is an icst.

    std::vector<P<BindingMap> > new_maps; // acquired (in the case the target's bm is not fully specified) while matching the target's bm with inputs.

    bool filter(View *input, _Fact *abstracted_input, BindingMap *bm);

    TPX(AutoFocusController *auto_focus, _Fact *target);
public:
    TPX(AutoFocusController *auto_focus, _Fact *target, _Fact *pattern, BindingMap *bindings);
    virtual ~TPX();

    _Fact *get_pattern() const
    {
        return abstracted_target;
    }
    BindingMap *get_bindings() const
    {
        return target_bindings;
    }

    virtual bool take_input(View *view, _Fact *abstracted_input, BindingMap *bm);
    virtual void signal(View *input);
    virtual void ack_pred_success(_Fact *predicted_f);
};

class REPLICODE_EXPORT _TPX:
    public TPX
{
private:
    static const uint64_t InputsInitialSize = 16;
protected:
    class Component   // for building csts.
    {
    public:
        _Fact *object;
        bool discarded;
        Component() {}
        Component(_Fact *object): object(object), discarded(false) {}
    };

    r_code::list<Input> inputs; // time-controlled buffer (inputs older than tpx_time_horizon from now are discarded).
    std::vector<P<Code> > mdls; // new mdls.
    std::vector<P<Code> > csts; // new csts.
    std::vector<P<_Fact> > icsts; // new icsts.

    void filter_icst_components(ICST *icst, uint64_t icst_index, std::vector<Component> &components);
    _Fact *_find_f_icst(_Fact *component, uint16_t &component_index);
    _Fact *find_f_icst(_Fact *component, uint16_t &component_index);
    _Fact *find_f_icst(_Fact *component, uint16_t &component_index, Code *&cst);
    Code *build_cst(const std::vector<Component> &components, BindingMap *bm, _Fact *main_component);

    Code *build_mdl_head(HLPBindingMap *bm, uint16_t tpl_arg_count, _Fact *lhs, _Fact *rhs, uint16_t &write_index);
    void build_mdl_tail(Code *mdl, uint16_t write_index);

    void inject_hlps() const;
    void inject_hlps(uint64_t analysis_starting_time);

    virtual std::string get_header() const = 0;

    _TPX(AutoFocusController *auto_focus, _Fact *target, _Fact *pattern, BindingMap *bindings);
    _TPX(AutoFocusController *auto_focus, _Fact *target);
public:
    virtual ~_TPX();

    void debug(View *input) {};
};

// Pattern extractor targeted at goal successes.
// Possible causes are younger than the production of the goal.
// Models produced are of the form: M1[cause -> goal_target], where cause can be an imdl and goal_target can be an imdl.
// M1 does not have template arguments.
// Commands are ignored (CTPX' job).
class REPLICODE_EXPORT GTPX: // target is the goal target.
    public _TPX
{
private:
    P<Fact> f_imdl; // that produced the goal.

    std::vector<P<_Fact> > predictions; // successful predictions that may invalidate the need for model building.

    bool build_mdl(_Fact *cause, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period);
    bool build_mdl(_Fact *f_icst, _Fact *cause_pattern, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period, Code *new_cst);

    std::string get_header() const override;
public:
    GTPX(AutoFocusController *auto_focus, _Fact *target, _Fact *pattern, BindingMap *bindings, Fact *f_imdl);
    ~GTPX();

    bool take_input(View *input, _Fact *abstracted_input, BindingMap *bm) override;
    void signal(View *input) override;
    void ack_pred_success(_Fact *predicted_f) override;
    void reduce(View *input); // input is v->f->success(target,input) or v->|f->success(target,input).
};

// Pattern extractor targeted at prediciton failures.
// Possible causes are older than the production of the prediction.
// Models produced are of the form: M1[cause -> |imdl M0] where M0 is the model that produced the failed prediction and cause can be an imdl.
// M1 does not have template arguments.
// Commands are ignored (CTPX' job).
class REPLICODE_EXPORT PTPX: // target is the prediction.
    public _TPX
{
private:
    P<Fact> f_imdl; // that produced the prediction (and for which the PTPX will find strong requirements).

    bool build_mdl(_Fact *cause, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period);
    bool build_mdl(_Fact *f_icst, _Fact *cause_pattern, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period, Code *new_cst);

    std::string get_header() const override;
public:
    PTPX(AutoFocusController *auto_focus, _Fact *target, _Fact *pattern, BindingMap *bindings, Fact *f_imdl);
    ~PTPX();

    void signal(View *input) override;
    void reduce(View *input); // input is v->f->success(target,input) or v->|f->success(target,input).
};

// Pattern extractor targeted at changes of repeated input facts (SYMC_PERIODIC or SYNC_HOLD).
// Models produced are of the form: [premise -> [cause -> consequent]], i.e. M1:[premise -> imdl M0], M0:[cause -> consequent].
// M0 has template args, i.e the value of the premise and its after timestamp.
// N.B.: before-after=upr of the group the input comes from.
// The Consequent is a value different from the expected repetition of premise.
// The premise is an icst assembled from inputs synchronous with the input expected to repeat.
// Guards on values (not only on timings) are computed: this is the only TPX that does so.
// Inputs with SYNC_HOLD: I/O devices are expected to send changes on such inputs as soon as possible.
class CTPX:
    public _TPX
{
private:
    bool stored_premise;
    P<View> premise;

    GuardBuilder *get_default_guard_builder(_Fact *cause, _Fact *consequent, uint64_t period);
    GuardBuilder *find_guard_builder(_Fact *cause, _Fact *consequent, uint64_t period);

    bool build_mdl(_Fact *cause, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period);
    bool build_mdl(_Fact *f_icst, _Fact *cause_pattern, _Fact *consequent, GuardBuilder *guard_builder, uint64_t period);

    bool build_requirement(HLPBindingMap *bm, Code *m0, uint64_t period);

    std::string get_header() const override;
public:
    CTPX(AutoFocusController *auto_focus, View *premise);
    ~CTPX();

    void store_input(r_exec::View *input);
    void reduce(r_exec::View *input); // asynchronous: build models of value change if not aborted asynchronously by ASTControllers.
    void signal(r_exec::View *input) override; // spawns mdl/cst building (reduce()).
};
}


#endif
