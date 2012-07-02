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
#include	"black_list.h"


namespace	r_exec{

	TPX::TPX(const	AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):_Object(),auto_focus(auto_focus),target(Input(target,pattern,bindings)){
	}

	TPX::TPX(const	TPX	*original):_Object(),auto_focus(original->auto_focus),target(Input(original->target.input,original->target.abstraction,original->target.bindings)){
	}

	TPX::~TPX(){
	}

	bool	TPX::take_input(Input	*input){

		return	input->bindings->intersect(target.bindings);
	}

	void	TPX::signal(View	*input)	const{
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	_TPX::_TPX(const	AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):TPX(auto_focus,target,pattern,bindings){
	}

	_TPX::~_TPX(){
	}

	bool	_TPX::take_input(Input	*input){	// push new input in the time-controlled buffer; old inputs are in front.

		if(!input->bindings->intersect(target.bindings))
			return	false;

		uint64	now=Now();
		uint64	THZ=_Mem::Get()->get_tpx_time_horizon();

		std::list<Input>::iterator	i;
		for(i=inputs.begin();i!=inputs.end();){	// trim the buffer down.

			if((*i).input->is_invalidated())
				i=inputs.erase(i);
			else{

				uint64	after=Utils::GetTimestamp<Code>((*i).input,FACT_AFTER);
				if(now-after>THZ)
					i=inputs.erase(i);
				else	// after this point all inputs are young enough.
					break;
			}
		}
		inputs.push_back(*input);
	}


	void	_TPX::reduce(View	*input){

		build_hlps();

		std::list<P<Code> >::const_iterator	hlp;
		for(hlp=hlps.begin();hlp!=hlps.end();++hlp){

			_Mem::Get()->pack_hlp(*hlp);
			//auto_focus->inject_hlp(*hlp);

			if(auto_focus->decompile_models()){

				std::string	header("> TPX:mdl -------------------\n\n");
				P<TDecompiler>	td=new	TDecompiler(1,header);
				td->add_object(*hlp);
				td->decompile();
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GTPX::GTPX(const	AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):_TPX(auto_focus,target,pattern,bindings){
	}

	GTPX::~GTPX(){
	}

	void	GTPX::signal(View	*input)	const{	// will be erased from the AF map upon return. P<> kept in reduction job.

		if(((_Fact	*)input->object)->is_fact()){	// goal success.

			ReductionJob<GTPX>	*j=new	ReductionJob<GTPX>(new	View(input),(GTPX	*)this);
			_Mem::Get()->pushReductionJob(j);
		}
	}

	void	GTPX::build_hlps(){

		// TODO. success->p->f and f==target, means that a mdl can solve the goal: abort.
		// else: take the input before the goal success and build a model; in the process, identify new CST if LHS and inject.
		// if selected input==success->f->pred->x by mdl M, use f->imdl M as the lhs of the new model.
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PTPX::PTPX(const	AutoFocusController	*auto_focus,_Fact	*target,_Fact	*pattern,BindingMap	*bindings):_TPX(auto_focus,target,pattern,bindings){
	}

	PTPX::~PTPX(){
	}

	void	PTPX::signal(View	*input)	const{	// will be erased from the AF map upon return. P<> kept in reduction job.

		if(input->object->code(0).asOpcode()==Opcodes::AntiFact){	// prediction failure.

			ReductionJob<PTPX>	*j=new	ReductionJob<PTPX>(new	View(input),(PTPX	*)this);
			_Mem::Get()->pushReductionJob(j);
		}
	}


	void	PTPX::build_hlps(){

		// TODO. success->p->f and f==target, means that a mdl can anticipate the failure of the pred: abort.
		// else: take the input before the pred failure and build a model (s_r); in the process, identify new CST if LHS and inject.
		// if selected input==success->f->pred->x by mdl M, use f->imdl M as the lhs of the new model.
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	CTPX::CTPX(const	AutoFocusController		*auto_focus,_Fact	*premise):_Object(),auto_focus(auto_focus),premise(premise),stored_premise(false){
	}

	CTPX::~CTPX(){
	}

	void	CTPX::store_input(_Fact	*input){

		raw_inputs.push_back(input);
		if(input==premise)
			stored_premise=true;
	}

	void	CTPX::signal(r_exec::View	*input){

		View	*_view=new	View(input);	// controller not copied.
		ReductionJob<CTPX>	*j=new	ReductionJob<CTPX>(_view,this);	// holds a reference on this.
		_Mem::Get()->pushReductionJob(j);
	}

	void	CTPX::reduce(r_exec::View	*input){

		uint64	begin;
		if(auto_focus->decompile_models())
			begin=Now();

		if(!stored_premise)
			raw_inputs.push_front((_Fact	*)premise);

		_Fact	*consequent=(_Fact	*)input->object;

		BindingMap	*end_bm;
		P<_Fact>	abstract_input=(_Fact	*)BindingMap::Abstract(consequent,end_bm);
		std::list<P<Code> >::const_iterator	r_i;
		for(r_i=raw_inputs.begin();r_i!=raw_inputs.end();){

			BindingMap	*bm;
			P<_Fact>	abstract_input=(_Fact	*)BindingMap::Abstract(*r_i,bm);
			if(end_bm->intersect(bm)){

				inputs.push_front(Input(*r_i,abstract_input,bm));
				++r_i;
			}else{

				delete	bm;
				r_i=raw_inputs.erase(r_i);
			}
		}
		raw_inputs.push_back(consequent);
		inputs.push_front(Input(consequent,abstract_input,end_bm));

		bool	need_guard=false;
		if(premise->get_reference(0)->code(0).asOpcode()==Opcodes::MkVal)
			need_guard=premise->get_reference(0)->code(MK_VAL_VALUE).isFloat();
		
		uint64			period=Utils::GetTimestamp<Code>(consequent,FACT_AFTER)-Utils::GetTimestamp<Code>(premise,FACT_AFTER);	// sampling period.
		GuardBuilder	*guard_builder;
		uint64			time_tolerance=Utils::GetTimeTolerance();

		std::list<Input>::const_iterator	i=inputs.begin();	// consequent.
		for(++i;i!=inputs.end();++i){	// start from the input following the consequent; build all possible models.

			if(premise==(*i).input)
				return;

			if((*i).input->get_after()>=consequent->get_after())	// exhaust inputs younger than the consequent.
				continue;

			if((*i).input->get_reference(0)->code(0).asOpcode()==Opcodes::ICst)
				continue;	// components will be evaluated first, then the icst will be identified.

			Input	cause=*i;

			if(cause.input->get_after()<=premise->get_after()+time_tolerance)	// cause in sync with the premise: abort.
				return;

			if(need_guard){
				
				if(!find_guard(cause.input,consequent,period,guard_builder))
					continue;
			}else
				guard_builder=new	TimingGuardBuilder(period);

			std::list<P<Code> >	hlps;

			uint16	cause_index;
			_Fact	*f_icst=find_f_icst(cause.input,cause_index);
			if(f_icst==NULL){	// the cause can never be the premise; m0:[premise.value premise.after premise.before][cause->consequent] and m1:[lhs1->imdl m0[...][...]] with lhs1 either the premise or an icst containing the premise.

				if(!build_mdl(cause.input,consequent,guard_builder,period,hlps))
					return;
			}else{

				Code	*cst=f_icst->get_reference(0)->get_reference(0)->get_reference(cst->references_size()-CST_HIDDEN_REFS);	// the cst is packed, retreive the pattern from the unpacked code.
				_Fact	*cause_pattern=(_Fact	*)cst->get_reference(cause_index);
				if(!build_mdl(f_icst,cause_pattern,consequent,guard_builder,period,hlps))	// m0:[premise.value premise.after premise.before][icst->consequent] and m1:[lhs1->imdl m0[...][...]] with lhs1 either the premise or an icst containing the premise.
					return;
			}

			if(auto_focus->decompile_models()){

				std::string	header("> CTPX:buffer -------------------\n\n");
				P<TDecompiler>	td=new	TDecompiler(1,header);
				td->add_objects(raw_inputs);
				td->decompile();

				uint64	end=Now();
				uint32	d=end-begin;
				char	_timing[255];
				itoa(d,_timing,10);
				header=Time::ToString_seconds(Now()-Utils::GetTimeReference());
				std::string	s0=( "> CTPX:production [");
				std::string	timing(_timing);
				std::string	s1("us]-------------------\n\n");
				header+=s0+timing+s1;
				td=new	TDecompiler(1,header);

				std::list<P<Code> >::const_iterator	hlp;
				for(hlp=hlps.begin();hlp!=hlps.end();++hlp){

					_Mem::Get()->pack_hlp(*hlp);
					td->add_object(*hlp);
				}

				auto_focus->inject_hlps(hlps);
				td->decompile();
			}else{

				std::list<P<Code> >::const_iterator	hlp;
				for(hlp=hlps.begin();hlp!=hlps.end();++hlp)
					_Mem::Get()->pack_hlp(*hlp);

				auto_focus->inject_hlps(hlps);
			}
		}
	}

	// 2 forms:
	// 0 - q1=q0+cmd_arg (if the cause is a cmd) or q1=q0*cmd_arg.
	// 1 - q1=q0+speed*period, with q1=consequent.value, q0=premise.value, speed=cause.value,
	bool	CTPX::find_guard(_Fact	*cause,_Fact	*consequent,uint64	period,GuardBuilder	*&guard_builder){

		Code	*cause_payload=cause->get_reference(0);
		uint16	opcode=cause_payload->code(0).asOpcode();
		if(opcode==Opcodes::Cmd){	// form 0.

			float32	q0=premise->get_reference(0)->code(MK_VAL_VALUE).asFloat();
			float32	q1=consequent->get_reference(0)->code(MK_VAL_VALUE).asFloat();

			float32	searched_for=q1-q0;
			uint16	cmd_arg_set_index=cause_payload->code(CMD_ARGS).asIndex();
			uint16	cmd_arg_count=cause_payload->code(cmd_arg_set_index).getAtomCount();
			for(uint16	i=1;i<=cmd_arg_count;++i){

				Atom	s=cause_payload->code(cmd_arg_set_index+i);
				if(!s.isFloat())
					continue;
				float32	_s=s.asFloat();
				if(Utils::Equal(_s,searched_for)){

					guard_builder=new	ACGuardBuilder(period,cmd_arg_set_index+i);
					return	true;
				}
			}

			if(q0!=0){
				
				searched_for=q1/q0;
				for(uint16	i=cmd_arg_set_index+1;i<=cmd_arg_count;++i){

					Atom	s=cause_payload->code(i);
					if(!s.isFloat())
						continue;
					float32	_s=s.asFloat();
					if(Utils::Equal(_s,searched_for)){

						guard_builder=new	MCGuardBuilder(period,i);
						return	true;
					}
				}
			}
		}else	if(opcode==Opcodes::MkVal){	// form 1.

			Atom	s=cause_payload->code(MK_VAL_VALUE);
			if(s.isFloat()){

				float32	_s=s.asFloat();
				float32	q0=premise->get_reference(0)->code(MK_VAL_VALUE).asFloat();
				float32	q1=consequent->get_reference(0)->code(MK_VAL_VALUE).asFloat();

				float32	searched_for=(q1-q0)/period;
				if(Utils::Equal(_s,searched_for)){

					uint64	offset=Utils::GetTimestamp<Code>(cause,FACT_AFTER)-Utils::GetTimestamp<Code>(premise,FACT_AFTER);
					guard_builder=new	SGuardBuilder(period,period-offset);
					return	true;
				}
			}
		}

		guard_builder=NULL;
		return	false;
	}

	// m0:[premise.value premise.after premise.before][cause->consequent].
	// m1:[icst->imdl m0[...][...]] with icst containing the premise.
	bool	CTPX::build_mdl(_Fact	*cause,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period,std::list<P<Code> >	&hlps){

		P<GuardBuilder>	eraser=guard_builder;
		P<BindingMap>	bm=new	BindingMap();
		bm->init(premise->get_reference(0),MK_VAL_VALUE);
		bm->init(premise,FACT_AFTER);
		bm->init(premise,FACT_BEFORE);

		Code	*m0=build_mdl(cause,consequent,guard_builder,bm);
		if(BlackList::Get()->contains(m0))
			return	false;
		hlps.push_back(m0);

		Fact	*f_im0=bm->build_f_ihlp(m0,Opcodes::IMdl,false);
		Utils::SetTimestamp<Code>(f_im0,FACT_AFTER,premise->get_after());
		Utils::SetTimestamp<Code>(f_im0,FACT_BEFORE,premise->get_before());

		uint16	premise_index;
		Code	*new_cst;
		_Fact	*f_icst=find_f_icst(premise,premise_index,new_cst);
		if(f_icst==NULL)
			return	false;	//m1=build_mdl(premise,f_im0,bm,period);
		
		Code	*unpacked_cst;
		if(new_cst==NULL){

			Code	*cst=f_icst->get_reference(0)->get_reference(0);
			unpacked_cst=cst->get_reference(cst->references_size()-CST_HIDDEN_REFS);	// the cst is packed, retreive the pattern from the unpacked code.
		}else
			unpacked_cst=new_cst;
		bm=new	BindingMap();
		_Fact	*premise_pattern=(_Fact	*)unpacked_cst->get_reference(premise_index);
		Code	*m1=build_mdl(f_icst,premise_pattern,f_im0,bm,period);
		if(BlackList::Get()->contains(m1))
			return	false;

		if(new_cst!=NULL)
			hlps.push_front(new_cst);
		hlps.push_back(m1);
		return	true;
	}

	// m0:[premise.value premise.after premise.before][icst->consequent] with icst containing the cause.
	// m1:[icst->imdl m0[...][...]] with icst containing the premise.
	bool	CTPX::build_mdl(_Fact	*f_icst,_Fact	*cause_pattern,_Fact	*consequent,GuardBuilder	*guard_builder,uint64	period,std::list<P<Code> >	&hlps){

		P<GuardBuilder>	eraser=guard_builder;
		P<BindingMap>	bm=new	BindingMap();
		bm->init(premise->get_reference(0),MK_VAL_VALUE);
		bm->init(premise,FACT_AFTER);
		bm->init(premise,FACT_BEFORE);

		Code	*m0=build_mdl(f_icst,cause_pattern,consequent,guard_builder,bm);
		if(BlackList::Get()->contains(m0))
			return	false;
		hlps.push_back(m0);

		Fact	*f_im0=bm->build_f_ihlp(m0,Opcodes::IMdl,false);
		Utils::SetTimestamp<Code>(f_im0,FACT_AFTER,premise->get_after());
		Utils::SetTimestamp<Code>(f_im0,FACT_BEFORE,premise->get_before());

		uint16	premise_index;
		Code	*cst;
		_Fact	*f_icst1=find_f_icst(premise,premise_index,cst);
		if(f_icst==NULL)
			return	false;	//m1=build_mdl(premise,f_im0,bm,period);
		
		Code	*unpacked_cst;
		if(cst==NULL)
			unpacked_cst=f_icst->get_reference(0)->get_reference(0)->get_reference(cst->references_size()-CST_HIDDEN_REFS);	// the cst is packed, retreive the pattern from the unpacked code.
		else
			unpacked_cst=cst;
		bm=new	BindingMap();
		_Fact	*premise_pattern=(_Fact	*)unpacked_cst->get_reference(premise_index);
		Code	*m1=build_mdl(f_icst1,premise_pattern,f_im0,bm,period);
		if(BlackList::Get()->contains(m1))
			return	false;
		
		if(cst!=NULL)
			hlps.push_front(cst);
		hlps.push_back(m1);
		return	true;
	}

	// m0:[premise.value premise.after premise.before][cause->consequent].
	Code	*CTPX::build_mdl(_Fact	*lhs,_Fact	*rhs,GuardBuilder	*guard_builder,BindingMap	*bm){

		uint16	write_index;
		Code	*mdl=build_mdl_head(bm,3,lhs,rhs,write_index);

		guard_builder->build(mdl,NULL,lhs,write_index);
		
		build_mdl_tail(mdl,write_index);

		return	mdl;
	}

	// m0:[premise.value premise.after premise.before][icst->consequent] with icst containing the cause.
	Code	*CTPX::build_mdl(_Fact	*lhs_f_icst,_Fact	*cause_pattern,_Fact	*rhs,GuardBuilder	*guard_builder,BindingMap	*bm){

		uint16	write_index;
		Code	*mdl=build_mdl_head(bm,3,lhs_f_icst,rhs,write_index);

		guard_builder->build(mdl,NULL,cause_pattern,write_index);
		
		build_mdl_tail(mdl,write_index);

		return	mdl;
	}

	// m1:[premise->im0].
	Code	*CTPX::build_mdl(_Fact	*lhs,_Fact	*rhs,BindingMap	*bm,uint64	period){

		uint16	write_index;
		Code	*mdl=build_mdl_head(bm,0,lhs,rhs,write_index);

		P<GuardBuilder>	guard_builder=new	GuardBuilder();
		guard_builder->build(mdl,NULL,NULL,write_index);
		
		build_mdl_tail(mdl,write_index);

		return	mdl;
	}

	// m1:[icst->im0] with icst containing the premise.
	Code	*CTPX::build_mdl(_Fact	*lhs_f_icst,_Fact	*premise_pattern,_Fact	*rhs,BindingMap	*bm,uint64	period){

		uint16	write_index;
		Code	*mdl=build_mdl_head(bm,0,lhs_f_icst,rhs,write_index);

		P<GuardBuilder>	guard_builder=new	GuardBuilder();
		guard_builder->build(mdl,premise_pattern,NULL,write_index);
		
		build_mdl_tail(mdl,write_index);

		return	mdl;
	}

	Code	*CTPX::build_mdl_head(BindingMap	*bm,uint16	tpl_arg_count,_Fact	*lhs,_Fact	*rhs,uint16	&write_index){

		Code	*mdl=_Mem::Get()->build_object(Atom::Model(Opcodes::Mdl,MDL_ARITY));

		mdl->add_reference(bm->abstract_object(lhs));	// reference lhs.
		mdl->add_reference(bm->abstract_object(rhs));	// reference rhs.

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

	void	CTPX::build_mdl_tail(Code	*mdl,uint16	write_index){

		mdl->code(MDL_OUT_GRPS)=Atom::IPointer(++write_index);
		mdl->code(write_index)=Atom::Set(1);	// only one group: the one the tpx lives in.
		mdl->code(++write_index)=Atom::RPointer(2);

		mdl->code(MDL_STRENGTH)=Atom::Float(1);
		mdl->code(MDL_CNT)=Atom::Float(1);
		mdl->code(MDL_SR)=Atom::Float(1);
		mdl->code(MDL_DSR)=Atom::Float(1);
		mdl->code(MDL_ARITY)=Atom::Float(1);	// psln_thr.
		
		mdl->add_reference(auto_focus->getView()->get_host());	// reference the output group.
	}

	_Fact	*CTPX::find_f_icst(_Fact	*component,uint16	&component_index){

		if(component->get_reference(0)->code(0).asOpcode()==Opcodes::Cmd)	// cmd cannot be components of a cst.
			return	NULL;

		std::list<Input>::const_iterator	i=inputs.begin();	// consequent.
		for(++i;i!=inputs.end();++i){

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

	_Fact	*CTPX::find_f_icst(_Fact	*component,uint16	&component_index,Code	*&cst){

		if(component->get_reference(0)->code(0).asOpcode()==Opcodes::Cmd){	// cmd cannot be components of a cst.

			cst=NULL;
			return	NULL;
		}

		std::list<Input>::const_iterator	i=inputs.begin();	// consequent.
		for(++i;i!=inputs.end();++i){

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

		i=inputs.begin();	// consequent.
		for(++i;i!=inputs.end();++i){

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
		cst=build_cst(icst,bm);
		uint32	rc=cst->references_size();
		Fact	*f_icst=bm->build_f_ihlp(cst,Opcodes::ICst,false);
		return	f_icst;
	}

	Code	*CTPX::build_cst(ICST	*icst,BindingMap	*bm){

		Code	*cst=_Mem::Get()->build_object(Atom::CompositeState(Opcodes::Cst,CST_ARITY));

		for(uint16	i=0;i<icst->components.size();++i)	// reference patterns;
			cst->add_reference(bm->abstract_object(icst->components[i]));

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
		cst->code(extent_index)=Atom::Set(1);	// only one group: the one the tpx lives in.
		cst->code(++extent_index)=Atom::RPointer(cst->references_size());

		cst->code(CST_ARITY)=Atom::Float(1);	// psln_thr.
		
		cst->add_reference(auto_focus->getView()->get_host());	// reference the output group.

		return	cst;
	}
}