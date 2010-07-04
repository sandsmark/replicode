//	view.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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

#include	"view.h"
#include	"../CoreLibrary/utils.h"
#include	"group.h"


namespace	r_exec{

	FastSemaphore	OID_sem(1,1);

	uint32	View::LastOID=0;

	uint32	View::GetOID(){

		OID_sem.acquire();
		uint32	oid=LastOID++;
		OID_sem.release();
		return	oid;
	}

	uint16	View::ViewOpcode;

	float32	View::MorphValue(float32	value,float32	source_thr,float32	destination_thr){

		if(value==0)
			return	destination_thr;
		
		if(source_thr>0){

			if(destination_thr>0){

				float32	r=value*destination_thr/source_thr;
				if(r>1)	//	handles precision errors.
					r=1;
				return	r;
			}else
				return	value;
		}
		return	destination_thr+value;
	}

	float32	View::MorphChange(float32	change,float32	source_thr,float32	destination_thr){	//	change is always >0.

		if(source_thr>0){

			if(destination_thr>0)
				return	change*destination_thr/source_thr;
			else
				return	change;
		}
		return	destination_thr+change;
	}

	View::View(View	*view,Group	*group):r_code::View(){

		Group	*source=view->get_host();
		object=view->object;
		memcpy(_code,view->_code,VIEW_CODE_MAX_SIZE*sizeof(Atom));
		_code[VIEW_OID].atom=GetOID();
		references[0]=group;		//	host.
		references[1]=source;	//	origin.

		//	morph ctrl values; NB: res is not morphed as it is expressed as a multiple of the upr.
		code(VIEW_SLN)=Atom::Float(MorphValue(view->code(VIEW_SLN).asFloat(),source->get_sln_thr(),group->get_sln_thr()));
		switch(object->code(0).getDescriptor()){
		case	Atom::GROUP:
			code(VIEW_ACT_VIS)=Atom::Float(MorphValue(view->code(VIEW_ACT_VIS).asFloat(),source->get_vis_thr(),group->get_vis_thr()));
		case	Atom::INSTANTIATED_PROGRAM:
			code(VIEW_ACT_VIS)=Atom::Float(MorphValue(view->code(VIEW_ACT_VIS).asFloat(),source->get_act_thr(),group->get_act_thr()));
		}

		reset_ctrl_values();
		reset_init_sln();
		reset_init_act();
	}

	void	View::set_object(r_code::Code	*object){

		this->object=object;
		reset_init_act();
	}

	void	View::reset_ctrl_values(){

		sln_changes=0;
		acc_sln=0;
		act_vis_changes=0;
		acc_act_vis=0;
		res_changes=0;
		acc_res=0;

		periods_at_low_sln=0;
		periods_at_high_sln=0;
		periods_at_low_act=0;
		periods_at_high_act=0;
	}

	void	View::reset_init_sln(){

		initial_sln=get_sln();
	}

	void	View::reset_init_act(){

		if(object!=NULL	&&	object->code(0).getDescriptor()==Atom::INSTANTIATED_PROGRAM)
			initial_act=get_act_vis();
		else
			initial_act=0;
	}

	float32	View::update_res(){

		if(res_changes){

			float32	new_res=get_res()+(float32)acc_res/(float32)res_changes;
			if(new_res<0)
				new_res=0;
			code(VIEW_RES)=r_code::Atom::Float(new_res);
		}
		acc_res=0;
		res_changes=0;
		return	get_res();
	}

	float32	View::update_sln(float32	low,float32	high){

		if(sln_changes){

			float32	new_sln=get_sln()+acc_sln/sln_changes;
			if(new_sln<0)
				new_sln=0;
			else	if(new_sln>1)
				new_sln=1;
			code(VIEW_SLN)=r_code::Atom::Float(new_sln);
		}
		acc_sln=0;
		sln_changes=0;
		if(get_sln()<low)
			++periods_at_low_sln;
		else{
			
			periods_at_low_sln=0;
			if(get_sln()>high)
				++periods_at_high_sln;
			else
				periods_at_high_sln=0;
		}
		return	get_sln();
	}

	float32	View::update_act(float32	low,float32	high){

		update_vis();
		if(get_act_vis()<low)
			++periods_at_low_act;
		else{
			
			periods_at_low_act=0;
			if(get_act_vis()>high)
				++periods_at_high_act;
			else
				periods_at_high_act=0;
		}
		return	get_act_vis();
	}

	float32	View::update_vis(){

		if(act_vis_changes){

			float32	new_act_vis=get_act_vis()+acc_act_vis/act_vis_changes;
			if(new_act_vis<0)
				new_act_vis=0;
			else	if(new_act_vis>1)
				new_act_vis=1;
			code(VIEW_ACT_VIS)=r_code::Atom::Float(new_act_vis);
		}
		acc_act_vis=0;
		act_vis_changes=0;
		return	get_act_vis();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	NotificationView::NotificationView(Group	*origin,Group	*destination,Code	*marker):View(){

		uint32	write_index=0;
		code(write_index++)=r_code::Atom::SSet(ViewOpcode,5);	//	Structured Set.
		code(write_index++)=r_code::Atom::IPointer(6);			//	iptr to ijt.
		code(write_index++)=r_code::Atom::Float(1);				//	sln.
		code(write_index++)=r_code::Atom::Float(1);				//	res.
		code(write_index++)=r_code::Atom::RPointer(0);			//	destination.
		code(write_index++)=r_code::Atom::RPointer(1);			//	origin.
		code(6)=r_code::Atom::Timestamp();						//	ijt will be set at injection time.
		references[0]=destination;
		references[1]=origin;
		reset_init_sln();

		object=marker;
	}
}