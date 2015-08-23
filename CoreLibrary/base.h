//	base.h
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

#ifndef core_base_h
#define core_base_h

#include <atomic>        // for atomic_int_fast64_t

#include "dll.h"         // for core_dll


namespace core
{

//class P;


// Root smart-pointable object class.
class _Object
{
    template<class C> friend class P;
    friend class _P;
protected:
    std::atomic_int_fast64_t refCount;
    _Object() : refCount(0) {}
public:
    virtual ~_Object() {}
    void incRef() { ++refCount; }
    virtual void decRef() {
        if (--refCount) {
            delete this;
        }
    }
};

// Smart pointer (ref counting, deallocates when ref count<=0).
// No circular refs (use std c++ ptrs).
// No passing in functions (cast P<C> into C*).
// Cannot be a value returned by a function (return C* instead).
template<class C> class P
{
private:
    _Object *object;
public:
    inline P() : object(nullptr) {}
    inline P(C *o) : object(o) {}
    inline P(const P<C> &p) : object(p.object)
    {
        if (object) {
            object->incRef();
        }
    }
    inline ~P() {
        if (object) {
            object->decRef();
        }
    }

    C *operator ->() const
    {
        return (C *)object;
    }

    template<class D> operator D *() const
    {
        return (D *)object;
    }

    bool operator ==(C *c) const
    {
        return object == c;
    }

    bool operator !=(C *c) const
    {
        return object != c;
    }

    bool operator !() const
    {
        return !object;
    }

    template<class D> bool operator ==(P<D> &p) const
    {
        return object == p.object;
    }

    template<class D> bool operator !=(P<D> &p) const
    {
        return object != p.object;
    }

    P<C> &operator =(C *c)
    {
        if (object == c) {
            return *this;
        }

        if (object) {
            object->decRef();
        }

        object = c;

        if (object) {
            object->incRef();
        }

        return *this;
    }

    P<C> &operator =(const P<C> &p)
    {
        return this->operator =((C *)p.object);
    }

    template<class D> P<C> &operator =(const P<D> &p)
    {
        return this->operator =((C *)p.object);
    }


    //bool operator <(C *c) const;
    //bool operator >(C *c) const;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


}



#endif
