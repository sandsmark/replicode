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

#if defined(WIN32) || defined(WIN64)
typedef HINSTANCE shared_object;
#else
typedef void* shared_object;
#endif

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
    static std::string ToString_seconds(uint64_t t); // seconds:milliseconds:microseconds since 01/01/1970.
    static std::string ToString_year(uint64_t t); // day_name day_number month year hour:minutes:seconds:milliseconds:microseconds GMT since 01/01/1970.
};

} // namespace core

#include "utils.tpl.cpp"


#endif
