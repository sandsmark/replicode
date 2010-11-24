//	r_group.inline.h
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

	inline	RGroup::RGroup(r_code::Mem	*m):Group(m),parent(NULL),substitutions(NULL){
	}

	inline	RGroup::RGroup(r_code::SysObject	*source,r_code::Mem	*m):Group(source,m),parent(NULL),substitutions(NULL){
	}

	inline	RGroup::~RGroup(){

		if(!parent	&&	substitutions){

			delete	substitutions;
			delete	substitutionsCS;
		}
	}

	inline	Code	*RGroup::get_fwd_model()	const{

		return	fwd_model;
	}

	inline	void	RGroup::set_fwd_model(Code	*mdl){

		fwd_model=mdl;
		UNORDERED_MAP<uint32,P<View> >::const_iterator	v;
		for(v=group_views.begin();v!=group_views.end();++v)
			((RGroup	*)v->second->object)->set_fwd_model(mdl);
	}

	inline	RGroup	*RGroup::get_parent()	const{

		return	parent;
	}
}