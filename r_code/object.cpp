#include	"object.h"

#include	<iostream>


namespace	r_code{

	SysView::SysView(){
	}

	SysView::SysView(View	*source){

		for(uint32	i=0;i<source->code.size();++i)
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
			view_set[i]=new	View(source->view_set[i]);
	}

	Object::~Object(){

		for(uint32	i=0;i<view_set.size();++i)
			delete	view_set[i];
	}

	uint32	Object::opcode(){

		return	code[0].asOpcode();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Group::Group(){
	}

	Group::~Group(){
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	View::View(){
	}

	View::View(SysView	*source){

		for(uint32	i=0;i<source->code.size();++i)
			code[i]=source->code[i];
	}

	View::~View(){
	}
}