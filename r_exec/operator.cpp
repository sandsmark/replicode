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
#include	"mem.h"
#include	"init.h"
#include	"opcodes.h"
#include	"group.h"
#include	"../CoreLibrary/utils.h"
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

				bool	r=Timestamp::Get(&lhs[0])>Timestamp::Get(&rhs[0]);
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

				bool	r=Timestamp::Get(&lhs[0])<Timestamp::Get(&rhs[0]);
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

				bool	r=Timestamp::Get(&lhs[0])>=Timestamp::Get(&rhs[0]);
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

				bool	r=Timestamp::Get(&lhs[0])<=Timestamp::Get(&rhs[0]);
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

					index=context.setTimestampResult(Timestamp::Get(&rhs[0])+lhs[0].asFloat());
					return	true;
				}
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				index=context.setTimestampResult(Timestamp::Get(&lhs[0])+Timestamp::Get(&rhs[0]));
				return	true;
			}else	if(rhs[0].isFloat()){

				if(rhs[0]!=Atom::PlusInfinity()){

					index=context.setTimestampResult(Timestamp::Get(&lhs[0])+rhs[0].asFloat());
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

				index=context.setTimestampResult(Timestamp::Get(&lhs[0])-Timestamp::Get(&rhs[0]));
				return	true;
			}else	if(rhs[0].isFloat()){

				if(rhs[0]!=Atom::PlusInfinity()){

					index=context.setTimestampResult(Timestamp::Get(&lhs[0])-rhs[0].asFloat());
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

				index=context.setAtomicResult(Atom::Float(Timestamp::Get(&rhs[0])*lhs[0].asFloat()));
				return	true;
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].isFloat()){

				index=context.setTimestampResult(Timestamp::Get(&lhs[0])*rhs[0].asFloat());
				return	true;
			}else	if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				index=context.setAtomicResult(Atom::Float(Timestamp::Get(&lhs[0])*Timestamp::Get(&lhs[0])));
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

				float64	rhs_t=(float64)Timestamp::Get(&rhs[0]);
				if(rhs_t!=0){

					index=context.setAtomicResult(Atom::Float(lhs[0].asFloat()/rhs_t));
					return	true;
				}
			}
		}else	if(lhs[0].getDescriptor()==Atom::TIMESTAMP){

			if(rhs[0].isFloat()){
				
				if(rhs[0].asFloat()!=0){

					float64	lhs_t=(float64)Timestamp::Get(&lhs[0]);
					index=context.setTimestampResult(lhs_t/rhs[0].asFloat());
					return	true;
				}
			}else	if(rhs[0].getDescriptor()==Atom::TIMESTAMP){

				float64	rhs_t=(float64)Timestamp::Get(&rhs[0]);
				if(rhs_t!=0){

					float64	lhs_t=(float64)Timestamp::Get(&lhs[0]);
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

				uint64	lhs_t=Timestamp::Get(&lhs[0]);
				uint64	rhs_t=Timestamp::Get(&rhs[0]);
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

		Context	object=*context.getChild(1);
		Context	args=*context.getChild(2);

		Code	*_object=object.getObject();
		if(_object->code(0).asOpcode()!=Opcodes::PGM	&&	_object->code(0).asOpcode()!=Opcodes::AntiPGM	&&		
			_object->code(0).asOpcode()!=Opcodes::Goal	&&	_object->code(0).asOpcode()!=Opcodes::AntiGoal){

			context.setAtomicResult(Atom::Nil());
			return	false;
		}

		if(_object	&&	args[0].getDescriptor()==Atom::SET){

			uint16	pattern_set_index=_object->code(PGM_TPL_ARGS).asIndex();	//	same index for goals.
			uint16	arg_count=args[0].getAtomCount();
			if(_object->code(pattern_set_index).getAtomCount()!=arg_count){

				context.setAtomicResult(Atom::Nil());
				return	false;
			}
			
			//	match args with the tpl patterns in _object.
			Context	pattern_set(_object,pattern_set_index);
			for(uint16	i=1;i<=arg_count;++i){

				Context	arg=*args.getChild(i);
				Context	skel=*(*pattern_set.getChild(i)).getChild(1);
				if(!skel.match(arg)){

					context.setAtomicResult(Atom::Nil());
					return	false;
				}
			}

			//	create an ipgm or an igol in the production array.
			Code	*instantiated_object=context.buildObject(_object->code(0));
			uint16	write_index=0;
			if(_object->code(0).asOpcode()==Opcodes::PGM	||	_object->code(0).asOpcode()==Opcodes::AntiPGM)			
				instantiated_object->code(write_index++)=Atom::InstantiatedProgram(Opcodes::IPGM,IPGM_IGOL_ARITY);
			else	if(_object->code(0).asOpcode()==Opcodes::Goal	||	_object->code(0).asOpcode()==Opcodes::AntiGoal)
				instantiated_object->code(write_index++)=Atom::Object(Opcodes::IGoal,IPGM_IGOL_ARITY);
			instantiated_object->code(write_index++)=Atom::RPointer(0);	//	points to the pgm/gol object.
			instantiated_object->code(write_index++)=Atom::IPointer(IPGM_IGOL_ARITY+1);
			instantiated_object->code(write_index++)=Atom::Float(1);	//	psln_thr.
			args.copy(instantiated_object,write_index);					//	writes after psln_thr.
			instantiated_object->set_reference(0,_object);
			context.setAtomicResult(Atom::ProductionPointer(context.addProduction(instantiated_object,true)));	// object may be new: we don't know at this point.
			return	true;
		}
		
		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	match(const	Context	&input,const	Context	&pattern){	//	in red, patterns like (ptn object: [guards]) are allowed.

		//	patch the pattern with a ptr to the input.
		if(input.is_reference()){

			uint16	ptr=pattern.addProduction(input.getObject(),false);	//	the object obviously is not new.
			pattern.patch_code(pattern.getIndex()+1,Atom::ProductionPointer(ptr));
		}else
			pattern.patch_code(pattern.getIndex()+1,Atom::IPointer(input.getIndex()));

		//	evaluate the set of guards.
		Context	guard_set=*pattern.getChild(2);
		uint16	guard_count=guard_set.getChildrenCount();
		uint16	unused_index;
		for(uint16	i=1;i<=guard_count;++i){

			if(!(*guard_set.getChild(i)).evaluate_no_dereference(unused_index))	//	WARNING: no check for duplicates.
				return	false;
		}
	}

	bool	match(const	Context	&input,const	Context	&pattern,const	Context	&productions,std::vector<uint16>	&production_indices){

		Context	&skeleton=Context();
		uint16	last_patch_index;
		if(pattern[0].asOpcode()==Opcodes::PTN){
			
			skeleton=*pattern.getChild(1);
			if(!skeleton.match(input))
				return	false;
			last_patch_index=pattern.get_last_patch_index();
			if(!match(input,pattern))
				return	false;

			goto	build_productions;
		}

		if(pattern[0].asOpcode()==Opcodes::AntiPTN){
			
			skeleton=*pattern.getChild(1);
			if(skeleton.match(input))
				return	false;
			last_patch_index=pattern.get_last_patch_index();
			if(match(input,pattern))
				return	false;

			goto	build_productions;
		}

		return	false;

build_productions:
		//	compute all productions for this input.
		uint16	production_count=productions.getChildrenCount();
		uint16	unused_index;
		uint16	production_index;
		for(uint16	i=1;i<=production_count;++i){

			Context	prod=productions.getChild(i);
//prod.trace();
			prod.evaluate(unused_index);
			prod.copy_to_value_array(production_index);
			production_indices.push_back(production_index);
		}

		pattern.unpatch_code(last_patch_index);
		return	true;
	}

	void	reduce(const	Context	&context,const	Context	&input_set,const	Context	&section,std::vector<uint16>	&input_indices,std::vector<uint16>	&production_indices){

		Context	pattern=*section.getChild(1);
		if(pattern[0].asOpcode()!=Opcodes::PTN	&&	pattern[0].asOpcode()!=Opcodes::AntiPTN)
			return;

		Context	productions=*section.getChild(2);
		if(productions[0].getDescriptor()!=Atom::SET)
			return;

		uint16	production_count=productions.getChildrenCount();
		if(!production_count)
			return;

		std::vector<uint16>::iterator	i;
		for(i=input_indices.begin();i!=input_indices.end();){	//	to be successful, at least one input must match the pattern.

			Context	c=*input_set.getChild(*i);
			if(c.is_undefined()){

				i=input_indices.erase(i);
				continue;
			}

			bool	failure=false;
			if(!match(c,pattern,productions,production_indices)){

				failure=true;
				break;
			}

			if(failure)
				++i;
			else	//	pattern matched: remove the input from the todo list.
				i=input_indices.erase(i);
		}
	}

	bool	red(const	Context	&context,uint16	&index){
//context.trace();
		uint16	unused_result_index;
		Context	input_set=*context.getChild(1);
		if(!input_set.evaluate_no_dereference(unused_result_index))
			return	false;

		//	a section is a set of one pattern and a set of productions.
		Context	positive_section=*context.getChild(2);
		if(!(*positive_section.getChild(1)).evaluate_no_dereference(unused_result_index))	//	evaluate the pattern only.
			return	false;

		Context	negative_section=*context.getChild(3);
		if(!(*negative_section.getChild(1)).evaluate_no_dereference(unused_result_index))	//	evaluate the pattern only.
			return	false;

		std::vector<uint16>	input_indices;		//	todo list of inputs to match.
		for(uint16	i=1;i<=input_set.getChildrenCount();++i)
			input_indices.push_back(i);

		std::vector<uint16>	production_indices;	//	list of productions built upon successful matches.

		if(input_set[0].getDescriptor()!=Atom::SET	&&
			input_set[0].getDescriptor()!=Atom::S_SET	&&
			positive_section[0].getDescriptor()!=Atom::SET	&&
			negative_section[0].getDescriptor()!=Atom::SET)
			goto	failure;

		uint16	input_count=input_set.getChildrenCount();
		if(!input_count)
			goto	failure;

		reduce(context,input_set,positive_section,input_indices,production_indices);	//	input_indices now filled only with the inputs that did not match the positive pattern.
		reduce(context,input_set,negative_section,input_indices,production_indices);	//	input_indices now filled only with the inputs that did not match the positive nor the negative pattern.
		if(production_indices.size()){

			//	build the set of all productions in the value array.
			index=context.setCompoundResultHead(Atom::Set(production_indices.size()));
			for(uint16	i=0;i<production_indices.size();++i)	//	fill the set with iptrs to productions: the latter are copied in the value array.
				context.addCompoundResultPart(Atom::IPointer(production_indices[i]));
			//(*context).trace();
			return	true;
		}
failure:
		index=context.setCompoundResultHead(Atom::Set(0));
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	fvw(const	Context	&context,uint16	&index){

		Context	object=*context.getChild(1);
		Context	group=*context.getChild(2);

		Code	*_object=object.getObject();
		Group	*_group=(Group	*)group.getObject();
		if(!_object	||	!_group){

			context.setAtomicResult(Atom::Nil());
			return	false;
		}

		View	*v=(View	*)_object->find_view(_group,true);	//	returns a copy of the view, if any.
		if(v){	//	copy the view in the value array: code on VIEW_CODE_MAX_SIZE followed by 2 atoms holding raw pointers to grp and org.

			index=context.setCompoundResultHead(v->code(0));
			for(uint16	i=1;i<VIEW_CODE_MAX_SIZE;++i)
				context.addCompoundResultPart(v->code(i));
			context.addCompoundResultPart(Atom((uint32)v->references[0]));
			context.addCompoundResultPart(Atom((uint32)v->references[1]));
			delete	v;
			return	true;
		}

		index=context.setAtomicResult(Atom::Nil());
		return	false;
	}
}