//	utils.h
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following Conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of Conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of Conditions and the following disclaimer in the
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

#ifndef utils_h
#define utils_h

#include	"types.h"
#include	<stdio.h>

//	Wrapping of OS-dependent functions
namespace	r_code{

	void PrintBinary(void* p, uint32 size, bool asInt, const char* title = NULL);

	class	dll	Thread{
	private:
		thread		_thread;
	protected:
		Thread();
	public:
		template<class	T>	static	T	*New(thread_function	f,void	*args);
		static	void	Wait(Thread	**threads,uint32	threadCount);
		static	void	Wait(Thread	*_thread);
		static	void	Sleep(int64	d);	//	ms
		static	void	Sleep();		//	inifnite
		virtual	~Thread();
		void	start(thread_function	f);
		void	suspend();
		void	resume();
	};

	class	dll	Time{	//	TODO:	make sure time stamps are consistent when computed by different cores
	friend	class	TimeProbe;
	private:
		static	float64	Period;
		static	int64	InitTime;
	public:
		static	void	Init(uint32	r);	//	detects the hardware timing capabilities; r: time resolution in us (on windows xp: max ~1000; use 1000, 2000, 5000 or 10000)
		static	int64	Get();	//	in us since 01/01/1970
	};

	class	dll	Mutex{
	private:
		mutex	m;
	protected:
		static	const	uint32	Infinite;
	public:
		Mutex();
		~Mutex();
		bool	acquire(uint32	timeout=Infinite);	//	returns true if timedout
		void	release();
	};

	class	dll	Atomic{
	public:
		static	int32	Increment32(int32	volatile	*v);									//	return the final value of *v
		static	int32	Decrement32(int32	volatile	*v);									//	return the final value of *v
		static	int32	CompareAndSwap32(int32	volatile	*target,int32	v1,int32	v2);	//	compares *target with v1, if equal, replace it with v2; return the initial value of *target
		static	int64	CompareAndSwap64(int64	volatile	*target,int64	v1,int64	v2);
		static	word	CompareAndSwap(word	volatile	*target,word	v1,word	v2);			//	uses the right version according to ARCH_xx
		static	int32	Swap32(int32	volatile	*target,int32	v);							//	writes v at target; return the initial value of *target
		static	int64	Swap64(int64	volatile	*target,int64	v);							//	ifndef ARCH_64, calls CompareAndSwap64(target,v,v)
		static	word	Swap(word	volatile	*target,word	v);								//	uses the right version according to ARCH_xx
#if defined	ARCH_64
		static	int64	Increment64(int64	volatile	*v);
		static	int64	Decrement64(int64	volatile	*v);
		static	int32	Add32(int32	volatile	*target,int32	v);	//	adds v to *target; return the final value of *target
		static	int64	Add64(int64	volatile	*target,int64	v);
#endif
	};
}


#include	"utils.tpl.cpp"


#endif
