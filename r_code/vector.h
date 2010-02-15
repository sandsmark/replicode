#ifndef	vector_h
#define	vector_h

#include	"types.h"


namespace	r_code{

	//	auto-resized vector
	template<typename	T>	class	vector:
	private	std::vector<T>{
	public:
		uint32	size()	const{	return	std::vector<T>::size();	}
		T	&operator	[](uint32	i){

			if(i>=size())
				resize(i+1);
			return	std::vector<T>::operator [](i);
		}
		void	push_back(T	t){	std::vector<T>::push_back(t);	}
	};
}


#endif