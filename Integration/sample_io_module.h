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

#define	N		module::Node
#define	NODE	module::Node::Get()
#define	OUTPUT	NODE->trace(N::APPLICATION)


using	namespace	io;

MODULE_CLASS_BEGIN(SampleIO,Module<SampleIO>)
private:
	void	initialize();
	void	finalize();
public:
	void	start(){
		initialize();
		OUTPUT<<"SampleIO "<<"started"<<std::endl;
	}
	void	stop(){
		finalize();
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
		InputCode	*_m=InputCode::Build<MkVal<Vec3> >();
		MkVal<Vec3>	*m=_m->as<MkVal<Vec3> >();
		m->entityID=1;
		m->attributeID=2;
		m->value.data[0]=0.1;
		m->value.data[1]=0.2;
		m->value.data[2]=0.3;
		NODE->send(this,_m,N::PRIMARY);
	}
	void	react(OutputCode	*p){
		OUTPUT<<"SampleIO "<<"got command"<<std::endl;
	}
MODULE_CLASS_END(SampleIO)


#endif
