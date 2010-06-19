//	sample_io_module.cpp
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

#include "sample_io_module.h"

#include	"image.h"
#include	"preprocessor.h"


#define	ENTITY_COUNT	3

LOAD_MODULE(SampleIO)

thread_ret thread_function_call	SampleIO::Sample(void	*args){

	SampleIO	*_this=(SampleIO	*)args;

	Timer	timer;
	timer.start(_this->sampling_rate,_this->sampling_rate);

	CodePayload	*entities[ENTITY_COUNT];
	for(uint8	i=0;i<ENTITY_COUNT;++i)
		entities[i]=Entity::New();

	float32	delta=0.1;

	while(1){

		timer.wait();

		//	send 3 entities and an update of their positions.
		for(uint8	i=0;i<ENTITY_COUNT;++i){

			NODE->send(_this,entities[i],NODE->id(),N::PRIMARY);

			CodePayload	*m=MkVal<Vec3>::New(entities[i],/*Get("position")*/NULL,Vec3(0.1+delta,0.2+delta,0.3+delta));
			NODE->send(_this,m,NODE->id(),N::PRIMARY);
		}

		delta+=0.1;
	}

	for(uint8	i=0;i<ENTITY_COUNT;++i)
		delete	entities[i];

	thread_ret_val(0);
}

void	SampleIO::initialize(){

	Thread::start(Sample);
}

void	SampleIO::finalize(){

	Thread::TerminateAndWait(this);
}