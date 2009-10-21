#include	"rview.h"


namespace	replicode{

	Code::Code(){

		data.ensure(24);
		pointers.ensure(4);
	}

	void	Code::trace(){

		for(uint16	i=0;i<data.count();++i){

			std::cout<<i<<" > ";
			data[i].trace();
			std::cout<<std::endl;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	RObject::RObject()/*:RPayload<RObject,StaticData,Memory>()*/{
	}

	RObject::~RObject(){
	}

	uint8	RObject::ptrCount(){

		return	2;
	}

	Code	*RObject::code(){

		return	&_code;
	}

	__Payload	*RObject::getPtr(uint16	i){

		switch(i){
		case	0:	return	_code.data.getPtr(0);
		case	1:	return	_code.pointers.getPtr(0);
		default:
			return	NULL;
		}
	}

	void	RObject::setPtr(uint16	i,__Payload	*p){

		switch(i){
		case	0:	_code.data.setPtr(0,p);break;
		case	1:	_code.pointers.setPtr(0,p);break;
		default:	break;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	RView::RView(RObject	*object)/*:StreamData<RView,StaticData,Memory>()*/:object(object){
	}

	RView::~RView(){
	}

	void	RView::setSID(RAtom	a){

		_sid=a.asOpcode();
		if(a.getDescriptor()==RAtom::MARKER)
			_sid|=0x8000;
	}

	Code	*RView::code(){

		return	&_code;
	}

	uint8	RView::ptrCount(){

		return	2+(object==NULL?1:0);
	}

	__Payload	*RView::getPtr(uint16	i){

		switch(i){
		case	0:	return	object;
		case	1:	return	_code.data.getPtr(0);
		case	2:	return	_code.pointers.getPtr(0);
		default:
			return	NULL;
		}
	}

	void	RView::setPtr(uint16	i,__Payload	*p){

		switch(i){
		case	0:	object=(RObject	*)p;
		case	1:	_code.data.setPtr(0,p);break;
		case	2:	_code.pointers.setPtr(0,p);break;
		default:	break;
		}
	}

	RObject	*RView::getObject()	const{

		return	object;
	}

	void	RView::setObject(RObject	*o){

		object=o;
	}
	void	RView::setResilienceIndex(uint16	i){

		res_index=i;
	}
	
	void	RView::setSaliencyIndex(uint16	i){

		sln_index=i;
	}
	
	int32	RView::getResilience(){

		return	_code.data[res_index].asFloat();
	}

	void	RView::setResilience(int32	res){

		_code.data[res_index]=RAtom::Float(res);
	}
	
	float32	RView::getSaliency(){

		return	_code.data[sln_index].asFloat();
	}

	void	RView::setSaliency(float32	sln){

		_code.data[sln_index]=RAtom::Float(sln);
	}
}