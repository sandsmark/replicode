#include	"pgm_overlay.h"
#include	"mem.h"
#include	"context.h"


//	pgm layout:
//
//	index											content
//
//	PGM_TPL_ARGS									>iptr to the tpl args set
//	PGM_INPUTS										>iptr to the inputs structured set
//	PGM_PRODS										>iptr to the production set
//	pgm_code[PGM_TPL_ARGS]							>tpl arg set #n0
//	pgm_code[PGM_TPL_ARGS]+1						>iptr to first tpl pattern
//	...												>...
//	pgm_code[PGM_TPL_ARGS]+n0						>iptr to last tpl pattern
//	pgm_code[pgm_code[PGM_TPL_ARGS]+1]				>opcode of the first tpl pattern
//	...												>...
//	pgm_code[PGM_INPUTS]							>inputs structured set
//	pgm_code[PGM_INPUTS]+1							>iptr to the input pattern set
//	pgm_code[PGM_INPUTS]+2							>iptr to the timing constraint set
//	pgm_code[PGM_INPUTS]+3							>iptr to the guard set
//	pgm_code[pgm_code[PGM_INPUTS]+1]				>input pattern set #n1
//	pgm_code[pgm_code[PGM_INPUTS]+1]+1				>iptr to first input pattern
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+1]+n1				>iptr to last input pattern
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+1]+1]	>opcode of the first input pattern
//	...												>...
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+1]+n1]	>opcode of the last input pattern
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+2]				>timing constraint set #n2
//	pgm_code[pgm_code[PGM_INPUTS]+2]+1				>iptr to first timing constraint
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+2]+n2				>iptr to last timing constraint
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+2]+1]	>opcode of the first timing constraint
//	...												>...
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+2]+n2]	>opcode of the last timing constraint
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+3]				>guard set #n3
//	pgm_code[pgm_code[PGM_INPUTS]+3]+1				>iptr to first guard
//	...												>...
//	pgm_code[pgm_code[PGM_INPUTS]+3]+n3				>iptr to last guard
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+3]+1]	>opcode of the first guard
//	...												>...
//	pgm_code[pgm_code[pgm_code[PGM_INPUTS]+3]+n3]	>opcode of the last guard
//	...												>...
//	pgm_code[pgm_code[PGM_PRODS]]					>production set #n4
//	pgm_code[pgm_code[PGM_PRODS]]+1					>iptr to first production
//	...												>...
//	pgm_code[pgm_code[PGM_PRODS]]+n4				>iptr to last production
//	pgm_code[pgm_code[pgm_code[PGM_PRODS]]+1]		>opcode of the first production
//	...												>...
//	pgm_code[pgm_code[pgm_code[PGM_PRODS]]+n4]		>opcode of the last production
//	...												>...

namespace	r_exec{

	Overlay::Overlay(IPGMController	*c):_Object(),controller(c){

		//	copy the original pgm code.
		pgm_code_size=getIPGM()->getPGM()->code.size();
		pgm_code=new	r_code::Atom[pgm_code_size];
		memcpy(pgm_code,&getIPGM()->getPGM()->code[0],pgm_code_size*sizeof(r_code::Atom));
		
		value_commit_index=0;

		patch_tpl_args();

		//	init the list of pattern indices.
		uint16	pattern_set_index=pgm_code[pgm_code[PGM_INPUTS].asIndex()+1].asIndex();
		uint16	first_pattern_index=pgm_code[pattern_set_index+1].asIndex();
		uint16	last_pattern_index=first_pattern_index+pgm_code[pattern_set_index].getAtomCount()-1;
		for(uint16	i=first_pattern_index;i<=last_pattern_index;++i)
			input_pattern_indices.push_back(i);

		//	init convenience indices.
		uint16	timing_constraint_set_index=pgm_code[pgm_code[PGM_INPUTS].asIndex()+2].asIndex();
		first_timing_constraint_index=pgm_code[timing_constraint_set_index+1].asIndex();
		last_timing_constraint_index=first_timing_constraint_index+pgm_code[timing_constraint_set_index].getAtomCount()-1;

		uint16	guard_set_index=pgm_code[pgm_code[PGM_INPUTS].asIndex()+3].asIndex();
		first_guard_index=pgm_code[guard_set_index+1].asIndex();
		last_guard_index=first_guard_index+pgm_code[guard_set_index].getAtomCount()-1;

		uint16	production_set_index=pgm_code[pgm_code[PGM_PRODS].asIndex()].asIndex();
		first_production_index=pgm_code[production_set_index+1].asIndex();
		last_production_index=first_production_index+pgm_code[production_set_index].getAtomCount()-1;
	}

