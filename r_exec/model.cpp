//	model.cpp
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

#include	"model.h"
#include	"mem.h"


namespace	r_exec{

	Model::Model(r_code::Mem	*m):LObject(m),CriticalSection(),output_count(0),success_count(0),failure_count(0){
	}

	Model::Model(r_code::SysObject	*source,r_code::Mem	*m):LObject(source,m),CriticalSection(),output_count(0),success_count(0),failure_count(0){
	}

	Model::~Model(){
	}

	void	Model::register_outcome(bool	measurement,float32	confidence){

		enter();
		++output_count;
		if(measurement)	//	one success.
			success_count+=confidence;
		else
			failure_count+=confidence;
		leave();
	}

	float32	Model::get_success_rate()	const{

		return	success_count/((float32)output_count);
	}

	float32	Model::get_failure_rate()	const{

		return	failure_count/((float32)output_count);
	}

	void	Model::inject_opposite(Code	*fact)	const{

		uint64	now=Now();
		uint16	out_group_set_index=code(MD_OUT_GRPS).asIndex();
		uint16	out_group_count=code(out_group_set_index).getAtomCount();

		Code	*f;
		if(fact->code(0).asOpcode()==Opcodes::Fact)
			f=factory::Object::AntiFact(fact->get_reference(0),now,1,1);
		else
			f=factory::Object::Fact(fact->get_reference(0),now,1,1);

		for(uint16	i=1;i<=out_group_count;++i){	//	inject the negative findings in the ouptut groups.

			Code	*out_group=get_reference(code(out_group_set_index+i).asIndex());
			View	*view=new	View(true,now,1,1,out_group,NULL,f);
			_Mem::Get()->inject(view);
		}
	}
}