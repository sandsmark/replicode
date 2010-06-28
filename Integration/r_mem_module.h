//	r_mem_module.h
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

#ifndef	r_mem_module_h
#define r_mem_module_h

#include	"integration.h"
#include	"module_node.h"
#include	"mem.h"
#include	"segments.h"

#include	"replicode_classes.h"

#define	N		module::Node
#define	NODE	module::Node::Get()
#define	OUTPUT	NODE->trace(N::APPLICATION)


template<class	U>	class	NetworkMem:
public	Module<U>,
public	r_exec::Mem<RObject>{
protected:
	NetworkMem();
	~NetworkMem();
public:
	void	eject(r_exec::View	*view,uint16	nodeID);		//	RMem to RMem.
	void	eject(r_exec::LObject	*command,uint16	nodeID);	//	RMem to I/O device.
};

#include	"r_mem_module.tpl.cpp"

MODULE_CLASS_BEGIN(RMem,NetworkMem<RMem>)
private:
	void	initialize();
	void	finalize();
	void	load(r_comp::Image	*image);
	void	decompile(r_comp::Image*image);
	void	inject(RObject	*object,uint16	nodeID,STDGroupID	destination);
public:
	void	start(){
		initialize();
		OUTPUT<<"RMem "<<"got started"<<std::endl;
	}
	void	stop(){
		finalize();
		OUTPUT<<"RMem "<<"stopped"<<std::endl;
	}
	template<class	T>	Decision	decide(T	*p){
		return	WAIT;
	}
	template<class	T>	void	react(T	*p){
		OUTPUT<<"RMem "<<"got message"<<std::endl;
	}
	void	react(SystemReady	*p){
		OUTPUT<<"RMem "<<"got SysReady"<<std::endl;
	}
	void	react(ImageMessage	*p){
		OUTPUT<<"RMem "<<"got an image"<<std::endl;

		r_comp::Image	*image=new	r_comp::Image();
		image->load<ImageMessage>(p);
		load(image);	//	stop the mem, reload and restart.
		delete	image;
	}
	void	react(CodePayload	*p){
		OUTPUT<<"RMem "<<"got input code"<<std::endl;
		inject(new	RObject(p,this),p->senderNodeID(),p->destination);	//	load input in the rMem.
	}
MODULE_CLASS_END(RMem)


#endif