	Overlay::Overlay(Overlay	*original,uint16	last_input_index,uint16	value_commit_index):_Object(){

		controller=original->controller;

		input_pattern_indices=original->input_pattern_indices;
		input_pattern_indices.push_back(last_input_index);		//	put back the last original's input index.

		for(uint16	i=0;i<original->input_views.size()-1;++i)	//	ommit the last original's input view.
			input_views.push_back(original->input_views[i]);

		pgm_code_size=original->pgm_code_size;
		pgm_code=new	r_code::Atom[pgm_code_size];
		memcpy(pgm_code,original->pgm_code,pgm_code_size*sizeof(r_code::Atom));	//	copy patched code.

		Atom	*original_code=&getIPGM()->getPGM()->code[0];
		for(uint16	i=0;i<original->patch_indices.size();++i)	//	unpatch code.
			pgm_code[i]=original_code[patch_indices[i]];

		this->value_commit_index=value_commit_index;
		for(uint16	i=0;i<value_commit_index;++i)	//	copy values up to the last commit index.
			values.push_back(original->values[i]);

		first_timing_constraint_index=original->first_timing_constraint_index;
		last_timing_constraint_index=original->last_timing_constraint_index;
		first_guard_index=original->first_guard_index;
		last_guard_index=original->last_guard_index;
		first_production_index=original->first_production_index;
		last_production_index=original->last_production_index;
	}

	inline	Overlay::~Overlay(){

		delete[]	pgm_code;
	}

	inline	void	Overlay::patch_tpl_args(){	//	patch the pgm code with ptrs to the tpl args actual location in the ipgm code (Atom::IPGM_PTR).
												//	no rollback on that part of the code.
		uint16	tpl_arg_set_index=pgm_code[PGM_TPL_ARGS].asIndex();
		uint16	arg_count=pgm_code[tpl_arg_set_index].getAtomCount();
		uint16	ipgm_arg_set_index=getIPGM()->code[IPGM_ARGS].asIndex();
		for(uint16	i=1;i<=arg_count;++i)
			pgm_code[tpl_arg_set_index+i]=Atom::IPGMPointer(ipgm_arg_set_index+i);
	}

	bool	Overlay::is_alive()	const{

		return	controller->is_alive();
	}

	inline	void	Overlay::rollback(){

		Atom	*original_code=&getIPGM()->getPGM()->code[0];
		for(uint16	i=0;i<patch_indices.size();++i)	//	upatch code.
			pgm_code[i]=original_code[patch_indices[i]];

		if(value_commit_index!=values.size()){	//	shrink the values down to the last commit index.

			values.resize(value_commit_index);
			value_commit_index=values.size();
		}
	}

	inline	void	Overlay::commit(){

		patch_indices.clear();
		value_commit_index=values.size();
	}

	inline	void	Overlay::reset(){

		memcpy(pgm_code,&getIPGM()->getPGM()->code[0],pgm_code_size*sizeof(r_code::Atom));	//	restore code to prisitne copy.
		patch_tpl_args();

		patch_indices.clear();
		input_views.clear();
		input_pattern_indices.clear();
		value_commit_index=0;
		values.clear();
		explicit_instantiations.clear();
	}

	void	Overlay::reduce(r_exec::View	*input,Mem	*mem){

		uint16	input_index;
		switch(match(input,input_index)){
		case	SUCCESS:
			if(input_pattern_indices.size()==0){	//	all patterns matched.

				if(check_timings()	&&	check_guards()	&&	inject_productions(mem)){

					controller->remove(this);
					return;
				}
			}else{

				Overlay	*offspring=new	Overlay(this,input_index,value_commit_index);
				controller->add(offspring);
				commit();
				return;
			}
		case	FAILURE:	//	just rollback: let the overlay match other inputs.
			input_pattern_indices.push_back(input_index);
			rollback();
		}
	}

	Overlay::MatchResult	Overlay::match(r_exec::View	*input,uint16	&input_index){

		input_views.push_back(input);
		bool	failed=false;
		std::list<uint16>::iterator	it;
		for(it=input_pattern_indices.begin();it!=input_pattern_indices.end();++it){

			MatchResult	r=_match(input,*it);
			switch(r){
			case	SUCCESS:
				input_index=*it;
				input_pattern_indices.erase(it);
				return	r;
			case	FAILURE:
				failed=true;
				rollback();	//	to try another pattern on a clean basis.
				break;
			}
		}
		input_views.pop_back();
		return	failed?FAILURE:IMPOSSIBLE;
	}

