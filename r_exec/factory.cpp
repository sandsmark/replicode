//	factory.cpp
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

#include	"factory.h"
#include	"mdl_controller.h"
#include	"mem.h"


namespace	r_exec{

	MkNew::MkNew(r_code::Mem	*m,Code	*object):LObject(m){

		uint16	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkNew,2);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
		set_reference(0,object);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkLowRes::MkLowRes(r_code::Mem	*m,Code	*object):LObject(m){

		uint16	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkLowRes,2);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
		set_reference(0,object);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkLowSln::MkLowSln(r_code::Mem	*m,Code	*object):LObject(m){

		uint16	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkLowSln,2);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
		set_reference(0,object);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkHighSln::MkHighSln(r_code::Mem	*m,Code	*object):LObject(m){

		uint16	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkHighSln,2);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
		set_reference(0,object);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkLowAct::MkLowAct(r_code::Mem	*m,Code	*object):LObject(m){

		uint16	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkLowAct,2);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
		set_reference(0,object);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkHighAct::MkHighAct(r_code::Mem	*m,Code	*object):LObject(m){

		uint16	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkHighAct,2);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
		set_reference(0,object);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkSlnChg::MkSlnChg(r_code::Mem	*m,Code	*object,float32	value):LObject(m){

		uint16	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkSlnChg,3);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::Float(value);	//	change.
		code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
		set_reference(0,object);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkActChg::MkActChg(r_code::Mem	*m,Code	*object,float32	value):LObject(m){

		uint16	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkActChg,3);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::Float(value);	//	change.
		code(write_index++)=r_code::Atom::Float(0);		//	psln_thr.
		set_reference(0,object);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	_Fact::_Fact():LObject(){
	}

	_Fact::_Fact(SysObject	*source):LObject(source){
	}

	_Fact::_Fact(_Fact	*f){

		for(uint16	i=0;i<f->code_size();++i)
			code(i)=f->code(i);
		for(uint16	i=0;i<f->references_size();++i)
			add_reference(f->get_reference(i));
	}

	_Fact::_Fact(uint16	opcode,Code	*object,uint64	after,uint64	before,float32	confidence,float32	psln_thr):LObject(){

		code(0)=Atom::Object(opcode,FACT_ARITY);
		code(FACT_OBJ)=Atom::RPointer(0);
		code(FACT_AFTER)=Atom::IPointer(FACT_ARITY+1);
		code(FACT_BEFORE)=Atom::IPointer(FACT_ARITY+4);
		code(FACT_CFD)=Atom::Float(confidence);
		code(FACT_ARITY)=Atom::Float(psln_thr);
		Utils::SetTimestamp<Code>(this,FACT_AFTER,after);
		Utils::SetTimestamp<Code>(this,FACT_BEFORE,before);
		add_reference(object);
	}

	inline	bool	_Fact::is_fact()	const{

		return	(code(0).asOpcode()==Opcodes::Fact);
	}

	inline	bool	_Fact::is_anti_fact()	const{

		return	(code(0).asOpcode()==Opcodes::AntiFact);
	}

	bool	_Fact::is_invalidated(){

		if(LObject::is_invalidated())
			return	true;
		if(get_reference(0)->is_invalidated()){

			invalidate();
			return	true;
		}
		return	false;
	}

	bool	_Fact::match_timings_overlap(const	_Fact	*evidence)	const{

		uint64	after=get_after();
		uint64	before=get_before();
		uint64	e_after=evidence->get_after();
		uint64	e_before=evidence->get_before();

		return	!(e_after>before	||	e_before<after);
	}

	bool	_Fact::match_timings_inclusive(const	_Fact	*evidence)	const{

		uint64	after=get_after();
		uint64	before=get_before();
		uint64	e_after=evidence->get_after();
		uint64	e_before=evidence->get_before();

		return	(e_after<=after	&&	e_before>=before);
	}

