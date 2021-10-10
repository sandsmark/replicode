//	Author: Eric Nivel, Thor List, Martin Sandsmark
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel, Thor List
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel or Thor List nor the
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


#ifndef REPLICODE_COMMON_H
#define REPLICODE_COMMON_H

#include <atomic>        // for atomic_int_fast64_t
#include <mutex>
#include <iostream>
#include <stdint.h>

#if defined(WIN32) || defined(WIN64)
#define REPLICODE_EXPORT __declspec(dllexport)
#else
#define REPLICODE_EXPORT __attribute__((visibility("default")))
#endif

namespace core
{

#define R_LIKELY(x)      __builtin_expect(!!(x), 1)
#define R_UNLIKELY(x)    __builtin_expect(!!(x), 0)

// Root smart-pointable object class.
class _Object
{
    template<class C> friend class P;
    friend class _P;
protected:
    _Object() : refCount(0) {}
public:
    std::atomic_int_fast64_t refCount;
    virtual ~_Object() {}

private:
    void incRef() { ++refCount; }
    void decRef() {
        refCount--;
        if (refCount <= 0) {
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
public:
    _Object *object;

    inline P() : object(nullptr) {}
    inline P(C *o) : object(o)
    {
        if (object) {
            object->incRef();
        }
    }

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
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace core

static std::mutex s_debugSection;

/// Thread safe debug output
class DebugStream
{
public:
    inline DebugStream(const std::string area)
    {
        s_debugSection.lock();
#if COLOR_DEBUG
        std::cout << "\033[1;34m" << area << "\033[1;37m>\033[1;32m";
#else
        std::cout << area;
#endif
    }

    ~DebugStream()
    {
#if COLOR_DEBUG
        std::cout << "\033[0m" << std::endl;
#else
        std::cout << std::endl;
#endif
        s_debugSection.unlock();
    }

    static std::string timestamp(const uint64_t t)
    {
        uint64_t us = t % 1000;
        uint64_t ms = t / 1000;
        uint64_t s = ms / 1000;
        ms = ms % 1000;
        std::string _s = std::to_string(s);
        _s += "s:";
        _s += std::to_string(ms);
        _s += "ms:";
        _s += std::to_string(us);
        _s += "us";
        return _s;
    }

    inline const DebugStream &operator<<(const std::string output) const
    {
        std::cout << " " << output ;
        return *this;
    }
    inline const DebugStream &operator<<(const double output) const
    {
        std::cout << " " << output;
        return *this;
    }
    inline const DebugStream &operator<<(const int output) const
    {
        std::cout << " " << output;
        return *this;
    }
    inline const DebugStream &operator<<(const int64_t output) const
    {
        std::cout << " " << output;
        return *this;
    }
    inline const DebugStream &operator<<(const uint64_t output) const
    {
        std::cout << " " << output;
        return *this;
    }
    inline const DebugStream &operator<<(const uint32_t output) const
    {
        std::cout << " " << output;
        return *this;
    }
    inline const DebugStream &operator<<(const uint16_t output) const
    {
        std::cout << " " << output;
        return *this;
    }
    inline const DebugStream &operator<<(const uint8_t output) const
    {
        std::cout << " " << (uint64_t)output;
        return *this;
    }
    inline const DebugStream &operator<<(const char *output) const
    {
        std::cout << " " << output;
        return *this;
    }
    inline const DebugStream &operator<<(const void *output) const
    {
        std::cout << std::hex <<  " 0x" <<  (uint64_t)output << std::dec;
        return *this;
    }
    inline const DebugStream &operator<<(const bool output) const
    {
        std::cout << (output ? " true" : " false");
        return *this;
    }
};

static inline const DebugStream debug(const std::string area)
{
    return DebugStream(area);
}


#endif // REPLICODE_COMMON_H
