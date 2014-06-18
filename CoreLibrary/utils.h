//	utils.h
//
//	Author: Eric Nivel, Thor List
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

#ifndef core_utils_h
#define core_utils_h

#include "types.h"

#include <mutex>

// Wrapping of OS-dependent functions
namespace core {

class core_dll SharedLibrary {
private:
    shared_object library;
public:
    static SharedLibrary *New(const char *fileName);
    SharedLibrary();
    ~SharedLibrary();
    SharedLibrary *load(const char *fileName);
    template<typename T> T getFunction(const char *functionName);
};


class core_dll Time { // TODO: make sure time stamps are consistent when computed by different cores
public:
    static std::string ToString_seconds(uint64 t); // seconds:milliseconds:microseconds since 01/01/1970.
    static std::string ToString_year(uint64 t); // day_name day_number month year hour:minutes:seconds:milliseconds:microseconds GMT since 01/01/1970.
};

class core_dll Semaphore {
private:
    semaphore s;
protected:
    static const uint64 Infinite;
public:
    Semaphore(uint64 initialCount, uint64 maxCount);
    ~Semaphore();
    bool acquire(uint64 timeout = Infinite); // returns true if timedout
    void release(uint64 count = 1);
    void reset();
};


static std::mutex s_debugSection;
static bool s_debugEnabled = false;

/// Thread safe debug output
class DebugStream
{
public:
    inline DebugStream(std::string area) {
        if (!s_debugEnabled) return;

        s_debugSection.lock();
        std::cout << "\033[1;34m" << area << "\033[1;37m>\033[0m";
    }

    ~DebugStream() {
        if (!s_debugEnabled) return;
        std::cout << std::endl;
        s_debugSection.unlock();
    }

    inline DebugStream &operator<<(std::string output) {
        if (!s_debugEnabled) return *this;
        std::cout << " \033[1;32m" << output << "\033[0m"; return *this;
    }
    inline DebugStream &operator<<(uint64_t output) { *this << std::to_string(output); return *this; }
    inline DebugStream &operator<<(const char *output) { *this << std::string(output); return *this; }
};


} //namespace core

static inline core::DebugStream debug(std::string area) { return core::DebugStream(area); }

#include "utils.tpl.cpp"


#endif
