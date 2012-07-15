//	model_base.cpp
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

#include	"model_base.h"
#include	"init.h"


namespace	r_exec{

	uint32	ModelBase::MEntry::ComputeHashCode(Code	*mdl){

		uint32	hash_code=(mdl->code(mdl->code(HLP_TPL_ARGS).asIndex()).getAtomCount()<<28);
		_Fact	*lhs=(_Fact	*)mdl->get_reference(0);
		_Fact	*rhs=(_Fact	*)mdl->get_reference(1);

		if(lhs->is_fact()){

			if(lhs->get_reference(0)->code(0).asOpcode()==Opcodes::IMdl)
				hash_code|=(1<<26);
			else
				hash_code|=(0<<26);
		}else	if(lhs->get_reference(0)->code(0).asOpcode()==Opcodes::IMdl)
				hash_code|=(3<<26);
			else
				hash_code|=(2<<26);

		if(rhs->is_fact()){

			if(rhs->get_reference(0)->code(0).asOpcode()==Opcodes::IMdl)
				hash_code|=(1<<24);
			else
				hash_code|=(0<<24);
		}else	if(rhs->get_reference(0)->code(0).asOpcode()==Opcodes::IMdl)
				hash_code|=(3<<24);
			else
				hash_code|=(2<<24);

		uint16	opcodes=0;
		for(uint16	i=0;i<lhs->code_size();++i)
			opcodes+=lhs->code(i).getDescriptor();
		for(uint16	i=0;i<rhs->code_size();++i)
			opcodes+=rhs->code(i).getDescriptor();
		hash_code+=opcodes;
		return	hash_code;
	}

	ModelBase::MEntry::MEntry():mdl(NULL),touch_time(0),hash_code(0){
	}

	ModelBase::MEntry::MEntry(Code	*mdl):mdl(mdl),touch_time(Now()),hash_code(ComputeHashCode(mdl)){
	}

	ModelBase::MEntry::~MEntry(){
	}

	bool	ModelBase::MEntry::match(const	MEntry	&e)	const{

		return	_Fact::MatchObject(mdl,e.mdl);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ModelBase	*ModelBase::Singleton=NULL;

	ModelBase	*ModelBase::Get(){

		return	Singleton;
	}

	ModelBase::ModelBase(){

		Singleton=this;
	}

	ModelBase::~ModelBase(){
	}

	bool	ModelBase::register_mdl(Code	*mdl){

		MEntry	e(mdl);

		mdlCS.enter();
		MdlSet::iterator	m=black_list.find(e);
		if(m!=black_list.end()){

			(*m).touch_time=Now();
			mdlCS.leave();
			return	true;
		}
		m=white_list.find(e);
		if(m!=white_list.end()){

			(*m).touch_time=Now();
			mdlCS.leave();
			return	true;
		}
		white_list.insert(e);
		mdlCS.leave();
		return	false;
	}

	void	ModelBase::register_mdl_failure(Code	*mdl){	// GC performed here.

		mdlCS.enter();
		uint64	now=Now();
		MdlSet::const_iterator	m;
		for(m=white_list.begin();m!=white_list.end();){

			if((*m).mdl==mdl)
				m=white_list.erase(m);
			else	if(now-(*m).touch_time>=white_thz)
				m=white_list.erase(m);
			else
				++m;
		}
		for(m=black_list.begin();m!=black_list.end();){

			if(now-(*m).touch_time>=black_thz)
				m=black_list.erase(m);
			else
				++m;
		}
		black_list.insert(MEntry(mdl));
		mdlCS.leave();
	}
}