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
#include <ctime>


#define R250_IA (sizeof(uint64)*103)
#define R250_IB (sizeof(uint64)*R250_LEN-R250_IA)
#define R521_IA (sizeof(uint64)*168)
#define R521_IB (sizeof(uint64)*R521_LEN-R521_IA)

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

uint64 GetTime() {
    struct timeval tv;
    if (gettimeofday(&tv, NULL))
        return 0;
    return (tv.tv_usec + tv.tv_sec * 1000000LL);
}
#endif

void Error::PrintBinary(void* p, uint64 size, bool asInt, const char* title) {
    if (title != NULL)
        printf("--- %s %lu ---\n", title, size);
    unsigned char c;
    for (uint64 n = 0; n < size; n++) {
        c = *(((unsigned char*)p) + n);
        if (asInt)
            printf("[%u] ", (unsigned int)c);
        else
            printf("[%c] ", c);
        if ((n > 0) && ((n + 1) % 10 == 0))
            printf("\n");
    }
    printf("\n");
}

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

void Thread::TerminateAndWait(Thread **threads, uint64 threadCount) {
    if (!threads)
        return;
    for (uint64 i = 0; i < threadCount; i++) {
        threads[i]->terminate();
        Thread::Wait(threads[i]);
    }
}

void Thread::TerminateAndWait(Thread *_thread) {
    if (!_thread)
        return;
    _thread->terminate();
    Thread::Wait(_thread);
}

void Thread::Wait(Thread **threads, uint64 threadCount) {

    if (!threads)
        return;
#if defined WINDOWS
    for (uint64 i = 0; i < threadCount; i++)
        WaitForSingleObject(threads[i]->_thread, INFINITE);
#elif defined LINUX
    for (uint64 i = 0; i < threadCount; i++)
        pthread_join(threads[i]->_thread, NULL);
#endif
}

void Thread::Wait(Thread *_thread) {

    if (!_thread)
        return;
#if defined WINDOWS
    WaitForSingleObject(_thread->_thread, INFINITE);
#elif defined LINUX
    pthread_join(_thread->_thread, NULL);
#endif
}

void Thread::Sleep(int64 ms) {
#if defined WINDOWS
    ::Sleep((uint64)ms);
#elif defined LINUX
    struct timespec to_sleep = { ms / 1000, // seconds
               (ms % 1000) * 1000
    }; // nanoseconds
    while (nanosleep(&to_sleep, &to_sleep) && errno == EINTR) {} // we need to do this because signals (like timer interrupts) will make *sleep() calls return
#endif
}

void Thread::Sleep() {
#if defined WINDOWS
    ::Sleep(INFINITE);
#elif defined LINUX
    while (true)
        sleep(1000);
#endif
}

Thread::Thread(): is_meaningful(false) {
    _thread = 0;
}

Thread::~Thread() {
#if defined WINDOWS
// ExitThread(0);
    if (is_meaningful)
        CloseHandle(_thread);
#elif defined LINUX
// delete(_thread);
#endif
}

void Thread::start(thread_function f) {
#if defined WINDOWS
    _thread = CreateThread(NULL, 65536, f, this, 0, NULL); // 64KB: minimum initial stack size
#elif defined LINUX
    pthread_create(&_thread, NULL, f, this);
#endif
    is_meaningful = true;
}

void Thread::suspend() {
#if defined WINDOWS
    SuspendThread(_thread);
#elif defined LINUX
    pthread_kill(_thread, SIGSTOP);
#endif
}

void Thread::resume() {
#if defined WINDOWS
    ResumeThread(_thread);
#elif defined LINUX
    pthread_kill(_thread, SIGCONT);
#endif
}

