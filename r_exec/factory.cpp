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
#include	"mem.h"


namespace	r_exec{
	namespace	factory{

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

		Code	*Object::Var(float32	tolerance,float32	psln_thr){

			Code	*v=_Mem::Get()->buildObject(Atom::Object(Opcodes::Var,VAR_ARITY));
			v->code(VAR_TOL)=Atom::Float(tolerance);
			v->code(VAR_ARITY)=Atom::Float(psln_thr);
			return	v;
		}

		Code	*Object::Fact(Code	*object,uint64	time,float32	confidence,float32	psln_thr){

			Code	*f=_Mem::Get()->buildObject(Atom::Object(Opcodes::Fact,FACT_ARITY));
			f->code(FACT_OBJ)=Atom::RPointer(0);
			f->code(FACT_TIME)=Atom::IPointer(FACT_ARITY+1);
			f->code(FACT_CFD)=Atom::Float(confidence);
			f->code(FACT_ARITY)=Atom::Float(psln_thr);
			Utils::SetTimestamp<Code>(f,FACT_TIME,time);
			f->set_reference(0,object);
			return	f;
		}

		Code	*Object::AntiFact(Code	*object,uint64	time,float32	confidence,float32	psln_thr){

			Code	*f=_Mem::Get()->buildObject(Atom::Object(Opcodes::AntiFact,FACT_ARITY));
			f->code(FACT_OBJ)=Atom::RPointer(0);
			f->code(FACT_TIME)=Atom::IPointer(FACT_ARITY+1);
			f->code(FACT_CFD)=Atom::Float(confidence);
			f->code(FACT_ARITY)=Atom::Float(psln_thr);
			Utils::SetTimestamp<Code>(f,FACT_TIME,time);
			f->set_reference(0,object);
			return	f;
		}

		Code	*Object::MkSim(Code	*object,Code	*source,float32	psln_thr){

			Code	*mk=_Mem::Get()->buildObject(Atom::Marker(Opcodes::MkSim,MK_SIM_ARITY));
			mk->code(MK_SIM_OBJ)=Atom::RPointer(0);
			mk->code(MK_SIM_SRC)=Atom::RPointer(1);
			mk->code(MK_SIM_ARITY)=Atom::Float(psln_thr);
			mk->set_reference(0,object);
			mk->set_reference(1,source);
			return	mk;
		}

		Code	*Object::MkPred(Code	*object,Code	*model,float32	confidence,float32	psln_thr){

			Code	*mk=_Mem::Get()->buildObject(Atom::Marker(Opcodes::MkPred,MK_PRED_ARITY));
			mk->code(MK_PRED_OBJ)=Atom::RPointer(0);
			mk->code(MK_PRED_FMD)=Atom::RPointer(1);
			mk->code(MK_PRED_CFD)=Atom::Float(confidence);
			mk->code(MK_PRED_ARITY)=Atom::Float(psln_thr);
			mk->set_reference(0,object);
			mk->set_reference(1,model);
			return	mk;
		}

		Code	*Object::MkAsmp(Code	*object,Code	*source,float32	confidence,float32	psln_thr){

			Code	*mk=_Mem::Get()->buildObject(Atom::Marker(Opcodes::MkAsmp,MK_ASMP_ARITY));
			mk->code(MK_ASMP_OBJ)=Atom::RPointer(0);
			mk->code(MK_ASMP_SRC)=Atom::RPointer(1);
			mk->code(MK_ASMP_CFD)=Atom::Float(confidence);
			mk->code(MK_ASMP_ARITY)=Atom::Float(psln_thr);
			mk->set_reference(0,object);
			mk->set_reference(1,source);
			return	mk;
		}

		Code	*Object::MkGoal(Code	*object,Code	*model,float32	psln_thr){

			Code	*mk=_Mem::Get()->buildObject(Atom::Marker(Opcodes::MkGoal,MK_GOAL_ARITY));
			mk->code(MK_GOAL_OBJ)=Atom::RPointer(0);
			mk->code(MK_GOAL_IMD)=Atom::RPointer(1);
			mk->code(MK_GOAL_ARITY)=Atom::Float(psln_thr);
			mk->set_reference(0,object);
			mk->set_reference(1,model);
			return	mk;
		}

		Code	*Object::MkSuccess(Code	*object,float32	p_rate,float32	n_rate,float32	psln_thr){

			Code	*mk=_Mem::Get()->buildObject(Atom::Marker(Opcodes::MkSuccess,MK_SUCCESS_ARITY));
			mk->code(MK_SUCCESS_OBJ)=Atom::RPointer(0);
			mk->code(MK_SUCCESS_P_RATE)=Atom::Float(p_rate);
			mk->code(MK_SUCCESS_N_RATE)=Atom::Float(n_rate);
			mk->code(MK_SUCCESS_ARITY)=Atom::Float(psln_thr);
			mk->set_reference(0,object);
			return	mk;
		}
	}
}