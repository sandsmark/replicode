//	utils.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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

#include	"utils.h"

#include	<iostream>

#if defined	WINDOWS
	#include <sys/timeb.h>
	#include <time.h>
#elif defined LINUX || defined OSX
	#include <dlfcn.h>
	#include <errno.h>
	#include <sys/utsname.h>
	#include <sys/time.h>
	#include <cstring>
	#include <cstdlib>
	#include <signal.h>
	#include <limits.h>
#endif


namespace	mBrane{

	void PrintBinary(void* p, uint32 size, bool asInt, const char* title) {
		if (title != NULL)
			printf("--- %s %u ---\n", title, size);
		unsigned char c;
		for (uint32 n=0; n<size; n++) {
			c = *(((unsigned char*)p)+n);
			if (asInt)
				printf("[%u] ", (unsigned int)c);
			else
				printf("[%c] ", c);
			if ( (n > 0) && ((n+1)%10 == 0) )
				printf("\n");
		}
		printf("\n");
	}

	void	Thread::Wait(Thread	**threads,uint32	threadCount){

		if(!threads)
			return;
#if defined	WINDOWS
		for(uint32	i=0;i<threadCount;i++)
			WaitForSingleObject(threads[i]->_thread,INFINITE);
#elif defined LINUX || defined OSX
		for(uint32	i=0;i<threadCount;i++)
			pthread_join(threads[i]->_thread, NULL);
#endif
	}

	void	Thread::Wait(Thread	*_thread){

		if(!_thread)
			return;
#if defined	WINDOWS
		WaitForSingleObject(_thread->_thread,INFINITE);
#elif defined LINUX || defined OSX
		pthread_join(_thread->_thread, NULL);
#endif
	}

	void	Thread::Sleep(int64	d){
#if defined	WINDOWS
		::Sleep((uint32)d);
#elif defined LINUX || defined OSX
		// we are actually being passed millisecond, so multiply up
		usleep(d*1000);
#endif
	}

	void	Thread::Sleep(){
#if defined	WINDOWS
		::Sleep(INFINITE);
#elif defined LINUX || defined OSX
		while(true)
			sleep(1000);
#endif
	}

	Thread::Thread(){
	}

	Thread::~Thread(){
#if defined	WINDOWS
		ExitThread(0);
#elif defined LINUX || defined OSX
		pthread_exit(0);
#endif
	}

	void	Thread::start(thread_function	f){
#if defined	WINDOWS
		_thread=CreateThread(NULL,0,f,this,0,NULL);
#elif defined LINUX || defined OSX
		pthread_create(&_thread, NULL, f, this);
#endif
	}

	void	Thread::suspend(){
#if defined	WINDOWS
		SuspendThread(_thread);
#elif defined LINUX || defined OSX
		printf("Thread suspending %p\n", _thread);
		pthread_kill(_thread, SIGSTOP);
#endif
	}

	void	Thread::resume(){
#if defined	WINDOWS
		ResumeThread(_thread);
#elif defined LINUX || defined OSX
		pthread_kill(_thread, SIGCONT);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////////////////////////

	float64	Time::Period;
	
	int64	Time::InitTime;

	void	Time::Init(uint32	r){
#if defined	WINDOWS
	NTSTATUS	nts;
	HMODULE	NTDll=::LoadLibrary("NTDLL");
	ULONG	actualResolution=0;
	if(NTDll){

		NSTR	pNSTR=(NSTR)::GetProcAddress(NTDll,"NtSetTimerResolution");	//	undocumented win xp sys internals
		if(pNSTR)
			nts=(*pNSTR)(10*r,true,&actualResolution);	//	in 100 ns units
	}
	LARGE_INTEGER	f;
	QueryPerformanceFrequency(&f);
	Period=1000000.0/f.QuadPart;	//	in us
	struct	_timeb	local_time;
	_ftime(&local_time);
	InitTime=(int64)(local_time.time*1000+local_time.millitm)*1000;	//	in us
#elif defined LINUX || defined OSX
		// we are actually setup a timer resolution of 1ms
		// we can simulate this by performing a gettimeofday call
		struct timeval tv;
		gettimeofday(&tv, NULL);
		InitTime = ((int64)tv.tv_sec * 1000000) + (tv.tv_usec);
#endif
	}

	int64	Time::Get(){
#if defined	WINDOWS
		LARGE_INTEGER	counter;
		QueryPerformanceCounter(&counter);
		return	(int64)(InitTime+counter.QuadPart*Period);
#elif defined LINUX || defined OSX
		timeval perfCount;
		struct timezone tmzone;
		gettimeofday(&perfCount, &tmzone);
		int64 r = ((int64)perfCount.tv_sec * 1000000) + perfCount.tv_usec;
		return	r;
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

#if defined LINUX
bool CalcTimeout(struct timespec &timeout, struct timeval &now, uint32 ms) {

	if (gettimeofday(&now, NULL) != 0)
		return false;
	ldiv_t div_result;
	div_result = ldiv( ms, 1000 );
	timeout.tv_sec = now.tv_sec + div_result.quot;
	long x = now.tv_usec + (div_result.rem * 1000);
	if (x >= 1000000) {
		timeout.tv_sec++;
		x -= 1000000;
	}
	timeout.tv_nsec = x * 1000;
	return true;
}
#endif

#if defined	WINDOWS
	const	uint32	Mutex::Infinite=INFINITE;
#elif defined LINUX || defined OSX
	/*
	 * Normally this should be SEM_VALUE_MAX but apparently the <semaphore.h> header
	 * does not define it. The documents I have read indicate that on Linux it is
	 * always equal to INT_MAX - so use that instead.
	 */
	const	uint32	Mutex::Infinite=INT_MAX;
#endif

	Mutex::Mutex(){
#if defined	WINDOWS
		m=CreateMutex(NULL,false,NULL);
#elif defined LINUX || defined OSX
		pthread_mutex_init(&m, NULL);
#endif
	}

	Mutex::~Mutex(){
#if defined	WINDOWS
		CloseHandle(m);
#elif defined LINUX || defined OSX
		pthread_mutex_destroy(&m);
#endif
	}

	bool	Mutex::acquire(uint32	timeout){
#if defined	WINDOWS
		uint32	r=WaitForSingleObject(m,timeout);
		return	r==WAIT_TIMEOUT;
#elif defined LINUX || defined OSX
		int64 start = Time::Get();
		int64 uTimeout = timeout*1000;

		while (pthread_mutex_trylock(&m) != 0) {
			Thread::Sleep(10);
			if (Time::Get() - start >= uTimeout)
				return false;
		}
		return true;

#endif
	}

	void	Mutex::release(){
#if defined	WINDOWS
		ReleaseMutex(m);
#elif defined LINUX || defined OSX
		pthread_mutex_unlock(&m);
#endif
	}

	////////////////////////////////////////////////////////////////////////////////////////////////


	////////////////////////////////////////////////////////////////////////////////////////////////

}
