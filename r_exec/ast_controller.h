//	ast_controller.h
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

#ifndef	ast_controller_h
#define	ast_controller_h

#include	"overlay.h"
#include	"factory.h"
#include	"pattern_extractor.h"


namespace	r_exec{

	// Atomic state controller. Attached to a null-pgm and monitoring to a (repeated) input fact (SYNC_PERIODIC or SYNC_HOLD).
	// Has a resilience of 2 times the upr of the group its target comes from.
	// Upon catching a counter-evidence, signal the TPX and kill the object (i.e. invalidate and kill views); this will kill the controller and TPX.
	// Catching a predicted evidence means that there is a model that predicts the next value of the object: kill the CTPX.
	// AST live in primary groups and take their inputs therefrom: these are filtered by the A/F WRT goals/predictions.
	// There is no control over AST: instead, computation is minimal (just pattern-matching) and CTPX are killed asap whenever a model predicts a value change.
	// There cannot be any control based on the semantics of the inputs as these are atomic and henceforth no icst is available at injection time.
	template<class	U>	class	ASTController:
	public	OController{
	protected:
		P<CTPX>		tpx;
		P<_Fact>	target;	// the repeated fact to be monitored.
		uint64		thz;	// time horizon: if an input is caught with ijt<thz, discard it.

		void	kill();

		ASTController(AutoFocusController	*auto_focus,_Fact	*target);
	public:
		virtual	~ASTController();

		Code	*get_core_object()	const{	return	getObject();	}

		void	take_input(r_exec::View	*input);
		void	reduce(View	*input);
	};

	// For SYNC_PERIODIC targets.
	class	PASTController:
	public	ASTController<PASTController>{
	public:
		PASTController(AutoFocusController	*auto_focus,_Fact	*target);
		~PASTController();

		void	reduce(View	*input){	this->ASTController<PASTController>::reduce(input);	}
		void	reduce(View	*v,_Fact	*input);
	};

	// For SYNC_HOLD targets.
	class	HASTController:
	public	ASTController<HASTController>{
	public:
		HASTController(AutoFocusController	*auto_focus,_Fact	*target);
		~HASTController();

		void	reduce(View	*input){	this->ASTController<HASTController>::reduce(input);	}
		void	reduce(View	*v,_Fact	*input);
	};
}


#include	"ast_controller.tpl.cpp"


#endif