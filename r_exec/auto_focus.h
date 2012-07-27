//	auto_focus.h
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

#ifndef	auto_focus_h
#define	auto_focus_h

#include	"../r_code/time_buffer.h"

#include	"overlay.h"
#include	"group.h"
#include	"pattern_extractor.h"
#include	"mem.h"


namespace	r_exec{

	class	r_exec_dll	AutoFocusController:
	public	Controller{
	private:
		// icpp_pgm parameters.
		bool					_pass_through;
		bool					_ctpx_on;
		bool					_gtpx_on;
		bool					_ptpx_on;
		bool					_trace_injections;
		bool					_decompile_models;
		std::vector<Group	*>	output_groups;	// 1st is the primary, 2nd the secondary, followed by other groups if any.

		class	Rating{
		public:
			uint32	evidences;
			uint32	positive_evidences;
			float32	SR;
			float32	dSR;

			static	bool	DSR(float32	dSR){

				return	dSR>0	&&	dSR<_Mem::Get()->get_tpx_dsr_thr();
			}

			Rating():evidences(0),positive_evidences(0),SR(0),dSR(1){}

			void	add_evidence(bool	success){

				++evidences;
				if(success)
					++positive_evidences;
				dSR=SR;
				SR=positive_evidences/evidences;
				dSR=SR-dSR;
			}
		};

		typedef	UNORDERED_MAP<P<_Fact>,P<TPX>,PHash<_Fact> >	TPXMap;

		TPXMap	goals;			// f->g->f->target.
		TPXMap	predictions;	// f->p->f->target.

		typedef	UNORDERED_MAP<P<_Fact>,Rating,PHash<_Fact> >	RatingMap;

		// entries are patterns, i.e. abstract targets.
		RatingMap	goal_ratings;
		RatingMap	prediction_ratings;

		static	const	uint32	CacheInitialSize=128;
		static	const	uint32	CrossBufferInitialSize=1024;

		time_buffer<CInput>	cache;			// contains all inputs we don't no yet if they are relevant or not; thz==sampling period.
		time_buffer<Input>	cross_buffer;	// contains all relevant inputs.

		void	notify(_Fact	*target,View	*input,TPXMap	&map);
		void	dispatch_pred_success(_Fact	*predicted_f,TPXMap	&map);
		void	dispatch(View	*input,_Fact	*abstract_input,BindingMap	*bm,bool	&injected,TPXMap	&map);
		void	dispatch_no_inject(View	*input,_Fact	*abstract_input,BindingMap	*bm,TPXMap	&map);
		template<class	T>	TPX	*build_tpx(_Fact	*target,_Fact	*pattern,BindingMap	*bm,RatingMap	&map,Fact	*f_imdl,bool	wr_enabled){

			if(!_gtpx_on	&&	!_ptpx_on)
				return	new	TPX(this,target,pattern,bm);

			if(wr_enabled)
				return	new	TPX(this,target,pattern,bm);

			RatingMap::const_iterator	r=map.find(pattern);
			if(r!=map.end()){

				if(Rating::DSR(r->second.dSR))	// target for which we don't see much improvement over time.
					return	new	TPX(this,target,pattern,bm);
				else
					return	new	T(this,target,pattern,bm,f_imdl);
			}else
				return	new	T(this,target,pattern,bm,f_imdl);
		}
		void	rate(_Fact	*target,bool	success,TPXMap	&map,RatingMap	&ratings);
	public:
		AutoFocusController(r_code::View	*view);
		~AutoFocusController();

		Code	*get_core_object()	const;
		
		void	take_input(r_exec::View	*input);
		void	reduce(r_exec::View	*input);

		View	*inject_input(View	*input);	// inject a filtered input into the output groups starting from 0; return the view injected in the primary group.
		void	inject_input(View	*input,uint32	start);								// inject an unfiltered input into the output groups starting from start.
		void	inject_input(View	*input,_Fact	*abstract_input,BindingMap	*bm);	// inject a filtered input into the output groups.
		void	inject_hlps(const	std::vector<P<Code> >	&hlps)	const;	// called by TPX; hlp is a mdl or a cst.
		
		bool	decompile_models()		const{	return	_decompile_models;	}
		bool	gtpx_on()				const{	return	_gtpx_on;	}
		bool	ptpx_on()				const{	return	_ptpx_on;	}
		Group	*get_primary_group()	const{	return	output_groups[0];	}

		void	copy_cross_buffer(r_code::list<Input>	&destination);
		time_buffer<CInput>	&get_cache(){	return	cache;	}
	};
}


#endif