	bool	Overlay::check_timings(){

		for(uint16	i=first_timing_constraint_index;i<=last_timing_constraint_index;++i)
			if(!evaluate(i))
				return	false;
		return	true;
	}

	bool	Overlay::check_guards(){

		for(uint16	i=first_guard_index;i<=last_guard_index;++i)
			if(!evaluate(i))
				return	false;
		return	true;
	}

	bool	Overlay::inject_productions(Mem	*mem){

		for(uint16	i=first_production_index;i<=last_production_index;++i){

			if(!evaluate(i)){

				explicit_instantiations.clear();
				return	false;
			}
		}

		uint16	write_index=0;
		uint16	extent_index=MK_RDX_ARITY+1;
		uint32	production_index;
		Object	*mk_rdx;

		bool	notify_rdx=getIPGM()->getPGM()->code[PGM_NFR]==1;
		if(notify_rdx){	//	the productions are command objects (cmd); all productions are notified.

			mk_rdx=new	Object();
			if(getIPGM()->code[0].asOpcode()==Object::PGMOpcode){

				mk_rdx->code[write_index++]=Atom::Object(Object::PGMOpcode,MK_RDX_ARITY);
				mk_rdx->code[write_index++]=Atom::RPointer(0);				//	code
				mk_rdx->reference_set.push_back(getIPGM());
				mk_rdx->code[write_index++]=Atom::IPointer(extent_index);	//	inputs
				mk_rdx->code[extent_index++]=Atom::Set(input_views.size());
				for(uint16	i=0;i<input_views.size();++i){

					mk_rdx->code[extent_index++]=Atom::RPointer(i+1);
					mk_rdx->reference_set.push_back(input_views[i]->object);
				}
			}else	if(getIPGM()->code[0].asOpcode()==Object::AntiPGMOpcode){

				mk_rdx->code[write_index++]=Atom::Object(Object::AntiPGMOpcode,MK_RDX_ARITY);
				mk_rdx->code[write_index++]=Atom::RPointer(0);				//	code
				mk_rdx->reference_set.push_back(getIPGM());
			}
			
			mk_rdx->code[write_index++]=Atom::IPointer(extent_index);		//	productions
			mk_rdx->code[write_index++]=Atom::View();
			mk_rdx->code[write_index++]=Atom::Vws();
			mk_rdx->code[write_index++]=Atom::Mks();
			mk_rdx->code[write_index++]=Atom::Float(1);						//	psln_thr.
			production_index=extent_index;
			mk_rdx->code[extent_index++]=Atom::Set(last_production_index-first_production_index+1);	//	number of productions.
		}
		
		for(uint16	i=first_production_index;i<=last_production_index;++i){	//	all productions have evaluated correctly; we can now execute the commands.

			Context	cmd(getIPGM()->getPGM(),NULL,pgm_code,i,this);
			cmd.dereference();

			Context	function=cmd.getChild(1);
			Context	device=cmd.getChild(2);

			//	layout of a command:
			//	0	>cmd opcode
			//	1	>function
			//	2	>device
			//	3	>iptr to the set of arguments
			//	4	>set
			//	5	>first arg

			//	call device functions.
			Context	args=cmd.getChild(4);
			if(device.getCode()->asOpcode()==EXECUTIVE_DEVICE){

				if(function.getCode()->asOpcode()==Object::InjectOpcode){	//	args:[object view]; create a view and an object.

					Context	_object=args.getChild(1);
					Object	*object=new	Object();
					_object.copy(object,0);

					Context	_view=args.getChild(2);
					View	*view=new	View();
					_view.copy(view,0);
					view->object=object;

					mem->inject(view);
				}else	if(function.getCode()->asOpcode()==Object::EjectOpcode){	//	args:[object view destination_node]; view.grp=destination grp (stdin ot stdout); create a view and an object; update mk_rdx.

					Context	_object=args.getChild(1);
					Object	*object=new	Object();
					_object.copy(object,0);

					Context	_view=args.getChild(2);
					View	*view=new	View();
					_view.copy(view,0);
					view->object=object;

					Context	node=args.getChild(3);

					mem->eject(view,node.getCode()->getNodeID());
				}else	if(function.getCode()->asOpcode()==Object::ModOpcode){	//	args:[cptr value]

					void	*object;
					Context::ObjectType	object_type;
					uint16	member_index;
					args.getChildAsMember(1,object,object_type,member_index);

					if(object){
						
						Context	_value=args.getChild(2);
						float32	value=_value.getCode()->asFloat();
						bool	r;
						switch(object_type){
						case	Context::TYPE_VIEW:
							((View	*)object)->acquire();
							r=((View	*)object)->mod(member_index,value);
							((View	*)object)->release();
							return	r;
						case	Context::TYPE_OBJECT:
							return	((Object	*)object)->mod(member_index,value);	//	protected internally.
						case	Context::TYPE_GROUP:
							((Group	*)object)->acquire();
							r=((Group	*)object)->mod(member_index,value);
							((Group	*)object)->release();
						default:
							return	false;
						}
					}
				}else	if(function.getCode()->asOpcode()==Object::SetOpcode){	//	args:[cptr value]

					void	*object;
					Context::ObjectType	object_type;
					uint16	member_index;
					args.getChildAsMember(1,object,object_type,member_index);

					if(object){
						
						Context	_value=args.getChild(2);
						float32	value=_value.getCode()->asFloat();
						bool	r;
						switch(object_type){
						case	Context::TYPE_VIEW:
							((View	*)object)->acquire();
							r=((View	*)object)->mod(member_index,value);
							((View	*)object)->release();
							return	r;
						case	Context::TYPE_OBJECT:
							return	((Object	*)object)->set(member_index,value);	//	protected internally.
						case	Context::TYPE_GROUP:
							((Group	*)object)->acquire();
							r=((Group	*)object)->set(member_index,value);
							((Group	*)object)->release();
						default:
							return	false;
						}
					}
				}else	if(function.getCode()->asOpcode()==Object::NewClassOpcode){

				}else	if(function.getCode()->asOpcode()==Object::DelClassOpcode){

				}else	if(function.getCode()->asOpcode()==Object::LDCOpcode){

				}else	if(function.getCode()->asOpcode()==Object::SwapOpcode){

				}else	if(function.getCode()->asOpcode()==Object::NewDevOpcode){

				}else	if(function.getCode()->asOpcode()==Object::DelDevOpcode){

				}else	if(function.getCode()->asOpcode()==Object::SuspendOpcode){	//	no args.

					mem->suspend();
				}else	if(function.getCode()->asOpcode()==Object::ResumeOpcode){	//	no args.

					mem->resume();
				}else	if(function.getCode()->asOpcode()==Object::StopOpcode){		//	no args.

					mem->stop();
				}else{	//	unknown function.

					explicit_instantiations.clear();
					return	false;
				}
			}else{	//	in case of an external device, create a cmd object and send it.

				Object	*command=new	Object();
				cmd.copy(command,0);

				mem->eject(command,command->code[CMD_DEVICE].getNodeID());
			}

			if(notify_rdx)
				cmd.copy(mk_rdx,extent_index,extent_index);
		}

		if(notify_rdx){

			NotificationView	*v=new	NotificationView(getIPGMView()->getHost(),getIPGMView()->getHost()->get_ntf_grp(),mk_rdx);
			mem->inject(v);
		}

		explicit_instantiations.clear();
		return	true;
	}