void Thread::terminate() {
#if defined WINDOWS
    TerminateThread(_thread, 0);
#elif defined LINUX
    pthread_cancel(_thread);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////

void TimeProbe::set() {

    cpu_counts = getCounts();
}

int64 TimeProbe::getCounts() {
#if defined WINDOWS
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart;
#elif defined LINUX
    static struct timeval tv;
    static struct timezone tz;
    gettimeofday(&tv, &tz);
    return (((int64)tv.tv_sec) * 1000000) + (int64)tv.tv_usec;
#endif
}

void TimeProbe::check() {

    cpu_counts = getCounts() - cpu_counts;
}

uint64 TimeProbe::us() {

    return (uint64)(cpu_counts * Time::Period);
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

float64 Time::Period;

int64 Time::InitTime;

void Time::Init(uint64 r) {
#if defined WINDOWS
    NTSTATUS nts;
    HMODULE NTDll =::LoadLibrary("NTDLL");
    ULONG actualResolution = 0;
    if (NTDll) {

        NSTR pNSTR = (NSTR)::GetProcAddress(NTDll, "NtSetTimerResolution"); // undocumented win xp sys internals
        if (pNSTR)
            nts = (*pNSTR)(10 * r, true, &actualResolution); // in 100 ns units
    }
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    Period = 1000000.0 / f.QuadPart; // in us
    struct _timeb local_time;
    _ftime(&local_time);
    InitTime = (int64)(local_time.time * 1000 + local_time.millitm) * 1000; // in us
#elif defined LINUX
// we are actually setup a timer resolution of 1ms
// we can simulate this by performing a gettimeofday call
    struct timeval tv;
    gettimeofday(&tv, NULL);
    InitTime = (((int64)tv.tv_sec) * 1000000) + (int64)tv.tv_usec;
    Period = 1; // we measure all time in us anyway, so conversion is 1-to-1
#endif
}

uint64 Time::Get() {
#if defined WINDOWS
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (uint64)(InitTime + counter.QuadPart * Period);
#elif defined LINUX
    timeval perfCount;
    struct timezone tmzone;
    gettimeofday(&perfCount, &tmzone);
    int64 r = (((int64)perfCount.tv_sec) * 1000000) + (int64)perfCount.tv_usec;
    return r;
#endif
}

std::string Time::ToString_seconds(uint64 t) {

    uint64 us = t % 1000;
    uint64 ms = t / 1000;
    uint64 s = ms / 1000;
    ms = ms % 1000;

    std::string _s = String::Uint2String(s);
    _s += "s:";
    _s += String::Uint2String(ms);
    _s += "ms:";
    _s += String::Uint2String(us);
    _s += "us";

    return _s;
}

std::string Time::ToString_year(uint64 t) {

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
    hh_mm_ss += String::Uint2String(ms);
    hh_mm_ss += ":";
    hh_mm_ss += String::Uint2String(us);

    _s.erase(_s.length() - 9, 9);
    _s += year;
    _s += " ";
    _s += hh_mm_ss;
    _s += " GMT";

    return _s;
}

////////////////////////////////////////////////////////////////////////////////////////////////

uint8 Host::Name(char *name) {
#if defined WINDOWS
    DWORD s = 255;
    GetComputerName(name, &s);
    return (uint8)s;
#elif defined LINUX
    struct utsname utsname;
    uname(&utsname);
    strcpy(name, utsname.nodename);
    return strlen(name);
#endif
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

////////////////////////////////////////////////////////////////////////////////////////////////

#if defined WINDOWS
const uint64 Mutex::Infinite = INFINITE;
#elif defined LINUX
/*
 * Normally this should be SEM_VALUE_MAX but apparently the <semaphore.h> header
 * does not define it. The documents I have read indicate that on Linux it is
 * always equal to INT_MAX - so use that instead.
 */
const uint64 Mutex::Infinite = INT_MAX;
#endif

Mutex::Mutex() {
#if defined WINDOWS
    m = CreateMutex(NULL, false, NULL);
#elif defined LINUX
    pthread_mutex_init(&m, NULL);
#endif
}

Mutex::~Mutex() {
#if defined WINDOWS
    CloseHandle(m);
#elif defined LINUX
    pthread_mutex_destroy(&m);
#endif
}

bool Mutex::acquire(uint64 timeout) {
#if defined WINDOWS
    uint64 r = WaitForSingleObject(m, timeout);
    return r == WAIT_TIMEOUT;
#elif defined LINUX
    uint64_t start = Time::Get();
    uint64_t uTimeout = timeout * 1000;

    while (pthread_mutex_trylock(&m) != 0) {
        Thread::Sleep(10);
        if (Time::Get() - start >= uTimeout)
            return false;
    }
    return true;

#endif
}

void Mutex::release() {
#if defined WINDOWS
    ReleaseMutex(m);
#elif defined LINUX
    pthread_mutex_unlock(&m);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG
static pthread_mutex_t printerlock = PTHREAD_MUTEX_INITIALIZER;
#endif

CriticalSection::CriticalSection() {
#if defined WINDOWS
    InitializeCriticalSection(&cs);
#elif defined LINUX
#ifdef DEBUG
    pthread_mutexattr_t attr;
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&cs, &attr);

    memset(storedBt, 0, 10);
#else //DEBUG
    pthread_mutex_init(&cs, NULL);
#endif //DEBUG
#endif //LINUX
}

CriticalSection::~CriticalSection() {
#if defined WINDOWS
    DeleteCriticalSection(&cs);
#elif defined LINUX
    pthread_mutex_destroy(&cs);
#endif
}

void CriticalSection::enter() {
#if defined WINDOWS
    EnterCriticalSection(&cs);
#elif defined LINUX
#ifdef DEBUG
    struct timespec timeToWait;
    struct timeval now;
    gettimeofday(&now, NULL);
    timeToWait.tv_sec = now.tv_sec + 5;
    timeToWait.tv_nsec = (now.tv_usec + 1000UL * 5) * 1000UL;
    int ret = pthread_mutex_timedlock(&cs, &timeToWait);
    if (ret != 0) {
        pthread_mutex_lock(&printerlock);
        std::cerr << "--- POTENTIAL DEADLOCK: " << strerror(ret) << " LOCKED BY: " << std::endl;
        backtrace_symbols_fd(storedBt, 20, STDERR_FILENO);
        std::cerr << "--- NOW TRYING TO LOCK FROM ---" << std::endl;
        void *arr[20];
        backtrace(arr, 20);
        backtrace_symbols_fd(arr, 20, STDERR_FILENO);
        std::cerr << "--- BACKTRACE END ---" << std::endl;
        exit(0);
        pthread_mutex_unlock(&printerlock);
        pthread_mutex_lock(&cs);
    }

    pthread_mutex_lock(&printerlock);
    backtrace(storedBt, 20);
    pthread_mutex_unlock(&printerlock);
#else //DEBUG
    pthread_mutex_lock(&cs);
#endif //DEBUG
#endif
}

void CriticalSection::leave() {
#if defined WINDOWS
    LeaveCriticalSection(&cs);
#elif defined LINUX
#ifdef DEBUG
    pthread_mutex_lock(&printerlock);
    memset(storedBt, 0, 20);
    pthread_mutex_unlock(&printerlock);
#endif //DEBUG
    pthread_mutex_unlock(&cs);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////

#if defined WINDOWS
const uint64 Timer::Infinite = INFINITE;
#elif defined LINUX
const uint64 Timer::Infinite = INT_MAX;

static void timer_signal_handler(int sig, siginfo_t *siginfo, void *context) {
    SemaTex* sematex = (SemaTex*) siginfo->si_value.sival_ptr;
    if (sematex == NULL)
        return;
    pthread_mutex_lock(&sematex->mutex);
    pthread_cond_broadcast(&sematex->semaphore);
    pthread_mutex_unlock(&sematex->mutex);
}
#endif

Timer::Timer() {
#if defined WINDOWS
    t = CreateWaitableTimer(NULL, false, NULL);
    if (t == NULL) {
        printf("Error creating timer\n");
    }
#elif defined LINUX
    pthread_cond_init(&sematex.semaphore, NULL);
    pthread_mutex_init(&sematex.mutex, NULL);

    struct sigaction sa;
    struct sigevent timer_event;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO; /* Real-Time signal */
    sa.sa_sigaction = timer_signal_handler;
    sigaction(SIGRTMIN, &sa, NULL);

    memset(&timer_event, 0, sizeof(sigevent));
    timer_event.sigev_notify = SIGEV_SIGNAL;
    timer_event.sigev_signo = SIGRTMIN;
    timer_event.sigev_value.sival_ptr = (void *)&sematex;
    int ret = timer_create(CLOCK_REALTIME, &timer_event, &timer);
    if (ret != 0) {
        printf("Error creating timer: %d\n", ret);
    }
#endif
}

Timer::~Timer() {
#if defined WINDOWS
    CloseHandle(t);
#elif defined LINUX
    pthread_cond_destroy(&sematex.semaphore);
    pthread_mutex_destroy(&sematex.mutex);
    timer_delete(timer);
#endif
}

void Timer::start(uint64 deadline, uint64 period) {
#if defined WINDOWS
    LARGE_INTEGER _deadline; // in 100 ns intervals
    _deadline.QuadPart = -10LL * deadline; // negative means relative
    bool r = SetWaitableTimer(t, &_deadline, (long)period, NULL, NULL, 0);
    if (!r) {
        printf("Error arming timer\n");
    }
#elif defined LINUX
    struct itimerspec newtv;
    sigset_t allsigs;

    uint64 t = deadline;
    uint64 p = period * 1000;
    newtv.it_interval.tv_sec = p / 1000000;
    newtv.it_interval.tv_nsec = (p % 1000000) * 1000;
    newtv.it_value.tv_sec = t / 1000000;
    newtv.it_value.tv_nsec = (t % 1000000) * 1000;

    pthread_mutex_lock(&sematex.mutex);

    int ret = timer_settime(timer, 0, &newtv, NULL);
    if (ret != 0) {
        printf("Error arming timer: %d\n", ret);
    }
    sigemptyset(&allsigs);

    pthread_mutex_unlock(&sematex.mutex);
#endif
}

bool Timer::wait(uint64 timeout) {
#if defined WINDOWS
    uint64 r = WaitForSingleObject(t, timeout);
    return r == WAIT_TIMEOUT;
#elif defined LINUX
    bool res;
    struct timespec ttimeout;

    pthread_mutex_lock(&sematex.mutex);
    if (timeout == INT_MAX) {
        res = (pthread_cond_wait(&sematex.semaphore, &sematex.mutex) == 0);
    }
    else {
        CalcTimeout(ttimeout, timeout);
        res = (pthread_cond_timedwait(&sematex.semaphore, &sematex.mutex, &ttimeout) == 0);
    }
    pthread_mutex_unlock(&sematex.mutex);
    return res;
#endif
}

bool Timer::wait(uint64 &us, uint64 timeout) {

    TimeProbe probe;
    probe.set();
    bool r = wait((uint64)timeout);
    probe.check();
    us = probe.us();
    return r;
}

////////////////////////////////////////////////////////////////////////////////////////////////

Event::Event() {
#if defined WINDOWS
    e = CreateEvent(NULL, true, false, NULL);
#elif defined LINUX
// TODO.
#endif
}

Event::~Event() {
#if defined WINDOWS
    CloseHandle(e);
#elif defined LINUX
// TODO.
#endif
}

void Event::wait() {
#if defined WINDOWS
    WaitForSingleObject(e, INFINITE);
#elif defined LINUX
// TODO.
#endif
}

void Event::fire() {
#if defined WINDOWS
    SetEvent(e);
#elif defined LINUX
// TODO.
#endif
}

void Event::reset() {
#if defined WINDOWS
    ResetEvent(e);
#elif defined LINUX
// TODO.
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////

void SignalHandler::Add(signal_handler h) {
#if defined WINDOWS
    if (SetConsoleCtrlHandler(h, true) == 0) {

        int e = GetLastError();
        std::cerr << "Error: " << e << " failed to add signal handler" << std::endl;
        return;
    }
#elif defined LINUX
    signal(SIGABRT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGBUS, SIG_IGN);
// signal(SIGHUP, h);
    signal(SIGTERM, h);
    signal(SIGINT, h);
    signal(SIGABRT, h);
// signal(SIGFPE, h);
// signal(SIGILL, h);
// signal(SIGSEGV, h);
#endif
}

void SignalHandler::Remove(signal_handler h) {
#if defined WINDOWS
    SetConsoleCtrlHandler(h, false);
#elif defined LINUX
    signal(SIGABRT, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGBUS, SIG_IGN);
// signal(SIGHUP, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
    signal(SIGINT, SIG_DFL);
    signal(SIGABRT, SIG_DFL);
// signal(SIGFPE, SIG_DFL);
// signal(SIGILL, SIG_DFL);
// signal(SIGSEGV, SIG_DFL);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////

int64 Atomic::Increment32(int64 volatile *v) {
#if defined WINDOWS
    return InterlockedIncrement((long*)v);
#elif defined LINUX
    __sync_add_and_fetch(v, 1);
    return *v;
#endif
}

int64 Atomic::Decrement32(int64 volatile *v) {
#if defined WINDOWS
    return InterlockedDecrement((long*)v);
#elif defined LINUX
    __sync_add_and_fetch(v, -1);
    return *v;
#endif
}

int64 Atomic::CompareAndSwap32(int64 volatile *target, int64 v1, int64 v2) {
#if defined WINDOWS
    return _InterlockedCompareExchange((long*)target, v2, v1);
#elif defined LINUX
// note that v1 and v2 are swapped for Linux!!!
    return __sync_val_compare_and_swap(target, v1, v2);
#endif
}

int64 Atomic::CompareAndSwap64(int64 volatile *target, int64 v1, int64 v2) {
#if defined WINDOWS
    return _InterlockedCompareExchange64(target, v2, v1);
#elif defined LINUX
// note that v1 and v2 are swapped for Linux!!!
    return __sync_val_compare_and_swap(target, v1, v2);
#endif
}

// word Atomic::CompareAndSwap(word volatile *target,word v1,word v2){
//#if defined ARCH_32
// return CompareAndSwap32(target,v1,v2);
//#elif defined ARCH_64
// return CompareAndSwap32((uint64*)target,v1,v2);
//#endif
// }

int64 Atomic::Swap32(int64 volatile *target, int64 v) {
#if defined WINDOWS
    return _InterlockedExchange((long*)target, v);
#elif defined LINUX
    return __sync_fetch_and_sub(target, v);
#endif
}

int64 Atomic::Swap64(int64 volatile *target, int64 v) {
#if defined WINDOWS
    return CompareAndSwap64(target, v, v);
#elif defined LINUX
    return __sync_fetch_and_sub(target, v);
#endif
}

// word Atomic::Swap(word volatile *target,word v){
//#if defined ARCH_32
// return Swap32(target,v);
//#elif defined ARCH_64
// return Swap32((uint64*)target,v);
//#endif
// }

////////////////////////////////////////////////////////////////////////////////////////////////

uint8 BSR(word data) {
#if defined WINDOWS
#if defined ARCH_32
    DWORD index;
    _BitScanReverse(&index, data);
    return (uint8)index;
#elif defined ARCH_64
    uint64 index;
    _BitScanReverse64(&index, data);
    return (uint8)index;
#endif
#elif defined LINUX
#if defined ARCH_32
    return (uint8)(31 - __builtin_clz((uint64_t)data));
#elif defined ARCH_64
    return (uint8)(63 - __builtin_clzll((uint64_t)data));
#endif
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////

FastSemaphore::FastSemaphore(uint64 initialCount, uint64 maxCount): Semaphore(initialCount > 0 ? 1 : 0, 1), count(initialCount), maxCount(maxCount) {
}

FastSemaphore::~FastSemaphore() {
}

void FastSemaphore::acquire() {

    int64 c;
    while ((c = Atomic::Decrement32(&count)) >= maxCount); // release calls can bring count over maxCount: acquire has to exhaust these extras
    if (c < 0)
        Semaphore::acquire();
}

void FastSemaphore::release() {

    int64 c = Atomic::Increment32(&count);
    if (c <= 0)
        Semaphore::release();
}

////////////////////////////////////////////////////////////////////////////////////////////////
/*
 FastMutex::FastMutex(uint64 initialCount):Semaphore(initialCount,1),count(initialCount){
 }

 FastMutex::~FastMutex(){
 }

 void FastMutex::acquire(){

 int64 former=Atomic::Swap32(&count,0);
 if(former==0)
 Semaphore::acquire();
 }

 void FastMutex::release(){

 int64 former=Atomic::Swap32(&count,1);
 if(former==0)
 Semaphore::release();
 }
 */
////////////////////////////////////////////////////////////////////////////////////////////////

bool Error::PrintLastOSErrorMessage(const char* title) {
    int64 err = Error::GetLastOSErrorNumber();
    char buf[1024];
    if (!Error::GetOSErrorMessage(buf, 1024, err))
        printf("%s: [%lu] (could not get error message)\n", title, err);
    else
        printf("%s: [%lu] %s\n", title, err, buf);
    return true;
}

int64 Error::GetLastOSErrorNumber() {
#ifdef WINDOWS
    int64 err = WSAGetLastError();
    WSASetLastError(0);
    return err;
#else
    return (int64) errno;
#endif
}

bool Error::GetOSErrorMessage(char* buffer, uint64 buflen, int64 err) {
    if (buffer == NULL)
        return false;
    if (buflen < 512) {
        strcpy(buffer, "String buffer not large enough");
        return false;
    }
    if (err < 0)
        err = Error::GetLastOSErrorNumber();

#ifdef WINDOWS
    if (err == WSANOTINITIALISED) {
        strcpy(buffer, "Cannot initialize WinSock!");
    }
    else if (err == WSAENETDOWN) {
        strcpy(buffer, "The network subsystem or the associated service provider has failed");
    }
    else if (err == WSAEAFNOSUPPORT) {
        strcpy(buffer, "The specified address family is not supported");
    }
    else if (err == WSAEINPROGRESS) {
        strcpy(buffer, "A blocking Windows Sockets 1.1 call is in progress, or the service provider is still processing a callback function");
    }
    else if (err == WSAEMFILE) {
        strcpy(buffer, "No more socket descriptors are available");
    }
    else if (err == WSAENOBUFS) {
        strcpy(buffer, "No buffer space is available. The socket cannot be created");
    }
    else if (err == WSAEPROTONOSUPPORT) {
        strcpy(buffer, "The specified protocol is not supported");
    }
    else if (err == WSAEPROTOTYPE) {
        strcpy(buffer, "The specified protocol is the wrong type for this socket");
    }
    else if (err == WSAESOCKTNOSUPPORT) {
        strcpy(buffer, "The specified socket type is not supported in this address family");
    }
    else if (err == WSAEADDRINUSE) {
        strcpy(buffer, "The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR. This error usually occurs during execution of the bind function, but could be delayed until this function if the bind was to a partially wildcard address (involving ADDR_ANY) and if a specific address needs to be committed at the time of this function");
    }
    else if (err == WSAEINVAL) {
        strcpy(buffer, "The socket has not been bound with bind");
    }
    else if (err == WSAEISCONN) {
        strcpy(buffer, "The socket is already connected");
    }
    else if (err == WSAENOTSOCK) {
        strcpy(buffer, "The descriptor is not a socket");
    }
    else if (err == WSAEOPNOTSUPP) {
        strcpy(buffer, "The referenced socket is not of a type that supports the listen operation");
    }
    else if (err == WSAEADDRNOTAVAIL) {
        strcpy(buffer, "The specified address is not a valid address for this machine");
    }
    else if (err == WSAEFAULT) {
        strcpy(buffer, "The name or namelen parameter is not a valid part of the user address space, the namelen parameter is too small, the name parameter contains an incorrect address format for the associated address family, or the first two bytes of the memory block specified by name does not match the address family associated with the socket descriptor s");
    }
    else if (err == WSAEMFILE) {
        strcpy(buffer, "The queue is nonempty upon entry to accept and there are no descriptors available");
    }
    else if (err == SOCKETWOULDBLOCK) {
        strcpy(buffer, "The socket is marked as nonblocking and no connections are present to be accepted");
    }
    else if (err == WSAETIMEDOUT) {
        strcpy(buffer, "Attempt to connect timed out without establishing a connection");
    }
    else if (err == WSAENETUNREACH) {
        strcpy(buffer, "The network cannot be reached from this host at this time");
    }
    else if (err == WSAEISCONN) {
        strcpy(buffer, "The socket is already connected (connection-oriented sockets only)");
    }
    else if (err == WSAECONNREFUSED) {
        strcpy(buffer, "The attempt to connect was forcefully rejected");
    }
    else if (err == WSAEAFNOSUPPORT) {
        strcpy(buffer, "Addresses in the specified family cannot be used with this socket");
    }
    else if (err == WSAEADDRNOTAVAIL) {
        strcpy(buffer, "The remote address is not a valid address (such as ADDR_ANY)");
    }
    else if (err == WSAEALREADY) {
        strcpy(buffer, "A nonblocking connect call is in progress on the specified socket");
    }
    else if (err == WSAECONNRESET) {
        strcpy(buffer, "Connection was reset");
    }
    else if (err == WSAECONNABORTED) {
        strcpy(buffer, "Software caused connection abort");
    }
    else {
        strcpy(buffer, "Socket error with no description");
    }

#else
    strcpy(buffer, strerror(err));
#endif

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool WaitForSocketReadability(socket s, int64 timeout) {

    int maxfd = 0;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd_set rdds;
// create a list of sockets to check for activity
    FD_ZERO(&rdds);
// specify mySocket
    FD_SET(s, &rdds);

#ifdef WINDOWS
#else
    maxfd = s + 1;
#endif

    if (timeout > 0) {
        ldiv_t d = ldiv(timeout * 1000, 1000000);
        tv.tv_sec = d.quot;
        tv.tv_usec = d.rem;
    }

// Check for readability
    int ret = select(maxfd, &rdds, NULL, NULL, &tv);
    return (ret > 0);
}

bool WaitForSocketWriteability(socket s, int64 timeout) {

    int maxfd = 0;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    fd_set wds;
// create a list of sockets to check for activity
    FD_ZERO(&wds);
// specify mySocket
    FD_SET(s, &wds);

#ifdef WINDOWS
#else
    maxfd = s + 1;
#endif

    if (timeout > 0) {
        ldiv_t d = ldiv(timeout * 1000, 1000000);
        tv.tv_sec = d.quot;
        tv.tv_usec = d.rem;
    }

// Check for readability
    return (select(maxfd, NULL, &wds, NULL, &tv) > 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool String::StartsWith(const std::string &s, const std::string &str) {
    if (str.size() > s.size()) return false;
    return (s.compare(0, str.length(), str) == 0);
}

bool String::EndsWith(const std::string &s, const std::string &str) {
    if (str.size() > s.size()) return false;
    return (s.compare(s.size() - str.size(), str.size(), str) == 0);
}

void String::MakeUpper(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), toupper);
}

void String::MakeLower(std::string &str)
{
    std::transform(str.begin(), str.end(), str.begin(), tolower);
}

void String::Trim(std::string& str, const char* chars2remove)
{
    TrimLeft(str, chars2remove);
    TrimRight(str, chars2remove);
}

void String::TrimLeft(std::string& str, const char* chars2remove)
{
    if (!str.empty())
    {
        std::string::size_type pos = str.find_first_not_of(chars2remove);

        if (pos != std::string::npos)
            str.erase(0, pos);
        else
            str.erase(str.begin() , str.end()); // make empty
    }
}

void String::TrimRight(std::string& str, const char* chars2remove)
{
    if (!str.empty())
    {
        std::string::size_type pos = str.find_last_not_of(chars2remove);

        if (pos != std::string::npos)
            str.erase(pos + 1);
        else
            str.erase(str.begin() , str.end()); // make empty
    }
}


void String::ReplaceLeading(std::string& str, const char* chars2replace, char c)
{
    if (!str.empty())
    {
        std::string::size_type pos = str.find_first_not_of(chars2replace);

        if (pos != std::string::npos)
            str.replace(0, pos, pos, c);
        else
        {
            int n = str.size();
            str.replace(str.begin(), str.end() - 1, n - 1, c);
        }
    }
}

std::string String::Int2String(int64 i) {
    char buffer[1024];
    sprintf(buffer, "%ld", i);
    return std::string(buffer);
}

std::string String::Uint2String(uint64 i) {
    char buffer[1024];
    sprintf(buffer, "%lu", i);
    return std::string(buffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////

int64 Random::r250_index;
int64 Random::r521_index;
uint64 Random::r250_buffer[R250_LEN];
uint64 Random::r521_buffer[R521_LEN];

void Random::Init() {

    int64 i = R521_LEN;
    uint64 mask1 = 1;
    uint64 mask2 = 0xFFFFFFFF;

    while (i-- > R250_LEN)
        r521_buffer[i] = rand();
    while (i-- > 31) {

        r250_buffer[i] = rand();
        r521_buffer[i] = rand();
    }

// Establish linear independence of the bit columns
// by setting the diagonal bits and clearing all bits above
    while (i-- > 0) {

        r250_buffer[i] = (rand() | mask1) & mask2;
        r521_buffer[i] = (rand() | mask1) & mask2;
        mask2 ^= mask1;
        mask1 >>= 1;
    }
    r250_buffer[0] = mask1;
    r521_buffer[0] = mask2;
    r250_index = 0;
    r521_index = 0;
}

double Random::operator()(uint64 range) {
    /*
    I prescale the indices by sizeof(unsigned long) to eliminate
    four shlwi instructions in the compiled code. This minor optimization
    increased perf by about 12%.

    I also carefully arrange index increments and comparisons to minimize
    instructions. gcc 3.3 seems a bit weak on instruction reordering. The
    j1/j2 branches are mispredicted, but nevertheless these optimizations
    increased perf by another 10%.
    */

    int64 i1 = r250_index;
    int64 i2 = r521_index;
    uint8 *b1 = (uint8 *)r250_buffer;
    uint8 *b2 = (uint8 *)r521_buffer;
    uint64 *tmp1, *tmp2;
    uint64 r, s;
    int64 j1, j2;

    j1 = i1 - R250_IB;
    if (j1 < 0)
        j1 = i1 + R250_IA;
    j2 = i2 - R521_IB;
    if (j2 < 0)
        j2 = i2 + R521_IA;

    tmp1 = (uint64 *)(b1 + i1);
    r = (*(uint64 *)(b1 + j1)) ^ (*tmp1);
    *tmp1 = r;
    tmp2 = (uint64 *)(b2 + i2);
    s = (*(uint64 *)(b2 + j2)) ^ (*tmp2);
    *tmp2 = s;

    i1 = (i1 != sizeof(uint64) * (R250_LEN - 1)) ? (i1 + sizeof(uint64)) : 0;
    r250_index = i1;
    i2 = (i2 != sizeof(uint64) * (R521_LEN - 1)) ? (i2 + sizeof(uint64)) : 0;
    r521_index = i2;

    double _r = r ^ s;
//return range*(_r/((double)ULONG_MAX));
    return _r;
}
}
