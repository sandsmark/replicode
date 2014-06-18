//	utils.cpp
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

#include "utils.h"

#if defined WINDOWS
#include <intrin.h>
#pragma intrinsic (_InterlockedDecrement)
#pragma intrinsic (_InterlockedIncrement)
#pragma intrinsic (_InterlockedExchange)
#pragma intrinsic (_InterlockedExchange64)
#pragma intrinsic (_InterlockedCompareExchange)
#pragma intrinsic (_InterlockedCompareExchange64)
#elif defined LINUX
#ifdef DEBUG
#include <map>
#include <execinfo.h>
#endif // DEBUG
#endif //LINUX

#include <algorithm>
#include <cctype>
#include <chrono>
#include <sys/time.h>
#include <string.h>

namespace core {


#if defined LINUX
bool CalcTimeout(struct timespec &timeout, uint64 ms) {

    struct timeval now;
    if (gettimeofday(&now, NULL) != 0)
        return false;

    timeout.tv_sec = now.tv_sec + ms / 1000;
    long us = now.tv_usec + ms % 1000;
    if (us >= 1000000) {
        timeout.tv_sec++;
        us -= 1000000;
    }
    timeout.tv_nsec = us * 1000; // usec -> nsec
    return true;
}

#endif

SharedLibrary *SharedLibrary::New(const char *fileName) {

    SharedLibrary *sl = new SharedLibrary();
    if (sl->load(fileName))
        return sl;
    else {

        delete sl;
        return NULL;
    }
}

SharedLibrary::SharedLibrary(): library(NULL) {
}

SharedLibrary::~SharedLibrary() {
#if defined WINDOWS
    if (library)
        FreeLibrary(library);
#elif defined LINUX
    if (library)
        dlclose(library);
#endif
}

SharedLibrary *SharedLibrary::load(const char *fileName) {
#if defined WINDOWS
    library = LoadLibrary(TEXT(fileName));
    if (!library) {

        DWORD error = GetLastError();
        std::cerr << "> Error: unable to load shared library " << fileName << " :" << error << std::endl;
        return NULL;
    }
#elif defined LINUX
    /*
    * libraries on Linux are called 'lib<name>.so'
    * if the passed in fileName does not have those
    * components add them in.
    */
    char *libraryName = (char *)calloc(1, strlen(fileName) + 6 + 1);
    /*if (strstr(fileName, "lib") == NULL) {
    strcat(libraryName, "lib");
    }*/
    strcat(libraryName, fileName);
    if (strstr(fileName + (strlen(fileName) - 3), ".so") == NULL) {
        strcat(libraryName, ".so");
    }
    library = dlopen(libraryName, RTLD_NOW | RTLD_GLOBAL);
    if (!library) {
        std::cout << "> Error: unable to load shared library " << fileName << " :" << dlerror() << std::endl;
        free(libraryName);
        return NULL;
    }
    free(libraryName);
#endif
    return this;
}

////////////////////////////////////////////////////////////////////////////////////////////////


#if defined WINDOWS
typedef LONG NTSTATUS;
typedef NTSTATUS(__stdcall *NSTR)(ULONG, BOOLEAN, PULONG);
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
bool NtSetTimerResolution(IN ULONG RequestedResolution, IN BOOLEAN Set, OUT PULONG ActualResolution);
#elif defined LINUX
// TODO
#endif

std::string Time::ToString_seconds(uint64 t)
{
    uint64 us = t % 1000;
    uint64 ms = t / 1000;
    uint64 s = ms / 1000;
    ms = ms % 1000;

    std::string _s = std::to_string(s);
    _s += "s:";
    _s += std::to_string(ms);
    _s += "ms:";
    _s += std::to_string(us);
    _s += "us";

    return _s;
}

std::string Time::ToString_year(uint64 t)
{
    uint64 us = t % 1000;
    uint64 ms = t / 1000;
//uint64 s=ms/1000;
    ms = ms % 1000;

    time_t _gmt_time;
    time(&_gmt_time);
    struct tm *_t = gmtime(&_gmt_time);

    std::string _s = asctime(_t); // _s is: Www Mmm dd hh:mm:ss yyyy but we want: Www Mmm dd yyyy hh:mm:ss:msmsms:ususus
    std::string year = _s.substr(_s.length() - 5, 4);
    _s.erase(_s.length() - 6, 5);
    std::string hh_mm_ss = _s.substr(_s.length() - 9, 8);
    hh_mm_ss += ":";
    hh_mm_ss += std::to_string(ms);
    hh_mm_ss += ":";
    hh_mm_ss += std::to_string(us);

    _s.erase(_s.length() - 9, 9);
    _s += year;
    _s += " ";
    _s += hh_mm_ss;
    _s += " GMT";

    return _s;
}

////////////////////////////////////////////////////////////////////////////////////////////////

#if defined WINDOWS
const uint64 Semaphore::Infinite = INFINITE;
#elif defined LINUX
/*
 * Normally this should be SEM_VALUE_MAX but apparently the <semaphore.h> header
 * does not define it. The documents I have read indicate that on Linux it is
 * always equal to INT_MAX - so use that instead.
 */
const uint64 Semaphore::Infinite = INT_MAX;
#endif

Semaphore::Semaphore(uint64 initialCount, uint64 maxCount) {
#if defined WINDOWS
    s = CreateSemaphore(NULL, initialCount, maxCount, NULL);
#elif defined LINUX
    sem_init(&s, 0, initialCount);
#endif
}

Semaphore::~Semaphore() {
#if defined WINDOWS
    CloseHandle(s);
#elif defined LINUX
    sem_destroy(&s);
#endif
}

bool Semaphore::acquire(uint64 timeout) {
#if defined WINDOWS
    uint64 r = WaitForSingleObject(s, timeout);
    return r == WAIT_TIMEOUT;
#elif defined LINUX
    struct timespec t;
    int r;

    CalcTimeout(t, timeout);
    r = sem_timedwait(&s, &t);
    return r == 0;
#endif
}

void Semaphore::release(uint64 count) {
#if defined WINDOWS
    ReleaseSemaphore(s, count, NULL);
#elif defined LINUX
    for (uint64 c = 0; c < count; c++)
        sem_post(&s);
#endif
}

void Semaphore::reset() {
#if defined WINDOWS
    bool r;
    do
        r = acquire(0);
    while (!r);
#elif defined LINUX
    bool r;
    do
        r = acquire(0);
    while (!r);
#endif
}
}
