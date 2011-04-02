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


namespace	r_exec{

	class	HLPController:
	public	OController{
	protected:
		P<BindingMap>	bindings;

		uint32	requirement_count;

		CriticalSection			g_monitorsCS;
		std::list<P<GMonitor> >	g_monitors;

		void	inject_sub_goal(uint64	now,
								uint64	deadline,
								Code	*target,
								Code	*super_goal,
								Code	*sub_goal,
								Code	*ntf_instance);
		void	notify_existing_sub_goal(	uint64	now,
											uint64	deadline,
											Code	*super_goal,
											Code	*sub_goal,
											Code	*ntf_instance);
		virtual	Code	*get_ntf_instance(BindingMap	*bm)	const=0;

		template<class	C>	void	_take_input(r_exec::View	*input){

			if(	input->object->code(0).asOpcode()!=Opcodes::Fact	&&
				input->object->code(0).asOpcode()!=Opcodes::AntiFact)	//	discard everything but facts and |facts.
				return;
			Controller::_take_input<C>(input);
		}

		virtual	void	add_monitor(	BindingMap	*bindings,
										Code		*goal,
										Code		*super_goal,
										Code		*matched_pattern,
										uint64		expected_time_high,
										uint64		expected_time_low);

		HLPController(r_code::View	*view);
	public:
		virtual	~HLPController();

		bool	monitor(Code	*input);
		void	add_outcome(Code	*target,bool	success,float32	confidence)	const;	//	target: mk.pred or mk.goal.
		void	produce_sub_goal(	BindingMap	*bm,
									Code		*super_goal,
									Code		*object,
									Code		*instance,
									bool		monitor);

		template<class	M>	void	add_monitor(GMonitor	*m){

			g_monitorsCS.enter();
			g_monitors.push_front(m);
			g_monitorsCS.leave();
			_Mem::Get()->pushTimeJob(new	MonitoringJob<M>((M	*)m,m->get_deadline()));
		}
		void	remove_monitor(GMonitor	*m);

		void	add_requirement();
		void	remove_requirement();

		Code	*get_instance(const	BindingMap	*bm,uint16	opcode)	const;
		Code	*get_instance(uint16	opcode)	const;
		virtual	uint16	get_instance_opcode()	const=0;

		uint16	get_out_group_count()	const;
		Code	*get_out_group(uint16	i)	const;	//	i starts at 1.
	};
}


#endif