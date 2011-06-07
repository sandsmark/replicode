//	object.h
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

#ifndef	r_exec_object_h
#define	r_exec_object_h

#include	"../../CoreLibrary/trunk/CoreLibrary/utils.h"
#include	"../r_code/object.h"
#include	"view.h"
#include	"opcodes.h"

#include	<list>


namespace	r_exec{

	r_exec_dll	bool		IsNotification(Code	*object);

	//	Shared resources:
	//		views: accessed by Mem::injectNow (via various sub calls) and Mem::update.
	//		psln_thr: accessed by reduction cores (via overlay mod/set).
	//		marker_set: accessed by Mem::injectNow ans Mem::_initiate_sln_propagation.
	template<class	C,class	U>	class	Object:
	public	C{
	private:
		size_t	hash_value;

		bool	invalidated;

		CriticalSection	psln_thr_sem;
		CriticalSection	views_sem;
		CriticalSection	markers_sem;
	protected:
		Object();
		Object(r_code::Mem	*mem);
	public:
		virtual	~Object();	//	un-registers from the rMem's object_register.

		r_code::View	*build_view(SysView	*source){

			return	Code::build_view<r_exec::View>(source);
		}

		void	bind(r_code::Mem	*mem){
			
			setOID(mem->get_oid());
		}

		bool	is_invalidated()	const;
		virtual	bool	invalidate();	//	return false when was not invalidated, true otherwise.

		void	compute_hash_value();

		float32	get_psln_thr();

		void	acq_views(){	views_sem.enter();	}
		void	rel_views(){	views_sem.leave();	}
		void	acq_markers(){	markers_sem.enter();	}
		void	rel_markers(){	markers_sem.leave();	}

		//	Target psln_thr only.
		void	set(uint16	member_index,float32	value);
		void	mod(uint16	member_index,float32	value);

		View	*find_view(Code	*group,bool	lock);	//	returns a copy of the found view if any, NULL otherwise.

		Code	*get_pred();
		Code	*get_goal();
		Code	*get_hyp();
		Code	*get_sim();
		Code	*get_asmp();

		void	kill();

		class	Hash{
		public:
			size_t	operator	()(U	*o)	const{
				
				if(o->hash_value==0)
					o->compute_hash_value();
				return	o->hash_value;
			}
		};

		class	Equal{
		public:
			bool	operator	()(const	U	*lhs,const	U	*rhs)	const{	//	lhs and rhs have the same hash value, i.e. same opcode, same code size and same reference size.
				
				if(lhs->code(0).asOpcode()==Opcodes::Ent	||	rhs->code(0).asOpcode()==Opcodes::Ent)
					return	lhs==rhs;

				uint16	i;
				for(i=0;i<lhs->references_size();++i)
					if(lhs->get_reference(i)!=rhs->get_reference(i))
						return	false;
				for(i=0;i<lhs->code_size();++i){

					switch(lhs->code(i).getDescriptor()){
					case	Atom::NUMERICAL_VARIABLE:
						if(rhs->code(i).getDescriptor()!=Atom::NUMERICAL_VARIABLE)
							return	false;
						break;
					case	Atom::BOOLEAN_VARIABLE:
						if(rhs->code(i).getDescriptor()!=Atom::BOOLEAN_VARIABLE)
							return	false;
						break;
					case	Atom::STRUCTURAL_VARIABLE:
						if(rhs->code(i).getDescriptor()!=Atom::STRUCTURAL_VARIABLE)
							return	false;
						break;
					}
					if(lhs->code(i)!=rhs->code(i))
						return	false;
				}
				return	true;
			}
		};

		//	for un-registering in the rMem upon deletion.
		typename	UNORDERED_SET<U	*,typename	Object::Hash,typename	Object::Equal>::const_iterator	position_in_object_register;
	};

	//	Local object.
	//	Used for r-code that does not travel across networks (groups and notifications) or when the rMem is not distributed.
	//	Markers are killed when at least one of their references dies (held by their views).
	//	Marker deletion is performed by registering pending delete operations in the groups they are projected onto.
	class	r_exec_dll	LObject:
	public	Object<r_code::LObject,LObject>{
	public:
		static	bool	RequiresPacking(){	return	false;	}
		static	LObject	*Pack(Code	*object,r_code::Mem	*mem){	return	(LObject	*)object;	}	//	object is always a LObject (local operation).
		LObject(r_code::Mem	*mem=NULL):Object<r_code::LObject,LObject>(mem){}
		LObject(r_code::SysObject	*source):Object<r_code::LObject,LObject>(){
		
			load(source);
		}
		virtual	~LObject(){}
	};
}


#endif