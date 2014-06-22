//	utils.cpp
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

#include "utils.h"
#include "object.h"

#include <math.h>
#include <string.h>
#include <CoreLibrary/debug.h>

namespace r_code {

uint64_t Utils::TimeReference = 0;
uint64_t Utils::BasePeriod = 0;
double Utils::FloatTolerance = 0;
uint64_t Utils::TimeTolerance = 0;

uint64_t Utils::GetTimeReference() {
    return TimeReference;
}
uint64_t Utils::GetBasePeriod() {
    return BasePeriod;
}
uint64_t Utils::GetFloatTolerance() {
    return FloatTolerance;
}
uint64_t Utils::GetTimeTolerance() {
    return TimeTolerance;
}

void Utils::SetReferenceValues(uint64_t base_period, double float_tolerance, double time_tolerance) {

    BasePeriod = base_period;
    FloatTolerance = float_tolerance;
    TimeTolerance = time_tolerance;
}

void Utils::SetTimeReference(uint64_t time_reference) {

    TimeReference = time_reference;
}

bool Utils::Equal(double l, double r) {

    if (l == r)
        return true;
    return fabs(l - r) < FloatTolerance;
}

bool Utils::Synchronous(uint64_t l, uint64_t r) {

    return uint64_t(abs((int64_t)(l - r))) < TimeTolerance;
}

uint64_t Utils::GetTimestamp(const Atom *iptr)
{
    return iptr[1].atom;
}

void Utils::SetTimestamp(Atom *iptr, uint64_t t) {

    iptr[0] = Atom::Timestamp();
    iptr[1].atom = t;
}

void Utils::SetTimestamp(Code *object, uint32_t index, uint64_t t) {

    object->code(index) = Atom::Timestamp();
    object->code(++index) = Atom(t);
}

std::string Utils::GetString(const Atom *iptr) {

    std::string s;
    char buffer[255];
    uint8_t char_count = (iptr[0].atom & 0x000000FF);
    memcpy(buffer, iptr + 1, char_count);
    buffer[char_count] = 0;
    s += buffer;
    return s;
}

void Utils::SetString(Atom *iptr, const std::string &s) {

    uint8_t l = (uint8_t)s.length();
    uint8_t index = 0;
    iptr[index] = Atom::String(l);
    uint64_t _st = 0;
    int8_t shift = 0;
    for (uint8_t i = 0; i < l; ++i) {

        _st |= s[i] << shift;
        shift += 8;
        if (shift == 64) {

            iptr[++index] = _st;
            _st = 0;
            shift = 0;
        }
    }
    if (l % 4)
        iptr[++index] = _st;
}

int64_t Utils::GetResilience(uint64_t now, uint64_t time_to_live, uint64_t upr) {

    if (time_to_live == 0 || upr == 0)
        return 1;
    uint64_t deadline = now + time_to_live;
    uint64_t last_upr = (now - TimeReference) / upr;
    uint64_t next_upr = (deadline - TimeReference) / upr;
    if ((deadline - TimeReference) % upr > 0)
        ++next_upr;
    return next_upr - last_upr;
}

int64_t Utils::GetGroupResilience(double resilience, double origin_upr, double destination_upr) {

    if (origin_upr == 0)
        return 1;
    if (destination_upr <= origin_upr)
        return 1;
    double r = origin_upr / destination_upr;
    double res = resilience * r;
    if (res < 1)
        return 1;
    return res;
}

std::string Utils::RelativeTime(uint64_t t)
{
    return DebugStream::timestamp(t - TimeReference);
}
}
