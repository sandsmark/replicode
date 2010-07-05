//	pgm_overlay.h
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

#ifndef	pgm_overlay_h
#define	pgm_overlay_h

#include	"base.h"
#include	"../r_code/object.h"
#include	"dll.h"

using	namespace	r_code;

namespace	r_exec{

	class	_Mem;
	class	View;

	class	InstantiatedProgram;
	class	IPGMController;
	class	Context;

	class	r_exec_dll	Overlay:
	public	_Object{
	friend	class	IPGMController;
	friend	class	Context;
	private:
		bool	alive;
	protected:
		//	Copy of the pgm code. Will be patched during matching and evaluation:
		//	any area indexed by a vl_ptr will be overwritten with:
		//		the evaluation result if it fits in a single atom,
		//		a ptr to the value array if the result is larger than a single atom,
		//		a ptr to an input if the result is a pattern input.
		Atom	*pgm_code;
		uint16	pgm_code_size;

		std::vector<Atom>		values;			//	value array.
		std::vector<P<Code> >	productions;	//	receives the results of ins, inj and eje; views are retrieved (fvw) or built (reduction) in the value array.

		bool	evaluate(uint16	index);			//	evaluates the pgm_code at the specified index.
		bool	inject_productions(_Mem	*mem);	//	return true upon successful evaluation.

		virtual	Code	*get_mk_rdx(uint16	&extent_index)	const;

		std::vector<uint16>	patch_indices;		//	indices where patches are applied; used for rollbacks.
		uint16				value_commit_index;	//	index of the last computed value+1; used for rollbacks.
		void				patch_code(uint16	index,Atom	value);
		void				patch_tpl_args();	//	no views in tpl args; patches the ptn skeleton's first atom with IPGM_PTR with an index in the ipgm arg set; patches wildcards with similar IPGM_PTRs.
		void				patch_tpl_code(uint16	pgm_code_index,uint16	ipgm_code_index);	//	to recurse.
		virtual	void		patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index);	//	defined in IOverlay.

		void	rollback();	//	reset the overlay to the last commited state: unpatch code and values.
		void	commit();	//	empty the patch_indices and set value_commit_index to values.size().

		IPGMController	*controller;

		Overlay();
		Overlay(IPGMController	*c);
	public:
		virtual	~Overlay();

		void	kill();
		bool	is_alive()	const;

		r_code::Code	*getIPGM()		const;
		r_exec::View	*getIPGMView()	const;

		virtual	r_code::Code	*getInputObject(uint16	i)	const;	//	defined in Overlay.

		r_code::Code	*buildObject(Atom	head)	const;
	};

	//	Overlay with inputs.
	class	r_exec_dll	IOverlay:
	public	Overlay{
	friend	class	IPGMController;
	friend	class	Context;
	protected:
		//	Convenience.
		uint16	first_timing_constraint_index;
		uint16	last_timing_constraint_index;
		uint16	first_guard_index;
		uint16	last_guard_index;
		uint16	first_production_index;
		uint16	last_production_index;

		std::list<uint16>				input_pattern_indices;	//	stores the input patterns still waiting for a match: will be plucked upon each successful match.
		std::vector<P<r_code::View> >	input_views;			//	copies of the inputs; vector updated at each successful match.

		typedef	enum{
			SUCCESS=0,
			FAILURE=1,
			IMPOSSIBLE=3	//	when the input's class does not even match the object class in the pattern's skeleton.
		}MatchResult;

		MatchResult	match(r_exec::View	*input,uint16	&input_index);	//	delegates to _match; input_index is set to the index of the pattern that matched the input.
		bool		check_timings();									//	return true upon successful evaluation.
		bool		check_guards();										//	return true upon successful evaluation.
		
		MatchResult	_match(r_exec::View	*input,uint16	pattern_index);	//	delegates to -match_pattern
		MatchResult	_match_pattern(r_exec::View	*input,uint16	pattern_index);	//	return SUCCESS upon a successful match, IMPOSSIBLE if the input is not of the right class, FAILURE otherwise.
		bool		_match_skeleton(r_exec::View	*input,uint16	pattern_index);

		void		reset();	//	reset to original state (pristine copy of the pgm code and empty value set).

		void	patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index);

		virtual	Code	*get_mk_rdx(uint16	&extent_index)	const;

		IOverlay(IPGMController	*c);
		IOverlay(IOverlay	*original,uint16	last_input_index,uint16	value_commit_index);	//	copy from the original and rollback.
	public:
		virtual	~IOverlay();

		virtual	void	reduce(r_exec::View	*input,_Mem	*mem);	//	called upon the processing of a reduction job.

		r_code::Code	*getInputObject(uint16	i)	const;
		//r_exec::View	*getInputView(uint16	i)	const;
	};

	class	r_exec_dll	AntiOverlay:
	public	IOverlay{
	friend	class	IPGMController;
	private:
		AntiOverlay(IPGMController	*c);
		AntiOverlay(AntiOverlay	*original,uint16	last_input_index,uint16	value_limit);
	protected:
		Code	*get_mk_rdx(uint16	&extent_index)	const;
	public:
		~AntiOverlay();

		void	reduce(r_exec::View	*input);	//	called upon the processing of a reduction job.
	};

	class	r_exec_dll	IPGMController:
	public	_Object{
	private:
		_Mem					*mem;
		r_code::View			*ipgm_view;
		bool					alive;
		std::list<P<Overlay> >	overlays;
		bool					successful_match;
	public:
		IPGMController(_Mem	*m,r_code::View	*ipgm_view):_Object(),mem(m),ipgm_view(ipgm_view),alive(true),successful_match(false){}
		~IPGMController(){}

		_Mem			*get_mem()		const{	return	mem;	}
		r_code::Code	*getIPGM()		const;
		r_exec::View	*getIPGMView()	const;

		void	kill();
		bool	is_alive()	const;

		void	take_input(r_exec::View	*input);	//	push one job for each overlay; called by the rMem at update time and at injection time.
		void	signal_anti_pgm();
		void	signal_input_less_pgm();

		void	remove(IOverlay	*overlay);
		void	add(IOverlay	*overlay);
		void	restart(AntiOverlay	*overlay,bool	match);
	};
}


#include	"pgm_overlay.inline.cpp"


#endif