	bool	_Fact::Match(const	Code	*lhs,uint16	lhs_base_index,uint16	lhs_index,const	Code	*rhs,uint16	rhs_index,uint16	lhs_arity){

		uint16	lhs_full_index=lhs_base_index+lhs_index;
		Atom	lhs_atom=lhs->code(lhs_full_index);
		Atom	rhs_atom=rhs->code(rhs_index);
		switch(lhs_atom.getDescriptor()){
		case	Atom::T_WILDCARD:
		case	Atom::WILDCARD:
		case	Atom::VL_PTR:
			break;
		case	Atom::I_PTR:
			switch(rhs_atom.getDescriptor()){
			case	Atom::I_PTR:
				if(!MatchStructure(lhs,lhs_atom.asIndex(),0,rhs,rhs_atom.asIndex()))
					return	false;
				break;
			case	Atom::T_WILDCARD:
			case	Atom::WILDCARD:
				break;
			default:
				return	false;
			}
			break;
		case	Atom::R_PTR:
			switch(rhs_atom.getDescriptor()){
			case	Atom::R_PTR:
				if(!MatchObject(lhs->get_reference(lhs_atom.asIndex()),rhs->get_reference(rhs_atom.asIndex())))
					return	false;
				break;
			case	Atom::T_WILDCARD:
			case	Atom::WILDCARD:
				break;
			default:
				return	false;
			}
			break;
		default:
			switch(rhs_atom.getDescriptor()){
			case	Atom::T_WILDCARD:
			case	Atom::WILDCARD:
				break;
			default:
				if(!MatchAtom(lhs_atom,rhs_atom))
					return	false;
				break;
			}
			break;
		}

		if(lhs_index==lhs_arity)
			return	true;
		return	Match(lhs,lhs_base_index,lhs_index+1,rhs,rhs_index+1,lhs_arity);
	}

	bool	_Fact::MatchAtom(Atom	lhs,Atom	rhs){

		if(lhs==rhs)
			return	true;

		if(lhs.isFloat()	&&	rhs.isFloat()){

			if(abs(lhs.asFloat()-rhs.asFloat())<=_Mem::Get()->get_float_tolerance())
				return	true;
		}
		return	false;
	}

	bool	_Fact::MatchStructure(const	Code	*lhs,uint16	lhs_base_index,uint16	lhs_index,const	Code	*rhs,uint16	rhs_index){

		uint16	lhs_full_index=lhs_base_index+lhs_index;
		Atom	lhs_atom=lhs->code(lhs_full_index);
		Atom	rhs_atom=rhs->code(rhs_index);
		if(lhs_atom!=rhs_atom)
			return	false;
		uint16	arity=lhs_atom.getAtomCount();
		if(arity==0)	// empty sets.
			return	true;
		if(lhs_atom.getDescriptor()==Atom::TIMESTAMP)
			return	abs((int32)(Utils::GetTimestamp(&lhs->code(lhs_full_index))-Utils::GetTimestamp(&rhs->code(rhs_index))))<=_Mem::Get()->get_time_tolerance();
		return	Match(lhs,lhs_base_index,lhs_index+1,rhs,rhs_index+1,arity);
	}

	bool	_Fact::MatchObject(const	Code	*lhs,const	Code	*rhs){

		if(lhs->code(0)!=rhs->code(0))
			return	false;
		uint16	lhs_opcode=lhs->code(0).asOpcode();
		if(	lhs_opcode==Opcodes::Ent	||
			lhs_opcode==Opcodes::Ont	||
			lhs_opcode==Opcodes::Mdl	||
			lhs_opcode==Opcodes::Cst)
			return	lhs==rhs;
		return	Match(lhs,0,1,rhs,1,lhs->code(0).getAtomCount());
	}

