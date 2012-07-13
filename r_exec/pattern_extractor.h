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
#include	"guard_builder.h"


namespace	r_exec{

	class	AutoFocusController;

	class	Input{
	public:
		P<BindingMap>	bindings;	// contains the values for the abstraction.
		P<_Fact>		abstraction;
		P<_Fact>		input;

		Input(_Fact	*input,_Fact	*abstraction,BindingMap	*bindings):input(input),abstraction(abstraction),bindings(bindings){}
		Input():input(NULL),abstraction(NULL),bindings(NULL){}
	};

	// Targeted Pattern eXtractor.
	// Does nothing.
	// Used for wr_enabled productions or for well-rated productions.
	class	r_exec_dll	TPX:
	public	_Object{
	protected:
		const	AutoFocusController	*auto_focus;

		Input	target;	// goal or prediction target; abstraction: lhs of a mdl for goals, rhs for predictions.
		TPX(const	AutoFocusController	*auto_focus,_Fact	*target);
	public:
		TPX(const	AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings);
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
		std::list<P<Code> >	hlps;	// new mdls/csts.

		void	inject_hlps(uint64	analysis_starting_time)	const;

		virtual	std::string	get_header()	const=0;

		_TPX(const	AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings);
		_TPX(const	AutoFocusController	*auto_focus,_Fact	*target);
	public:
		virtual	~_TPX();

		bool	take_input(Input	*input);	// input->input is a fact.
	};

	// Pattern extractor targeted at goal successes.
	// Possible causes are younger than the production of the goal.
	// Models produced are of the form: M1[cause -> goal_target], where cause can be an imdl and goal_target can be an imdl.
	// M1 does not have template arguments.
	// Commands are ignored (CTPX' job).
	class	r_exec_dll	GTPX:	// target is a goal.
	public	_TPX{
	private:
		std::string	get_header()	const;
	public:
		GTPX(const	AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings);
		~GTPX();

		void	signal(View	*input)	const;
		void	reduce(View	*input);	// input is v->f->success(target,input) or v->|f->success(target,input).
	};

	// Pattern extractor targeted at prediciton failures.
	// Possible causes are older than the production of the prediction.
	// Models produced are of the form: M1[cause -> |imdl M0] where M0 is the model that produced the failed prediction and cause can be an imdl.
	// M1 does not have template arguments. As a general rule, requirements cannot have requirements.
	// Commands are ignored (CTPX' job).
	class	r_exec_dll	PTPX:	// target is a prediction.
	public	_TPX{
	private:
		std::string	get_header()	const;
	public:
		PTPX(const	AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings);
		~PTPX();

		void	signal(View	*input)	const;
		void	reduce(View	*input);	// input is v->f->success(target,input) or v->|f->success(target,input).
	};

	class	ICST;

	// Pattern extractor targeted at changes of repeated input facts (SYMC_PERIODIC or SYNC_HOLD).
	// Models produced are of the form: [premise -> [cause -> consequent]], i.e. M1:[premise -> imdl M0], M0:[cause -> consequent].
	// M0 has template args, i.e the value of the premise and its after timestamp.
	// N.B.: before-after=upr of the group the input comes from.
	// The Consequent is a value different from the expected repetition of premise.
	// The premise is an icst assembled from inputs synchronous with the input expected to repeat.
	// Guards on values (not only on timings) are computed: this is the only TPX that does so.
	// Inputs with SYNC_HOLD: I/O devices are expected to send changes on such inputs as soon as available.
	class	CTPX:
	public	_TPX{
	private:
		bool	stored_premise;

		GuardBuilder	*get_default_guard_builder(_Fact	*cause,_Fact	*consequent,uint64	period);
		GuardBuilder	*find_guard_builder(_Fact	*cause,_Fact	*consequent,uint64	period);
		_Fact	*find_f_icst(_Fact	*component,uint16	&component_index);
		_Fact	*find_f_icst(_Fact	*component,uint16	&component_index,Code	*&cst);

		bool	build_mdl(_Fact	*cause,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period);
		bool	build_mdl(_Fact	*f_icst,_Fact	*cause_pattern,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period);

		bool	build_requirement(BindingMap	*bm,Code	*m0,uint64	period);

		Code	*build_cst(ICST	*icst,BindingMap	*bm,_Fact	*component);
		Code	*build_mdl_head(BindingMap	*bm,uint16	tpl_arg_count,_Fact	*lhs,_Fact	*rhs,uint16	&write_index);
		void	build_mdl_tail(Code	*mdl,uint16	write_index);

		std::string	get_header()	const;
	public:
		CTPX(const	AutoFocusController	*auto_focus,_Fact	*premise);
		~CTPX();

		void	store_input(_Fact	*input);
		void	reduce(r_exec::View	*input);	// asynchronous: build models of value change if not aborted asynchronously by ASTControllers.
		void	signal(r_exec::View	*input);	// spawns mdl/cst building (reduce()).
	};
}


#endif