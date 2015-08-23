//	pgm_overlay.h
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

#ifndef pgm_overlay_h
#define pgm_overlay_h


#include <r_code/atom.h>       // for Atom
#include <r_code/list.h>       // for list
#include <r_exec/overlay.h>    // for Overlay
#include <stdint.h>            // for uint16_t, int16_t, uint64_t
#include <vector>              // for vector

#include <replicode_common.h>  // for P
#include <replicode_common.h>   // for REPLICODE_EXPORT

namespace r_code {
class Code;
class View;
}  // namespace r_code
namespace r_exec {
class View;
}  // namespace r_exec

namespace r_exec
{

// Overlays for input-less programs.
// Base class for other programs (with inputs, anti-programs).
class REPLICODE_EXPORT InputLessPGMOverlay:
    public Overlay
{
    friend class PGMController;
    friend class InputLessPGMController;
    friend class IPGMContext;
protected:
    std::vector<P<Code> > productions; // receives the results of ins, inj and eje; views are retrieved (fvw) or built (reduction) in the value array.

    bool evaluate(uint16_t index); // evaluates the pgm_code at the specified index.

    virtual Code *get_mk_rdx(uint16_t &extent_index) const;

    void patch_tpl_args(); // no views in tpl args; patches the ptn skeleton's first atom with IPGM_PTR with an index in the ipgm arg set; patches wildcards with similar IPGM_PTRs.
    void patch_tpl_code(uint16_t pgm_code_index, uint16_t ipgm_code_index); // to recurse.
    virtual void patch_input_code(uint16_t pgm_code_index, uint16_t input_index, uint16_t input_code_index, int16_t parent_index = -1); // defined in PGMOverlay.

    InputLessPGMOverlay();
    InputLessPGMOverlay(Controller *c);
public:
    virtual ~InputLessPGMOverlay();

    virtual void reset(); // reset to original state (pristine copy of the pgm code and empty value set).

    bool inject_productions(); // return true upon successful evaluation; no existence check in simulation mode.
};

// Overlay with inputs.
// Several ReductionCores can attempt to reduce the same overlay simultaneously (each with a different input).
class REPLICODE_EXPORT PGMOverlay:
    public InputLessPGMOverlay
{
    friend class PGMController;
    friend class IPGMContext;
private:
    bool is_volatile;
    uint64_t birth_time; // used for ipgms: overlays older than ipgm->tsc are killed; birth_time set to the time of the first match, 0 if no match occurred.
protected:
    r_code::list<uint16_t> input_pattern_indices; // stores the input patterns still waiting for a match: will be plucked upon each successful match.
    std::vector<P<r_code::View> > input_views; // copies of the inputs; vector updated at each successful match.

    typedef enum {
        SUCCESS = 0,
        FAILURE = 1,
        IMPOSSIBLE = 3 // when the input's class does not even match the object class in the pattern's skeleton.
    } MatchResult;

    MatchResult match(r_exec::View *input, uint16_t &input_index); // delegates to _match; input_index is set to the index of the pattern that matched the input.
    bool check_guards(); // return true upon successful evaluation.

    MatchResult _match(r_exec::View *input, uint16_t pattern_index); // delegates to __match.
    MatchResult __match(r_exec::View *input, uint16_t pattern_index); // return SUCCESS upon a successful match, IMPOSSIBLE if the input is not of the right class, FAILURE otherwise.

    Code *dereference_in_ptr(Atom a);
    void patch_input_code(uint16_t pgm_code_index, uint16_t input_index, uint16_t input_code_index, int16_t parent_index = -1);

    virtual Code *get_mk_rdx(uint16_t &extent_index) const;

    void init();

    PGMOverlay(Controller *c);
    PGMOverlay(PGMOverlay *original, uint16_t last_input_index, uint16_t value_commit_index); // copy from the original and rollback.
public:
    virtual ~PGMOverlay();

    void reset();

    virtual Overlay *reduce(r_exec::View *input); // called upon the processing of a reduction job.

    r_code::Code *getInputObject(uint16_t i) const;
    r_code::View *getInputView(uint16_t i) const;

    uint64_t get_birth_time() const
    {
        return birth_time;
    }

    bool is_invalidated();
};

// Several ReductionCores can attempt to reduce the same overlay simultaneously (each with a different input).
// In addition, ReductionCores and signalling jobs can attempt to inject productions concurrently.
// Usues the same mk.rdx as for InputLessPGMOverlays.
class REPLICODE_EXPORT AntiPGMOverlay:
    public PGMOverlay
{
    friend class AntiPGMController;
private:
    AntiPGMOverlay(Controller *c);
    AntiPGMOverlay(AntiPGMOverlay *original, uint16_t last_input_index, uint16_t value_limit);
public:
    ~AntiPGMOverlay();

    Overlay *reduce(r_exec::View *input); // called upon the processing of a reduction job.
};
}

#endif