	bool	_Fact::CounterEvidence(const	Code *lhs,const	Code	*rhs){

		uint16	opcode=lhs->code(0).asOpcode();
		if(	opcode==Opcodes::Ent	||
			opcode==Opcodes::Ont	||
			opcode==Opcodes::IMdl)
			return	false;
		if(lhs->code(0)!=rhs->code(0))
			return	false;
		if(opcode==Opcodes::MkVal){

			if(	lhs->get_reference(MK_VAL_OBJ_REF)==rhs->get_reference(MK_VAL_OBJ_REF)	&&
				lhs->get_reference(MK_VAL_ATTR_REF)==rhs->get_reference(MK_VAL_ATTR_REF)){	// same attribute for the same object; value: r_ptr, atomic value or structure.
				
				Atom	atom=lhs->code(MK_VAL_VALUE);
				uint16	desc=atom.getDescriptor();
				if(desc!=rhs->code(MK_VAL_VALUE).getDescriptor())	// values of different types.
					return	false;
				switch(desc){
				case	Atom::T_WILDCARD:
				case	Atom::WILDCARD:
					return	false;
				case	Atom::R_PTR:
					return	!MatchObject(lhs->get_reference(MK_VAL_VALUE_REF),rhs->get_reference(MK_VAL_VALUE_REF));
				case	Atom::I_PTR:
					return	!MatchStructure(lhs,MK_VAL_VALUE,atom.asIndex(),rhs,rhs->code(MK_VAL_VALUE).asIndex());
				default:
					return	!MatchAtom(atom,rhs->code(MK_VAL_VALUE));
				}
			}
		}else	if(opcode==Opcodes::ICst){

			if(lhs->get_reference(0)!=rhs->get_reference(0))	// check if the icsts instantiate the same cst.
				return	false;

			for(uint32	i=0;i<((ICST	*)lhs)->components.size();++i){	// compare all components 2 by 2.

				if(CounterEvidence(((ICST	*)lhs)->components[i],((ICST	*)rhs)->components[i]))
					return	true;
			}
		}

		return	false;
	}

	MatchResult	_Fact::is_evidence(const	_Fact *target) const{

		if(MatchObject(get_reference(0),target->get_reference(0))){

			MatchResult	r;
			if(target->code(0)==code(0))
				r=MATCH_SUCCESS_POSITIVE;
			else
				r=MATCH_SUCCESS_NEGATIVE;

			if(target->match_timings_overlap(this))
				return	r;
		}else	if(target->code(0)==code(0)){	// check for a counter-evidence only if bothe lhs and rhs are of the same kind of fact.
				
			if(target->match_timings_inclusive(this)){	// check timings first as this is less expensive that the counter-evidence check.
			
				if(CounterEvidence(get_reference(0),target->get_reference(0)))
					return	MATCH_SUCCESS_NEGATIVE;
			}
		}
		return	MATCH_FAILURE;
	}

	inline	float32	_Fact::get_cfd()	const{

		return	code(FACT_CFD).asFloat();
	}

	inline	void	_Fact::set_cfd(float32	cfd){

		code(FACT_CFD)=Atom::Float(cfd);
	}

	inline	Pred	*_Fact::get_pred()	const{

		Code	*pred=get_reference(0);
		if(pred->code(0).asOpcode()==Opcodes::Pred)
			return	(Pred	*)pred;
		return	NULL;
	}

	inline	Goal	*_Fact::get_goal()	const{

		Code	*goal=get_reference(0);
		if(goal->code(0).asOpcode()==Opcodes::Goal)
			return	(Goal	*)goal;
		return	NULL;
	}

	uint64	_Fact::get_after()	const{

		return	Utils::GetTimestamp<Code>(this,FACT_AFTER);
	}

	uint64	_Fact::get_before()	const{

		return	Utils::GetTimestamp<Code>(this,FACT_BEFORE);
	}

	////////////////////////////////////////////////////////////////

	Fact::Fact():_Fact(){
	}

	Fact::Fact(SysObject	*source):_Fact(source){
	}

	Fact::Fact(Fact	*f):_Fact(f){
	}

	Fact::Fact(Code	*object,uint64	after,uint64	before,float32	confidence,float32	psln_thr):_Fact(Opcodes::Fact,object,after,before,confidence,psln_thr){
	}

	////////////////////////////////////////////////////////////////

	void	*AntiFact::operator	new(size_t	s){

		return	_Mem::Get()->_build_object(Atom::Object(Opcodes::AntiFact,FACT_ARITY));
	}

	AntiFact::AntiFact():_Fact(){
	}

	AntiFact::AntiFact(SysObject	*source):_Fact(source){
	}

	AntiFact::AntiFact(AntiFact	*f):_Fact(f){
	}

