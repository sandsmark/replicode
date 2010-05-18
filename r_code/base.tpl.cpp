namespace	r_code{

	template<class	C>	inline	P<C>::P():object(NULL){
	}

	template<class	C>	inline	P<C>::P(C	*o):object(o){

		if(object)
			object->incRef();
	}

	template<class	C>	inline	P<C>::P(const P<C>	&p):object(p.object){

		if(object)
			object->incRef();
	}

	template<class	C>	inline	P<C>::~P(){

		if(object)
			object->decRef();
	}

	template<class	C>	inline	C	*P<C>::operator	->()	const{

		return	(C	*)object;
	}

	template<class	C>	inline	bool	P<C>::operator	==(C	*c)	const{

		return	object==c;
	}

	template<class	C>	inline	bool	P<C>::operator	!=(C	*c)	const{

		return	object!=c;
	}

	template<class	C>	template<class	D>	inline	bool	P<C>::operator	==(P<D>	&p)	const{

		return	object==p.object;
	}

	template<class	C>	template<class	D>	inline	bool	P<C>::operator	!=(P<D>	&p)	const{

		return	object!=p.object;
	}

	template<class	C>	inline	bool	P<C>::operator	!()	const{

		return	!object;
	}

	template<class	C>	inline	P<C>&	P<C>::operator	=(C	*c){

		if(object==c)
			return	*this;
		if(object)
			object->decRef();
		if(object=c)
			object->incRef();

		return	*this;
	}

	template<class	C>	template<class	D>	inline	P<C>	&P<C>::operator	=(const P<D>	&p){

		return	this->operator	=((C	*)p.object);
	}

	template<class	C>	inline	P<C>	&P<C>::operator	=(const P<C>	&p){

		return	this->operator	=((C	*)p.object);
	}
}