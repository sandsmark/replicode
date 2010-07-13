//	visualization_module.h
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
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

#ifndef	vizualization_module_h
#define vizualization_module_h

#include	"integration.h"
#include	"module_node.h"

#define	N		module::Node
#define	NODE	module::Node::Get()
#define	OUTPUT	NODE->trace(N::APPLICATION)


//	Module that compiles code and broadcast the resulting image.
//	Will be replaced by the Visualization System.
MODULE_CLASS_BEGIN(Visualizer,Module<Visualizer>)
private:
	void	initialize(){}
	void	finalize(){}
public:
	void	loadParameters(const	std::vector<word32>	&numbers,const	std::vector<std::string>	&strings){
		//	define parameters in rmem.xml - see loader module def for an example.
		//	parameters will appear in the vectors in the order they are declared in the xml file - per type.
	}
	void	start(){
		initialize();
		OUTPUT<<"Visualizer "<<"started"<<std::endl;
	}
	void	stop(){
		finalize();
		OUTPUT<<"Visualizer "<<"stopped"<<std::endl;
	}
	template<class	T>	Decision	decide(T	*p){
		return	WAIT;
	}
	template<class	T>	void	react(T	*p){
		OUTPUT<<"Visualizer "<<"got message"<<std::endl;
	}
	void	react(SystemReady	*p){
		OUTPUT<<"Visualizer "<<"got SysReady"<<std::endl;
	}
MODULE_CLASS_END(Visualizer)


#endif