	AntiFact::AntiFact(Code	*object,uint64	after,uint64	before,float32	confidence,float32	psln_thr):_Fact(Opcodes::AntiFact,object,after,before,confidence,psln_thr){
	}

	////////////////////////////////////////////////////////////////

	Pred::Pred():LObject(){
	}

	Pred::Pred(SysObject	*source):LObject(source){
	}

	Pred::Pred(_Fact	*target,float32	psln_thr):LObject(){

		code(0)=Atom::Object(Opcodes::Pred,PRED_ARITY);
		code(PRED_TARGET)=Atom::RPointer(0);
		code(PRED_ARITY)=Atom::Float(psln_thr);
		add_reference(target);
	}

	bool	Pred::is_invalidated(){

		if(LObject::is_invalidated())
			return	true;
		for(uint32	i=0;i<simulations.size();++i){

			if(simulations[i]->is_invalidated()){

				invalidate();
				return	true;
			}
		}
		for(uint32	i=0;i<grounds.size();++i){

			if(grounds[i]->is_invalidated()){

				invalidate();
				return	true;
			}
		}
		return	false;
	}

	bool	Pred::grounds_invalidated(_Fact	*evidence){

		for(uint32	i=0;i<grounds.size();++i){

			switch(evidence->is_evidence(grounds[i])){
			case	MATCH_SUCCESS_NEGATIVE:
				return	true;
			default:
				break;
			}
		}
		return	false;
	}

	inline	_Fact	*Pred::get_target()	const{

		return	(_Fact	*)get_reference(0);
	}

	inline	bool	Pred::is_simulation()	const{

		return	simulations.size()>0;
	}

	inline	Sim	*Pred::get_simulation(Controller	*root)	const{

		for(uint32	i=0;i<simulations.size();++i){

			if(simulations[i]->root==root)
				return	simulations[i];
		}

		return	NULL;
	}

	////////////////////////////////////////////////////////////////

	Goal::Goal():LObject(),sim(NULL),ground(NULL){
	}

	Goal::Goal(SysObject	*source):LObject(source),sim(NULL),ground(NULL){
	}

	Goal::Goal(_Fact	*target,Code	*actor,float32	psln_thr):LObject(),sim(NULL),ground(NULL){

		code(0)=Atom::Object(Opcodes::Goal,GOAL_ARITY);
		code(GOAL_TARGET)=Atom::RPointer(0);
		code(GOAL_ACTR)=Atom::RPointer(1);
		code(GOAL_ARITY)=Atom::Float(psln_thr);
		add_reference(target);
		add_reference(actor);
	}

	bool	Goal::invalidate(){		// return false when was not invalidated, true otherwise.

		if(sim!=NULL)
			sim->invalidate();
		return	LObject::invalidate();
	}
	
	bool	Goal::is_invalidated(){

		if(LObject::is_invalidated())
			return	true;
		if(sim!=NULL	&&	sim->super_goal!=NULL	&&	sim->super_goal->is_invalidated()){

			invalidate();
			return	true;
		}
		return	false;
	}

	bool	Goal::ground_invalidated(_Fact	*evidence){

		if(ground!=NULL)
			return	ground->get_pred()->grounds_invalidated(evidence);
		return	false;
	}

	bool	Goal::is_requirement()	const{

		if(sim!=NULL	&&	sim->is_requirement)
			return	true;
		return	false;
	}

	inline	bool	Goal::is_self_goal()	const{

		return	(get_actor()==_Mem::Get()->get_self());
	}

	inline	bool	Goal::is_drive()	const{

		return	(sim==NULL	&&	is_self_goal());
	}

	inline	_Fact	*Goal::get_target()	const{

		return	(_Fact	*)get_reference(0);
	}

	inline	_Fact	*Goal::get_super_goal()	const{

		return	sim->super_goal;
	}

	inline	Code	*Goal::get_actor()	const{

		return	get_reference(GOAL_ACTR_REF);
	}

