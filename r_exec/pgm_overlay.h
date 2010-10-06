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

#include	"../CoreLibrary/base.h"
#include	"../CoreLibrary/utils.h"
#include	"../r_code/object.h"
#include	"dll.h"

using	namespace	r_code;

namespace	r_exec{

	class	_Mem;
	class	View;

	class	InstantiatedProgram;
	class	Controller;
	class	_PGMController;
	class	InputLessPGMController;
	class	Context;

	//	Shared resources:
	//	- alive: read by ReductionCore (via reductionJob::update()), written by TimeCore (via the controller).
	class	r_exec_dll	Overlay:
	public	_Object{
	friend	class	Controller;
	friend	class	InputLessPGMController;
	friend	class	Context;
	protected:
		bool			alive;
		CriticalSection	reductionCS;

		//	Copy of the pgm code. Will be patched during matching and evaluation:
		//	any area indexed by a vl_ptr will be overwritten with:
		//		the evaluation result if it fits in a single atom,
		//		a ptr to the value array if the result is larger than a single atom,
		//		a ptr to an input if the result is a pattern input.
		Atom	*pgm_code;
		uint16	pgm_code_size;

		r_code::vector<Atom>	values;			//	value array.
		std::vector<P<Code> >	productions;	//	receives the results of ins, inj and eje; views are retrieved (fvw) or built (reduction) in the value array.

		bool	evaluate(uint16	index);									//	evaluates the pgm_code at the specified index.
		bool	inject_productions(_Mem	*mem,_PGMController	*origin);	//	return true upon successful evaluation.

		virtual	Code	*get_mk_rdx(uint16	&extent_index)	const;

		std::vector<uint16>	patch_indices;		//	indices where patches are applied; used for rollbacks.
		uint16				value_commit_index;	//	index of the last computed value+1; used for rollbacks.
		void				patch_code(uint16	index,Atom	value);
		uint16				get_last_patch_index();
		void				unpatch_code(uint16	patch_index);
		void				patch_tpl_args();	//	no views in tpl args; patches the ptn skeleton's first atom with IPGM_PTR with an index in the ipgm arg set; patches wildcards with similar IPGM_PTRs.
		void				patch_tpl_code(uint16	pgm_code_index,uint16	ipgm_code_index);	//	to recurse.
		virtual	void		patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index);	//	defined in IOverlay.

		void	rollback();	//	reset the overlay to the last commited state: unpatch code and values.
		void	commit();	//	empty the patch_indices and set value_commit_index to values.size().

		Controller	*controller;

		Overlay();
		Overlay(Controller	*c);
	public:
		virtual	~Overlay();

		void	kill();

		virtual	void	reset();	//	reset to original state (pristine copy of the pgm code and empty value set).

		r_code::Code	*getIPGM()		const;
		r_exec::View	*getIPGMView()	const;

		r_code::Code	*buildObject(Atom	head)	const;

		_Mem	*get_mem()	const;
	};

	//	Overlay with inputs.
	//	Several ReductionCores can attempt to reduce the same overlay simultaneously (each with a different input).
	class	r_exec_dll	IOverlay:
	public	Overlay{
	friend	class	PGMController;
	friend	class	Context;
	private:
		uint64	birth_time;	// used for ipgms: overlays older than ipgm->pgm->tsc are killed.
	protected:
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
		
		MatchResult	_match(r_exec::View	*input,uint16	pattern_index);		//	delegates to __match.
		MatchResult	__match(r_exec::View	*input,uint16	pattern_index);	//	return SUCCESS upon a successful match, IMPOSSIBLE if the input is not of the right class, FAILURE otherwise.

		void	patch_input_code(uint16	pgm_code_index,uint16	input_index,uint16	input_code_index);

		virtual	Code	*get_mk_rdx(uint16	&extent_index)	const;

		void	init();

		IOverlay(Controller	*c);
		IOverlay(PGMController	*c);
		IOverlay(IOverlay	*original,uint16	last_input_index,uint16	value_commit_index);	//	copy from the original and rollback.
	public:
		virtual	~IOverlay();

		void	reset();

		virtual	void	reduce(r_exec::View	*input,_Mem	*mem);	//	called upon the processing of a reduction job.

		r_code::Code	*getInputObject(uint16	i)	const;
		r_code::View	*getInputView(uint16	i)	const;
	};

	//	Several ReductionCores can attempt to reduce the same overlay simultaneously (each with a different input).
	//	In addition, ReductionCores and signalling jobs can attempt to inject productions concurrently.
	class	r_exec_dll	AntiOverlay:
	public	IOverlay{
	friend	class	AntiPGMController;
	private:
		AntiOverlay(AntiPGMController	*c);
		AntiOverlay(AntiOverlay	*original,uint16	last_input_index,uint16	value_limit);
	protected:
		Code	*get_mk_rdx(uint16	&extent_index)	const;
	public:
		~AntiOverlay();

		void	reduce(r_exec::View	*input,_Mem	*mem);	//	called upon the processing of a reduction job.
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//	Shared resources:
	//	- alive: written by TimeCores (via UpdateJob::update() and _Mem::update()), read by TimeCores (via SignalingJob::update()).
	//	- overlays: modified by take_input, executed by TimeCores (via UpdateJob::update() and _Mem::update()) and ReductionCore::Run() (via ReductionJob::update(), IOverlay::reduce(), _Mem::inject() and add()/remove()/restart()).
	class	r_exec_dll	Controller:
	public	_Object{
	protected:
		_Mem					*mem;
		r_code::View			*ipgm_view;

		bool					alive;
		CriticalSection			aliveCS;

		std::list<P<Overlay> >	overlays;
		CriticalSection			overlayCS;

		uint64					tsc;

		Controller(_Mem	*m,r_code::View	*ipgm_view);
	public:
		virtual	~Controller();

		_Mem			*get_mem()		const;
		r_code::Code	*getIPGM()		const;
		r_exec::View	*getIPGMView()	const;

		void	kill();
		bool	is_alive();
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

	class	r_exec_dll	_PGMController:
	public	Controller{
	protected:
		_PGMController(_Mem	*m,r_code::View	*ipgm_view);
	public:
		virtual	~_PGMController();

		virtual	void	take_input(r_exec::View	*input,_PGMController	*origin)=0;	//	push one job for each overlay; called by the rMem at update time and at injection time.

		void	add(Overlay	*overlay);
	};

	//	No need for signaling jobs: upon invocation of take_input() the overlays older than tsc are killed, assuming stc>0; otherwise, overlays live unitl the ipgm dies.
	class	r_exec_dll	PGMController:
	public	_PGMController{
	public:
		PGMController(_Mem	*m,r_code::View	*ipgm_view);
		~PGMController();

		void	take_input(r_exec::View	*input,_PGMController	*origin=NULL);
		void	remove(Overlay	*overlay);
	};

	//	Signaled by TimeCores (holding AntiPGMSignalingJob).
	//	Possible recursive locks: signal_anti_pgm()->overlay->inject_productions()->mem->inject()->injectNow()->inject_reduction_jobs()->overlay->take_input().
	class	r_exec_dll	AntiPGMController:
	public	_PGMController{
	private:
		bool	successful_match;

		void	push_new_signaling_job();
	public:
		AntiPGMController(_Mem	*m,r_code::View	*ipgm_view);
		~AntiPGMController();

		void	take_input(r_exec::View	*input,_PGMController	*origin=NULL);
		void	signal_anti_pgm();

		void	restart(AntiOverlay	*overlay);
	};
}


#include	"pgm_overlay.inline.cpp"


#endif