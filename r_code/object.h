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


using	namespace	core;

namespace	r_code{

	//	I/O from/to r_code::Image ////////////////////////////////////////////////////////////////////////

	class	dll_export	ImageObject{
	public:
		r_code::vector<Atom>	code;
		r_code::vector<uint32>	references;	//	for views: 0, 1 or 2 elements; these are indexes in the relocation segment for grp (exception: not for root) and possibly org
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
		r_code::vector<uint32>		markers;		//	indexes in the relocation segment
		r_code::vector<SysView	*>	views;

		SysObject();
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
	public:
		P<Code>	object;						//	viewed object.
		Code	*references[2];				//	does not include the viewed object; no smart pointer here (a view is held by a group and holds a ref to said group).

		View();
		View(SysView	*source,Code	*object);
		virtual	~View();

		Atom	&code(uint16	i){	return	_code[i];	}
		Atom	code(uint16	i)	const{	return	_code[i];	}
	};

	class	dll_export	Code:
	public	_Object{
	protected:
		void	load(SysObject	*source);
		template<class	V>	void	build_views(SysObject	*source){

			for(uint16	i=0;i<source->views.size();++i)
				initial_views[i]=new	V(source->views[i],this);
		}
	public:
		virtual	Atom	&code(uint16	i)=0;
		virtual	Atom	&code(uint16	i)	const=0;
		virtual	uint16	code_size()	const=0;
		virtual	P<Code>	&references(uint16	i)=0;
		virtual	P<Code>	&references(uint16	i)	const=0;
		virtual	uint16	references_size()	const=0;

		r_code::vector<P<Code> >	markers;

		r_code::vector<P<View> >	initial_views;

		Code();
		virtual	~Code();

		virtual	void	mod(uint16	member_index,float32	value){};
		virtual	void	set(uint16	member_index,float32	value){};
		virtual	View	*find_view(Code	*group){	return	NULL;	}
		virtual	bool	is_compact()	const{	return	false;	}
		virtual	void	add_reference(Code	*object)	const{}
	};

	class	dll_export	Object:
	public	Code{
	private:
		r_code::vector<Atom>		_code;
		r_code::vector<P<Code> >	_references;
	public:
		Object();
		Object(SysObject	*source);
		virtual	~Object();

		Atom	&code(uint16	i){	return	_code[i];	}
		Atom	&code(uint16	i)	const{	return	(*_code.as_std())[i];	}
		uint16	code_size()	const{	return	_code.size();	}
		P<Code>	&references(uint16	i){	return	_references[i];	}
		P<Code>	&references(uint16	i)	const{	return	(*_references.as_std())[i];	}
		uint16	references_size()	const{	return	_references.size();	}

		void	add_reference(Code	*object)	const{	_references.as_std()->push_back(object);	}
	};

	class	Mem{
	public:
		virtual	Code	*buildObject(SysObject	*source)=0;
		virtual	Code	*buildGroup(SysObject	*source)=0;
		virtual	void	deleteObject(Code	*object)=0;
		virtual	View	*find_view(Code	*object,Code	*group)=0;
	};
}


#endif