	inline	Overlay::MatchResult	Overlay::_match(r_exec::View	*input,uint16	pattern_index){

		uint16	object_class=pgm_code[pgm_code[pattern_index+1].asIndex()].asOpcode();	//	pgm_code[pattern_index+1] is an iptr to the skeleton.

		if(pgm_code[pattern_index].asOpcode()==Object::AntiPTNOpcode){

			MatchResult	r=_match_pattern(input,pattern_index);
			switch(r){
			case	IMPOSSIBLE:
			case	FAILURE:
				return	SUCCESS;
			case	SUCCESS:
				return	FAILURE;
			}
		}else
			return	_match_pattern(input,pattern_index);
	}

	inline	Overlay::MatchResult	Overlay::_match_pattern(r_exec::View	*input,uint16	pattern_index){

		if(pgm_code[pgm_code[pattern_index+1].asIndex()].asOpcode()!=input->object->code[0].asOpcode())	//	pgm_code[pattern_index+1] is an iptr to the skeleton.
			return	IMPOSSIBLE;
		
		//	patterns are of the form (ptn var:(class ::) [guards]);
		//	patch the pattern with a ptr to the input, i.e. pgm_code at var index = input ptr.
		patch_code(pattern_index,Atom::InputPointer(input_views.size()-1,0));	//	the input has just been pushed on input_views (see match).

		//	match: evaluate the set of guards.
		uint16	guard_set_index=pgm_code[pattern_index+2].asIndex();
		if(!evaluate(guard_set_index))
			return	FAILURE;
		return	SUCCESS;
	}

