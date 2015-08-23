//	hlp_overlay.h
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

#ifndef hlp_overlay_h
#define hlp_overlay_h


#include <r_code/list.h>         // for list
#include <r_exec/binding_map.h>  // for HLPBindingMap
#include <r_exec/overlay.h>      // for Overlay
#include <stdint.h>              // for uint16_t

#include <replicode_common.h>    // for P

namespace r_code {
class Atom;
class Code;
}  // namespace r_code
namespace r_exec {
class _Fact;
}  // namespace r_exec

namespace r_exec
{

// HLP: high-level patterns.
class HLPOverlay:
    public Overlay
{
    friend class HLPContext;
protected:
    P<HLPBindingMap> bindings;

    r_code::list<P<_Fact> > patterns;

    bool evaluate_guards(uint16_t guard_set_iptr_index);
    bool evaluate_fwd_guards();
    bool evaluate(uint16_t index);

    bool check_fwd_timings();

    bool scan_bwd_guards();
    bool scan_location(uint16_t index);
    bool scan_variable(uint16_t index);

    void store_evidence(_Fact *evidence, bool prediction, bool simulation); // stores both actual and non-simulated predicted evidences.

    HLPOverlay(Controller *c, HLPBindingMap *bindings);
public:
    static bool EvaluateBWDGuards(Controller *c, HLPBindingMap *bindings); // updates the bindings.
    static bool CheckFWDTimings(Controller *c, HLPBindingMap *bindings); // updates the bindings.
    static bool ScanBWDGuards(Controller *c, HLPBindingMap *bindings); // does not update the bindings.

    HLPOverlay(Controller *c, const HLPBindingMap *bindings, bool load_code);
    virtual ~HLPOverlay();

    HLPBindingMap *get_bindings() const
    {
        return bindings;
    }

    Atom *get_value_code(uint16_t id) const;
    uint16_t get_value_code_size(uint16_t id) const;

    Code *get_unpacked_object() const;

    bool evaluate_bwd_guards();
};
}


#endif
