#include	"base.h"
#include	"../CoreLibrary/utils.h"


namespace	r_code{

	_Object::_Object():refCount(0){
	}

	_Object::~_Object(){
	}

	void	_Object::incRef(){

		Atomic::Increment32(&refCount);
	}

	inline	void	_Object::decRef(){

		if(Atomic::Decrement32(&refCount)==0)
			delete	this;
	}
}