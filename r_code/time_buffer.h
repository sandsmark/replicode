//	time_buffer.h
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

#ifndef	r_code_time_buffer_h
#define	r_code_time_buffer_h

#include	"list.h"


using	namespace	core;

namespace	r_code{

	// Time limited buffer.
	// T is expected a function: bool is_invalidated(uint64	time_reference,uint32	thz) const where time_reference and thz are valuated with the buffer's own.
	template<typename	T,class	IsInvalidated>	class	time_buffer:
	public	list<T>{
	protected:
		uint32	thz;	// time horizon.
		uint64	time_reference;
	public:
		time_buffer():list(),thz(Utils::MaxTHZ){}

		void	set_thz(uint32	thz){	this->thz=thz;	}
		
		class	iterator{
		friend	class	time_buffer;
		private:
			time_buffer	*buffer;
			int32	_cell;
			iterator(time_buffer	*b,int32	c):buffer(b),_cell(c){}
		public:
			iterator():buffer(NULL),_cell(null){}
			T	&operator	*()		const{	return	buffer->cells[_cell].data;	}
			T	*operator	->()	const{	return	&(buffer->cells[_cell].data);	}
			iterator	&operator	++(){	// moves to the next time-compliant cell and erase old cells met in the process.
				
				_cell=buffer->cells[_cell].next;
				if(_cell!=null){

					IsInvalidated	i;
check:				if(i(buffer->cells[_cell].data,buffer->time_reference,buffer->thz)){

						_cell=buffer->_erase(_cell);
						if(_cell!=null)
							goto	check;
					}
				}
				return	*this;
			}
			bool	operator==(iterator	&i)	const{	return	_cell==i._cell;	}
			bool	operator!=(iterator	&i)	const{	return	_cell!=i._cell;	}
		};
	private:
		static	iterator	end_iterator;
	public:
		iterator	begin(uint64	time_reference){
		
			this->time_reference=time_reference;
			return	iterator(this,used_cells_head);
		}
		iterator	&end(){	return	end_iterator;	}
		iterator	find(uint64	time_reference,const	T	&t){

			iterator	i;
			for(i=begin(time_reference);i!=end();++i){

				if((*i)==t)
					return	i;
			}
			return	end_iterator;
		}
		iterator	find(const	T	&t){

			for(int32	c=used_cells_head;c!=null;c=_cells[c].next){

				if(_cells[c].data==t)
					return	iterator(this,c);
			}
			return	end_iterator;
		}
		iterator	erase(iterator	&i){	return	iterator(this,_erase(i._cell));	}
	};

	template<typename	T,class	IsInvalidated>	typename	time_buffer<T,IsInvalidated>::iterator	time_buffer<T,IsInvalidated>::end_iterator;
}


#endif
