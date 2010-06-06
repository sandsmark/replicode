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
			code[i]=source->code[i];
	}

	void	SysView::write(word32	*data){

		data[0]=code.size();
		data[1]=reference_set.size();
		uint32	i=0;
		for(;i<code.size();++i)
			data[2+i]=code[i].atom;
		for(uint32	j=0;j<reference_set.size();++j)
			data[2+i+j]=reference_set[j];
	}

	void	SysView::read(word32	*data){

		uint32	code_size=data[0];
		uint32	reference_set_size=data[1];
		uint32	i;
		uint32	j;
		for(i=0;i<code_size;++i)
			code.push_back(Atom(data[2+i]));
		for(j=0;j<reference_set_size;++j)
			reference_set.push_back(data[2+i+j]);
	}

	uint32	SysView::getSize()	const{

		return	2+code.size()+reference_set.size();
	}

	void	SysView::trace(){

		std::cout<<" code size: "<<code.size()<<std::endl;
		std::cout<<" reference set size: "<<reference_set.size()<<std::endl;
		std::cout<<"---code---"<<std::endl;
		for(uint32	i=0;i<code.size();++i){

			code[i].trace();
			std::cout<<std::endl;
		}
		std::cout<<"---reference set---"<<std::endl;
		for(uint32	i=0;i<reference_set.size();++i)
			std::cout<<reference_set[i]<<std::endl;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SysObject::SysObject(){
	}

	SysObject::SysObject(Object	*source){

		uint32	i;
		for(i=0;i<source->code.size();++i)
			code[i]=source->code[i];

		for(i=0;i<source->view_set.size();++i)
			view_set[i]=new	SysView(source->view_set[i]);
	}

	SysObject::~SysObject(){

		for(uint32	i=0;i<view_set.size();++i)
			delete	view_set[i];
	}

	void	SysObject::write(word32	*data){

		data[0]=code.size();
		data[1]=reference_set.size();
		data[2]=marker_set.size();
		data[3]=view_set.size();
		uint32	i;
		uint32	j;
		uint32	k;
		uint32	l;
		for(i=0;i<code.size();++i)
			data[4+i]=code[i].atom;
		for(j=0;j<reference_set.size();++j)
			data[4+i+j]=reference_set[j];
		for(k=0;k<marker_set.size();++k)
			data[4+i+j+k]=marker_set[k];
		uint32	offset=0;
		for(l=0;l<view_set.size();++l){

			view_set[l]->write(data+4+i+j+k+offset);
			offset+=view_set[l]->getSize();
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
			reference_set.push_back(data[4+i+j]);
		for(k=0;k<marker_set_size;++k)
			marker_set.push_back(data[4+i+j+k]);
		uint32	offset=0;
		for(l=0;l<view_set_size;++l){

			SysView	*v=new	SysView();
			v->read(data+4+i+j+k+offset);
			view_set.push_back(v);
			offset+=v->getSize();
		}
	}

	uint32	SysObject::getSize(){

		uint32	view_set_size=0;
		for(uint32	i=0;i<view_set.size();++i)
			view_set_size+=view_set[i]->getSize();
		return	4+code.size()+reference_set.size()+marker_set.size()+view_set_size;
	}

	void	SysObject::trace(){

		std::cout<<"\n---object---\n";
		std::cout<<"code size: "<<reference_set.size()<<std::endl;
		std::cout<<"reference set size: "<<reference_set.size()<<std::endl;
		std::cout<<"marker set size: "<<marker_set.size()<<std::endl;
		std::cout<<"view set size: "<<view_set.size()<<std::endl;
		std::cout<<"\n---code---\n";
		uint32	i;
		for(i=0;i<code.size();++i){

			std::cout<<i<<" ";
			code[i].trace();
			std::cout<<std::endl;
		}
		std::cout<<"\n---reference set---\n";
		for(i=0;i<reference_set.size();++i)
			std::cout<<i<<" "<<reference_set[i]<<std::endl;
		std::cout<<"\n---marker set---\n";
		for(i=0;i<marker_set.size();++i)
			std::cout<<i<<" "<<marker_set[i]<<std::endl;
		std::cout<<"\n---view set---\n";
		for(uint32	k=0;k<view_set.size();++k){

			std::cout<<"view["<<k<<"]"<<std::endl;
			std::cout<<"reference set size: "<<view_set[k]->reference_set.size()<<std::endl;
			std::cout<<"-code-"<<std::endl;
			uint32	j;
			for(j=0;j<view_set[k]->code.size();++i,++j){

				std::cout<<j<<" ";
				view_set[k]->code[j].trace();
				std::cout<<std::endl;
			}
			std::cout<<"-reference set-"<<std::endl;
			for(j=0;j<view_set[k]->reference_set.size();++i,++j)
				std::cout<<j<<" "<<view_set[k]->reference_set[j]<<std::endl;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Object::Object(){
	}

	Object::Object(SysObject	*source){

		uint32	i;
		for(i=0;i<source->code.size();++i)
			code[i]=source->code[i];

		for(i=0;i<source->view_set.size();++i)
			view_set[i]=new	View(source->view_set[i],this);
	}

	Object::~Object(){

		for(uint32	i=0;i<view_set.size();++i)
			delete	view_set[i];
	}

	uint16	Object::opcode()	const{

		return	(*code.as_std())[OBJECT_CLASS].asOpcode();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	View::View():object(NULL){

		reference_set[0]=reference_set[1]=NULL;
	}

	View::View(SysView	*source,Object	*object):object(object){

		for(uint32	i=0;i<source->code.size();++i)
			code[i]=source->code[i];
		reference_set[0]=reference_set[1]=NULL;
	}

	View::~View(){
	}
}