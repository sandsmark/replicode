//	pgm_overlay.h
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

#ifndef	pgm_overlay_h
#define	pgm_overlay_h

#include	"overlay.h"


namespace	r_exec{

	class	PGMController;
	class	InputLessPGMController;
	class	Context;

	// Overlays for input-less programs.
	// Base class for other programs (with inputs, anti-programs).
	class	r_exec_dll	InputLessPGMOverlay:
	public	Overlay{
	friend	class	PGMController;
	friend	class	InputLessPGMController;
	friend	class	Context;
	protected:
		//	Copy of the pgm code. Will be patched during matching and evaluation:
		//	any area indexed by a vl_ptr will be overwritten with:
		//		the evaluation result if it fits in a single atom,
		//		a ptr to the value array if the result is larger than a single atom,
		//		a ptr to an input if the result is a pattern input.
		Atom	*pgm_code;
		uint16	pgm_code_size;

		r_code::vector<Atom>	values;			//	value array.
		std::vector<P<Code> >	productions;	//	receives the results of ins, inj and eje; views are retrieved (fvw) or built (reduction) in the value array.

		bool	evaluate(uint16	index);			//	evaluates the pgm_code at the specified index.

		virtual	Code	*get_mk_rdx(uint16	&extent_index)	const;

		std::vector<uint16>	patch_indices;		//	indices where patches are applied; used for rollbacks.
		uint16				value_commit_index;	//	index of the last computed value+1; used for rollbacks.
		void				patch_code(uint16	index,Atom	value);
		uint16				get_last_patch_index();
		void				unpatch_code(uint16	patch_index);
		void				patch_tpl_args();	//	no views in tpl args; patches the ptn skeleton's first atom with IPGM_PTR with an index in the ipgm arg set; patches wildcards with similar IPGM_PTRs.
		void				patch_tpl_code(uint16	pgm_code_index,uint16	ipgm_code_index);	//	to recurse.
		virtual	void		patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index);	//	defined in PGMOverlay.

		void	rollback();	//	reset the overlay to the last commited state: unpatch code and values.
		void	commit();	//	empty the patch_indices and set value_commit_index to values.size().

		InputLessPGMOverlay();
		InputLessPGMOverlay(Controller	*c);
	public:
		virtual	~InputLessPGMOverlay();

		virtual	void	reset();	//	reset to original state (pristine copy of the pgm code and empty value set).

		bool	inject_productions(Controller	*origin);	//	return true upon successful evaluation.
	};

	//	Overlay with inputs.
	//	Several ReductionCores can attempt to reduce the same overlay simultaneously (each with a different input).
	class	r_exec_dll	PGMOverlay:
	public	InputLessPGMOverlay{
	friend	class	PGMController;
	friend	class	Context;
	private:
		uint64	birth_time;	// used for ipgms: overlays older than ipgm->tsc are killed.
	protected:
		std::list<uint16>				input_pattern_indices;	//	stores the input patterns still waiting for a match: will be plucked upon each successful match.
		std::vector<P<r_code::View> >	input_views;			//	copies of the inputs; vector updated at each successful match.

		P<Overlay>	source;	//	points to an overlay from which the reduction was triggered.

		typedef	enum{
			SUCCESS=0,
			FAILURE=1,
			IMPOSSIBLE=3	//	when the input's class does not even match the object class in the pattern's skeleton.
		}MatchResult;

		MatchResult	match(r_exec::View	*input,uint16	&input_index);	//	delegates to _match; input_index is set to the index of the pattern that matched the input.
		bool		check_timings();									//	return true upon successful evaluation.
		bool		check_guards();										//	return true upon successful evaluation.
		
		MatchResult	_match(r_exec::View	*input,uint16	pattern_index);		//	delegates to __match.
		MatchResult	__match(r_exec::View	*input,uint16	pattern_index);	//	return SUCCESS upon a successful match, IMPOSSIBLE if the input is not of the right class, FAILURE otherwise.

		void	patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index);

		virtual	Code	*get_mk_rdx(uint16	&extent_index)	const;

		void	init();

		void	_reduce(r_exec::View	*input);	//	convenience; called by the reduce() functions.

		PGMOverlay(Controller	*c);
		PGMOverlay(PGMOverlay	*original,uint16	last_input_index,uint16	value_commit_index);	//	copy from the original and rollback.
	public:
		virtual	~PGMOverlay();

		void	reset();

		virtual	void	reduce(r_exec::View	*input);					//	called upon the processing of a reduction job.
				void	reduce(r_exec::View	*input,Overlay	*source);	//	called from another overlay's reduce() call.

		r_code::Code	*getInputObject(uint16	i)	const;
		r_code::View	*getInputView(uint16	i)	const;

		Overlay	*getSource()	const{	return	source;	}
	};

	//	Several ReductionCores can attempt to reduce the same overlay simultaneously (each with a different input).
	//	In addition, ReductionCores and signalling jobs can attempt to inject productions concurrently.
	class	r_exec_dll	AntiPGMOverlay:
	public	PGMOverlay{
	friend	class	AntiPGMController;
	private:
		AntiPGMOverlay(Controller	*c);
		AntiPGMOverlay(AntiPGMOverlay	*original,uint16	last_input_index,uint16	value_limit);
	protected:
		Code	*get_mk_rdx(uint16	&extent_index)	const;
	public:
		~AntiPGMOverlay();

		void	reduce(r_exec::View	*input);	//	called upon the processing of a reduction job.
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Controller for programs with inputs.
	class	r_exec_dll	PGMController:
	public	Controller{
	public:
		PGMController(_Mem	*m,r_code::View	*ipgm_view);
		virtual	~PGMController();

		void	take_input(r_exec::View	*input,Controller	*origin=NULL);
		void	take_input(r_exec::View	*input,Overlay	*source);
	};

	//	TimeCores holding InputLessPGMSignalingJob trigger the injection of the productions.
	//	No contention on overlays.
	class	r_exec_dll	InputLessPGMController:
	public	Controller{
	public:
		InputLessPGMController(_Mem	*m,r_code::View	*ipgm_view);
		~InputLessPGMController();

		void	signal_input_less_pgm();
	};

	//	Signaled by TimeCores (holding AntiPGMSignalingJob).
	//	Possible recursive locks: signal_anti_pgm()->overlay->inject_productions()->mem->inject()->injectNow()->inject_reduction_jobs()->overlay->take_input().
	class	r_exec_dll	AntiPGMController:
	public	Controller{
	private:
		bool	successful_match;

		void	push_new_signaling_job();
	public:
		AntiPGMController(_Mem	*m,r_code::View	*ipgm_view);
		~AntiPGMController();

		void	take_input(r_exec::View	*input,Controller	*origin=NULL);
		void	signal_anti_pgm();

		void	restart(AntiPGMOverlay	*overlay);
	};
}


#include	"pgm_overlay.inline.cpp"


#endif