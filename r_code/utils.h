//	utils.h
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

#ifndef r_code_utils_h
#define r_code_utils_h

#include <r_code/atom.h>       // for Atom
#include <stddef.h>            // for size_t, NULL
#include <stdint.h>            // for uint64_t, uint16_t, uint8_t, int64_t, etc
#include <iostream>            // for char_traits, ostream
#include <string>              // for string

#include "CoreLibrary/base.h"  // for P
#include "CoreLibrary/dll.h"   // for REPLICODE_EXPORT

using namespace core;

namespace r_code
{

// For use in STL containers.
template<class C> class PHash
{
public:
    size_t operator()(P<C> c) const
    {
        return (size_t)(C *)c;
    }
};

// Debugging facility.
class NullOStream:
    public std::ostream
{
public:
    NullOStream(): std::ostream(NULL) {}
    template<typename T> NullOStream& operator <<(T &t)
    {
        return *this;
    }
};

class Code;

class REPLICODE_EXPORT Utils
{
private:
    static uint64_t TimeReference; // starting time.
    static uint64_t BasePeriod;
    static double FloatTolerance;
    static uint64_t TimeTolerance;
public:
    static uint64_t GetTimeReference();
    static uint64_t GetBasePeriod();
    static uint64_t GetFloatTolerance();
    static uint64_t GetTimeTolerance();
    static void SetReferenceValues(uint64_t base_period, double float_tolerance, double time_tolerance);
    static void SetTimeReference(uint64_t time_reference);

    static bool Equal(double l, double r);
    static bool Synchronous(uint64_t l, uint64_t r);

    static uint64_t GetTimestamp(const Atom *iptr);
    static void SetTimestamp(Atom *iptr, uint64_t t);
    static void SetTimestamp(Code *object, uint16_t index, uint64_t t); // allocates atoms.

    static const uint64_t MaxTime = 0xFFFFFFFFFFFFFFFF;
    static const uint64_t MaxTHZ = 0xFFFFFFFF;

    template<class O> static uint64_t GetTimestamp(const O *object, uint16_t index)
    {
        uint16_t t_index = object->code(index).asIndex();
        uint64_t high = object->code(t_index + 1).atom;
        return high << 32 | object->code(t_index + 2).atom;
    }

    template<class O> static void SetIndirectTimestamp(O *object, uint16_t index, uint64_t t)
    {
        uint16_t t_index = object->code(index).asIndex();
        object->code(t_index) = Atom::Timestamp();
        object->code(t_index + 1).atom = t >> 32;
        object->code(t_index + 2).atom = t & 0x00000000FFFFFFFF;
    }

    static std::string GetString(const Atom *iptr);
    static void SetString(Atom *iptr, const std::string &s);

    template<class O> static std::string GetString(const O *object, uint16_t index)
    {
        uint16_t s_index = object->code(index).asIndex();
        char buffer[255];
        uint8_t char_count = (object->code(s_index).atom & 0x000000FF);
        buffer[char_count] = 0;

        for (int i = 0; i < char_count; i += 4) {
            uint64_t val = object->code(s_index + 1 + i / 4);
            buffer[i] = (val & 0x000000ff);
            buffer[i + 1] = (val & 0x0000ff00) >> 8;
            buffer[i + 2] = (val & 0x00ff0000) >> 16;
            buffer[i + 3] = (val & 0xff000000) >> 24;
        }

        return std::string(buffer);
    }

    template<class O> static void SetString(O *object, uint16_t index, const std::string &s)
    {
        uint16_t s_index = object->code(index).asIndex();
        uint8_t l = (uint8_t)s.length();
        object->code(s_index) = Atom::String(l);
        uint64_t _st = 0;
        int8_t shift = 0;

        for (uint8_t i = 0; i < l; ++i) {
            _st |= s[i] << shift;
            shift += 8;

            if (shift == 32) {
                object->code(++s_index) = _st;
                _st = 0;
                shift = 0;
            }
        }

        if (l % 4) {
            object->code(++s_index) = _st;
        }
    }

    static int64_t GetResilience(uint64_t now, uint64_t time_to_live, uint64_t upr); // ttl: us, upr: us.
    static int64_t GetGroupResilience(double resilience, double origin_upr, double destination_upr); // express the res in destination group, given the res in origin group.

    static std::string RelativeTime(uint64_t t);
};
}


#endif
