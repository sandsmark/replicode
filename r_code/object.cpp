//	object.cpp
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

#include	"object.h"
#include	"replicode_defs.h"

#include	<iostream>


namespace	r_code{

	SysView::SysView(){
	}

	SysView::SysView(View	*source){

		for(uint32	i=0;i<VIEW_CODE_MAX_SIZE;++i)
			code[i]=source->code(i);
	}

	void	SysView::write(word32	*data){

		data[0]=code.size();
		data[1]=references.size();
		uint32	i=0;
		for(;i<code.size();++i)
			data[2+i]=code[i].atom;
		for(uint32	j=0;j<references.size();++j)
			data[2+i+j]=references[j];
	}

	void	SysView::read(word32	*data){

		uint32	code_size=data[0];
		uint32	reference_set_size=data[1];
		uint32	i;
		uint32	j;
		for(i=0;i<code_size;++i)
			code.push_back(Atom(data[2+i]));
		for(j=0;j<reference_set_size;++j)
			references.push_back(data[2+i+j]);
	}

	uint32	SysView::getSize()	const{

		return	2+code.size()+references.size();
	}

	void	SysView::trace(){

		std::cout<<" code size: "<<code.size()<<std::endl;
		std::cout<<" reference set size: "<<references.size()<<std::endl;
		std::cout<<"---code---"<<std::endl;
		for(uint32	i=0;i<code.size();++i){

			code[i].trace();
			std::cout<<std::endl;
		}
		std::cout<<"---reference set---"<<std::endl;
		for(uint32	i=0;i<references.size();++i)
			std::cout<<references[i]<<std::endl;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SysObject::SysObject(){
	}

	SysObject::SysObject(Code	*source){

		uint32	i;
		for(i=0;i<source->code_size();++i)
			code[i]=source->code(i);

		UNORDERED_SET<View	*,View::Hash,View::Equal>::const_iterator	v;
		for(i=0,v=source->views.begin();v!=source->views.end();++i,++v)
			views[i]=new	SysView(*v);
	}

	SysObject::~SysObject(){

		for(uint32	i=0;i<views.size();++i)
			delete	views[i];
	}

	void	SysObject::write(word32	*data){

		data[0]=code.size();
		data[1]=references.size();
		data[2]=markers.size();
		data[3]=views.size();
		uint32	i;
		uint32	j;
		uint32	k;
		uint32	l;
		for(i=0;i<code.size();++i)
			data[4+i]=code[i].atom;
		for(j=0;j<references.size();++j)
			data[4+i+j]=references[j];
		for(k=0;k<markers.size();++k)
			data[4+i+j+k]=markers[k];
		uint32	offset=0;
		for(l=0;l<views.size();++l){

			views[l]->write(data+4+i+j+k+offset);
			offset+=views[l]->getSize();
		}
	}

	void	SysObject::read(word32	*data){

		uint32	code_size=data[0];
		uint32	reference_set_size=data[1];
		uint32	marker_set_size=data[2];
		uint32	view_set_size=data[3];
		uint32	i;
		uint32	j;
		uint32	k;
		uint32	l;
		for(i=0;i<code_size;++i)
			code.push_back(Atom(data[4+i]));
		for(j=0;j<reference_set_size;++j)
			references.push_back(data[4+i+j]);
		for(k=0;k<marker_set_size;++k)
			markers.push_back(data[4+i+j+k]);
		uint32	offset=0;
		for(l=0;l<view_set_size;++l){

			SysView	*v=new	SysView();
			v->read(data+4+i+j+k+offset);
			views.push_back(v);
			offset+=v->getSize();
		}
	}

	uint32	SysObject::getSize(){

		uint32	view_set_size=0;
		for(uint32	i=0;i<views.size();++i)
			view_set_size+=views[i]->getSize();
		return	4+code.size()+references.size()+markers.size()+view_set_size;
	}

	void	SysObject::trace(){

		std::cout<<"\n---object---\n";
		std::cout<<"code size: "<<references.size()<<std::endl;
		std::cout<<"reference set size: "<<references.size()<<std::endl;
		std::cout<<"marker set size: "<<markers.size()<<std::endl;
		std::cout<<"view set size: "<<views.size()<<std::endl;
		std::cout<<"\n---code---\n";
		uint32	i;
		for(i=0;i<code.size();++i){

			std::cout<<i<<" ";
			code[i].trace();
			std::cout<<std::endl;
		}
		std::cout<<"\n---reference set---\n";
		for(i=0;i<references.size();++i)
			std::cout<<i<<" "<<references[i]<<std::endl;
		std::cout<<"\n---marker set---\n";
		for(i=0;i<markers.size();++i)
			std::cout<<i<<" "<<markers[i]<<std::endl;
		std::cout<<"\n---view set---\n";
		for(uint32	k=0;k<views.size();++k){

			std::cout<<"view["<<k<<"]"<<std::endl;
			std::cout<<"reference set size: "<<views[k]->references.size()<<std::endl;
			std::cout<<"-code-"<<std::endl;
			uint32	j;
			for(j=0;j<views[k]->code.size();++i,++j){

				std::cout<<j<<" ";
				views[k]->code[j].trace();
				std::cout<<std::endl;
			}
			std::cout<<"-reference set-"<<std::endl;
			for(j=0;j<views[k]->references.size();++i,++j)
				std::cout<<j<<" "<<views[k]->references[j]<<std::endl;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Code::Code(){
	}

	Code::~Code(){
	}

	void	Code::load(SysObject	*source){

		for(uint16	i=0;i<source->code.size();++i)
			code(i)=source->code[i];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Object::Object():Code(){
	}

	Object::Object(SysObject	*source){

		load(source);
		build_views<View>(source);
	}

	Object::~Object(){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	View::View():object(NULL){

		references[0]=references[1]=NULL;
	}

	View::View(SysView	*source,Code	*object):object(object){

		for(uint32	i=0;i<source->code.size();++i)
			_code[i]=source->code[i];
		references[0]=references[1]=NULL;
	}

	View::~View(){
	}
}