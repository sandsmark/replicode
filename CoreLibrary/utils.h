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
#include <stdio.h>

#include <iostream>
#include <string>

#if defined WINDOWS
#include <sys/timeb.h>
#include <time.h>
#elif defined LINUX
#include <dlfcn.h>
#include <errno.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <cstring>
#include <cstdlib>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <mutex>
#include <atomic>
// #undef HANDLE
// #define HANDLE pthread_cond_t*
#endif

#ifdef WINDOWS
#define SOCKETWOULDBLOCK WSAEWOULDBLOCK
#else
#define SOCKETWOULDBLOCK EWOULDBLOCK
#endif

#ifndef SD_BOTH
#define SD_BOTH 2
#endif

#define R250_LEN 250
#define R521_LEN 521

// Wrapping of OS-dependent functions
namespace core {

bool core_dll WaitForSocketReadability(socket s, int64 timeout);
bool core_dll WaitForSocketWriteability(socket s, int64 timeout);

class core_dll Error {
public:
    static int64 GetLastOSErrorNumber();
    static bool GetOSErrorMessage(char* buffer, uint64 buflen, int64 err = -1);
    static bool PrintLastOSErrorMessage(const char* title);
    static void PrintBinary(void* p, uint64 size, bool asInt, const char* title = NULL);
};

#if defined WINDOWS
#elif defined LINUX
struct SemaTex {
    pthread_mutex_t mutex;
    pthread_cond_t semaphore;
};
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

class core_dll Thread {
private:
    thread _thread;
    bool is_meaningful;
protected:
    Thread();
public:
    template<class T> static T *New(thread_function f, void *args);
    static void TerminateAndWait(Thread **threads, uint64 threadCount);
    static void TerminateAndWait(Thread *_thread);
    static void Wait(Thread **threads, uint64 threadCount);
    static void Wait(Thread *_thread);
    static void Sleep(int64 ms);
    static void Sleep(); // inifnite
    virtual ~Thread();
    void start(thread_function f);
    void suspend();
    void resume();
    void terminate();
};

class core_dll TimeProbe { // requires Time::Init()
private:
    int64 cpu_counts;
    int64 getCounts();
public:
    void set(); // initialize
    void check(); // gets the cpu count elapsed between set() and check()
    uint64 us(); // converts cpu counts in us
};

class core_dll Time { // TODO: make sure time stamps are consistent when computed by different cores
    friend class TimeProbe;
private:
    static float64 Period;
    static int64 InitTime;
public:
    static void Init(uint64 r); // detects the hardware timing capabilities; r: time resolution in us (on windows xp: max ~1000; use 1000, 2000, 5000 or 10000)
    static uint64 Get(); // in us since 01/01/1970

    static std::string ToString_seconds(uint64 t); // seconds:milliseconds:microseconds since 01/01/1970.
    static std::string ToString_year(uint64 t); // day_name day_number month year hour:minutes:seconds:milliseconds:microseconds GMT since 01/01/1970.
};

class core_dll Host {
public:
    typedef char host_name[255];
    static uint8 Name(char *name); // name size=255; return the actual size
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

class core_dll Timer {
private:
#if defined WINDOWS
    timer t;
#elif defined LINUX
    timer_t timer;
    struct SemaTex sematex;
#endif
protected:
    static const uint64 Infinite;
public:
    Timer();
    ~Timer();
    void start(uint64 deadline, uint64 period = 0); // deadline in us, period in ms.
    bool wait(uint64 timeout = Infinite); // timeout in ms; returns true if timedout.
    bool wait(uint64 &us, uint64 timeout = Infinite); // idem; updates the us actually spent.
};

class core_dll Event {
private:
#if defined WINDOWS
    event e;
#elif defined LINUX
// TODO.
#endif
public:
    Event();
    ~Event();
    void wait();
    void fire();
    void reset();
};

class core_dll SignalHandler {
public:
    static void Add(signal_handler h);
    static void Remove(signal_handler h);
};

uint8 core_dll BSR(word data); // BitScanReverse

class core_dll FastSemaphore: // lock-free under no contention
    public Semaphore {
private:
    std::atomic_int_fast64_t count; // minus the number of waiting threads
    const int64 maxCount; // max number of threads allowed to run
public:
    FastSemaphore(uint64 initialCount, uint64 maxCount); // initialCount >=0
    ~FastSemaphore();
    void acquire();
    void release();
};

class core_dll String {
public:
    static bool StartsWith(const std::string &s, const std::string &str);
    static bool EndsWith(const std::string &s, const std::string &str);
    static void MakeUpper(std::string &str);
    static void MakeLower(std::string &str);
    static void Trim(std::string& str, const char* chars2remove = " ");
    static void TrimLeft(std::string& str, const char* chars2remove = " ");
    static void TrimRight(std::string& str, const char* chars2remove = " ");
    static void ReplaceLeading(std::string& str, const char* chars2replace, char c);
    static std::string Int2String(int64 i);
    static std::string Uint2String(uint64 i);
};

static std::mutex s_debugSection;

/// Thread safe debug output
class DebugStream
{
public:
    inline DebugStream(std::string area) {
        s_debugSection.lock();
        std::cout << "\033[1;34m" << area << "\033[1;37m>\033[0m";
    }


    ~DebugStream() {
        std::cout << std::endl;
        s_debugSection.unlock();
    }

    inline DebugStream &operator<<(std::string output) { std::cout << " \033[1;32m" << output << "\033[0m"; return *this; }
    inline DebugStream &operator<<(uint64_t output) { *this << std::to_string(output); return *this; }
    inline DebugStream &operator<<(const char *output) { *this << std::string(output); return *this; }
};

} //namespace core

static inline core::DebugStream debug(std::string area) { return core::DebugStream(area); }

#include "utils.tpl.cpp"


#endif
