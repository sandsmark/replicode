//	operator.cpp
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

#include	"operator.h"
#include	"context.h"
#include	"mem.h"
#include	"init.h"
#include	"opcodes.h"
#include	"group.h"
#include	"../../CoreLibrary/trunk/CoreLibrary/utils.h"
#include	"../r_code/utils.h"
#include	<math.h>


namespace	r_exec{

	r_code::vector<Operator>	Operator::Operators;

	void	Operator::Register(uint16	opcode,bool	(*o)(const	Context	&,uint16	&index)){

		if(Operators[opcode]._operator)
			Operators[opcode].setOverload(o);
		else
			Operators[opcode]=Operator(o);
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	now(const	Context	&context,uint16	&index){

		index=context.setTimestampResult(Now());
		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	rnd(const	Context	&context,uint16	&index){

		Context	range=*context.getChild(1);

		if(!range[0].isFloat()){

			index=context.setAtomicResult(Atom::Nil());
			return	false;
		}

		/*Random	r;float32	rng=range[0].asFloat();
		float32	result=r(range[0].asFloat());
		result/=ULONG_MAX;*/
		float32	result=(((float32)(rand()%100))/100)*range[0].asFloat();
		index=context.setAtomicResult(Atom::Float(result));
		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	equ(const	Context	&context,uint16	&index){

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		bool	r=(lhs==rhs);
		index=context.setAtomicResult(Atom::Boolean(r));
		return	r;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	neq(const	Context	&context,uint16	&index){

		bool	r=*context.getChild(1)!=*context.getChild(2);
		index=context.setAtomicResult(Atom::Boolean(r));
		return	r;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	gtr(const	Context	&context,uint16	&index){

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		if(lhs[0].isFloat()){

			if(rhs[0].isFloat()){

				bool	r=lhs[0].asFloat()>rhs[0].asFloat();
				index=context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				bool	r=Utils::GetTimestamp(&lhs[0])>Utils::GetTimestamp(&rhs[0]);
				index=context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}

		index=context.setAtomicResult(Atom::UndefinedBoolean());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	lsr(const	Context	&context,uint16	&index){

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		if(lhs[0].isFloat()){

			if(rhs[0].isFloat()){

				bool	r=lhs[0].asFloat()<rhs[0].asFloat();
				index=context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				bool	r=Utils::GetTimestamp(&lhs[0])<Utils::GetTimestamp(&rhs[0]);
				index=context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}

		index=context.setAtomicResult(Atom::UndefinedBoolean());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	gte(const	Context	&context,uint16	&index){

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		if(lhs[0].isFloat()){

			if(rhs[0].isFloat()){

				bool	r=lhs[0].asFloat()>=rhs[0].asFloat();
				index=context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				bool	r=Utils::GetTimestamp(&lhs[0])>=Utils::GetTimestamp(&rhs[0]);
				index=context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}

		index=context.setAtomicResult(Atom::UndefinedBoolean());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	lse(const	Context	&context,uint16	&index){

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		if(lhs[0].isFloat()){

			if(rhs[0].isFloat()){

				bool	r=lhs[0].asFloat()<=rhs[0].asFloat();
				index=context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				bool	r=Utils::GetTimestamp(&lhs[0])<=Utils::GetTimestamp(&rhs[0]);
				index=context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}

		index=context.setAtomicResult(Atom::UndefinedBoolean());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	add(const	Context	&context,uint16	&index){

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		if(lhs[0].isFloat()){

			if(rhs[0].isFloat()){

				if(lhs[0]==Atom::PlusInfinity()){

					index=context.setAtomicResult(Atom::PlusInfinity());
					return	true;
				}

				if(rhs[0]==Atom::PlusInfinity()){

					index=context.setAtomicResult(Atom::PlusInfinity());
					return	true;
				}

				index=context.setAtomicResult(Atom::Float(lhs[0].asFloat()+rhs[0].asFloat()));
				return	true;
			}else	if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				if(lhs[0]!=Atom::PlusInfinity()){

					index=context.setTimestampResult(Utils::GetTimestamp(&rhs[0])+lhs[0].asFloat());
					return	true;
				}
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				index=context.setTimestampResult(Utils::GetTimestamp(&lhs[0])+Utils::GetTimestamp(&rhs[0]));
				return	true;
			}else	if(rhs[0].isFloat()){

				if(rhs[0]!=Atom::PlusInfinity()){

					index=context.setTimestampResult(Utils::GetTimestamp(&lhs[0])+rhs[0].asFloat());
					return	true;
				}
			}
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	sub(const	Context	&context,uint16	&index){

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		if(lhs[0].isFloat()){

			if(rhs[0].isFloat()){

				if(lhs[0]==Atom::PlusInfinity()){

					index=context.setAtomicResult(Atom::PlusInfinity());
					return	true;
				}

				if(rhs[0]==Atom::PlusInfinity()){

					index=context.setAtomicResult(Atom::Float(0));
					return	true;
				}

				index=context.setAtomicResult(Atom::Float(lhs[0].asFloat()-rhs[0].asFloat()));
				return	true;
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				index=context.setTimestampResult(Utils::GetTimestamp(&lhs[0])-Utils::GetTimestamp(&rhs[0]));
				return	true;
			}else	if(rhs[0].isFloat()){

				if(rhs[0]!=Atom::PlusInfinity()){

					index=context.setTimestampResult(Utils::GetTimestamp(&lhs[0])-rhs[0].asFloat());
					return	true;
				}
			}
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	mul(const	Context	&context,uint16	&index){

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		if(lhs[0].isFloat()){

			if(rhs[0].isFloat()){

				if(lhs[0]==Atom::PlusInfinity()){
					
					if(rhs[0]==Atom::PlusInfinity()){

						index=context.setAtomicResult(Atom::PlusInfinity());
						return	true;
					}

					if(rhs[0].asFloat()>0){

						index=context.setAtomicResult(Atom::PlusInfinity());
						return	true;
					}

					if(rhs[0].asFloat()<=0){

						index=context.setAtomicResult(Atom::Float(0));
						return	true;
					}
				}

				if(rhs[0]==Atom::PlusInfinity()){

					if(lhs[0].asFloat()>0){

						index=context.setAtomicResult(Atom::PlusInfinity());
						return	true;
					}

					if(lhs[0].asFloat()<=0){

						index=context.setAtomicResult(Atom::Float(0));
						return	true;
					}
				}

				index=context.setAtomicResult(Atom::Float(lhs[0].asFloat()*rhs[0].asFloat()));
				return	true;
			}else	if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				index=context.setAtomicResult(Atom::Float(Utils::GetTimestamp(&rhs[0])*lhs[0].asFloat()));
				return	true;
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].isFloat()){

				index=context.setTimestampResult(Utils::GetTimestamp(&lhs[0])*rhs[0].asFloat());
				return	true;
			}else	if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				index=context.setAtomicResult(Atom::Float(Utils::GetTimestamp(&lhs[0])*Utils::GetTimestamp(&lhs[0])));
				return	true;
			}
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	div(const	Context	&context,uint16	&index){

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		if(lhs[0].isFloat()){

			if(rhs[0].isFloat()){
				
				if(rhs[0].asFloat()!=0){

					if(lhs[0]==Atom::PlusInfinity()){

						if(rhs[0]==Atom::PlusInfinity()){

							index=context.setAtomicResult(Atom::PlusInfinity());
							return	true;
						}

						if(rhs[0].asFloat()>0){

							index=context.setAtomicResult(Atom::PlusInfinity());
							return	true;
						}

						if(rhs[0].asFloat()<=0){

							index=context.setAtomicResult(Atom::Float(0));
							return	true;
						}
					}

					if(rhs[0]==Atom::PlusInfinity()){

						if(lhs[0].asFloat()>0){

							index=context.setAtomicResult(Atom::PlusInfinity());
							return	true;
						}

						if(lhs[0].asFloat()<=0){

							index=context.setAtomicResult(Atom::Float(0));
							return	true;
						}
					}

					index=context.setAtomicResult(Atom::Float(lhs[0].asFloat()/rhs[0].asFloat()));
					return	true;
				}
			}else	if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				float64	rhs_t=(float64)Utils::GetTimestamp(&rhs[0]);
				if(rhs_t!=0){

					index=context.setAtomicResult(Atom::Float(lhs[0].asFloat()/rhs_t));
					return	true;
				}
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].isFloat()){
				
				if(rhs[0].asFloat()!=0){

					float64	lhs_t=(float64)Utils::GetTimestamp(&lhs[0]);
					index=context.setTimestampResult(lhs_t/rhs[0].asFloat());
					return	true;
				}
			}else	if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				float64	rhs_t=(float64)Utils::GetTimestamp(&rhs[0]);
				if(rhs_t!=0){

					float64	lhs_t=(float64)Utils::GetTimestamp(&lhs[0]);
					index=context.setAtomicResult(Atom::Float(lhs_t/rhs_t));
					return	true;
				}
			}
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	dis(const	Context	&context,uint16	&index){	

		Context	lhs=*context.getChild(1);
		Context	rhs=*context.getChild(2);

		if(lhs[0].isFloat()){

			if(rhs[0].isFloat()){

				index=context.setAtomicResult(Atom::Float(abs(lhs[0].asFloat()-rhs[0].asFloat())));
				return	true;
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				uint64	lhs_t=Utils::GetTimestamp(&lhs[0]);
				uint64	rhs_t=Utils::GetTimestamp(&rhs[0]);
				index=context.setTimestampResult(abs((float64)(lhs_t-rhs_t)));
				return	true;
			}
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	ln(const	Context	&context,uint16	&index){

		Context	arg=*context.getChild(1);
		
		if(arg[0].isFloat()){

			if(arg[0].asFloat()!=0){
				
				index=context.setAtomicResult(Atom::Float(::log(arg[0].asFloat())));
				return	true;
			}
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	exp(const	Context	&context,uint16	&index){

		Context	arg=*context.getChild(1);
		
		if(arg[0].isFloat()){

			index=context.setAtomicResult(Atom::Float(::exp(arg[0].asFloat())));
			return	true;
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	log(const	Context	&context,uint16	&index){

		Context	arg=*context.getChild(1);
		
		if(arg[0].isFloat()){

			if(arg[0].asFloat()!=0){
				
				index=context.setAtomicResult(Atom::Float(log10(arg[0].asFloat())));
				return	true;
			}
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	e10(const	Context	&context,uint16	&index){

		Context	arg=*context.getChild(1);
		
		if(arg[0].isFloat()){

			index=context.setAtomicResult(Atom::Float(pow(10,arg[0].asFloat())));
			return	true;
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	syn(const	Context	&context,uint16	&index){

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	ins(const	Context	&context,uint16	&index){

		return	IPGMContext::Ins(*(IPGMContext	*)context.get_implementation(),index);
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	red(const	Context	&context,uint16	&index){

		return	IPGMContext::Red(*(IPGMContext	*)context.get_implementation(),index);
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	fvw(const	Context	&context,uint16	&index){

		return	IPGMContext::Fvw(*(IPGMContext	*)context.get_implementation(),index);
	}
}