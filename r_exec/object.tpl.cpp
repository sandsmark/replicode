//	object.tpl.cpp
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

namespace	r_exec{

	template<class	C,class	U>	Object<C,U>::Object(r_code::Mem	*mem):C(),mem(mem),hash_value(0),invalidated(false){
	}

	template<class	C,class	U>	Object<C,U>::~Object(){

		invalidate();
	}

	template<class	C,class	U>	bool	Object<C,U>::is_invalidated()	const{

		return	invalidated;
	}

	template<class	C,class	U>	bool	Object<C,U>::invalidate(){

		if(invalidated)
			return	true;
		invalidated=true;

		if(mem){

			if(code(0).getDescriptor()==Atom::MARKER){

				for(uint16	i=0;i<references_size();++i)
					get_reference(i)->remove_marker(this);
			}
		
			mem->deleteObject(this);
		}

		return	false;
	}

	template<class	C,class	U>	void	Object<C,U>::compute_hash_value(){

		hash_value=code(0).asOpcode()<<20;				//	12 bits for the opcode.
		hash_value|=(code_size()	&	0x00000FFF)<<8;	//	12 bits for the code size.
		hash_value|=references_size()	&	0x000000FF;	//	8 bits for the reference set size.
	}

	template<class	C,class	U>	float32	Object<C,U>::get_psln_thr(){

		psln_thr_sem.enter();
		float32	r=code(code_size()-1).asFloat();	//	psln is always the lat atom in code.
		psln_thr_sem.leave();
		return	r;
	}

	template<class	C,class	U>	void	Object<C,U>::mod(uint16	member_index,float32	value){

		if(member_index!=code_size()-1)
			return;
		float32	v=code(member_index).asFloat()+value;
		if(v<0)
			v=0;
		else	if(v>1)
			v=1;

		psln_thr_sem.enter();
		code(member_index)=Atom::Float(v);
		psln_thr_sem.leave();
	}

	template<class	C,class	U>	void	Object<C,U>::set(uint16	member_index,float32	value){

		if(member_index!=code_size()-1)
			return;

		psln_thr_sem.enter();
		code(member_index)=Atom::Float(value);
		psln_thr_sem.leave();
	}

	template<class	C,class	U>	View	*Object<C,U>::find_view(Code	*group,bool	lock){

		if(lock)
			acq_views();

		r_code::View	probe;
		probe.references[0]=group;

		UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	v=views.find(&probe);
		if(v!=views.end()){

			if(lock)
				rel_views();
			return	new	View((r_exec::View	*)*v);
		}

		if(lock)
			rel_views();
		return	NULL;
	}
}