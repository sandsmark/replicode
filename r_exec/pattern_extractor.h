//	pattern_extractor.h
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

#ifndef	pattern_extractor_h
#define	pattern_extractor_h

#include	"binding_map.h"


namespace	r_exec{

	class	AutoFocusController;

	class	Input{
	public:
		P<BindingMap>	bindings;	// contains the values for the abstraction.
		P<_Fact>		abstraction;
		P<_Fact>		input;

		Input(_Fact	*input,_Fact	*abstraction,BindingMap	*bindings):input(input),abstraction(abstraction),bindings(bindings){}
	};

	// Targeted Pattern eXtractor.
	// Does nothing.
	// Used for wr_enabled productions or for well-rated productions.
	class	r_exec_dll	TPX:
	public	_Object{
	protected:
		const	AutoFocusController	*controller;

		Input	target;	// goal or prediction target; abstraction: lhs of a mdl for goals, rhs for predictions.
	public:
		TPX(const	AutoFocusController	*controller,_Fact	*target,_Fact	*pattern,BindingMap	*bindings);
		TPX(const	TPX	*original);
		virtual	~TPX();

		_Fact		*get_pattern()	const{	return	target.abstraction;	}
		BindingMap	*get_bindings()	const{	return	target.bindings;	}

		virtual	bool	take_input(Input	*input);	// input->input is a fact; return true if the input shares a value with the target.
		virtual	void	signal(View	*input)	const;
	};

	class	r_exec_dll	_TPX:
	public	TPX{
	protected:
		std::list<Input>	inputs;	// time-controlled buffer (inputs older than tpx_time_horizon from now are discarded).

		virtual	Code	*build_model()=0;
		
		_TPX(const	AutoFocusController	*controller,_Fact	*target,_Fact	*pattern,BindingMap	*bindings);
	public:
		virtual	~_TPX();

		bool	take_input(Input	*input);	// input->input is a fact.
		void	reduce(View	*input);	// input is v->f->success(target,input) or v->|f->success(target,input).
	};

	class	r_exec_dll	GTPX:	// target is a goal.
	public	_TPX{
	private:
		Code	*build_model();
	public:
		GTPX(const	AutoFocusController	*controller,_Fact	*target,_Fact	*pattern,BindingMap	*bindings);
		~GTPX();

		void	signal(View	*input)	const;
	};

	class	r_exec_dll	PTPX:	// target is a prediction.
	public	_TPX{
	private:
		Code	*build_model();
	public:
		PTPX(const	AutoFocusController	*controller,_Fact	*target,_Fact	*pattern,BindingMap	*bindings);
		~PTPX();

		void	signal(View	*input)	const;
	};
}


#endif