	inline	bool	Overlay::evaluate(uint16	index){

		Context	c(getIPGM()->getPGM(),NULL,pgm_code,index,this);
		return	c.evaluate();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	AntiOverlay::AntiOverlay(IPGMController	*c):Overlay(c),alive(true){
	}

	inline	AntiOverlay::AntiOverlay(AntiOverlay	*original,uint16	last_input_index,uint16	value_limit):Overlay(original,last_input_index,value_limit),alive(true){
	}

	inline	AntiOverlay::~AntiOverlay(){
	}

	void	AntiOverlay::reduce(r_exec::View	*input,Mem	*mem){

		uint16	input_index;
		switch(match(input,input_index)){
		case	SUCCESS:
			if(input_pattern_indices.size()==0){	//	all patterns matched.

				if(check_timings()	&&	check_guards()){

					controller->restart(this,mem,true);
					return;
				}
			}else{

				AntiOverlay	*offspring=new	AntiOverlay(this,input_index,value_commit_index);
				controller->add(offspring);
				commit();
				return;
			}
		case	FAILURE:	//	just rollback: let the overl match other inputs.
			input_pattern_indices.push_back(input_index);
			rollback();
		}
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void	IPGMController::kill(){
		
		alive=false;
		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o)
			(*o)->kill();
	}

	void	IPGMController::take_input(r_exec::View	*input,Mem	*mem){	//	will never be called on an input-less controller.

		if(overlays.size()==0)
			overlays.push_back(getIPGM()->getType()==ANTI_IPGM?new	AntiOverlay(this):new	Overlay(this));

		uint64	now=Mem::Now();

		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o){

			ReductionJob	j(new	View(input),*o,now+getIPGM()->get_tsc());
			mem->pushReductionJob(j);
		}
	}

	void	IPGMController::signal_anti_pgm(Mem	*mem){	//	next job will be pushed by the rMem upon processing the current signaling job, i.e. right after exiting this function.

		AntiOverlay	*o;
		if(overlays.size()==0){

			o=new	AntiOverlay(this);
			overlays.push_back(o);
		}else
			o=(AntiOverlay	*)*overlays.begin();

		if(!successful_match)
			o->inject_productions(mem);
		restart(o,mem,false);
	}

	void	IPGMController::signal_input_less_pgm(Mem	*mem){	//	next job will be pushed by the rMem upon processing the current signaling job, i.e. right after exiting this function.

		Overlay	*o;
		if(overlays.size()==0){

			o=new	Overlay(this);
			overlays.push_back(o);
		}else
			o=*overlays.begin();

		o->inject_productions(mem);

		Group	*host=getIPGMView()->getHost();
		if(getIPGMView()->get_act_vis()>host->get_act_thr()	&&	//	active ipgm.
			host->get_c_act()>host->get_c_act_thr()			&&	//	c-active group.
			host->get_c_sln()>host->get_c_sln_thr()){			//	c-salient group.

			TimeJob	next_job(new	InputLessPGMSignalingJob(this),Mem::Now()+host->get_spr()*mem->get_base_period());
			mem->pushTimeJob(next_job);
		}
	}

	inline	void	IPGMController::remove(Overlay	*overlay){

		if(overlays.size()==1)
			overlay->reset();
		else
			overlays.remove(overlay);
	}

	inline	void	IPGMController::add(Overlay	*overlay){	//	o has just matched an input; builds a copy of o.

		overlays.push_back(overlay);
	}

	void	IPGMController::restart(AntiOverlay	*overlay,Mem	*mem,bool	match){	//	one overlay matched all its inputs, timings and guards: push a new signaling job, 
																					//	keep the overlay alive (ita has been reset and shall take new inputs) and kill all others.
		overlay->reset();
		
		Group	*host=getIPGMView()->getHost();
		if(getIPGMView()->get_act_vis()>host->get_act_thr()	&&	//	active ipgm.
			host->get_c_act()>host->get_c_act_thr()	&&		//	c-active group.
			host->get_c_sln()>host->get_c_sln_thr()){		//	c-salient group.

			TimeJob	next_job(new	AntiPGMSignalingJob(this),Mem::Now()+getIPGM()->get_tsc());
			mem->pushTimeJob(next_job);
		}

		std::list<P<Overlay> >::const_iterator	o;
		for(o=overlays.begin();o!=overlays.end();++o)
			if(overlay!=*o)
				((AntiOverlay	*)*o)->kill();
		overlays.clear();

		successful_match=match;
	}
}