//	pattern_extractor.cpp
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

#include	"auto_focus.h"
#include	"reduction_job.h"
#include	"mem.h"
#include	"model_base.h"


namespace	r_exec{

	void	Input::FillBuffer(std::list<Input>	&buffer,_Fact	*input,bool	eligible_cause,_Fact	*abstracted_input,BindingMap	*bm){

		uint64	THZ=_Mem::Get()->get_tpx_time_horizon();
		uint64	now=Now();
		std::list<Input>::iterator	i;
		for(i=buffer.begin();i!=buffer.end();){	// trim the buffer down.

			if((*i).input->is_invalidated())
				i=buffer.erase(i);
			else{

				uint64	after=(*i).input->get_after();
				if(now-after>THZ)
					i=buffer.erase(i);
				else	// after this point all inputs are young enough.
					break;
			}
		}
		buffer.push_back(Input(input,eligible_cause,abstracted_input,bm));
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	TPX::TPX(AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):_Object(),auto_focus(auto_focus),target(Input(target,false,pattern,bindings)){
	}

	TPX::TPX(AutoFocusController	*auto_focus,_Fact	*target):_Object(),auto_focus(auto_focus){

		BindingMap	*bm=new	BindingMap();
		_Fact		*abstracted_target=(_Fact	*)BindingMap::Abstract(target,bm);

		this->target.input=target;
		this->target.abstraction=abstracted_target;
		this->target.bindings=bm;
	}

	TPX::TPX(const	TPX	*original):_Object(),auto_focus(original->auto_focus),target(original->target){
	}

	TPX::~TPX(){
	}

	bool	TPX::take_input(_Fact	*input,_Fact	*abstracted_input,BindingMap	*bm){

		return	bm->intersect(target.bindings);
	}

	void	TPX::signal(View	*input)	const{
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	_TPX::_TPX(AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):TPX(auto_focus,target,pattern,bindings){
	}

	_TPX::_TPX(AutoFocusController	*auto_focus,_Fact	*target):TPX(auto_focus,target){
	}

	_TPX::~_TPX(){
	}

	_Fact	*_TPX::find_f_icst(_Fact	*component,uint16	&component_index){

		uint16	opcode=component->get_reference(0)->code(0).asOpcode();
		if(opcode==Opcodes::Cmd	||	opcode==Opcodes::IMdl)	// cmds/imdls cannot be components of a cst.
			return	NULL;

		std::list<Input>::const_iterator	i;
		for(i=inputs.begin();i!=inputs.end();++i){

			Code	*candidate=(*i).input->get_reference(0);
			if(candidate->code(0).asOpcode()==Opcodes::ICst){

				ICST	*icst=(ICST	*)candidate;
				for(uint32	j=0;j<icst->components.size();++j){

					if(icst->components[j]==component){

						component_index=j;
						return	(_Fact	*)candidate;
					}
				}
			}
		}

		return	NULL;
	}

	_Fact	*_TPX::find_f_icst(_Fact	*component,uint16	&component_index,Code	*&cst){

		uint16	opcode=component->get_reference(0)->code(0).asOpcode();
		if(opcode==Opcodes::Cmd	||	opcode==Opcodes::IMdl){	// cmds/imdls cannot be components of a cst.

			cst=NULL;
			return	NULL;
		}

		std::list<Input>::const_iterator	i;
		for(i=inputs.begin();i!=inputs.end();++i){

			Code	*candidate=(*i).input->get_reference(0);
			if(candidate->code(0).asOpcode()==Opcodes::ICst){

				ICST	*icst=(ICST	*)candidate;
				for(uint32	j=0;j<icst->components.size();++j){

					if(icst->components[j]==component){

						component_index=j;
						cst=NULL;
						return	(*i).input;
					}
				}
			}
		}

		ICST	*icst=new	ICST();	// no icst found, try to identify a cst.

		for(i=inputs.begin();i!=inputs.end();++i){

			if(component==(*i).input){

				component_index=icst->components.size();
				icst->components.push_back(component);
			}else	if(component->match_timings_sync((*i).input))
				icst->components.push_back((*i).input);
		}

		if(icst->components.size()<=1){	// contains only the provided component.

			delete	icst;
			cst=NULL;
			return	NULL;
		}

		P<BindingMap>	bm=new	BindingMap();
		cst=build_cst(icst,bm,component);
		uint32	rc=cst->references_size();
		Fact	*f_icst=bm->build_f_ihlp(cst,Opcodes::ICst,false);
		return	f_icst;
	}

	Code	*_TPX::build_cst(ICST	*icst,BindingMap	*bm,_Fact	*component){

		_Fact	*abstracted_component=(_Fact	*)bm->abstract_object(component,false);

		Code	*cst=_Mem::Get()->build_object(Atom::CompositeState(Opcodes::Cst,CST_ARITY));

		for(uint16	i=0;i<icst->components.size();++i){	// reference patterns;

			if(icst->components[i]==component)
				cst->add_reference(abstracted_component);
			else
				cst->add_reference(bm->abstract_object(icst->components[i],true));
		}

		uint16	extent_index=CST_ARITY;

		cst->code(CST_TPL_ARGS)=Atom::IPointer(++extent_index);
		cst->code(extent_index)=Atom::Set(0);	// no tpl args.
		
		cst->code(CST_OBJS)=Atom::IPointer(++extent_index);
		cst->code(extent_index)=Atom::Set(icst->components.size());
		for(uint16	i=0;i<icst->components.size();++i)
			cst->code(++extent_index)=Atom::RPointer(i);

		cst->code(CST_FWD_GUARDS)=Atom::IPointer(++extent_index);
		cst->code(extent_index)=Atom::Set(0);	// no fwd guards.

		cst->code(CST_BWD_GUARDS)=Atom::IPointer(++extent_index);
		cst->code(extent_index)=Atom::Set(0);	// no bwd guards.

		cst->code(CST_OUT_GRPS)=Atom::IPointer(++extent_index);
		cst->code(extent_index)=Atom::Set(1);	// only one output group: the one the tpx lives in.
		cst->code(++extent_index)=Atom::RPointer(cst->references_size());

		cst->code(CST_ARITY)=Atom::Float(1);	// psln_thr.

		cst->add_reference(auto_focus->getView()->get_host());	// reference the output group.

		return	cst;
	}

	Code	*_TPX::build_mdl_head(BindingMap	*bm,uint16	tpl_arg_count,_Fact	*lhs,_Fact	*rhs,uint16	&write_index){

		Code	*mdl=_Mem::Get()->build_object(Atom::Model(Opcodes::Mdl,MDL_ARITY));

		mdl->add_reference(bm->abstract_object(lhs,false));	// reference lhs.
		mdl->add_reference(bm->abstract_object(rhs,false));	// reference rhs.

		write_index=MDL_ARITY;

		mdl->code(MDL_TPL_ARGS)=Atom::IPointer(++write_index);
		mdl->code(write_index)=Atom::Set(tpl_arg_count);
		for(uint16	i=0;i<tpl_arg_count;++i)
			mdl->code(++write_index)=Atom::VLPointer(i);
		
		mdl->code(MDL_OBJS)=Atom::IPointer(++write_index);
		mdl->code(write_index)=Atom::Set(2);
		mdl->code(++write_index)=Atom::RPointer(0);
		mdl->code(++write_index)=Atom::RPointer(1);

		return	mdl;
	}

	void	_TPX::build_mdl_tail(Code	*mdl,uint16	write_index){

		mdl->code(MDL_OUT_GRPS)=Atom::IPointer(++write_index);
		mdl->code(write_index)=Atom::Set(1);	// only one group: the one the tpx lives in.
		mdl->code(++write_index)=Atom::RPointer(2);

		mdl->code(MDL_STRENGTH)=Atom::Float(0);
		mdl->code(MDL_CNT)=Atom::Float(1);
		mdl->code(MDL_SR)=Atom::Float(1);
		mdl->code(MDL_DSR)=Atom::Float(1);
		mdl->code(MDL_ARITY)=Atom::Float(1);	// psln_thr.
		
		mdl->add_reference(auto_focus->getView()->get_host());	// reference the output group.
	}

	void	_TPX::inject_hlps()	const{

		std::list<P<Code> >::const_iterator	c;
		for(c=csts.begin();c!=csts.end();++c)
			_Mem::Get()->pack_hlp(*c);
		auto_focus->inject_hlps(csts);
		auto_focus->inject_hlps(mdls);
	}

	void	_TPX::inject_hlps(uint64	analysis_starting_time)	const{

		if(auto_focus->decompile_models()){

			std::list<P<Code> >	tmp;
			std::list<Input>::const_iterator	i;
			for(i=inputs.begin();i!=inputs.end();++i)
				tmp.push_back((Code	*)(*i).input);

			std::string	header("> from buffer -------------------\n\n");

			P<TDecompiler>	td=new	TDecompiler(1,header);
			td->add_objects(tmp);
			td->decompile();

			uint64	analysis_end=Now();
			uint32	d=analysis_end-analysis_starting_time;
			char	_timing[255];
			itoa(d,_timing,10);
			header=Time::ToString_seconds(Now()-Utils::GetTimeReference());
			std::string	s0=(" > ");
			s0+=get_header()+std::string(":production [");
			std::string	timing(_timing);
			std::string	s1("us] -------------------\n\n");
			header+=s0+timing+s1;

			td=new	TDecompiler(1,header);
			td->add_objects(mdls);
			inject_hlps();
			td->decompile();
		}else
			inject_hlps();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GTPX::GTPX(AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):_TPX(auto_focus,target,pattern,bindings){
	}

	GTPX::~GTPX(){
	}

	bool	GTPX::take_input(_Fact	*input,bool	eligible_input,_Fact	*abstracted_input,BindingMap	*bm){	// push new input in the time-controlled buffer; old inputs are in front.

		if(!bm->intersect(target.bindings))
			return	false;

		Input::FillBuffer(inputs,input,eligible_input,abstracted_input,bm);
		return	true;
	}

	void	GTPX::signal(View	*input)	const{	// will be erased from the AF map upon return. P<> kept in reduction job.

		if(((_Fact	*)input->object)->is_fact()){	// goal success.

			ReductionJob<GTPX>	*j=new	ReductionJob<GTPX>(new	View(input),(GTPX	*)this);
			_Mem::Get()->pushReductionJob(j);
		}
	}

	void	GTPX::reduce(r_exec::View	*input){

		uint64	analysis_starting_time=Now();

		_Fact	*consequent=(_Fact	*)input->object;	// goal success.

		P<GuardBuilder>	guard_builder;
		uint64	period;
		uint64	lhs_duration;
		uint64	rhs_duration;

		std::list<Input>::const_iterator	i;
		for(i=inputs.begin();i!=inputs.end();++i){

			if((*i).input->get_after()>=consequent->get_after())	// exhaust inputs younger than the consequent.
				continue;

			if((*i).input->get_reference(0)->code(0).asOpcode()==Opcodes::ICst)
				continue;	// components will be evaluated first, then the icst will be identified.

			Input	cause=*i;

			if(!cause.eligible_cause)
				continue;

			period=consequent->get_after()-cause.input->get_after();
			lhs_duration=cause.input->get_before()-cause.input->get_after();
			rhs_duration=consequent->get_before()-consequent->get_after();
			guard_builder=new	TimingGuardBuilder(period);	// TODO: use the durations.

			uint16	cause_index;
			Code	*new_cst;
			_Fact	*f_icst=find_f_icst(cause.input,cause_index);
			if(f_icst==NULL){

				if(!build_mdl(cause.input,consequent,guard_builder,period))
					return;
			}else{

				Code	*unpacked_cst;
				if(new_cst==NULL){

					Code	*cst=f_icst->get_reference(0)->get_reference(0);
					unpacked_cst=cst->get_reference(cst->references_size()-CST_HIDDEN_REFS);	// the cst is packed, retreive the pattern from the unpacked code.
				}else
					unpacked_cst=new_cst;

				_Fact	*cause_pattern=(_Fact	*)unpacked_cst->get_reference(cause_index);
				if(!build_mdl(f_icst,cause_pattern,consequent,guard_builder,period,new_cst))
					return;
			}

			inject_hlps(analysis_starting_time);
		}
	}

	bool	GTPX::build_mdl(_Fact	*cause,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period){
		
		P<BindingMap>	bm=new	BindingMap();

		uint16	write_index;
		P<Code>	m0=build_mdl_head(bm,0,cause,consequent,write_index);
		guard_builder->build(m0,NULL,cause,write_index);
		build_mdl_tail(m0,write_index);

		Code	*_m0=ModelBase::Get()->check_existence(m0);
		if(_m0==NULL)
			return	false;
		else	if(_m0==m0)
			mdls.push_back(m0);
	}

	bool	GTPX::build_mdl(_Fact	*f_icst,_Fact	*cause_pattern,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period,Code	*new_cst){

		P<BindingMap>	bm=new	BindingMap();

		uint16	write_index;
		P<Code>	m0=build_mdl_head(bm,0,f_icst,consequent,write_index);
		guard_builder->build(m0,NULL,cause_pattern,write_index);
		build_mdl_tail(m0,write_index);

		Code	*_m0=ModelBase::Get()->check_existence(m0);
		if(_m0==NULL)
			return	false;
		else	if(_m0==m0){

			if(new_cst)
				csts.push_back(new_cst);
			mdls.push_back(m0);
		}	// if m0 already exist, new_cst==NULL.
	}

	std::string	GTPX::get_header()	const{

		return	std::string("GTPX");
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PTPX::PTPX(AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):_TPX(auto_focus,target,pattern,bindings){
	}

	PTPX::~PTPX(){
	}

	void	PTPX::signal(View	*input)	const{	// will be erased from the AF map upon return. P<> kept in reduction job.

		if(((_Fact	*)input->object)->is_anti_fact()){	// prediction failure.

			ReductionJob<PTPX>	*j=new	ReductionJob<PTPX>(new	View(input),(PTPX	*)this);
			_Mem::Get()->pushReductionJob(j);
		}
	}

	void	PTPX::reduce(r_exec::View	*input){
		
		auto_focus->copy_cross_buffer(inputs);	// the cause of the prediction failure comes before the prediction.

		uint64	analysis_starting_time=Now();

		_Fact	*consequent=new	Fact((Fact	*)f_imdl);	// input->object is the prediction failure: ignore and consider |f->imdl instead.
		consequent->set_opposite();

		BindingMap	*end_bm;
		P<_Fact>	abstract_input=(_Fact	*)BindingMap::Abstract(consequent,end_bm);
		std::list<Input>::const_iterator	i;
		for(i=inputs.begin();i!=inputs.end();){	// filter out inputs irrelevant for the prediction.

			if(i->input->code(0).asOpcode()==Opcodes::Cmd)	// no cmds as req lhs (because no bwd-operational); prefer: cmd->effect, effect->imdl.
				i=inputs.erase(i);
			else	if(!i->bindings->intersect(end_bm))
				i=inputs.erase(i);
			else
				++i;
		}

		P<GuardBuilder>	guard_builder;
		uint64	period;
		uint64	lhs_duration;
		uint64	rhs_duration;

		for(i=inputs.begin();i!=inputs.end();++i){

			if((*i).input->get_after()>=consequent->get_after())	// exhaust inputs younger than the consequent.
				continue;

			if((*i).input->get_reference(0)->code(0).asOpcode()==Opcodes::ICst)
				continue;	// components will be evaluated first, then the icst will be identified.

			Input	cause=*i;

			if(!cause.eligible_cause)
				continue;

			period=consequent->get_after()-cause.input->get_after();
			lhs_duration=cause.input->get_before()-cause.input->get_after();
			rhs_duration=consequent->get_before()-consequent->get_after();
			guard_builder=new	TimingGuardBuilder(period);	// TODO: use the durations.

			uint16	cause_index;
			Code	*new_cst;
			_Fact	*f_icst=find_f_icst(cause.input,cause_index,new_cst);
			if(f_icst==NULL){

				if(!build_mdl(cause.input,consequent,guard_builder,period))
					return;
			}else{

				Code	*unpacked_cst;
				if(new_cst==NULL){

					Code	*cst=f_icst->get_reference(0)->get_reference(0);
					unpacked_cst=cst->get_reference(cst->references_size()-CST_HIDDEN_REFS);	// the cst is packed, retreive the pattern from the unpacked code.
				}else
					unpacked_cst=new_cst;

				_Fact	*cause_pattern=(_Fact	*)unpacked_cst->get_reference(cause_index);
				if(!build_mdl(f_icst,cause_pattern,consequent,guard_builder,period,new_cst))
					return;
			}

			inject_hlps(analysis_starting_time);
		}
	}

	bool	PTPX::build_mdl(_Fact	*cause,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period){

		P<BindingMap>	bm=new	BindingMap();

		uint16	write_index;
		P<Code>	m0=build_mdl_head(bm,0,cause,consequent,write_index);
		guard_builder->build(m0,NULL,cause,write_index);
		build_mdl_tail(m0,write_index);

		Code	*_m0=ModelBase::Get()->check_existence(m0);
		if(_m0==NULL)
			return	false;
		else	if(_m0==m0)
			mdls.push_back(m0);
	}

	bool	PTPX::build_mdl(_Fact	*f_icst,_Fact	*cause_pattern,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period,Code	*new_cst){

		P<BindingMap>	bm=new	BindingMap();

		uint16	write_index;
		P<Code>	m0=build_mdl_head(bm,0,f_icst,consequent,write_index);
		guard_builder->build(m0,NULL,cause_pattern,write_index);
		build_mdl_tail(m0,write_index);

		Code	*_m0=ModelBase::Get()->check_existence(m0);
		if(_m0==NULL)
			return	false;
		else	if(_m0==m0){

			if(new_cst)
				csts.push_back(new_cst);
			mdls.push_back(m0);
		}	// if m0 already exist, new_cst==NULL.
	}

	std::string	PTPX::get_header()	const{

		return	std::string("PTPX");
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CTPX::CTPX(AutoFocusController		*auto_focus,_Fact	*premise):_TPX(auto_focus,premise),stored_premise(false){
	}

	CTPX::~CTPX(){
	}

	void	CTPX::store_input(r_exec::View	*input){

		_Fact	*input_object=(_Fact	*)input->object;
		BindingMap	*bm=new	BindingMap();
		_Fact		*abstracted_input=(_Fact	*)BindingMap::Abstract(input_object,bm);
		Input	i(input_object,Input::IsEligibleCause(input),abstracted_input,bm);
		inputs.push_back(i);
		if(input_object==target.input)
			stored_premise=true;
	}

	void	CTPX::signal(r_exec::View	*input){

		View	*_view=new	View(input);	// controller not copied.
		ReductionJob<CTPX>	*j=new	ReductionJob<CTPX>(_view,this);	// holds a reference to this.
		_Mem::Get()->pushReductionJob(j);
	}

	void	CTPX::reduce(r_exec::View	*input){

		uint64	analysis_starting_time=Now();

		if(!stored_premise)
			inputs.push_back(target);

		_Fact	*consequent=(_Fact	*)input->object;

		BindingMap	*end_bm;
		P<_Fact>	abstract_input=(_Fact	*)BindingMap::Abstract(consequent,end_bm);
		std::list<Input>::const_iterator	i;
		for(i=inputs.begin();i!=inputs.end();){

			if(!i->bindings->intersect(end_bm))
				i=inputs.erase(i);
			else
				++i;
		}

		bool	need_guard;
		if(target.input->get_reference(0)->code(0).asOpcode()==Opcodes::MkVal)
			need_guard=target.input->get_reference(0)->code(MK_VAL_VALUE).isFloat();
		else
			need_guard=false;
		
		uint64			period=Utils::GetTimestamp<Code>(consequent,FACT_AFTER)-Utils::GetTimestamp<Code>(target.input,FACT_AFTER);	// sampling period.
		P<GuardBuilder>	guard_builder;

		for(i=inputs.begin();i!=inputs.end();++i){

			if(target.input==(*i).input)
				continue;

			if((*i).input->get_after()>=consequent->get_after())	// exhaust inputs younger than the consequent.
				continue;

			if((*i).input->get_reference(0)->code(0).asOpcode()==Opcodes::ICst)
				continue;	// components will be evaluated first, then the icst will be identified.

			Input	cause=*i;

			if(!cause.eligible_cause)
				continue;

			if(Utils::Synchronous(cause.input->get_after(),target.input->get_after()))	// cause in sync with the premise: abort.
				return;

			if(need_guard){
				
				if((guard_builder=find_guard_builder(cause.input,consequent,period))==NULL)
					continue;
			}else
				guard_builder=get_default_guard_builder(cause.input,consequent,period);

			uint16	cause_index;
			_Fact	*f_icst=find_f_icst(cause.input,cause_index);
			if(f_icst==NULL){	// the cause can never be the premise; m0:[premise.value premise.after premise.before][cause->consequent] and m1:[lhs1->imdl m0[...][...]] with lhs1 either the premise or an icst containing the premise.

				if(!build_mdl(cause.input,consequent,guard_builder,period))
					return;
			}else{

				Code	*cst=f_icst->get_reference(0)->get_reference(0)->get_reference(cst->references_size()-CST_HIDDEN_REFS);	// the cst is packed, retreive the pattern from the unpacked code.
				_Fact	*cause_pattern=(_Fact	*)cst->get_reference(cause_index);
				if(!build_mdl(f_icst,cause_pattern,consequent,guard_builder,period))	// m0:[premise.value premise.after premise.before][icst->consequent] and m1:[lhs1->imdl m0[...][...]] with lhs1 either the premise or an icst containing the premise.
					return;
			}

			inject_hlps(analysis_starting_time);
		}
	}

	GuardBuilder	*CTPX::get_default_guard_builder(_Fact	*cause,_Fact	*consequent,uint64	period){

		Code	*cause_payload=cause->get_reference(0);
		uint16	opcode=cause_payload->code(0).asOpcode();
		if(opcode==Opcodes::Cmd){

			uint64	offset=consequent->get_after()-cause->get_after();
			uint64	cmd_duration=cause->get_before()-cause->get_after();
			return	new	NoArgCmdGuardBuilder(period,offset,cmd_duration);
		}

		return	new	TimingGuardBuilder(period);
	}

	// 5 forms:
	// 0 - q1=q0+cmd_arg (if the cause is a cmd) or q1=q0*cmd_arg with q0!=0.
	// 1 - q1=q0+speed*period, with q1=consequent.value, q0=premise.value, speed=cause.value,
	// 3 - q1=q0+constant or q1=q0*constant with q0!=0.
	GuardBuilder	*CTPX::find_guard_builder(_Fact	*cause,_Fact	*consequent,uint64	period){

		Code	*cause_payload=cause->get_reference(0);
		uint16	opcode=cause_payload->code(0).asOpcode();
		if(opcode==Opcodes::Cmd){	// form 0.

			float32	q0=target.input->get_reference(0)->code(MK_VAL_VALUE).asFloat();
			float32	q1=consequent->get_reference(0)->code(MK_VAL_VALUE).asFloat();

			float32	searched_for=q1-q0;
			uint16	cmd_arg_set_index=cause_payload->code(CMD_ARGS).asIndex();
			uint16	cmd_arg_count=cause_payload->code(cmd_arg_set_index).getAtomCount();
			for(uint16	i=1;i<=cmd_arg_count;++i){

				Atom	s=cause_payload->code(cmd_arg_set_index+i);
				if(!s.isFloat())
					continue;
				float32	_s=s.asFloat();
				if(Utils::Equal(_s,searched_for))
					return	new	ACGuardBuilder(period,cmd_arg_set_index+i);
			}

			if(q0!=0){
				
				searched_for=q1/q0;
				for(uint16	i=cmd_arg_set_index+1;i<=cmd_arg_count;++i){

					Atom	s=cause_payload->code(i);
					if(!s.isFloat())
						continue;
					float32	_s=s.asFloat();
					if(Utils::Equal(_s,searched_for))
						return	new	MCGuardBuilder(period,i);
				}
			}
		}else	if(opcode==Opcodes::MkVal){

			Atom	s=cause_payload->code(MK_VAL_VALUE);
			if(s.isFloat()){

				float32	_s=s.asFloat();
				float32	q0=target.input->get_reference(0)->code(MK_VAL_VALUE).asFloat();
				float32	q1=consequent->get_reference(0)->code(MK_VAL_VALUE).asFloat();

				float32	searched_for=(q1-q0)/period;
				if(Utils::Equal(_s,searched_for)){	// form 1.

					uint64	offset=Utils::GetTimestamp<Code>(cause,FACT_AFTER)-Utils::GetTimestamp<Code>(target.input,FACT_AFTER);
					return	new	SGuardBuilder(period,period-offset);
				}

				if(q0!=0){	// form 2.

					uint64	offset=Utils::GetTimestamp<Code>(cause,FACT_AFTER)-Utils::GetTimestamp<Code>(target.input,FACT_AFTER);
					return	new	MGuardBuilder(period,q1/q0,offset);
				}

				uint64	offset=Utils::GetTimestamp<Code>(cause,FACT_AFTER)-Utils::GetTimestamp<Code>(target.input,FACT_AFTER);
				return	new	AGuardBuilder(period,q1-q0,offset);
			}
		}

		return	NULL;
	}

	// m0:[premise.value premise.after premise.before][cause->consequent].
	// m1:[icst->imdl m0[...][...]] with icst containing the premise.
	bool	CTPX::build_mdl(_Fact	*cause,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period){

		P<BindingMap>	bm=new	BindingMap();
		bm->init(target.input->get_reference(0),MK_VAL_VALUE);
		bm->init(target.input,FACT_AFTER);
		bm->init(target.input,FACT_BEFORE);

		uint16	write_index;
		P<Code>	m0=build_mdl_head(bm,3,cause,consequent,write_index);
		guard_builder->build(m0,NULL,cause,write_index);
		build_mdl_tail(m0,write_index);

		return	build_requirement(bm,m0,period);	// existence checks performed there.
	}

	// m0:[premise.value premise.after premise.before][icst->consequent] with icst containing the cause.
	// m1:[icst->imdl m0[...][...]] with icst containing the premise.
	bool	CTPX::build_mdl(_Fact	*f_icst,_Fact	*cause_pattern,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period){

		P<BindingMap>	bm=new	BindingMap();
		bm->init(target.input->get_reference(0),MK_VAL_VALUE);
		bm->init(target.input,FACT_AFTER);
		bm->init(target.input,FACT_BEFORE);

		uint16	write_index;
		Code	*m0=build_mdl_head(bm,3,f_icst,consequent,write_index);
		guard_builder->build(m0,NULL,cause_pattern,write_index);
		build_mdl_tail(m0,write_index);

		return	build_requirement(bm,m0,period);	// existence checks performed there.
	}

	bool	CTPX::build_requirement(BindingMap	*bm,Code	*m0,uint64	period){	// check for mdl existence at the same time (ModelBase::mdlCS-wise).

		uint16	premise_index;
		Code	*new_cst;
		_Fact	*f_icst=find_f_icst(target.input,premise_index,new_cst);
		if(f_icst==NULL)
			return	false;

		P<Fact>	f_im0=bm->build_f_ihlp(m0,Opcodes::IMdl,false);
		Utils::SetTimestamp<Code>(f_im0,FACT_AFTER,f_icst->get_after());
		Utils::SetTimestamp<Code>(f_im0,FACT_BEFORE,f_icst->get_before());
		
		Code	*unpacked_cst;
		if(new_cst==NULL){

			Code	*cst=f_icst->get_reference(0)->get_reference(0);
			unpacked_cst=cst->get_reference(cst->references_size()-CST_HIDDEN_REFS);	// the cst is packed, retreive the pattern from the unpacked code.
		}else
			unpacked_cst=new_cst;
		_Fact	*premise_pattern=(_Fact	*)unpacked_cst->get_reference(premise_index);

		P<BindingMap>	_bm=new	BindingMap();

		uint16	write_index;
		P<Code>	m1=build_mdl_head(_bm,0,f_icst,f_im0,write_index);
		P<GuardBuilder>	guard_builder=new	GuardBuilder();
		guard_builder->build(m1,premise_pattern,NULL,write_index);
		build_mdl_tail(m1,write_index);

		Code	*_m0;
		Code	*_m1;
		ModelBase::Get()->check_existence(m0,m1,_m0,_m1);
		if(_m1==NULL)
			return	false;
		else	if(_m1==m1){

			if(_m0==NULL)
				return	false;
			else	if(_m0==m0)
				mdls.push_back(m0);
			if(new_cst!=NULL)
				csts.push_back(new_cst);
			mdls.push_back(m1);
		}	// if m1 alrady exists, new_cst==NULL.
		return	true;
	}

	std::string	CTPX::get_header()	const{

		return	std::string("CTPX");
	}
}