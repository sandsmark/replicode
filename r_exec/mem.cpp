#include	"mem.h"
#include	"replicode_defs.h"
#include	"operator.h"
#include	"../r_code/utils.h"
#include	<math.h>


namespace	r_exec{

	SharedLibrary	Mem::UserLibrary;

	uint64	(*Mem::_Now)()=NULL;

	void	Mem::Init(r_comp::ClassImage	*class_image,uint64	(*time_base)(),const	char	*user_operator_library_path){

		_Now=time_base;

		UNORDERED_MAP<std::string,uint16>	opcodes;
		UNORDERED_MAP<std::string,r_comp::Class>::iterator it;
		for(it=class_image->classes.begin();it!=class_image->classes.end();++it){

			opcodes[it->first]=it->second.atom.asOpcode();
			std::cout<<it->first<<":"<<it->second.atom.asOpcode()<<std::endl;
		}
		for(it=class_image->sys_classes.begin();it!=class_image->sys_classes.end();++it){

			opcodes[it->first]=it->second.atom.asOpcode();
			std::cout<<it->first<<":"<<it->second.atom.asOpcode()<<std::endl;
		}

		//	load class opcodes.
		View::ViewOpcode=opcodes.find("view")->second;

		Object::GroupOpcode=opcodes.find("grp")->second;

		Object::PTNOpcode=opcodes.find("ptn")->second;
		Object::AntiPTNOpcode=opcodes.find("|ptn")->second;

		Object::IPGMOpcode=opcodes.find("ipgm")->second;
		Object::PGMOpcode=opcodes.find("pgm")->second;
		Object::AntiPGMOpcode=opcodes.find("|pgm")->second;

		Object::IGoalOpcode=opcodes.find("igol")->second;
		Object::GoalOpcode=opcodes.find("gol")->second;
		Object::AntiGoalOpcode=opcodes.find("|gol")->second;

		Object::MkRdx=opcodes.find("mk.rdx")->second;
		Object::MkAntiRdx=opcodes.find("mk.|rdx")->second;

		Object::MkNewOpcode=opcodes.find("mk.new")->second;

		Object::MkLowResOpcode=opcodes.find("mk.low_res")->second;
		Object::MkLowSlnOpcode=opcodes.find("mk.low_sln")->second;
		Object::MkHighSlnOpcode=opcodes.find("mk.high_sln")->second;
		Object::MkLowActOpcode=opcodes.find("mk.low_act")->second;
		Object::MkHighActOpcode=opcodes.find("mk.high_act")->second;
		Object::MkSlnChgOpcode=opcodes.find("mk.sln_chg")->second;
		Object::MkActChgOpcode=opcodes.find("mk.act_chg")->second;

		//	load executive function opcodes.
		Object::InjectOpcode=opcodes.find("_inj")->second;
		Object::EjectOpcode=opcodes.find("_eje")->second;
		Object::ModOpcode=opcodes.find("_eje")->second;
		Object::SetOpcode=opcodes.find("_eje")->second;
		Object::NewClassOpcode=opcodes.find("_new_class")->second;
		Object::DelClassOpcode=opcodes.find("_del_class")->second;
		Object::LDCOpcode=opcodes.find("_ldc")->second;
		Object::SwapOpcode=opcodes.find("_swp")->second;
		Object::NewDevOpcode=opcodes.find("_new_dev")->second;
		Object::DelDevOpcode=opcodes.find("_del_dev")->second;
		Object::SuspendOpcode=opcodes.find("_suspend")->second;
		Object::ResumeOpcode=opcodes.find("_resume")->second;
		Object::StopOpcode=opcodes.find("_stop")->second;

		//	load std operators.
		uint16	operator_opcode=0;
		Operator::Register(operator_opcode++,now);
		Operator::Register(operator_opcode++,equ);
		Operator::Register(operator_opcode++,neq);
		Operator::Register(operator_opcode++,gtr);
		Operator::Register(operator_opcode++,lsr);
		Operator::Register(operator_opcode++,gte);
		Operator::Register(operator_opcode++,lse);
		Operator::Register(operator_opcode++,add);
		Operator::Register(operator_opcode++,sub);
		Operator::Register(operator_opcode++,mul);
		Operator::Register(operator_opcode++,div);
		Operator::Register(operator_opcode++,dis);
		Operator::Register(operator_opcode++,ln);
		Operator::Register(operator_opcode++,exp);
		Operator::Register(operator_opcode++,log);
		Operator::Register(operator_opcode++,e10);
		Operator::Register(operator_opcode++,syn);
		Operator::Register(operator_opcode++,ins);
		Operator::Register(operator_opcode++,at);
		Operator::Register(operator_opcode++,red);
		Operator::Register(operator_opcode++,com);
		Operator::Register(operator_opcode++,spl);
		Operator::Register(operator_opcode++,mrg);
		Operator::Register(operator_opcode++,ptc);
		Operator::Register(operator_opcode++,fvw);

		//	load usr operators.
		if(!(UserLibrary.load(user_operator_library_path)))
			exit(-1);

		typedef	void	(*UserInit)(UNORDERED_MAP<std::string,uint16>	&);
		UserInit	Init=UserLibrary.getFunction<UserInit>("Init");
		if(!Init)
			exit(-1);

		typedef	bool	(*UserOperator)(const	Context	&,uint16	&);
		typedef	uint16	(*UserGetOperatorCount)();
		UserGetOperatorCount	GetOperatorCount=UserLibrary.getFunction<UserGetOperatorCount>("GetOperatorCount");
		if(!GetOperatorCount)
			exit(-1);

		typedef	void	(*UserGetOperator)(UserOperator	&,std::string	&);
		UserGetOperator	GetOperator=UserLibrary.getFunction<UserGetOperator>("GetOperator");
		if(!GetOperator)
			exit(-1);

		std::cout<<"> User-defined operator library "<<user_operator_library_path<<" loaded"<<std::endl;

		Init(opcodes);
		uint16	operatorCount=GetOperatorCount();
		for(uint16	i=0;i<operatorCount;++i){

			UserOperator	op;
			std::string		op_name;
			GetOperator(op,op_name);

			UNORDERED_MAP<std::string,uint16>::iterator	it=opcodes.find(op_name);
			if(it==opcodes.end()){

				std::cerr<<"Operator "<<op_name<<" is undefined"<<std::endl;
				exit(-1);
			}
			Operator::Register(it->second,op);
		}
	}

