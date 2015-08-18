//	object.tpl.cpp
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

#include <mutex>

namespace r_exec
{

template<class C, class U> Object<C, U>::Object(): C(), hash_value(0), invalidated(0)
{
}

template<class C, class U> Object<C, U>::Object(r_code::Mem *mem): C(), hash_value(0), invalidated(0)
{
    this->set_oid(UNDEFINED_OID);
}

template<class C, class U> Object<C, U>::~Object()
{
    this->invalidate();
}

template<class C, class U> bool Object<C, U>::is_invalidated()
{
    return invalidated == 1;
}

template<class C, class U> bool Object<C, U>::invalidate()
{
    if (invalidated) {
        return true;
    }

    invalidated = 1; //std::cout<<std::dec<<get_oid()<<" invalidated\n";
    acq_views();
    this->views.clear();
    rel_views();

    if (this->code(0).getDescriptor() == r_code::Atom::MARKER) {
        for (uint16_t i = 0; i < this->references_size(); ++i) {
            this->get_reference(i)->remove_marker(this);
        }
    }

    if (this->is_registered()) {
        r_code::Mem::Get()->delete_object(this);
    }

    return false;
}

template<class C, class U> void Object<C, U>::compute_hash_value()
{
    hash_value = this->code(0).asOpcode() << 20; // 12 bits for the opcode.
    hash_value |= (this->code_size() & 0x00000FFF) << 8; // 12 bits for the code size.
    hash_value |= this->references_size() & 0x000000FF; // 8 bits for the reference set size.
}

template<class C, class U> double Object<C, U>::get_psln_thr()
{
    std::lock_guard<std::mutex> guard(m_pslnThrMutex);
    float r = this->code(this->code(0).getAtomCount()).asFloat(); // psln is always the last member of an object.
    return r;
}

template<class C, class U> void Object<C, U>::mod(uint16_t member_index, float value)
{
    if (member_index != this->code_size() - 1) {
        return;
    }

    float v = this->code(member_index).asFloat() + value;

    if (v < 0) {
        v = 0;
    } else if (v > 1) {
        v = 1;
    }

    std::lock_guard<std::mutex> guard(m_pslnThrMutex);
    this->code(member_index) = r_code::Atom::Float(v);
}

template<class C, class U> void Object<C, U>::set(uint16_t member_index, float value)
{
    if (member_index != this->code_size() - 1) {
        return;
    }

    std::lock_guard<std::mutex> guard(m_pslnThrMutex);
    this->code(member_index) = r_code::Atom::Float(value);
}

template<class C, class U> r_code::View *Object<C, U>::get_view(r_code::Code *group, bool lock)
{
    if (lock) {
        acq_views();
    }

    r_code::View probe;
    probe.references[0] = group;
    std::unordered_set<r_code::View *, r_code::View::Hash, r_code::View::Equal>::const_iterator v = this->views.find(&probe);

    if (v != this->views.end()) {
        if (lock) {
            rel_views();
        }

        return (r_exec::View *)*v;
    }

    if (lock) {
        rel_views();
    }

    return NULL;
}
}
