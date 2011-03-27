//	factory.h
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

#ifndef	factory_h
#define	factory_h

#include	"object.h"
#include	"dll.h"


namespace	r_exec{
	namespace	factory{

		//	Notification markers are not put in their references marker sets.
		//	They are not used to propagate saliency changes.
		//	They are encoded as Atom::Object instead of Atom::Marker.

		class	r_exec_dll	MkNew:
		public	LObject{
		public:
			MkNew(r_code::Mem	*m,Code	*object);
		};

		class	r_exec_dll	MkLowRes:
		public	LObject{
		public:
			MkLowRes(r_code::Mem	*m,Code	*object);
		};

		class	r_exec_dll	MkLowSln:
		public	LObject{
		public:
			MkLowSln(r_code::Mem	*m,Code	*object);
		};

		class	r_exec_dll	MkHighSln:
		public	LObject{
		public:
			MkHighSln(r_code::Mem	*m,Code	*object);
		};

		class	r_exec_dll	MkLowAct:
		public	LObject{
		public:
			MkLowAct(r_code::Mem	*m,Code	*object);
		};

		class	r_exec_dll	MkHighAct:
		public	LObject{
		public:
			MkHighAct(r_code::Mem	*m,Code	*object);
		};

		class	r_exec_dll	MkSlnChg:
		public	LObject{
		public:
			MkSlnChg(r_code::Mem	*m,Code	*object,float32	value);
		};

		class	r_exec_dll	MkActChg:
		public	LObject{
		public:
			MkActChg(r_code::Mem	*m,Code	*object,float32	value);
		};

		//	Non-notification constructs: all builders rely on _Mem::build_object().
		//	Their base class is O as in Mem<O>.
		class	r_exec_dll	Object{
		public:
			static	Code	*Var(float32	psln_thr);
			static	Code	*Fact(Code	*object,uint64	time,float32	confidence,float32	psln_thr);
			static	Code	*AntiFact(Code	*object,uint64	time,float32	confidence,float32	psln_thr);
			static	Code	*MkSim(Code	*object,Code	*source,float32	psln_thr);
			static	Code	*MkPred(Code	*object,float32	confidence,float32	psln_thr);
			static	Code	*MkAsmp(Code	*object,Code	*source,float32	confidence,float32	psln_thr);
			static	Code	*MkSuccess(Code	*object,float32	psln_thr);
			static	Code	*MkGoal(Code	*object,Code	*actor,float32	psln_thr);
			static	Code	*MkRdx(Code	*imdl_fact,Code	*input,Code	*output,float32	psln_thr);	//	for mdl.
			static	Code	*MkRdx(Code	*icst_fact,std::vector<P<Code> > *inputs,float32	psln_thr);	//	for cst.
		};
	}
}


#endif