	inline	float32	Goal::get_strength(uint64	now)	const{

		_Fact	*target=get_target();
		return	target->get_cfd()/(target->get_before()-now);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Sim::Sim():_Object(),super_goal(NULL),root(NULL),sol(NULL),is_requirement(false),opposite(false),invalidated(0),sol_cfd(0),sol_before(0){
	}

	Sim::Sim(Sim	*s):_Object(),mode(s->mode),thz(s->thz),super_goal(s->super_goal),root(s->root),sol(s->sol),is_requirement(false),opposite(s->opposite),invalidated(0),sol_cfd(s->sol_cfd),sol_before(s->sol_before){
	}

	Sim::Sim(SimMode	mode,uint64	thz,Fact	*super_goal,bool	opposite,Controller	*root):_Object(),mode(mode),thz(thz),super_goal(super_goal),root(root),sol(NULL),is_requirement(false),opposite(opposite),invalidated(0),sol_cfd(0),sol_before(0){
	}

	Sim::Sim(SimMode	mode,uint64	thz,Fact	*super_goal,bool	opposite,Controller	*root,Controller	*sol,float32	sol_cfd,uint64	sol_deadline):_Object(),mode(mode),thz(thz),super_goal(super_goal),root(root),sol(sol),is_requirement(false),opposite(opposite),invalidated(0),sol_cfd(sol_cfd),sol_before(sol_before){
	}

	void	Sim::invalidate(){

		invalidated=1;
	}

	bool	Sim::is_invalidated(){

		if(invalidated==1)
			return	true;
		if(super_goal!=NULL	&&	super_goal->is_invalidated()){

			invalidate();
			return	true;
		}
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkRdx::MkRdx():LObject(),bindings(NULL){
	}

	MkRdx::MkRdx(SysObject	*source):LObject(source),bindings(NULL){
	}

	MkRdx::MkRdx(Code	*imdl_fact,Code	*input,Code	*output,float32	psln_thr,BindingMap	*binding_map):LObject(),bindings(binding_map){

		uint16	extent_index=MK_RDX_ARITY+1;
		code(0)=Atom::Marker(Opcodes::MkRdx,MK_RDX_ARITY);
		code(MK_RDX_CODE)=Atom::RPointer(0);				//	code.
		add_reference(imdl_fact);
		code(MK_RDX_INPUTS)=Atom::IPointer(extent_index);	//	inputs.
		code(MK_RDX_ARITY)=Atom::Float(psln_thr);
		code(extent_index++)=Atom::Set(1);					//	set of one input.
		code(extent_index++)=Atom::RPointer(1);
		add_reference(input);
		code(MK_RDX_PRODS)=Atom::IPointer(extent_index);	//	set of one production.
		code(extent_index++)=Atom::Set(1);
		code(extent_index++)=Atom::RPointer(2);
		add_reference(output);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Success::Success():LObject(){
	}

	Success::Success(_Fact	*object,_Fact	*evidence,float32	psln_thr):LObject(){

		code(0)=Atom::Object(Opcodes::Success,SUCCESS_ARITY);
		code(SUCCESS_OBJ)=Atom::RPointer(0);
		code(SUCCESS_EVD)=Atom::RPointer(1);
		code(SUCCESS_ARITY)=Atom::Float(psln_thr);
		add_reference(object);
		add_reference(evidence);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Perf::Perf():LObject(){
	}

	Perf::Perf(uint32	reduction_job_avg_latency,int32	d_reduction_job_avg_latency,uint32	time_job_avg_latency,int32	d_time_job_avg_latency):LObject(){

		code(0)=Atom::Object(Opcodes::Perf,PERF_ARITY);
		code(PERF_RDX_LTCY)=Atom::Float(reduction_job_avg_latency);
		code(PERF_D_RDX_LTCY)=Atom::Float(d_reduction_job_avg_latency);
		code(PERF_TIME_LTCY)=Atom::Float(time_job_avg_latency);
		code(PERF_D_TIME_LTCY)=Atom::Float(d_time_job_avg_latency);
		code(PERF_ARITY)=Atom::Float(1);
	}

	////////////////////////////////////////////////////////////////

	ICST::ICST():LObject(){
	}

	ICST::ICST(SysObject	*source):LObject(source){
	}
}