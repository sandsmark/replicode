//	object.h
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

#ifndef r_code_object_h
#define r_code_object_h

#include "atom.h"
#include "vector.h"
#include "list.h"
#include "replicode_defs.h"
#include "utils.h"

#include "CoreLibrary/base.h"
#include <unordered_set>

using namespace core;

namespace r_code {

// I/O from/to r_code::Image ////////////////////////////////////////////////////////////////////////

class dll_export ImageObject
{
public:
    r_code::vector<Atom> code;
    r_code::vector<uint32_t> references;
    virtual ~ImageObject() {}

    virtual void write(uint32_t *data) = 0;
    virtual void read(uint32_t *data) = 0;
    virtual void trace() = 0;
};

class View;

class dll_export SysView : public ImageObject
{
public:
    SysView();
    SysView(View *source);

    void write(uint32_t *data);
    void read(uint32_t *data);
    size_t get_size() const;
    void trace();
};

class Code;

#define UNDEFINED_OID 0xFFFFFFFF

class dll_export SysObject : public ImageObject
{
public:
    r_code::vector<uint32_t> markers; // indexes in the relocation segment
    r_code::vector<SysView *> views;

    uint32_t oid;

    SysObject();
    SysObject(Code *source);
    ~SysObject();

    void write(uint32_t *data);
    void read(uint32_t *data);
    size_t get_size();

    void trace();

private:
    static uint32_t LastOID;
};

// Interfaces for r_exec classes ////////////////////////////////////////////////////////////////////////

class Object;

class dll_export View : public _Object
{
public:
    /// does not include the viewed object; no smart pointer here (a view is held by a group and holds a ref to said group in references[0]).
    Code *references[2];
    /// viewed object.
    P<Code> object;

    View(): object(NULL) {

        references[0] = references[1] = NULL;
    }

    View(SysView *source, Code *object) {

        for (uint16_t i = 0; i < source->code.size(); ++i)
            _code[i] = source->code[i];
        references[0] = references[1] = NULL;
        this->object = object;
    }

    virtual ~View() {}

    Atom &code(uint16_t i) {
        return _code[i];
    }
    Atom code(uint16_t i) const {
        return _code[i];
    }

    typedef enum {
        SYNC_ONCE = 0,
        SYNC_PERIODIC = 1,
        SYNC_HOLD = 2,
        SYNC_AXIOM = 3,
        SYNC_ONCE_AXIOM = 4
    } SyncMode;

    SyncMode get_sync() const {
        return (SyncMode)(uint64_t)_code[VIEW_SYNC];
    }
    uint64_t get_ijt() const {
        return Utils::GetTimestamp(_code + _code[VIEW_IJT].asIndex());
    }
    void set_ijt(uint64_t ijt) {
        Utils::SetTimestamp(_code + _code[VIEW_IJT].asIndex(), ijt);
    }

    struct Hash {
        size_t operator()(View *v) const {
            return (size_t)(Code *)v->references[0]; // i.e. the group the view belongs to.
        }
    };

    struct Equal {
        bool operator()(const View *lhs, const View *rhs) const {
            return lhs->references[0] == rhs->references[0];
        }
    };

