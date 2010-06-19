//	loader_module.h
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

#ifndef	loader_module_h
#define loader_module_h

#include	"integration.h"
#include	"module_node.h"

#define	N		module::Node
#define	NODE	module::Node::Get()
#define	OUTPUT	NODE->trace(N::APPLICATION)


//	Module that compiles code and broadcast the resulting image.
//	Will be replaced by the Visualization System.
MODULE_CLASS_BEGIN(Loader,Module<Loader>)
private:
	P<ImageMessage>	image;
	void	initialize();
	void	finalize();
	void	compile(const	std::string	&filename);
public:
	void	start(){
		initialize();
	}
	void	stop(){
		finalize();
		OUTPUT<<"Loader "<<"stopped"<<std::endl;
	}
	template<class	T>	Decision	decide(T	*p){
		return	WAIT;
	}
	template<class	T>	void	react(T	*p){
		OUTPUT<<"Loader "<<"got message"<<std::endl;
	}
	void	react(SystemReady	*p){
		OUTPUT<<"Loader "<<"got SysReady"<<std::endl;
		std::string		filename("c:/work/replicode/test/test.4.replicode");	//	WARNING: remove !load user.classes.replicode: global refs are maintained between compilations.
		compile(filename);
		OUTPUT<<"Loader "<<"started"<<std::endl;
		if(image!=NULL)
			NODE->send(this,image,N::PRIMARY);
	}
MODULE_CLASS_END(Loader)


#endif
