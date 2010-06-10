#include	"operator.h"
#include	"mem.h"
#include	"../CoreLibrary/utils.h"
#include	"../r_code/utils.h"
#include	<math.h>


namespace	r_exec{

	std::vector<Operator>	Operator::Operators;

	void	Operator::Register(uint16	opcode,bool	(*o)(const	Context	&)){

		if(opcode>=Operators.size())
			Operators.reserve(opcode+1);
			
		if(Operators[opcode]._operator)
			Operators[opcode].setOverload(o);
		else
			Operators[opcode]=Operator(o);
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	now(const	Context	&context){

		context.setTimestampResult(Mem::Now());
		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	equ(const	Context	&context){

		bool	r=context.getChild(1)==context.getChild(2);
		context.setAtomicResult(Atom::Boolean(r));
		return	r;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	neq(const	Context	&context){

		bool	r=context.getChild(1)!=context.getChild(2);
		context.setAtomicResult(Atom::Boolean(r));
		return	r;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	gtr(const	Context	&context){

		Context	lhs=context.getChild(1);
		Context	rhs=context.getChild(2);

		if(lhs.getCode()->isFloat()){

			if(rhs.getCode()->isFloat()){

				bool	r=lhs.getCode()->asFloat()>rhs.getCode()->asFloat();
				context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}else	if(lhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

			if(rhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

				bool	r=Timestamp::Get(lhs.getCode())>Timestamp::Get(rhs.getCode());
				context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}

		context.setAtomicResult(Atom::UndefinedBoolean());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	lsr(const	Context	&context){

		Context	lhs=context.getChild(1);
		Context	rhs=context.getChild(2);

		if(lhs.getCode()->isFloat()){

			if(rhs.getCode()->isFloat()){

				bool	r=lhs.getCode()->asFloat()<rhs.getCode()->asFloat();
				context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}else	if(lhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

			if(rhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

				bool	r=Timestamp::Get(lhs.getCode())<Timestamp::Get(rhs.getCode());
				context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}

		context.setAtomicResult(Atom::UndefinedBoolean());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	gte(const	Context	&context){

		Context	lhs=context.getChild(1);
		Context	rhs=context.getChild(2);

		if(lhs.getCode()->isFloat()){

			if(rhs.getCode()->isFloat()){

				bool	r=lhs.getCode()->asFloat()>=rhs.getCode()->asFloat();
				context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}else	if(lhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

			if(rhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

				bool	r=Timestamp::Get(lhs.getCode())>=Timestamp::Get(rhs.getCode());
				context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}

		context.setAtomicResult(Atom::UndefinedBoolean());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	lse(const	Context	&context){

		Context	lhs=context.getChild(1);
		Context	rhs=context.getChild(2);

		if(lhs.getCode()->isFloat()){

			if(rhs.getCode()->isFloat()){

				bool	r=lhs.getCode()->asFloat()<=rhs.getCode()->asFloat();
				context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}else	if(lhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

			if(rhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

				bool	r=Timestamp::Get(lhs.getCode())<=Timestamp::Get(rhs.getCode());
				context.setAtomicResult(Atom::Boolean(r));
				return	r;
			}
		}

		context.setAtomicResult(Atom::UndefinedBoolean());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	add(const	Context	&context){

		Context	lhs=context.getChild(1);
		Context	rhs=context.getChild(2);

		if(lhs.getCode()->isFloat()){

			if(rhs.getCode()->isFloat()){

				context.setAtomicResult(Atom::Float(lhs.getCode()->asFloat()+rhs.getCode()->asFloat()));
				return	true;
			}
		}else	if(lhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

			if(rhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

				context.setTimestampResult(Timestamp::Get(lhs.getCode())+Timestamp::Get(rhs.getCode()));
				return	true;
			}
		}

		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	sub(const	Context	&context){

		Context	lhs=context.getChild(1);
		Context	rhs=context.getChild(2);

		if(lhs.getCode()->isFloat()){

			if(rhs.getCode()->isFloat()){

				context.setAtomicResult(Atom::Float(lhs.getCode()->asFloat()-rhs.getCode()->asFloat()));
				return	true;
			}
		}else	if(lhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

			if(rhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

				uint64	lhs_t=Timestamp::Get(lhs.getCode());
				uint64	rhs_t=Timestamp::Get(rhs.getCode());
				if(lhs_t>=rhs_t){

					context.setTimestampResult(lhs_t-rhs_t);
					return	true;
				}
			}
		}

		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	mul(const	Context	&context){

		Context	lhs=context.getChild(1);
		Context	rhs=context.getChild(2);

		if(lhs.getCode()->isFloat()){

			if(rhs.getCode()->isFloat()){

				context.setAtomicResult(Atom::Float(lhs.getCode()->asFloat()*rhs.getCode()->asFloat()));
				return	true;
			}else	if(rhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

				if(lhs.getCode()->asFloat()>=0){

					context.setAtomicResult(Atom::Float(Timestamp::Get(rhs.getCode())*lhs.getCode()->asFloat()));
					return	true;
				}
			}
		}else	if(lhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

			if(rhs.getCode()->isFloat()	&&	rhs.getCode()->asFloat()>=0){

				context.setTimestampResult(Timestamp::Get(lhs.getCode())*rhs.getCode()->asFloat());
				return	true;
			}
		}

		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	div(const	Context	&context){

		Context	lhs=context.getChild(1);
		Context	rhs=context.getChild(2);

		if(lhs.getCode()->isFloat()){

			if(rhs.getCode()->isFloat()	&&	rhs.getCode()->asFloat()>0){

				context.setAtomicResult(Atom::Float(lhs.getCode()->asFloat()/rhs.getCode()->asFloat()));
				return	true;
			}
		}else	if(lhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

			if(rhs.getCode()->isFloat()	&&	rhs.getCode()->asFloat()>0){

				context.setTimestampResult(Timestamp::Get(lhs.getCode())/rhs.getCode()->asFloat());
				return	true;
			}
		}

		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	dis(const	Context	&context){	

		Context	lhs=context.getChild(1);
		Context	rhs=context.getChild(2);

		if(lhs.getCode()->isFloat()){

			if(rhs.getCode()->isFloat()){

				context.setAtomicResult(Atom::Float(abs(lhs.getCode()->asFloat()-rhs.getCode()->asFloat())));
				return	true;
			}
		}else	if(lhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

			if(rhs.getCode()->getDescriptor()==Atom::TIMESTAMP){

				uint64	lhs_t=Timestamp::Get(lhs.getCode());
				uint64	rhs_t=Timestamp::Get(rhs.getCode());
				context.setTimestampResult(abs((float64)(lhs_t-rhs_t)));
				return	true;
			}
		}

		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	ln(const	Context	&context){

		Context	arg=context.getChild(1);
		
		if(arg.getCode()->isFloat()){

			if(arg.getCode()!=0){
				
				context.setAtomicResult(Atom::Float(::log(arg.getCode()->asFloat())));
				return	true;
			}
		}

		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	exp(const	Context	&context){

		Context	arg=context.getChild(1);
		
		if(arg.getCode()->isFloat()){

			context.setAtomicResult(Atom::Float(::exp(arg.getCode()->asFloat())));
			return	true;
		}

		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	log(const	Context	&context){

		Context	arg=context.getChild(1);
		
		if(arg.getCode()->isFloat()){

			if(arg.getCode()!=0){
				
				context.setAtomicResult(Atom::Float(log10(arg.getCode()->asFloat())));
				return	true;
			}
		}

		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	e10(const	Context	&context){

		Context	arg=context.getChild(1);
		
		if(arg.getCode()->isFloat()){

			context.setAtomicResult(Atom::Float(pow(10,arg.getCode()->asFloat())));
			return	true;
		}

		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	syn(const	Context	&context){

		return	false;	//	TODO.
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	ins(const	Context	&context){

		Context	object=context.getChild(1);
		Context	args=context.getChild(2);

		Object	*_object=object.getObject();
		if(_object	&&	args.getCode()->getDescriptor()==Atom::SET){

			uint16	pattern_set_index=_object->code[_object->code[PGM_INPUTS].asIndex()+1].asIndex();
			uint16	arg_count=args.getCode()->getAtomCount();
			if(_object->code[pattern_set_index].getAtomCount()!=arg_count){

				context.setAtomicResult(Atom::Nil());
				return	false;
			}
			
			//	match args with the tpl patterns in _object.
			for(uint16	i=1;i<arg_count;++i){

				Context	arg=args.getChild(i);
				if(!match(arg,&_object->code[pattern_set_index+i])){

					context.setAtomicResult(Atom::Nil());
					return	false;
				}
			}

			//	create an ipgm or an igol in the explicit_instantiations array.
			if(_object->code[0].asOpcode()==Object::PGMOpcode	||	_object->code[0].asOpcode()==Object::AntiPGMOpcode){

				Object	*ipgm=new	Object();

				uint16	write_index=0;
				ipgm->code[write_index++]=Atom::Object(Object::IPGMOpcode,IPGM_IGOL_ARITY);
				ipgm->code[write_index++]=Atom::RPointer(0);	//	points to the pgm object.
				ipgm->code[write_index++]=Atom::IPointer(IPGM_IGOL_ARITY+1);
				ipgm->code[write_index++]=Atom::View();
				ipgm->code[write_index++]=Atom::Vws();
				ipgm->code[write_index++]=Atom::Mks();
				ipgm->code[write_index++]=Atom::Float(1);	//	psln_thr.

				args.copy(_object,write_index);	//	writes after psln_thr.

				ipgm->reference_set[0]=_object;
				context.setAtomicResult(Atom::ProductionPointer(context.addExplicitInstantiation(ipgm)));
				return	true;
			}

			if(_object->code[0].asOpcode()==Object::GoalOpcode	||	_object->code[0].asOpcode()==Object::AntiGoalOpcode){

				Object	*igol=new	Object();

				uint16	write_index=0;
				igol->code[write_index++]=Atom::Object(Object::IGoalOpcode,IPGM_IGOL_ARITY);
				igol->code[write_index++]=Atom::RPointer(0);	//	points to the goal object.
				igol->code[write_index++]=Atom::IPointer(IPGM_IGOL_ARITY+1);
				igol->code[write_index++]=Atom::View();
				igol->code[write_index++]=Atom::Vws();
				igol->code[write_index++]=Atom::Mks();
				igol->code[write_index++]=Atom::Float(1);	//	psln_thr.

				args.copy(_object,write_index);	//	writes after psln_thr.

				igol->reference_set[0]=_object;
				context.setAtomicResult(Atom::ProductionPointer(context.addExplicitInstantiation(igol)));
				return	true;
			}
		}
		
		context.setAtomicResult(Atom::Nil());
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	at(const	Context	&context){

		return	false;	//	TODO.
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	_match(const	Context	&input,const	Context	&pattern){

		//	TODO: equal instead.
		if(input.getCode()->asOpcode()!=pattern.getChild(1).getCode()->asOpcode())	//	pattern.getChild(1) is an iptr to the skeleton (dereferenced).
			return	false;

		//	patch the pattern with an iptr to the input, i.e. with input.index.
		pattern.patch_input_code(pattern.getIndex(),input.getIndex());

		//	match: evaluate the set of guards.
		Context	guard_set=pattern.getChild(2);
		uint16	guard_count=guard_set.getChildrenCount();
		for(uint16	i=1;i<=guard_count;++i){

			if(!guard_set.getChild(i).evaluate()){	//	WARNING: no check for duplicates.

				pattern.rollback();
				return	false;
			}
		}

		return	true;
	}

	inline	bool	match(const	Context	&input,const	Context	&pattern){
		
		if(pattern.getCode()->asOpcode()==Object::AntiPTNOpcode)
			return	!_match(input,pattern);
		else
			return	_match(input,pattern);
	}

	uint8	reduce(const	Context	&context,const	Context	&input_set,const	Context	&pattern_set,std::vector<uint16>	&input_indices){

		uint16	pattern_count=pattern_set.getChildrenCount();
		if(!pattern_count)
			return	0;

		uint8	r=0;
		std::vector<uint16>::iterator	i;
		for(i=input_indices.begin();i!=input_indices.end();){	//	to be successful, at least one input must match all the patterns.

			bool	failure=false;
			for(uint16	j=0;j<pattern_count;++j){

				if(!match(input_set.getChild(*i),pattern_set.getChild(j))){

					failure=true;
					break;
				}
			}

			if(failure)
				++i;
			else{	//	all patterns matched: validate the results and remove the input from the todo list.
			
				context.commit();
				i=input_indices.erase(i);
				++r;
			}
		}

		return	r;
	}

	bool	red(const	Context	&context){	//	reminder: all inputs have already been evaluated (but not the patterns).

		Context	input_set=context.getChild(1);
		Context	positive_section=context.getChild(2);
		Context	negative_section=context.getChild(3);

		std::vector<uint16>	input_indices;	//	todo list of matching jobs intially filled with all input indices.

		if(input_set.getCode()->getDescriptor()!=Atom::SET	||
			positive_section.getCode()->getDescriptor()!=Atom::SET	||
			negative_section.getCode()->getDescriptor()!=Atom::SET)
			goto	failure;

		uint16	input_count=input_set.getChildrenCount();
		if(!input_count)
			goto	failure;

		for(uint16	i=1;i<=input_count;++i)
			input_indices.push_back(i);

		uint8	success=0;
		success+=reduce(context,input_set,positive_section,input_indices);	//	input_indices now filled only with the inputs that did not match all (positive) patterns.
		success+=reduce(context,input_set,negative_section,input_indices);	//	input_indices now filled only with the inputs that did not match all (negative) patterns.
		if(success)
			return	true;
failure:
		context.setCompoundResultHead(Atom::Set(0));
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////
	
	bool	com(const	Context	&context){

		return	false;	//	TODO.
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	spl(const	Context	&context){

		return	false;	//	TODO.
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	mrg(const	Context	&context){

		return	false;	//	TODO.
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	ptc(const	Context	&context){

		return	false;	//	TODO.
	}

	////////////////////////////////////////////////////////////////////////////////

	bool	fvw(const	Context	&context){

		Context	object=context.getChild(1);
		Context	group=context.getChild(2);

		Object	*_object=object.getObject();
		Group	*_group=(Group	*)group.getObject();
		if(!_object	||	!_group){

			context.setAtomicResult(Atom::Nil());
			return	false;
		}

		_object->acq_view_map();
		UNORDERED_MAP<Group	*,View	*>::iterator	it=_object->view_map.find(_group);
		if(it!=_object->view_map.end()){	//	copy the view in the value array.

			View	*v=it->second;
			uint16	atom_count=v->code[0].getAtomCount();
			context.setCompoundResultHead(v->code[0]);
			for(uint16	i=1;i<=atom_count;++i)
				context.addCompoundResultPart(v->code[i]);
			_object->rel_view_map();
			return	true;
		}

		_object->rel_view_map();
		context.setAtomicResult(Atom::Nil());
		return	false;
	}
}