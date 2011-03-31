//	view.inline.cpp
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

#include	"../r_code/utils.h"
#include	"opcodes.h"


namespace	r_exec{

	inline	View::View():r_code::View(),controller(NULL){

		_code[VIEW_OID].atom=GetOID();
		reset_ctrl_values();
	}

	inline	View::View(r_code::SysView	*source,r_code::Code	*object):r_code::View(source,object),controller(NULL){

		_code[VIEW_OID].atom=GetOID();
		reset();
	}

	inline	View::View(const	View	*view,bool	new_OID):r_code::View(),controller(NULL){

		object=view->object;
		memcpy(_code,view->_code,VIEW_CODE_MAX_SIZE*sizeof(Atom)+2*sizeof(Code	*));	//	reference_set is contiguous to code; memcpy in one go.
		if(new_OID)
			_code[VIEW_OID].atom=GetOID();
		controller=view->controller;
		reset();
	}

	inline	View::View(bool		sync,
						uint64	ijt,
						float32	sln,
						int32	res,
						Code	*destination,
						Code	*origin,
						Code	*object):r_code::View(),controller(NULL){
	
		code(VIEW_OPCODE)=Atom::SSet(Opcodes::View,VIEW_ARITY);
		init(sync,ijt,sln,res,destination,origin,object);
	}

	inline	View::View(bool		sync,
						uint64	ijt,
						float32	sln,
						int32	res,
						Code	*destination,
						Code	*origin,
						Code	*object,
						float32	act):r_code::View(),controller(NULL){
	
		code(VIEW_OPCODE)=Atom::SSet(Opcodes::PgmView,PGM_VIEW_ARITY);
		init(sync,ijt,sln,res,destination,origin,object);
		code(VIEW_ACT)=Atom::Float(act);
	}

	inline	void	View::init(bool		sync,
								uint64	ijt,
								float32	sln,
								int32	res,
								Code	*destination,
								Code	*origin,
								Code	*object){

		_code[VIEW_OID].atom=GetOID();
		reset_ctrl_values();
		
		code(VIEW_SYNC)=Atom::Boolean(sync);
		code(VIEW_IJT)=Atom::IPointer(code(VIEW_OPCODE).getAtomCount()+1);
		Utils::SetTimestamp<View>(this,VIEW_IJT,ijt);
		code(VIEW_SLN)=Atom::Float(sln);
		code(VIEW_RES)=res<0?Atom::PlusInfinity():Atom::Float(res);
		code(VIEW_HOST)=Atom::RPointer(0);
		code(VIEW_ORG)=origin?Atom::RPointer(1):Atom::Nil();

		references[0]=destination;
		references[1]=origin;

		set_object(object);
	}

	inline	View::~View(){

		if(controller!=NULL)
			controller->kill();
	}

	inline	void	View::reset(){

		reset_ctrl_values();
		reset_init_sln();
		reset_init_act();
	}

	inline	uint32	View::getOID()	const{

		return	_code[VIEW_OID].atom;
	}

	inline	bool	View::isNotification()	const{

		return	false;
	}

	inline	Group	*View::get_host(){

		uint32	host_reference=code(VIEW_HOST).asIndex();
		return	(Group	*)references[host_reference];
	}

	inline	bool	View::get_sync(){

		return	code(VIEW_SYNC).asBoolean();
	}

	inline	float32	View::get_res(){

		return	code(VIEW_RES).asFloat();
	}

	inline	float32	View::get_sln(){

		return	code(VIEW_SLN).asFloat();
	}

	inline	float32	View::get_act(){

		return	code(VIEW_ACT).asFloat();
	}

	inline	float32	View::get_vis(){

		return	code(GRP_VIEW_VIS).asFloat();
	}

	inline	bool	View::get_cov(){

		if(object->code(0).getDescriptor()==Atom::GROUP)
			return	code(GRP_VIEW_COV).asBoolean();
		return	false;
	}

	inline	void	View::mod_res(float32	value){

		if(code(VIEW_RES)==Atom::PlusInfinity())
			return;
		acc_res+=value;
		++res_changes;
	}

	inline	void	View::set_res(float32	value){

		if(code(VIEW_RES)==Atom::PlusInfinity())
			return;
		acc_res+=value-get_res();
		++res_changes;
	}

	inline	void	View::mod_sln(float32	value){

		acc_sln+=value;
		++sln_changes;
	}

	inline	void	View::set_sln(float32	value){

		acc_sln+=value-get_sln();
		++sln_changes;
	}

	inline	void	View::mod_act(float32	value){

		acc_act+=value;
		++act_changes;
	}

	inline	void	View::set_act(float32	value){

		acc_act+=value-get_act();
		++act_changes;
	}

	inline	void	View::mod_vis(float32	value){

		acc_vis+=value;
		++vis_changes;
	}

	inline	void	View::set_vis(float32	value){

		acc_vis+=value-get_vis();
		++vis_changes;
	}

	inline	float32	View::update_sln_delta(){

		float32	delta=get_sln()-initial_sln;
		initial_sln=get_sln();
		return	delta;
	}

	inline	float32	View::update_act_delta(){

		float32	act=get_act();
		float32	delta=act-initial_act;
		initial_act=act;
		return	delta;
	}

	inline	void	View::mod(uint16	member_index,float32	value){

		switch(member_index){
		case	VIEW_SLN:
			mod_sln(value);
			break;
		case	VIEW_RES:
			mod_res(value);
			break;
		case	VIEW_ACT:
			mod_act(value);
			break;
		case	GRP_VIEW_VIS:
			mod_vis(value);
			break;
		}
	}

	inline	void	View::set(uint16	member_index,float32	value){

		switch(member_index){
		case	VIEW_SLN:
			set_sln(value);
			break;
		case	VIEW_RES:
			set_res(value);
			break;
		case	VIEW_ACT:
			set_act(value);
			break;
		case	GRP_VIEW_VIS:
			set_vis(value);
			break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	bool	NotificationView::isNotification()	const{

		return	true;
	}
}