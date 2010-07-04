//	object.h
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

#ifndef	r_code_object_h
#define	r_code_object_h

#include	"atom.h"
#include	"vector.h"
#include	"base.h"
#include	"replicode_defs.h"

#include	<list>


using	namespace	core;

namespace	r_code{

	//	I/O from/to r_code::Image ////////////////////////////////////////////////////////////////////////

	class	dll_export	ImageObject{
	public:
		r_code::vector<Atom>	code;
		r_code::vector<uint16>	references;	//	for views: 0, 1 or 2 elements; these are indexes in the relocation segment for grp (exception: not for root) and possibly org
											//	for sys-objects: any number
		virtual	void	write(word32	*data)=0;
		virtual	void	read(word32		*data)=0;
		virtual	void	trace()=0;
	};

	class	View;

	class	dll_export	SysView:
	public	ImageObject{
	public:
		SysView();
		SysView(View	*source);

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	getSize()	const;
		void	trace();
	};

	class	Code;

	class	dll_export	SysObject:
	public	ImageObject{
	public:
		typedef	enum{
			ROOT_GRP=0,
			STDIN_GRP=1,
			STDOUT_GRP=2,
			SELF_ENT=3,
			NON_STD=4
		}Axiom;

		r_code::vector<uint32>		markers;		//	indexes in the relocation segment
		r_code::vector<SysView	*>	views;

		Axiom	axiom;

		SysObject(Axiom	a);
		SysObject(Code	*source);
		~SysObject();

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	getSize();
		void	trace();
	};

	// Interfaces for r_exec classes ////////////////////////////////////////////////////////////////////////

	class	Object;

	class	dll_export	View:
	public	_Object{
	protected:
		Atom	_code[VIEW_CODE_MAX_SIZE];	//	dimensioned to hold the largest view (group view): head atom, oid, iptr to ijt, sln, res, rptr to grp, rptr to org, vis, cov, 3 atoms for ijt's timestamp.
	private:
		uint16	index;						//	for unpacking: index is the index of the view in the SysObject.
	public:
		P<Code>	object;						//	viewed object.
		Code	*references[2];				//	does not include the viewed object; no smart pointer here (a view is held by a group and holds a ref to said group in references[0]).

		View():object(NULL){

			references[0]=references[1]=NULL;
		}

		View(SysView	*source,Code	*object){

			for(uint16	i=0;i<source->code.size();++i)
				_code[i]=source->code[i];
			references[0]=references[1]=NULL;
		}

		virtual	~View(){}

		Atom	&code(uint16	i){	return	_code[i];	}
		Atom	code(uint16	i)	const{	return	_code[i];	}

		class	Hash{
		public:
			size_t	operator	()(View	*v)	const{
				return	(size_t)(Code	*)v->references[0];	//	i.e. the group the view belongs to.
			}
		};

		class	Equal{
		public:
			bool	operator	()(const	View	*lhs,const	View	*rhs)	const{
				return	lhs->references[0]==rhs->references[0];
			}
		};
	};

	class	dll_export	Code:
	public	_Object{
	private:
		SysObject::Axiom	axiom;
	protected:
		void	load(SysObject	*source){

			for(uint16	i=0;i<source->code.size();++i)
				code(i)=source->code[i];
			axiom=source->axiom;
		}
		template<class	V>	void	build_view(SysView	*source,uint16	reference_index,Code	*referenced_object){

			V	*view=new	V(source,this);
			view->references[reference_index]=referenced_object;
			views.insert(view);
		}
	public:
		SysObject::Axiom	get_axiom()	const{	return	axiom;	}

		virtual	Atom	&code(uint16	i)=0;
		virtual	Atom	&code(uint16	i)	const=0;
		virtual	uint16	code_size()	const=0;
		virtual	void	set_reference(uint16	i,Code	*object)=0;
		virtual	Code	*get_reference(uint16	i)	const=0;
		virtual	uint16	references_size()	const=0;

		virtual	bool	is_compact()		const{	return	false;	}
		virtual	bool	is_invalidated()	const{	return	false;	}
		virtual	bool	invalidate()	{ return	false;	}

		std::list<Code	*>								markers;
		UNORDERED_SET<View	*,View::Hash,View::Equal>	views;	//	indexed by groups.

		virtual	void	build_view(SysView	*source,uint16	reference_index,Code	*referenced_object)=0;

		virtual	void	acq_views()		const{}
		virtual	void	rel_views()		const{}
		virtual	void	acq_markers()	const{}
		virtual	void	rel_markers()	const{}

		virtual	float32	get_psln_thr(){	return	1;	}

		Code():axiom(SysObject::NON_STD){}
		virtual	~Code(){}

		virtual	void	mod(uint16	member_index,float32	value){};
		virtual	void	set(uint16	member_index,float32	value){};
		virtual	View	*find_view(Code	*group,bool	lock){	return	NULL;	}
		virtual	void	add_reference(Code	*object)	const{}	//	called only on local objects.
		void	remove_marker(Code	*m){
			
			acq_markers();
			std::list<Code	*>::const_iterator	_m;
			for(_m=markers.begin();_m!=markers.end();){

				if(*_m==m)
					_m=markers.erase(_m);
				else
					++m;
			}
			rel_markers();
		}

		std::list<Code	*>::const_iterator	position_in_objects;
	};

	//	Implementation for local objects (non distributed).
	class	dll_export	LObject:
	public	Code{
	protected:
		r_code::vector<Atom>		_code;
		r_code::vector<P<Code> >	_references;
	public:
		LObject():Code(){}
		LObject(SysObject	*source):Code(){
			
			load(source);
		}
		virtual	~LObject(){}

		void	build_view(SysView	*source,uint16	reference_index,Code	*referenced_object){

			return	Code::build_view<View>(source,reference_index,referenced_object);
		}

		Atom	&code(uint16	i){	return	_code[i];	}
		Atom	&code(uint16	i)	const{	return	(*_code.as_std())[i];	}
		uint16	code_size()	const{	return	_code.size();	}
		void	set_reference(uint16	i,Code	*object){	_references[i]=object;	}
		Code	*get_reference(uint16	i)	const{	return	(*_references.as_std())[i];	}
		uint16	references_size()	const{	return	_references.size();	}
		void	add_reference(Code	*object)	const{	_references.as_std()->push_back(object);	}
	};

	class	Mem{
	public:
		virtual	Code	*buildObject(SysObject	*source)=0;
		virtual	void	deleteObject(Code	*object)=0;
	};
}


#endif