//	hlp_controller.h
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

#ifndef	hlp_controller_h
#define	hlp_controller_h

#include	"overlay.h"
#include	"binding_map.h"
#include	"g_monitor.h"
#include	"group.h"


namespace	r_exec{

	typedef	enum{
		WR_DISABLED=0,
		SR_DISABLED_NO_WR=1,
		SR_DISABLED_WR=2,
		WR_ENABLED=3,
		NO_R=4
	}ChainingStatus;

	class	HLPController:
	public	OController{
	private:
		uint32	strong_requirement_count;	// number of active strong requirements in the same group; updated dynamically.
		uint32	weak_requirement_count;		// number of active weak requirements in the same group; updated dynamically.
		uint32	requirement_count;			// sum of the two above.
	protected:
		class	EEntry{	// evidences.
		private:
			void	load_data(_Fact	*evidence);
		public:
			P<_Fact>	evidence;
			uint64		after;
			uint64		before;
			float32		confidence;

			EEntry(_Fact	*evidence);
			EEntry(_Fact	*evidence,_Fact	*payload);

			bool	is_too_old(uint64	now)	const{	return	(evidence->is_invalidated()	||	before<now);	}
		};

		class	PEEntry:	// predicted evidences.
		public	EEntry{
		public:
			PEEntry(_Fact	*evidence);
		};

		template<class	E>	class	Cache{
		public:
			CriticalSection	CS;
			std::list<E>	evidences;
		};

		Cache<EEntry>	evidences;
		Cache<PEEntry>	predicted_evidences;

		template<class	E>	void	_store_evidence(Cache<E>	*cache,_Fact	*evidence){

			E	e(evidence);
			cache->CS.enter();
			uint64	now=Now();
			std::list<E>::const_iterator	_e;
			for(_e=cache->evidences.begin();_e!=cache->evidences.end();){

				if((*_e).evidence->is_invalidated()	||	(*_e).before<now)	// garbage collection.
					_e=cache->evidences.erase(_e);
				else
					++_e;
			}
			cache->evidences.push_front(e);
			cache->CS.leave();
		}

		P<BindingMap>	bindings;

		bool	evaluate_bwd_guards(BindingMap	*bm);

		void	set_opposite(_Fact	*fact)	const;

		_Fact	*get_absentee(_Fact	*fact)	const;

		MatchResult	check_evidences(_Fact	*target,_Fact	*&evidence);			// evidence with the match (positive or negative), get_absentee(target) otherwise.
		MatchResult	check_predicted_evidences(_Fact	*target,_Fact	*&evidence);	// evidence with the match (positive or negative), NULL otherwise.

		HLPController(r_code::View	*view);
	public:
		virtual	~HLPController();

		Code	*get_core_object()	const{	return	getObject();	}	// cst or mdl.
		Code	*get_unpacked_object()	const{	// the unpacked version of the core object.
			
			Code	*core_object=get_core_object();
			return	core_object->get_reference(core_object->references_size()-1);
		}

		void	add_requirement(bool	strong);
		void	remove_requirement(bool	strong);

		uint32	get_requirement_count(uint32	&weak_requirement_count,uint32	&strong_requirement_count);
		uint32	get_requirement_count();

		void	store_evidence(_Fact	*evidence){			_store_evidence<EEntry>(&evidences,evidence);	}
		void	store_predicted_evidence(_Fact	*evidence){	_store_evidence <PEEntry>(&predicted_evidences,evidence);	}
		
		virtual	Fact	*get_f_ihlp(const	BindingMap	*bindings,bool	wr_enabled)	const=0;

		uint16	get_out_group_count()	const;
		Code	*get_out_group(uint16	i)	const;	// i starts at 1.
		Group	*get_host()	const;

		bool	inject_prediction(Fact	*prediction,Fact	*f_ihlp,float32	confidence,uint64	time_to_live,Code	*mk_rdx)	const;	// here, resilience=time to live, in us; returns true if the prediction has actually been injected.
		void	inject_prediction(Fact	*prediction,float32	confidence)	const;	// for simulated predictions.
	};
}


#endif