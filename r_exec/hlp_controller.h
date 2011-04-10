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

	class	NullMonitor{};

	class	HLPController:
	public	OController{
	protected:
		P<BindingMap>	bindings;

		uint32	requirement_count;

		CriticalSection			g_monitorsCS;
		std::list<P<GMonitor> >	g_monitors;

		void	inject_sub_goal(uint64	now,
								uint64	deadline,
								Code	*super_goal,
								Code	*sub_goal,
								Code	*ntf_instance);
		void	notify_existing_sub_goal(	uint64	now,
											uint64	deadline,
											Code	*super_goal,
											Code	*sub_goal,
											Code	*ntf_instance);

		template<class	C>	class	PHash{	public:	size_t	operator	()(P<C>	p)	const{	return	(size_t)p.operator	->();	}	};
		typedef	UNORDERED_MAP<P<Code>,P<BindingMap>,PHash<Code> >	GoalRecords;
		GoalRecords	goal_records;	//	<fact->mk.goal,bm>.
									//	used to prevent recursion on goal injection (WRT requirements).
		template<class	C>	void	_take_input(r_exec::View	*input){

			if(	input->object->code(0).asOpcode()!=Opcodes::Fact	&&
				input->object->code(0).asOpcode()!=Opcodes::AntiFact)	//	discard everything but facts and |facts.
				return;
			Controller::_take_input<C>(input);
		}

		Code	*get_sub_goal(	BindingMap	*bm,
								Code		*super_goal,
								Code		*sub_goal_target,
								Code		*instance,
								uint64		&now,
								uint64		&deadline_high,
								uint64		&deadline_low,
								Code		*&matched_pattern);

		template<class	M>	void	add_monitor(BindingMap	*bindings,
												Code		*goal,			//	fact.
												Code		*super_goal,	//	fact.
												Code		*matched_pattern,
												uint64		expected_time_high,
												uint64		expected_time_low){

			M	*m=new	M(	this,
							bindings,
							goal->get_reference(0),
							super_goal,
							matched_pattern,
							expected_time_high,
							expected_time_low);
			g_monitorsCS.enter();
			g_monitors.push_front(m);
			g_monitorsCS.leave();
			_Mem::Get()->pushTimeJob(new	MonitoringJob<M>(m,expected_time_high));
		}
		template<>	void	add_monitor<NullMonitor>(	BindingMap	*bindings,
														Code		*goal,
														Code		*super_goal,
														Code		*matched_pattern,
														uint64		expected_time_high,
														uint64		expected_time_low){}

		HLPController(r_code::View	*view);
	public:
		virtual	~HLPController();

		bool	monitor(Code	*input);
		void	add_outcome(Code	*target,bool	success,float32	confidence)	const;	//	target: mk.pred or mk.goal.

		void	remove_monitor(GMonitor	*m);

		void	add_requirement();
		void	remove_requirement();

		Code	*get_instance(const	BindingMap	*bm,uint16	opcode)	const;
		Code	*get_instance(uint16	opcode)	const;
		virtual	uint16	get_instance_opcode()	const=0;

		uint16	get_out_group_count()	const;
		Code	*get_out_group(uint16	i)	const;	//	i starts at 1.

		template<class	M>	void	produce_sub_goal(	BindingMap	*bm,
														Code		*super_goal,		//	fact->mk.goal->fact; its time may be a variable.
														Code		*sub_goal_target,	//	fact; its time may be a variable.
														Code		*instance){

			uint64	now;
			uint64	deadline_high;
			uint64	deadline_low;
			Code	*matched_pattern;
			Code	*sub_goal_fact=get_sub_goal(bm,
												super_goal,
												sub_goal_target,
												instance,
												now,
												deadline_high,
												deadline_low,
												matched_pattern);
			if(!sub_goal_fact)
				return;

			if(sub_goal_fact->get_reference(0)->get_reference(0)->get_reference(0)->code(0).asOpcode()!=Opcodes::Cmd)	//	no monitoring for I/O device commands.
				add_monitor<M>(	bm,
								sub_goal_fact,
								super_goal,
								matched_pattern,
								deadline_high,
								deadline_low);
			
			inject_sub_goal(now,
							deadline_high,
							super_goal,
							sub_goal_fact,
							get_instance(get_instance_opcode()));
		}
	};
}


#endif