	Mem::Mem(uint32	base_period,
			uint32	reduction_core_count,
			uint32	time_core_count,
			Comm	*comm):base_period(base_period),
							reduction_core_count(reduction_core_count),
							time_core_count(time_core_count),
							comm(comm){

		uint32	i;
		reduction_cores=new	ReductionCore	*[reduction_core_count];
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]=new	ReductionCore(this);
		time_cores=new	TimeCore	*[time_core_count];
		for(i=0;i<time_core_count;++i)
			time_cores[i]=new	TimeCore(this);

		object_register_sem=new	FastSemaphore(1,1);
		object_io_map_sem=new	FastSemaphore(1,1);
	}

	Mem::~Mem(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			delete	reduction_cores[i];
		delete[]	reduction_cores;
		for(i=0;i<time_core_count;++i)
			delete	time_cores[i];
		delete[]	time_cores;

		delete	object_register_sem;
		delete	object_io_map_sem;
	}

	////////////////////////////////////////////////////////////////

	r_code::Object	*Mem::buildObject(r_code::SysObject	*source){

		return	new	Object(source,this);	//	TODO: set OID
	}

	r_code::Object	*Mem::buildGroup(r_code::SysObject	*source){

		return	new	Group(source);
	}

	r_code::Object	*Mem::buildInstantiatedProgram(r_code::SysObject	*source){

		return	new	InstantiatedProgram(source,this);
	}

	r_code::Object	*Mem::buildMarker(r_code::SysObject	*source){

		return	new	Marker(source,this);
	}

	////////////////////////////////////////////////////////////////

	void	Mem::init(std::vector<r_code::Object	*>	*objects){	//	NB: no cov at init time.

		//	load root
		root=(Group	*)(*objects)[0];
		//	load conveniences
		_stdin=(Group	*)(*objects)[1];
		_stdout=(Group	*)(*objects)[2];
		_self=(Object	*)(*objects)[3];

		for(uint32	i=1;i<objects->size();++i){

			Object	*object=(Object	*)(*objects)[i];

			for(uint32	j=0;j<object->view_set.size();++j){

				//	init hosts' member_set and object's view_map.
				View	*view=(View	*)object->view_set[j];
				Group	*host=view->getHost();
				object->view_map[host]=view;

				if(object->getType()==GROUP){
					
					host->group_views[view->getOID()]=view;

					//	init viewing_group.
					bool	viewing_c_active=host->get_c_act()>host->get_c_act_thr();
					bool	viewing_c_salient=host->get_c_sln()>host->get_c_sln_thr();
					bool	viewed_visible=view->get_act_vis()>host->get_vis_thr();
					if(viewing_c_active	&&	viewing_c_salient	&&	viewed_visible)	//	visible group in a c-salient, c-active group.
						((Group	*)object)->viewing_groups[host]=view->get_cov()==0?false:true;	//	init the group's viewing groups.
				}else	if(object->isIPGM()){

					switch(object->getType()){
					case	INPUT_LESS_IPGM:
						host->input_less_ipgm_views[view->getOID()]=view;
						break;
					case	ANTI_IPGM:
						host->anti_ipgm_views[view->getOID()]=view;
						break;
					default:
						host->ipgm_views[view->getOID()]=view;
						break;
					}
					
					if(view->get_act_vis()>host->get_act_thr()){	//	active ipgm.

						IPGMController	*o=new	IPGMController(view);	//	now will be added to the deadline at start time.
						view->controller=o;	//	init the view's overlay.
					}
				}else
					host->other_views[view->getOID()]=view;
			}

			if(!object->getType()==GROUP){

				//	load non-group object in regsister and io map.
				object_register.insert(object);
				if(comm)
					object_io_map[object->getOID()]=object;
			}else
				initial_groups.push_back((Group	*)object);	//	convenience to create initial update jobs - see start().
		}
	}

	void	Mem::start(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->start(ReductionCore::Run);
		for(i=0;i<time_core_count;++i)
			time_cores[i]->start(TimeCore::Run);

		uint64	now=_Now();
		for(i=0;i<initial_groups.size();++i){

			Group	*g=initial_groups[i];
			bool	c_active=g->get_c_act()>g->get_c_act_thr();
			bool	c_salient=g->get_c_sln()>g->get_c_sln_thr();

			if(c_active){

				UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	v;

				//	build signaling jobs for active input-less overlays.
				for(v=g->input_less_ipgm_views_begin();v!=g->input_less_ipgm_views_end();v=g->next_input_less_ipgm_view(v)){

					TimeJob	j(new	InputLessPGMSignalingJob(v->second->controller),now+g->get_spr()*base_period);
					time_job_queue.push(j);
				}

				//	build signaling jobs for active anti-pgm overlays.
				for(v=g->anti_ipgm_views_begin();v!=g->anti_ipgm_views_end();v=g->next_anti_pgm_view(v)){

					TimeJob	j(new	AntiPGMSignalingJob(v->second->controller),now+v->second->controller->getIPGM()->get_tsc());
					time_job_queue.push(j);
				}

				if(c_salient){

					//	build reduction jobs for each salient view and each active overlay.
					UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	v;
					for(v=g->views_begin();v!=g->views_end();v=g->next_view(v)){
						
						r_code::Timestamp::Set(&v->second->code[VIEW_IJT],now);	//	init injection time for the view.
						if(v->second->get_sln()>g->get_sln_thr()){	//	salient view.

							g->newly_salient_views.push_back(v->second);
							_inject_reduction_jobs(v->second,g);
						}
					}
				}
			}

			//	inject the next update job for the group.
			TimeJob	j(new	UpdateJob(g),now+g->get_upr()*base_period);
			time_job_queue.push(j);
		}

		initial_groups.clear();
	}

	void	Mem::stop(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			Thread::TerminateAndWait(reduction_cores[i]);
		for(i=0;i<time_core_count;++i)
			Thread::TerminateAndWait(time_cores[i]);
	}
	
	void	Mem::suspend(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->suspend();
		for(i=0;i<time_core_count;++i)
			time_cores[i]->suspend();
	}
	
	void	Mem::resume(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->resume();
		for(i=0;i<time_core_count;++i)
			time_cores[i]->resume();
	}

	inline	ReductionJob	Mem::popReductionJob(){

		return	reduction_job_queue.pop();
	}

	inline	void	Mem::pushReductionJob(ReductionJob	j){

		reduction_job_queue.push(j);
	}

	inline	TimeJob	Mem::popTimeJob(){

		return	time_job_queue.pop();
	}

	inline	void	Mem::pushTimeJob(TimeJob	j){

		time_job_queue.push(j);
	}

	inline	void	Mem::deleteObject(Object	*object){

		//	erase from object_register and object_io_map.
		object_register_sem->acquire();
		object_register.erase(object);
		object_register_sem->release();

		if(object->getType()!=GROUP	&&	comm){

			object_io_map_sem->acquire();
			object_io_map.erase(object->getOID());
			object_io_map_sem->release();
		}
	}

	////////////////////////////////////////////////////////////////

	Object	*Mem::retrieve(uint32	OID){

		object_io_map_sem->acquire();
		UNORDERED_MAP<uint32,Object	*>::iterator	it=object_io_map.find(OID);
		if(it==object_io_map.end()){

			object_io_map_sem->release();
			return	NULL;
		}
		object_io_map_sem->release();
		return	it->second;
	}

	r_comp::CodeImage	*Mem::getCodeImage(){

		r_comp::CodeImage	*code_image=new	r_comp::CodeImage();
		UNORDERED_SET<Object	*,Object::Hash,Object::Equal>::iterator	i;
		for(i=object_register.begin();i!=object_register.end();++i)
			code_image->operator	<<(*i);
		return	code_image;
	}

	////////////////////////////////////////////////////////////////

	inline	void	Mem::inject(Object	*object,View	*view){

		view->object=object;
		inject(view);
	}

	inline	void	Mem::inject(View	*view){

		uint64	now=_Now();
		uint64	ijt=r_code::Timestamp::Get(&view->code[VIEW_IJT]);
		if(ijt<=now)
			injectNow(view);
		else{

			TimeJob	j(new	InjectionJob(view),ijt);
			time_job_queue.push(j);
		}
	}

	inline	void	Mem::eject(View	*view,uint16	nodeID){

		if(comm){

			Comm::STDGroupID	destination;
			if(view->getHost()==_stdin)
				destination=Comm::STDIN;
			else
				destination=Comm::STDOUT;
			comm->eject(view->object,nodeID,destination);
		}
	}

	inline	void	Mem::eject(Object	*command,uint16	nodeID){

		if(comm)
			comm->eject(command,nodeID);
	}

	////////////////////////////////////////////////////////////////

	inline	void	Mem::update(UpdateJob	*j){

		update((Group	*)j->group);
	}

	inline	void	Mem::update(AntiPGMSignalingJob	*j){

		j->controller->signal_anti_pgm(this);
	}

	inline	void	Mem::update(InputLessPGMSignalingJob	*j){

		j->controller->signal_input_less_pgm(this);
	}

	inline	void	Mem::update(InjectionJob	*j){

		injectNow(j->view);
	}

	inline	void	Mem::update(SaliencyPropagationJob	*j){
		
		propagate_sln(j->object,j->sln_change,j->source_sln_thr);
	}

	////////////////////////////////////////////////////////////////

	inline	void	Mem::injectNow(View	*view){

		Group	*host=view->getHost();
		Object	*object=view->object;
		if(object->getType()==GROUP)
			_inject_group_now(view,(Group	*)object,host);
		else{

			object_register_sem->acquire();
			UNORDERED_SET<Object	*,Object::Hash,Object::Equal>::const_iterator	it=object_register.find(object);
			if(it!=object_register.end()){

				object_register_sem->release();
				_inject_existing_object_now(view,*it,host);
			}else{	//	no equivalent object already exists: we have a new view on a new object; no need to protect either the view or the object.

				host->acquire();

				object->view_map[host]=view;
				uint64	now=Now();
				r_code::Timestamp::Set(&view->code[VIEW_IJT],now);

				object_register.insert(object);
				object_register_sem->release();
				if(comm){

					object_io_map_sem->acquire();
					object_io_map[object->getOID()]=object;
					object_io_map_sem->release();
				}

				switch(object->getType()){
				case	IPGM:{

					host->ipgm_views[view->getOID()]=view;
					IPGMController	*o=new	IPGMController(view);
					view->controller=o;
					if(view->get_act_vis()>host->get_act_thr()	&&	host->get_c_sln()>host->get_c_sln_thr()	&&	host->get_c_act()>host->get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

						for(uint32	i=0;i<host->newly_salient_views.size();++i)
							o->take_input(host->newly_salient_views[i],this);	//	view will be copied.
					}
					break;
				}case	ANTI_IPGM:{
					host->anti_ipgm_views[view->getOID()]=view;
					IPGMController	*o=new	IPGMController(view);
					view->controller=o;
					if(view->get_act_vis()>host->get_act_thr()	&&	host->get_c_sln()>host->get_c_sln_thr()	&&	host->get_c_act()>host->get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

						for(uint32	i=0;i<host->newly_salient_views.size();++i)
							o->take_input(host->newly_salient_views[i],this);	//	view will be copied.

						TimeJob	j(new	AntiPGMSignalingJob(o),now+o->getIPGM()->get_tsc());
						time_job_queue.push(j);
		
					}
					break;
				}case	INPUT_LESS_IPGM:{
					host->input_less_ipgm_views[view->getOID()]=view;
					IPGMController	*o=new	IPGMController(view);
					view->controller=o;
					if(view->get_act_vis()>host->get_act_thr()	&&	host->get_c_sln()>host->get_c_sln_thr()	&&	host->get_c_act()>host->get_c_act_thr()){	//	active ipgm in a c-salient and c-active group.

						TimeJob	j(new	InputLessPGMSignalingJob(o),now+host->get_spr()*base_period);
						time_job_queue.push(j);
					}
					break;
				}case	MARKER:	//	the marker does not exist yet: add it to the mks of its references.
					for(uint32	i=0;i<object->reference_set.size();++i){

						((Object	*)object->reference_set[i])->acq_marker_set();
						object->reference_set[i]->marker_set.push_back(object);
						((Object	*)object->reference_set[i])->rel_marker_set();
					}
				case	OTHER:{
					host->other_views[view->getOID()]=view;
					//	cov, i.e. injecting now newly salient views in the viewing groups for which group is visible and has cov.
					UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
					for(vg=host->viewing_groups.begin();vg!=host->viewing_groups.end();++vg){

						if(vg->second)	//	cov==true, vieiwing group c-salient and c-active (otherwise it wouldn't be a viewing group).
							injectCopyNow(view,vg->first);
					}
					break;
				}
				}

				if(host->get_c_sln()>host->get_c_sln_thr()	&&	view->get_sln()>host->get_sln_thr())	//	host is c-salient and view is salient.
					_inject_reduction_jobs(view,host);

				if(host->get_ntf_new()==1)	//	the view cannot be a ntf view (would use injectNotificationNow instead).
					injectNotificationNow(new	NotificationView(host,host->get_ntf_grp(),new	MkNew(object)));	//	the object appears for the first time in the group: notify.

				host->release();
			}
		}
	}

	inline	void	Mem::injectCopyNow(View	*view,Group	*destination){

		View	*copied_view=new	View(view,destination);	//	ctrl values are morphed.
		_inject_existing_object_now(copied_view,view->object,destination);
	}

	inline	void	Mem::_inject_existing_object_now(View	*view,Object	*object,Group	*host){

		view->object=object;	//	the object already exists (content-wise): have the view point to the existing one.

		object->acq_view_map();
		UNORDERED_MAP<Group	*,View	*>::const_iterator	it=object->view_map.find(host);
		if(it==object->view_map.end()){	//	no existing view: add the view to the group and to the object's view_map.

			host->acquire();
			switch(object->getType()){
			case	IPGM:
				host->ipgm_views[view->getOID()]=view;
				break;
			case	ANTI_IPGM:
				host->anti_ipgm_views[view->getOID()]=view;
				break;
			case	INPUT_LESS_IPGM:
				host->input_less_ipgm_views[view->getOID()]=view;
				break;
			case	OTHER:
			case	MARKER:
				host->other_views[view->getOID()]=view;
				break;
			}
			host->release();

			object->view_map[host]=view;
			object->rel_view_map();
		}else{	//	call set on the ctrl values of the existing view with the new view's ctrl values. NB: org left unchanged.

			object->rel_view_map();

			host->pending_operations.push_back(Group::PendingOperation(it->second->getOID(),VIEW_RES,Group::SET,view->get_res()));
			host->pending_operations.push_back(Group::PendingOperation(it->second->getOID(),VIEW_SLN,Group::SET,view->get_sln()));
			if(object->isIPGM())
				host->pending_operations.push_back(Group::PendingOperation(it->second->getOID(),IPGM_VIEW_ACT,Group::SET,view->get_act_vis()));
		}
	}

	inline	void	Mem::_inject_group_now(View	*view,Group	*object,Group	*host){	//	groups are always new; no cov for groups; no need to protect object.

		host->acquire();

		host->group_views[view->getOID()]=view;

		uint64	now=_Now();
		r_code::Timestamp::Set(&view->code[VIEW_IJT],now);
		object->view_map[host]=view;

		if(host->get_c_sln()>host->get_c_sln_thr()	&&	view->get_sln()>host->get_sln_thr()){	//	host is c-salient and view is salient.

			if(view->get_act_vis()>host->get_vis_thr())	//	new visible group in a c-active and c-salient host.
				host->viewing_groups[object]=view->get_cov()==0?false:true;

			_inject_reduction_jobs(view,host);
		}

		//	inject the next update job for the group.
		TimeJob	j(new	UpdateJob((Group	*)object),now+((Group	*)object)->get_upr()*base_period);
		time_job_queue.push(j);

		if(host->get_ntf_new()==1)
			injectNotificationNow(new	NotificationView(host,host->get_ntf_grp(),new	MkNew(object)));	//	the group appears for the first time in the group: notify.

		host->release();
	}

	void	Mem::injectNotificationNow(View	*view){	//	no notification for notifications; no registration either (object_register and object_io_map) and no cov.
													//	notifications are ephemeral: they are not held by the marker sets of the object they refer to; this implies no propagation of saliency changes trough notifications.
		Group	*host=view->getHost();
		Object	*object=view->object;
		
		host->acquire();

		r_code::Timestamp::Set(&view->code[VIEW_IJT],_Now());
		host->notification_views[view->getOID()]=view;

		object->acq_view_map();
		object->view_map[host]=view;
		object->rel_view_map();

		if(host->get_c_sln()>host->get_c_sln_thr()	&&	view->get_sln()>host->get_sln_thr())	//	host is c-salient and view is salient.
			_inject_reduction_jobs(view,host);

		host->release();
	}

	inline	void	Mem::update(Group	*group){

		uint64	now=_Now();

		group->acquire();

		group->newly_salient_views.clear();

		//	execute pending operations.
		for(uint32	i=0;i<group->pending_operations.size();++i){

			View	*v=group->getView(group->pending_operations[i].oid);
			if(v)
				switch(group->pending_operations[i].operation){
				case	Group::MOD:
					v->mod(group->pending_operations[i].member_index,group->pending_operations[i].value);
					break;
				case	Group::SET:
					v->set(group->pending_operations[i].member_index,group->pending_operations[i].value);
					break;
				}
		}
		group->pending_operations.clear();

		//	update group's ctrl values.
		group->update_sln_thr();	//	applies decay on sln thr. 
		group->update_act_thr();
		group->update_vis_thr();

		bool	group_was_c_active=group->get_c_act()>group->get_c_act_thr();
		bool	group_is_c_active=group->update_c_act()>group->get_c_act_thr();
		bool	group_was_c_salient=group->get_c_sln()>group->get_c_sln_thr();
		bool	group_is_c_salient=group->update_c_sln()>group->get_c_sln_thr();

		group->reset_stats();

		UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	v;
		for(v=group->views_begin();v!=group->views_end();v=group->next_view(v)){

			//	update resilience.
			v->second->mod_res(-1);
			float32	res=group->update_res(v->second,this);
			if(res<=0	&&	now-v->second->get_ijt()>=group->get_upr()){	//	if now-ijt<upr, let the view live for one upr.

				//	update saliency (apply decay).
				bool	wiew_was_salient=v->second->get_sln()>group->get_sln_thr();
				float32	sln_change;
				bool	wiew_is_salient=group->update_sln(v->second,sln_change,this)>group->get_sln_thr();

				if(group_is_c_salient	&&	!wiew_was_salient	&&	wiew_is_salient)	//	record as a newly salient view.
					group->newly_salient_views.push_back(v->second);

				_initiate_sln_propagation((Object	*)v->second->object,sln_change,group->get_sln_thr());	//	inject sln propagation jobs.

				if(((Object	*)v->second->object)->getType()==GROUP){

					//	update visibility.
					bool	view_was_visible=v->second->get_act_vis()>group->get_vis_thr();
					bool	view_is_visible=v->second->update_vis()>group->get_vis_thr();
					bool	cov=v->second->get_cov()==0?false:true;

					//	update viewing groups.
					if(group_was_c_active	&&	group_was_c_salient){

						if(!group_is_c_active	||	!group_is_c_salient)	//	group is not c-active and c-salient anymore: unregister as a viewing group.
							((Group	*)v->second->object)->viewing_groups.erase(group);
						else{	//	group remains c-active and c-salient.

							if(!view_was_visible){
								
								if(view_is_visible)		//	newly visible view.
									((Group	*)v->second->object)->viewing_groups[group]=cov;
							}else{
								
								if(!view_is_visible)	//	the view is no longer visible.
									((Group	*)v->second->object)->viewing_groups.erase(group);
								else					//	the view is still visible, cov might have changed.
									((Group	*)v->second->object)->viewing_groups[group]=cov;
							}
						}
					}else	if(group_is_c_active	&&	group_is_c_salient){	//	group becomes c-active and c-salient.

						if(view_is_visible)		//	update viewing groups for any visible group.
							((Group	*)v->second->object)->viewing_groups[group]=cov;
					}	
				}else	if(((Object	*)v->second->object)->isIPGM()){

					//	update activation
					bool	view_was_active=v->second->get_act_vis()>group->get_act_thr();
					bool	view_is_active=group->update_act(v->second,this)>group->get_act_thr();

					//	kill newly inactive controllers, register newly active ones.
					if(group_was_c_active	&&	group_was_c_salient){

						if(!group_is_c_active	||	!group_is_c_salient)	//	group is not c-active and c-salient anymore: kill the view's controller.
							v->second->controller->kill();
						else{	//	group remains c-active and c-salient.

							if(!view_was_active){
						
								if(view_is_active)	//	register the overlay for the newly active ipgm view.
									group->new_controllers.push_back(v->second->controller);
							}else{
								
								if(!view_is_active)	//	kill the newly inactive ipgm view's overlays.
									v->second->controller->kill();
							}
						}
					}else	if(group_is_c_active	&&	group_is_c_salient){	//	group becomes c-active and c-salient.

						if(view_is_active)	//	register the overlay for any active ipgm view.
							group->new_controllers.push_back(v->second->controller);
					}
				}
			}else{	//	view has no resilience.

				if(((Object	*)v->second->object)->isIPGM())	//	if ipgm view, kill the overlay.
					v->second->controller->kill();

				((Object	*)v->second->object)->acq_view_map();
				((Object	*)v->second->object)->view_map.erase(group);	//	delete view from object's view_map.
				((Object	*)v->second->object)->rel_view_map();

				//	delete the view.
				if(v->second->isNotification())
					group->notification_views.erase(v->first);
				else	switch(((Object	*)v->second->object)->getType()){
				case	IPGM:
					group->ipgm_views.erase(v->first);
					break;
				case	ANTI_IPGM:
					group->anti_ipgm_views.erase(v->first);
					break;
				case	INPUT_LESS_IPGM:
					group->input_less_ipgm_views.erase(v->first);
					break;
				case	OTHER:
				case	MARKER:
					group->other_views.erase(v->first);
					break;
				case	GROUP:
					group->group_views.erase(v->first);
					break;
				}
			}
		}

		if(group_is_c_salient){	//	build reduction jobs.

			//	cov, i.e. injecting now newly salient views in the viewing groups for which group is visible and has cov.
			UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
			for(vg=group->viewing_groups.begin();vg!=group->viewing_groups.end();++vg){

				if(vg->second)	//	cov==true.
					for(uint32	i=0;i<group->newly_salient_views.size();++i)
						if(!((Object	*)group->newly_salient_views[i]->object)->isIPGM()			&&	//	no cov for pgm, groups or notifications.
							((Object	*)group->newly_salient_views[i]->object)->getType()!=GROUP	&&
							!group->newly_salient_views[i]->isNotification())
								injectCopyNow(group->newly_salient_views[i],vg->first);	//	no need to protect group->newly_salient_views[i] since the support values for the ctrl values are not even read.
			}

			for(uint32	i=0;i<group->newly_salient_views.size();++i)
				_inject_reduction_jobs(group->newly_salient_views[i],group);
		}

		if(group_is_c_active	&&	group_is_c_salient){	//	build signaling jobs for new ipgms.

			for(uint32	i=0;i<group->new_controllers.size();++i){

				if(group->new_controllers[i]->getIPGM()->getType()==ANTI_IPGM){	//	inject signaling jobs for |ipgm (tsc).

					TimeJob	j(new	AntiPGMSignalingJob(group->new_controllers[i]),now+group->new_controllers[i]->getIPGM()->get_tsc());
					time_job_queue.push(j);
				}else	if(group->new_controllers[i]->getIPGM()->getType()==INPUT_LESS_IPGM){	//	inject a signaling job for an input-less pgm (sfr).

					TimeJob	j(new	InputLessPGMSignalingJob(group->new_controllers[i]),now+group->get_spr()*base_period);
					time_job_queue.push(j);
				}
			}

			group->new_controllers.clear();
		}

		group->update_stats(this);	//	triggers notifications.

		//	inject the next update job for the group.
		TimeJob	j(new	UpdateJob(group),now+group->get_upr()*base_period);
		time_job_queue.push(j);

		group->release();
	}

	inline	void	Mem::propagate_sln(Object	*object,float32	change,float32	source_sln_thr){

		//	apply morphed change to views.
		//	feedback can happen, i.e. m:(mk o1 o2); o1.vw.g propag -> o1 propag ->m propag -> o2 propag o2.vw.g, next upr in g, o2 propag -> m propag -> o1 propag -> o1,vw.g: loop!
		//	to avoid this, have the psln_thr set to 1 in o2: this is applicaton-dependent.
		object->acq_view_map();
		UNORDERED_MAP<Group	*,View	*>::const_iterator	it;
		for(it=object->view_map.begin();it!=object->view_map.end();++it){

			float32	morphed_sln_change=View::MorphChange(change,source_sln_thr,it->first->get_sln_thr());
			it->first->pending_operations.push_back(Group::PendingOperation(it->second->getOID(),VIEW_RES,Group::MOD,morphed_sln_change));
		}
		object->rel_view_map();
	}

	////////////////////////////////////////////////////////////////

	inline	void	Mem::_inject_reduction_jobs(View	*view,Group	*host){	//	host is assumed to be c-salient; host already protected.

		if(host->get_c_act()>host->get_c_act_thr()){	//	host is c-active.

			//	build reduction jobs from host's own inputs and own overlays.
			UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	v;
			for(v=host->ipgm_views_with_inputs_begin();v!=host->ipgm_views_with_inputs_end();v=host->next_ipgm_view_with_inputs(v)){

				if(v->second->get_act_vis()>host->get_sln_thr())	//	active ipgm view.
					v->second->controller->take_input(v->second,this);	//	view will be copied.
			}
		}

		//	build reduction jobs from host's own inputs and overlays from viewing groups, if no cov and view is not a notification.
		//	NB: visibility is not transitive;
		//	no shadowing: if a view alresady exists in the viewing group, there will be twice the reductions: all of the identicals will be trimmed down at injection time.
		UNORDERED_MAP<Group	*,bool>::const_iterator	vg;
		for(vg=host->viewing_groups.begin();vg!=host->viewing_groups.end();++vg){

			if(vg->second	||	view->isNotification())	//	cov==true or notification.
				continue;

			UNORDERED_MAP<uint32,r_code::P<View> >::const_iterator	v;
			for(v=vg->first->ipgm_views_with_inputs_begin();v!=vg->first->ipgm_views_with_inputs_end();v=vg->first->next_ipgm_view_with_inputs(v)){

				if(v->second->get_act_vis()>vg->first->get_sln_thr())	//	active ipgm view.
					v->second->controller->take_input(v->second,this);			//	view will be copied.
			}
		}
	}

	void	Mem::_initiate_sln_propagation(Object	*object,float32	change,float32	source_sln_thr){
		
		if(fabs(change)>object->get_psln_thr()){

			std::vector<Object	*>	path;
			path.push_back(object);

			if(object->getType()==MARKER){	//	if marker, propagate to references.

				for(uint32	i=0;object->reference_set.size();++i)
					_propagate_sln((Object	*)object->reference_set[i],change,source_sln_thr,path);
			}

			//	propagate to markers
			object->acq_marker_set();
			for(uint32	i=0;object->marker_set.size();++i)
				_propagate_sln((Object	*)object->marker_set[i],change,source_sln_thr,path);
			object->rel_marker_set();
		}
	}

	void	Mem::_initiate_sln_propagation(Object	*object,float32	change,float32	source_sln_thr,std::vector<Object	*>	&path){
		
		//	prevent loops.
		for(uint32	i=0;i<path.size();++i)
			if(path[i]==object)
				return;
		
		if(fabs(change)>object->get_psln_thr()){

			path.push_back(object);

			if(object->getType()==MARKER)	//	if marker, propagate to references.
				for(uint32	i=0;object->reference_set.size();++i)
					_propagate_sln((Object	*)object->reference_set[i],change,source_sln_thr,path);

			//	propagate to markers
			object->acq_marker_set();
			for(uint32	i=0;object->marker_set.size();++i)
				_propagate_sln((Object	*)object->marker_set[i],change,source_sln_thr,path);
			object->rel_marker_set();
		}
	}

	void	Mem::_propagate_sln(Object	*object,float32	change,float32	source_sln_thr,std::vector<Object	*>	&path){

		//	prevent loops.
		for(uint32	i=0;i<path.size();++i)
			if(path[i]==object)
				return;
		path.push_back(object);

		TimeJob	j(new	SaliencyPropagationJob(object,change,source_sln_thr),0);
		time_job_queue.push(j);
		
		_initiate_sln_propagation(object,change,source_sln_thr,path);
	}
}