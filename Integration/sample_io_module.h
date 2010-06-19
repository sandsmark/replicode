//	sample_io_module.h
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

#ifndef	sample_io_module_h
#define sample_io_module_h

#include	"integration.h"
#include	"module_node.h"
#include	"replicode_classes.h"
#include	"opcodes.h"

#define	N		module::Node
#define	NODE	module::Node::Get()
#define	OUTPUT	NODE->trace(N::APPLICATION)


template<class	U>	class	ThreadedModule:
public	Module<U>,
public	Thread{
};

//	Demonstrates how to interface an I/O module to rMems.
//	As an example, the module samples its environment: it identifies entities in the environment and periodically updates their positions (markers).
//	This is performed in an internal thread controlled by a timer.
//	Notice that entities have a resilience: if the module does not send them repeatedly, they will expire in the rMems,
//	meaning that they will be considered not exisitng anymore.
//	As a general rule, objects are sent to the rMems flat, i.e. they reference objects by their OIDs, instead of using pointers.
//	Thus, such objects (e.g. entities) have to be sent first, and markers next.
MODULE_CLASS_BEGIN(SampleIO,ThreadedModule<SampleIO>)
private:
	static	thread_ret thread_function_call	Sample(void	*args);

	uint32	sampling_rate;	//	in us (49 days max).

	void	initialize();	//	starts the thread.
	void	finalize();		//	kills the thread.
public:
	void	start(){
		OUTPUT<<"SampleIO "<<"started"<<std::endl;
	}
	void	stop(){
		OUTPUT<<"SampleIO "<<"stopped"<<std::endl;
	}
	template<class	T>	Decision	decide(T	*p){
		return	WAIT;
	}
	template<class	T>	void	react(T	*p){
		OUTPUT<<"SampleIO "<<"got message"<<std::endl;
	}
	void	react(SystemReady	*p){
		OUTPUT<<"SampleIO "<<"got SysReady"<<std::endl;
	}
	//	Command layout:
	//		cmd opcode
	//		device id
	//		function id
	//		iptr to arg set
	//		set
	//		arg 1
	//		...
	//		arg n
	void	react(uint16	deviceID,Command	*p){
		OUTPUT<<"SampleIO "<<"got command"<<std::endl;
		if(p->data(CMD_FUNCTION).asOpcode()==r_exec::GetOpcode("sample_io_start")){	//	function names as defined in usr.classes.replicode.
		
			sampling_rate=p->data(p->data(CMD_ARGS).asIndex()+1).asFloat();
			initialize();
		}else	if(p->data(CMD_FUNCTION).asOpcode()==r_exec::GetOpcode("sample_io_stop"))
			finalize();
	}
MODULE_CLASS_END(SampleIO)


#endif
