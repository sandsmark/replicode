//	guard_builder.h
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

#ifndef	guard_builder_h
#define	guard_builder_h

#include	"factory.h"


namespace	r_exec{

	class	GuardBuilder:
	public	_Object{
	public:
		GuardBuilder();
		virtual	~GuardBuilder();

		virtual	void	build(Code	*mdl,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;
	};

	// fwd: t2=t0+period, t3=t1+period.
	// bwd: t0=t2-period, t1=t3-period.
	class	TimingGuardBuilder:
	public	GuardBuilder{
	protected:
		uint64	period;

		void	write_guard(Code *mdl,uint16	l,uint16	r,uint16	opcode,uint64	offset,uint16 &write_index,uint16 &extent_index) const;
		void	_build(Code *mdl,uint16	t0,uint16	t1,uint16 &write_index) const;
	public:
		TimingGuardBuilder(uint64	period);
		virtual	~TimingGuardBuilder();

		virtual	void	build(Code	*mdl,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;
	};

	// fwd: q1=q0+speed*period.
	// bwd: speed=(q1-q0)/period, speed.after=q1.after-offset, speed.before=q1.before-offset.
	class	SGuardBuilder:
	public	TimingGuardBuilder{
	private:
		uint64	offset;	// period-(speed.after-t0).

		void	_build(Code *mdl,uint16	q0,uint16	t0,uint16	t1,uint16 &write_index)	const;
	public:
		SGuardBuilder(uint64	period,uint64	offset);
		~SGuardBuilder();

		void	build(Code	*mdl,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;
	};

	// bwd: cmd.after=q1.after-offset, cmd.before=cmd.after+cmd_duration.
	class	NoArgCmdGuardBuilder:
	public	TimingGuardBuilder{
	protected:
		uint64	offset;
		uint64	cmd_duration;

		void	_build(Code *mdl,uint16	q0,uint16	t0,uint16	t1,uint16 &write_index)	const;
	public:
		NoArgCmdGuardBuilder(uint64	period,uint64	offset,uint64	cmd_duration);
		~NoArgCmdGuardBuilder();

		void	build(Code	*mdl,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;
	};

	// bwd: cmd.after=q1.after-period, cmd.before=q1.before-period.
	class	CmdGuardBuilder:
	public	TimingGuardBuilder{
	protected:
		uint16	cmd_arg_index;

		void	_build(Code *mdl,uint16	fwd_opcode,uint16	bwd_opcode,uint16	q0,uint16	t0,uint16	t1,uint16 &write_index) const;
		void	_build(Code	*mdl,uint16	fwd_opcode,uint16	bwd_opcode,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;

		CmdGuardBuilder(uint64	period,uint16	cmd_arg_index);
	public:
		virtual	~CmdGuardBuilder();
	};

	// fwd: q1=q0*cmd_arg.
	// bwd: cmd_arg=q1/q0.
	class	MCGuardBuilder:
	public	CmdGuardBuilder{
	public:
		MCGuardBuilder(uint64	period,float32	cmd_arg_index);
		~MCGuardBuilder();

		void	build(Code	*mdl,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;
	};

	// fwd: q1=q0+cmd_arg.
	// bwd: cmd_arg=q1-q0.
	class	ACGuardBuilder:
	public	CmdGuardBuilder{
	private:
		
	public:
		ACGuardBuilder(uint64	period,uint16	cmd_arg_index);
		~ACGuardBuilder();

		void	build(Code	*mdl,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;
	};

	// bwd: cause.after=t2-offset, cause.before=t3-offset.
	class	ConstGuardBuilder:
	public	TimingGuardBuilder{
	protected:
		float32	constant;
		uint64	offset;

		void	_build(Code *mdl,uint16	fwd_opcode,uint16	bwd_opcode,uint16	q0,uint16	t0,uint16	t1,uint16 &write_index) const;
		void	_build(Code	*mdl,uint16	fwd_opcode,uint16	bwd_opcode,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;

		ConstGuardBuilder(uint64	period,float32	constant,uint64	offset);
	public:
		~ConstGuardBuilder();
	};

	// fwd: q1=q0*constant.
	// bwd: q0=q1/constant.
	class	MGuardBuilder:
	public	ConstGuardBuilder{
	public:
		MGuardBuilder(uint64	period,float32	constant,uint64	offset);
		~MGuardBuilder();

		void	build(Code	*mdl,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;
	};

	// fwd: q1=q0+constant.
	// bwd: q0=q1-constant.
	class	AGuardBuilder:
	public	ConstGuardBuilder{
	public:
		AGuardBuilder(uint64	period,float32	constant,uint64	offset);
		~AGuardBuilder();

		void	build(Code	*mdl,_Fact	*premise_pattern,_Fact	*cause_pattern,uint16	&write_index)	const;
	};
}


#endif