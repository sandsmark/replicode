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


namespace	r_code{

	//	I/O from/to r_code::Image ////////////////////////////////////////////////////////////////////////

	class	dll_export	ImageObject{
	public:
		r_code::vector<Atom>	code;
		r_code::vector<uint32>	reference_set;	//	for views: 0, 1 or 2 elements; these are indexes in the relocation segment for grp (exception: not for root) and possibly org
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

	class	Object;

	class	dll_export	SysObject:
	public	ImageObject{
	public:
		r_code::vector<uint32>		marker_set;		//	indexes in the relocation segment
		r_code::vector<SysView	*>	view_set;

		SysObject();
		SysObject(Object	*source);
		~SysObject();

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	getSize();
		void	trace();
	};

	// Interfaces for r_exec classes ////////////////////////////////////////////////////////////////////////

	class	View;

	class	dll_export	Object:
	public	_Object{
	public:
		r_code::vector<Atom>		code;
		r_code::vector<P<Object> >	marker_set;
		r_code::vector<P<Object> >	reference_set;
		r_code::vector<View	*>		view_set;	//	used only for initialization from an image.

		Object();
		Object(SysObject	*source);
		virtual	~Object();

		uint16	opcode()	const;
	};

	class	dll_export	View:
	public	_Object{
	public:
		P<Object>	object;						//	viewed object.
		Atom		code[VIEW_CODE_MAX_SIZE];	//	dimensioned to hold the largest view (group view): head atom, iptr to ijt, sln, res, rptr to grp, rptr to org, vis, cov, 3 atoms for ijt's timestamp.
		Object		*reference_set[2];			//	does not include the viewed object; no smart pointer here (a view is held by a group and holds a ref to said group).

		View();
		View(SysView	*source,Object	*object);
		virtual	~View();
	};

	class	Mem{
	public:
		virtual	Object	*buildObject(SysObject	*source)=0;
		virtual	Object	*buildGroup(SysObject	*source)=0;
		virtual	Object	*buildInstantiatedProgram(SysObject	*source)=0;
		virtual	Object	*buildMarker(SysObject	*source)=0;
	};
}


#endif