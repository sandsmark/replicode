//	binding_map.h
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

#ifndef binding_map_h
#define binding_map_h

#include <r_code/atom.h>       // for r_code::Atom
#include <stdint.h>            // for uint16_t, uint64_t, int16_t, uint8_t
#include <vector>              // for vector

#include <replicode_common.h>  // for P, _Object
#include <replicode_common.h>   // for REPLICODE_EXPORT

namespace r_code {
class Code;
}  // namespace r_code


namespace r_exec
{

class AtomValue;
class BindingMap;
class ObjectValue;
class StructureValue;

class REPLICODE_EXPORT Value:
    public core::_Object
{
protected:
    BindingMap *map;
    Value(BindingMap *map);
public:
    virtual Value *copy(BindingMap *map) const = 0;
    virtual void valuate(r_code::Code *destination, uint16_t write_index, uint16_t &extent_index) const = 0;
    virtual bool match(const r_code::Code *object, uint16_t index) = 0;
    virtual r_code::Atom *get_code() = 0;
    virtual r_code::Code *get_object() = 0;
    virtual uint16_t get_code_size() = 0;

    virtual bool intersect(const Value *v) const
    {
        return false;
    }
    virtual bool _intersect(const AtomValue *v) const
    {
        return false;
    }
    virtual bool _intersect(const StructureValue *v) const
    {
        return false;
    }
    virtual bool _intersect(const ObjectValue *v) const
    {
        return false;
    }

    virtual bool contains(const r_code::Atom a) const
    {
        return false;
    }
    virtual bool contains(const r_code::Atom *s) const
    {
        return false;
    }
    virtual bool contains(const r_code::Code *o) const
    {
        return false;
    }
};

class REPLICODE_EXPORT BoundValue:
    public Value
{
protected:
    BoundValue(BindingMap *map);
public:
};

class REPLICODE_EXPORT UnboundValue:
    public Value
{
private:
    uint8_t index;
public:
    UnboundValue(BindingMap *map, uint8_t index);
    ~UnboundValue();

    Value *copy(BindingMap *map) const;
    void valuate(r_code::Code *destination, uint16_t write_index, uint16_t &extent_index) const;
    bool match(const r_code::Code *object, uint16_t index);
    r_code::Atom *get_code();
    r_code::Code *get_object();
    uint16_t get_code_size();
};

class REPLICODE_EXPORT AtomValue:
    public BoundValue
{
private:
    r_code::Atom atom;
public:
    AtomValue(BindingMap *map, r_code::Atom atom);

    Value *copy(BindingMap *map) const;
    void valuate(r_code::Code *destination, uint16_t write_index, uint16_t &extent_index) const;
    bool match(const r_code::Code *object, uint16_t index);
    r_code::Atom *get_code();
    r_code::Code *get_object();
    uint16_t get_code_size();

    bool intersect(const Value *v) const;
    bool _intersect(const AtomValue *v) const;

    bool contains(const r_code::Atom a) const;
};

class REPLICODE_EXPORT StructureValue:
    public BoundValue
{
private:
    core::P<r_code::Code> structure;
    StructureValue(BindingMap *map, const r_code::Code *structure);
public:
    StructureValue(BindingMap *map, const r_code::Code *source, uint16_t structure_index);
    StructureValue(BindingMap *map, r_code::Atom *source, uint16_t structure_index);
    StructureValue(BindingMap *map, uint64_t time);

    Value *copy(BindingMap *map) const;
    void valuate(r_code::Code *destination, uint16_t write_index, uint16_t &extent_index) const;
    bool match(const r_code::Code *object, uint16_t index);
    r_code::Atom *get_code();
    r_code::Code *get_object();
    uint16_t get_code_size();

    bool intersect(const Value *v) const;
    bool _intersect(const StructureValue *v) const;

    bool contains(const r_code::Atom *s) const;
};

class REPLICODE_EXPORT ObjectValue:
    public BoundValue
{
private:
    const core::P<r_code::Code> object;
public:
    ObjectValue(BindingMap *map, r_code::Code *object);

    Value *copy(BindingMap *map) const;
    void valuate(r_code::Code *destination, uint16_t write_index, uint16_t &extent_index) const;
    bool match(const r_code::Code *object, uint16_t index);
    r_code::Atom *get_code();
    r_code::Code *get_object();
    uint16_t get_code_size();

    bool intersect(const Value *v) const;
    bool _intersect(const ObjectValue *v) const;

    bool contains(const r_code::Code *o) const;
};

typedef enum {
    MATCH_SUCCESS_POSITIVE = 0,
    MATCH_SUCCESS_NEGATIVE = 1,
    MATCH_FAILURE = 2
} MatchResult;

class Fact;
class _Fact;

class REPLICODE_EXPORT BindingMap:
    public core::_Object
{
    friend class UnboundValue;
protected:
    std::vector<core::P<Value> > map; // indexed by vl-ptrs.

    uint64_t unbound_values;

    void add_unbound_value(uint8_t id);

    uint16_t first_index; // index of the first value found in the first fact.
    int16_t fwd_after_index; // tpl args (if any) are located before fwd_after_index.
    int16_t fwd_before_index;

    bool match_timings(uint64_t stored_after, uint64_t stored_before, uint64_t after, uint64_t before, uint64_t destination_after_index, uint64_t destination_before_index);
    bool match_fwd_timings(const _Fact *f_object, const _Fact *f_pattern);
    bool match(const r_code::Code *object, uint16_t o_base_index, uint16_t o_index, const r_code::Code *pattern, uint16_t p_index, uint16_t o_arity);

    void abstract_member(r_code::Code *object, uint16_t index, r_code::Code *abstracted_object, uint16_t write_index, uint16_t &extent_index);
    r_code::Atom get_atom_variable(r_code::Atom a);
    r_code::Atom get_structure_variable(r_code::Code *object, uint16_t index);
    r_code::Atom get_object_variable(r_code::Code *object);
public:
    BindingMap();
    BindingMap(const BindingMap *source);
    BindingMap(const BindingMap &source);
    virtual ~BindingMap();

    BindingMap& operator =(const BindingMap &source);
    void load(const BindingMap *source);

    virtual void clear();

    void init(r_code::Code *object, uint16_t index);

    _Fact *abstract_f_ihlp(_Fact *fact) const; // for icst and imdl.
    _Fact *abstract_fact(_Fact *fact, _Fact *original, bool force_sync);
    r_code::Code *abstract_object(r_code::Code *object, bool force_sync);

    void reset_fwd_timings(_Fact *reference_fact); // reset after and before from the timings of the reference object.

    MatchResult match_fwd_lenient(const _Fact *f_object, const _Fact *f_pattern); // use for facts when we are lenient about fact vs |fact.
    bool match_fwd_strict(const _Fact *f_object, const _Fact *f_pattern); // use for facts when we need sharp match.

    uint64_t get_fwd_after() const; // assumes the timings are valuated.
    uint64_t get_fwd_before() const; // idem.

    bool match_object(const r_code::Code *object, const r_code::Code *pattern);
    bool match_structure(const r_code::Code *object, uint16_t o_base_index, uint16_t o_index, const r_code::Code *pattern, uint16_t p_index);
    bool match_atom(r_code::Atom o_atom, r_code::Atom p_atom);

    void bind_variable(BoundValue *value, uint8_t id);
    void bind_variable(r_code::Atom *code, uint8_t id, uint16_t value_index, r_code::Atom *intermediate_results);

    r_code::Atom *get_value_code(uint16_t id);
    uint16_t get_value_code_size(uint16_t id);

    bool intersect(BindingMap *bm);
    bool is_fully_specified() const;

    r_code::Atom *get_code(uint16_t i) const
    {
        return map[i]->get_code();
    }
    r_code::Code *get_object(uint16_t i) const
    {
        return map[i]->get_object();
    }
    uint16_t get_fwd_after_index() const
    {
        return fwd_after_index;
    }
    uint16_t get_fwd_before_index() const
    {
        return fwd_before_index;
    }
    bool scan_variable(uint16_t id) const; // return true if id<first_index or map[id] is not an UnboundValue.
};

class REPLICODE_EXPORT HLPBindingMap:
    public BindingMap
{
private:
    int16_t bwd_after_index;
    int16_t bwd_before_index;

    bool match_bwd_timings(const _Fact *f_object, const _Fact *f_pattern);

    bool need_binding(r_code::Code *pattern) const;
    void init_from_pattern(const r_code::Code *source, int16_t position); // first source is f->obj.
public:
    HLPBindingMap();
    HLPBindingMap(const HLPBindingMap *source);
    HLPBindingMap(const HLPBindingMap &source);
    ~HLPBindingMap();

    HLPBindingMap& operator =(const HLPBindingMap &source);
    void load(const HLPBindingMap *source);
    void clear();

    void init_from_hlp(const r_code::Code *hlp);
    void init_from_f_ihlp(const _Fact *f_ihlp);
    Fact *build_f_ihlp(r_code::Code *hlp, uint16_t opcode, bool wr_enabled) const; // return f->ihlp.
    r_code::Code *bind_pattern(r_code::Code *pattern) const;

    void reset_bwd_timings(_Fact *reference_fact); // idem for the last 2 unbound variables (i.e. timings of the second pattern in a mdl).

    MatchResult match_bwd_lenient(const _Fact *f_object, const _Fact *f_pattern); // use for facts when we are lenient about fact vs |fact.
    bool match_bwd_strict(const _Fact *f_object, const _Fact *f_pattern); // use for facts when we need sharp match.

    uint64_t get_bwd_after() const; // assumes the timings are valuated.
    uint64_t get_bwd_before() const; // idem.
};
}


#endif
