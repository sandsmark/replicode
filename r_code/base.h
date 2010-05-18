#ifndef	r_code_base_h
#define	r_code_base_h

#include	"types.h"


using	namespace	core;

namespace	r_code{

	//	Smart pointer (ref counting, deallocates when ref count<=0).
	//	No circular refs (use std c++ ptrs).
	//	No passing in functions (cast P<C> into C*).
	//	Cannot be a value returned by a function (return C* instead).
	template<class	C>	class	P{
	private:
		C	*object;
	public:
		P();
		P(C	*o);
		P(const P<C>	&p);
		~P();
		C	*operator	->()	const;
		template<class	D>	operator	D	*()	const{

			return	(D	*)object;
		}
		bool	operator	==(C	*c)	const;
		bool	operator	!=(C	*c)	const;
		bool	operator	!()	const;
		template<class	D>	bool	operator	==(P<D>	&p)	const;
		template<class	D>	bool	operator	!=(P<D>	&p)	const;
		P<C>	&operator	=(C	*c);
		P<C>	&operator	=(const  P<C>	&p);
		template<class	D>	P<C>	&operator	=(const P<D>	&p);
	};

	//	Root smart-pointable object class.
	class	dll_export	_Object{
	template<class	C>	friend	class	P;
	private:
		int32	volatile	refCount;
		void	incRef();	//	atomic operation
		void	decRef();	//	atomic operation
	protected:
		_Object();
	public:
		virtual	~_Object();
	};
}


#include	"base.tpl.cpp"


#endif