    struct Less {
        bool operator()(const View *lhs, const View *rhs) const {
            return lhs->get_ijt() < rhs->get_ijt();
        }
    };

protected:
    /// Dimensioned to hold the largest view (group view):
    /// head atom, iptr to ijt, sln, res, rptr to grp, rptr to org, vis, cov, 3 atoms for ijt's timestamp; oid is the last word32 (not an atom).
    Atom _code[VIEW_CODE_MAX_SIZE];
};

class dll_export Code:
    public _Object {
public:
    static const int64_t null_storage_index = -1;
    static const uint64_t CodeMarkersInitialSize = 8;
protected:
    int64_t storage_index; // -1: not sored; >0 index of the object in a vector-based container.

    void load(SysObject *source) {

        for (uint16_t i = 0; i < source->code.size(); ++i)
            code(i) = source->code[i];
        set_oid(source->oid);
    }
    template<class V> View *build_view(SysView *source) {

        return new V(source, this);
    }
public:
    void set_stroage_index(int64_t i) {
        storage_index = i;
    }
    bool is_registered() const {
        return storage_index > null_storage_index;
    }
    int64_t get_storage_index() const {
        return storage_index;
    }

    virtual uint64_t get_oid() const = 0;
    virtual void set_oid(uint64_t oid) = 0;

    virtual Atom &code(uint16_t i) = 0;
    virtual const Atom &code(uint16_t i) const = 0;
    virtual uint16_t code_size() const = 0;
    virtual void resize_code(uint16_t new_size) = 0;
    virtual void set_reference(uint16_t i, Code *object) = 0;
    virtual Code *get_reference(uint16_t i) const = 0;
    virtual uint16_t references_size() const = 0;
    virtual void clear_references() = 0;
    virtual void set_references(std::vector<P<Code> > &new_references) = 0;

    virtual bool is_compact() const {
        return false;
    }
    virtual bool is_invalidated() {
        return false;
    }
    virtual bool invalidate() {
        return false;
    }

    r_code::list<Code *> markers;
    std::unordered_set<View *, View::Hash, View::Equal> views; // indexed by groups.

    virtual View *build_view(SysView *source) = 0;

    virtual void acq_views() {}
    virtual void rel_views() {}
    virtual void acq_markers() {}
    virtual void rel_markers() {}

    virtual double get_psln_thr() {
        return 1;
    }

    Code(): storage_index(null_storage_index) {
        markers.reserve(CodeMarkersInitialSize);
    }
    virtual ~Code() {}

    virtual void mod(uint16_t member_index, float value) {};
    virtual void set(uint16_t member_index, float value) {};
    virtual View *get_view(Code *group, bool lock) {
        return NULL;
    }
    virtual void add_reference(Code *object) {} // called only on local objects.
    void remove_marker(Code *m) {

        acq_markers();
        markers.remove(m);
        rel_markers();
    }

    void trace() const {

        std::cout << "--------\n";
        for (uint16_t i = 0; i < code_size(); ++i) {

            std::cout << i << "\t";
            code(i).trace();
            std::cout << std::endl;
        }
        std::cout << "OID: " << get_oid() << std::endl;
    }
};

// Implementation for local objects (non distributed).
class dll_export LObject:
    public Code {
protected:
    uint64_t _oid;
    r_code::vector<Atom> _code;
    r_code::vector<P<Code> > _references;
public:
    LObject(): Code() {}
    LObject(SysObject *source): Code() {

        load(source);
    }
    virtual ~LObject() {}

    View *build_view(SysView *source) {

        return Code::build_view<View>(source);
    }

    uint64_t get_oid() const {
        return _oid;
    }
    void set_oid(uint64_t oid) {
        _oid = oid;
    }

    const Atom &code(uint16_t i) const {
        return _code[i];
    }
    Atom &code(uint16_t i) {
        return _code[i];
    }
    uint16_t code_size() const {
        return _code.size();
    }
    void resize_code(uint16_t new_size) {
        _code.as_std()->resize(new_size);
    }
    void set_reference(uint16_t i, Code *object) {
        _references[i] = object;
    }
    Code *get_reference(uint16_t i) const {
        return _references[i];
    }
    uint16_t references_size() const {
        return _references.size();
    }
    void clear_references() {
        _references.as_std()->clear();
    }
    void set_references(std::vector<P<Code> > &new_references) {
        (*_references.as_std()) = new_references;
    }
    void add_reference(Code *object) {
        _references.push_back(object);
    }
};

class dll_export Mem {
protected:
    static Mem *Singleton;
    Mem();
public:
    static Mem *Get();

    virtual Code *build_object(SysObject *source) const = 0;
    virtual void delete_object(Code *object) = 0;
};
}


#endif
