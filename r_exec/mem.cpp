//	mem.cpp
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

#include	"mem.h"
#include	"mdl_controller.h"
#include	"model_base.h"


namespace	r_exec{

	_Mem::_Mem():r_code::Mem(),state(NOT_STARTED),gc(NULL),invalidated_object_count(0),registered_object_count(0){

		new	ModelBase();
	}

	_Mem::~_Mem(){

		if(state==RUNNING)
			stop();
		_root=NULL;
		if(gc!=NULL)
			delete	gc;
	}

	void	_Mem::init(	uint32	base_period,
						uint32	reduction_core_count,
						uint32	time_core_count,
						float32	mdl_inertia_sr_thr,
						uint32	mdl_inertia_cnt_thr,
						float32	tpx_dsr_thr,
						uint32	min_sim_time_horizon,
						uint32	max_sim_time_horizon,
						float32	sim_time_horizon,
						uint32	tpx_time_horizon,
						uint32	perf_sampling_period,
						float32	float_tolerance,
						uint32	time_tolerance,
						uint32	primary_thz,
						uint32	secondary_thz,
						bool	debug,
						uint32	ntf_mk_res,
						uint32	goal_pred_success_res,
						uint32	probe_level){

		this->base_period=base_period;

		this->reduction_core_count=reduction_core_count;
		this->time_core_count=time_core_count;

		this->mdl_inertia_sr_thr=mdl_inertia_sr_thr;
		this->mdl_inertia_cnt_thr=mdl_inertia_cnt_thr;
		this->tpx_dsr_thr=tpx_dsr_thr;
		this->min_sim_time_horizon=min_sim_time_horizon;
		this->max_sim_time_horizon=max_sim_time_horizon;
		this->sim_time_horizon=sim_time_horizon;
		this->tpx_time_horizon=tpx_time_horizon;
		this->perf_sampling_period=perf_sampling_period;
		this->float_tolerance=float_tolerance;
		this->time_tolerance=time_tolerance;
		this->primary_thz=primary_thz*1000000;
		this->secondary_thz=secondary_thz*1000000;

		this->debug=debug;
		this->ntf_mk_res=ntf_mk_res;
		this->goal_pred_success_res=goal_pred_success_res;

		this->probe_level=probe_level;

		reduction_job_count=time_job_count=0;
		reduction_job_avg_latency=_reduction_job_avg_latency=0;
		time_job_avg_latency=_time_job_avg_latency=0;
	}

	void	_Mem::reset(){

		uint32	i;
		for(i=0;i<reduction_core_count;++i)
			delete	reduction_cores[i];
		delete[]	reduction_cores;
		for(i=0;i<time_core_count;++i)
			delete	time_cores[i];
		delete[]	time_cores;

		delete	reduction_job_queue;
		delete	time_job_queue;

		delete	stop_sem;
	}

	////////////////////////////////////////////////////////////////

	Code	*_Mem::get_root()	const{

		return	_root;
	}

	Code	*_Mem::get_stdin()	const{

		return	_stdin;
	}

	Code	*_Mem::get_stdout()	const{

		return	_stdout;
	}

	Code	*_Mem::get_self()	const{

		return	_self;
	}

	////////////////////////////////////////////////////////////////

	_Mem::State	_Mem::check_state(){

		State	s;
		stateCS.enter();
		s=state;
		stateCS.leave();

		return	s;
	}

	void	_Mem::start_core(){

		core_countCS.enter();
		if(++core_count==1)
			stop_sem->acquire();
		core_countCS.leave();
	}

	void	_Mem::shutdown_core(){

		core_countCS.enter();
		if(--core_count==0)
			stop_sem->release();
		core_countCS.leave();
	}

	////////////////////////////////////////////////////////////////

	uint32	_Mem::get_oid(){

		return	last_oid++;
	}

	////////////////////////////////////////////////////////////////

	uint64	_Mem::start(){

		if(state!=STOPPED	&&	state!=NOT_STARTED)
			return	0;

		core_count=0;
		stop_sem=new	Semaphore(1,1);

		time_job_queue=new	PipeNN<P<TimeJob>,1024>();
		reduction_job_queue=new	PipeNN<P<_ReductionJob>,1024>();

		std::vector<std::pair<View	*,Group	*>	>	initial_reduction_jobs;

		uint32	i;
		uint64	now=Now();
		Utils::SetReferenceValues(now,base_period,float_tolerance,time_tolerance);
		ModelBase::Get()->set_thz(secondary_thz);
		init_timings(now);

		for(i=0;i<initial_groups.size();++i){

			Group	*g=initial_groups[i];
			bool	c_active=g->get_c_act()>g->get_c_act_thr();
			bool	c_salient=g->get_c_sln()>g->get_c_sln_thr();
			
			FOR_ALL_VIEWS_BEGIN(g,v)
				Utils::SetTimestamp<View>(v->second,VIEW_IJT,now);	// init injection time for the view.
			FOR_ALL_VIEWS_END

			if(c_active){

				UNORDERED_MAP<uint32,P<View> >::const_iterator	v;

				// build signaling jobs for active input-less overlays.
				for(v=g->input_less_ipgm_views.begin();v!=g->input_less_ipgm_views.end();++v){

					if(v->second->controller!=NULL	&&	v->second->controller->is_activated()){

						P<TimeJob>	j=new	InputLessPGMSignalingJob(v->second,now+Utils::GetTimestamp<Code>(v->second->object,IPGM_TSC));
						time_job_queue->push(j);
					}
				}

				// build signaling jobs for active anti-pgm overlays.
				for(v=g->anti_ipgm_views.begin();v!=g->anti_ipgm_views.end();++v){

					if(v->second->controller!=NULL	&&	v->second->controller->is_activated()){

						P<TimeJob>	j=new	AntiPGMSignalingJob(v->second,now+Utils::GetTimestamp<Code>(v->second->object,IPGM_TSC));
						time_job_queue->push(j);
					}
				}
			}

			if(c_salient){

				// build reduction jobs for each salient view and each active overlay - regardless of the view's sync mode.
				FOR_ALL_VIEWS_BEGIN(g,v)
					
					if(v->second->get_sln()>g->get_sln_thr()){	// salient view.

						g->newly_salient_views.insert(v->second);
						initial_reduction_jobs.push_back(std::pair<View	*,Group	*>(v->second,g));
					}
				FOR_ALL_VIEWS_END
			}
			
			if(g->get_upr()>0){	// inject the next update job for the group.

				P<TimeJob>	j=new	UpdateJob(g,g->get_next_upr_time(now));
				time_job_queue->push(j);
			}
		}

		initial_groups.clear();

		state=RUNNING;

		P<TimeJob>	j=new	PerfSamplingJob(now+perf_sampling_period,perf_sampling_period);
		time_job_queue->push(j);

		for(i=0;i<reduction_core_count;++i)
			reduction_cores[i]->start(ReductionCore::Run);
		for(i=0;i<time_core_count;++i)
			time_cores[i]->start(TimeCore::Run);

		for(uint32	i=0;i<initial_reduction_jobs.size();++i)
			initial_reduction_jobs[i].second->inject_reduction_jobs(initial_reduction_jobs[i].first);

		gcCS.enter();
		gc=Thread::New<GCThread>(GC,this);

		return	now;
	}

	void	_Mem::stop(){

		stateCS.enter();
		if(state!=RUNNING){

			stateCS.leave();
			return;
		}
		
		uint32	i;
		P<_ReductionJob>	r;
		for(i=0;i<reduction_core_count;++i)
			reduction_job_queue->push(r=new	ShutdownReductionCore());
		P<TimeJob>	t;
		for(i=0;i<time_core_count;++i)
			time_job_queue->push(t=new	ShutdownTimeCore());

		state=STOPPED;
		stateCS.leave();

		gcCS.leave();	// unlocks the garbage collector.

		for(i=0;i<time_core_count;++i)
			Thread::Wait(time_cores[i]);

		for(i=0;i<reduction_core_count;++i)
			Thread::Wait(reduction_cores[i]);

		stop_sem->acquire();	// wait for delegates.

		reset();
	}

	void	_Mem::trigger_gc(){

		if(registered_object_count>=200){	// unlock the garbage collector when 10% of the objects are invalidated.

			uint32	initial_invalidated_object_count=Atomic::CompareAndSwap32(&invalidated_object_count,20,0);
			if(initial_invalidated_object_count==20)
				gcCS.leave();
			else
				Atomic::Increment32(&invalidated_object_count);
		}
	}

	thread_ret	_Mem::GC(void	*args){

		_Mem	*_this=(_Mem	*)args;
		
		while(1){

			_this->gcCS.enter();
			if(_this->check_state()==_Mem::STOPPED)
				break;
			_this->trim_objects();
			ModelBase::Get()->trim_objects();
		}

		thread_ret_val(0);
	}

	////////////////////////////////////////////////////////////////

	_ReductionJob	*_Mem::popReductionJob(){

		if(state==STOPPED)
			return	NULL;
		return	reduction_job_queue->pop();
	}

	void	_Mem::pushReductionJob(_ReductionJob	*j){

		if(state==STOPPED)
			return;
		j->ijt=Now();
		P<_ReductionJob>	_j=j;
		reduction_job_queue->push(_j);
	}

	TimeJob	*_Mem::popTimeJob(){

		if(state==STOPPED)
			return	NULL;
		return	time_job_queue->pop();
	}

	void	_Mem::pushTimeJob(TimeJob	*j){

		if(state==STOPPED)
			return;
		P<TimeJob>	_j=j;
		time_job_queue->push(_j);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::eject(View	*view,uint16	nodeID){
	}

	void	_Mem::eject(Code	*command){
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::inject_copy(View	*view,Group	*destination){

		View	*copied_view=new	View(view,destination);	// ctrl values are morphed.
		inject_existing_object(copied_view,view->object,destination);
	}

	void	_Mem::inject_existing_object(View	*view,Code	*object,Group	*host){

		view->set_object(object);	// the object already exists (content-wise): have the view point to the existing one.
		host->inject_existing_object(view);
	}

	void	_Mem::inject_null_program(Controller	*c,Group *group,uint64	time_to_live,bool	take_past_inputs){

		uint64	now=Now();

		Code	*null_pgm=new	LObject();
		null_pgm->code(0)=Atom::NullProgram(take_past_inputs);

		uint32	res=Utils::GetResilience(now,time_to_live,group->get_upr()*Utils::GetBasePeriod());

		View	*view=new	View(View::SYNC_ONCE,now,0,res,group,NULL,null_pgm,1);
		view->controller=c;

		c->set_view(view);

		inject_async(view);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::register_reduction_job_latency(uint64	latency){

		reduction_jobCS.enter();
		++reduction_job_count;
		reduction_job_avg_latency+=latency;
		reduction_jobCS.leave();
	}
	void	_Mem::register_time_job_latency(uint64	latency){

		time_jobCS.enter();
		++time_job_count;
		time_job_avg_latency+=latency;
		time_jobCS.leave();
	}

	void	_Mem::inject_perf_stats(){

		reduction_jobCS.enter();
		time_jobCS.enter();

		int64	d_reduction_job_avg_latency;
		if(reduction_job_count>0){

			reduction_job_avg_latency/=reduction_job_count;
			d_reduction_job_avg_latency=reduction_job_avg_latency-_reduction_job_avg_latency;
		}else
			reduction_job_avg_latency=d_reduction_job_avg_latency=0;

		int64	d_time_job_avg_latency;
		if(time_job_count>0){

			time_job_avg_latency/=time_job_count;
			d_time_job_avg_latency=time_job_avg_latency-_time_job_avg_latency;
		}else
			time_job_avg_latency=d_time_job_avg_latency=0;

		Code	*perf=new	Perf(reduction_job_avg_latency,d_reduction_job_avg_latency,time_job_avg_latency,d_time_job_avg_latency);

		// reset stats.
		reduction_job_count=time_job_count=0;
		_reduction_job_avg_latency=reduction_job_avg_latency;
		_time_job_avg_latency=time_job_avg_latency;

		time_jobCS.leave();
		reduction_jobCS.leave();

		// inject f->perf in stdin.
		uint64	now=Now();
		Code	*f_perf=new	Fact(perf,now,now+perf_sampling_period,1,1);
		View	*view=new	View(View::SYNC_ONCE,now,1,1,_stdin,NULL,f_perf);	// sync front, sln=1, res=1.
		inject(view);
	}

	////////////////////////////////////////////////////////////////

	void	_Mem::propagate_sln(Code	*object,float32	change,float32	source_sln_thr){

		//	apply morphed change to views.
		//	loops are prevented within one call, but not accross several upr:
		//		- feedback can happen, i.e. m:(mk o1 o2); o1.vw.g propag -> o1 propag ->m propag -> o2 propag o2.vw.g, next upr in g, o2 propag -> m propag -> o1 propag -> o1,vw.g: loop spreading accross several upr.
		//		- to avoid this, have the psln_thr set to 1 in o2: this is applicaton-dependent.
		object->acq_views();

		if(object->views.size()==0){

			object->invalidate();
			object->rel_views();
			return;
		}

		UNORDERED_SET<r_code::View	*,r_code::View::Hash,r_code::View::Equal>::const_iterator	it;
		for(it=object->views.begin();it!=object->views.end();++it){

			float32	morphed_sln_change=View::MorphChange(change,source_sln_thr,((r_exec::View*)*it)->get_host()->get_sln_thr());
			if(morphed_sln_change!=0)
				((r_exec::View*)*it)->get_host()->pending_operations.push_back(new	Group::Mod(((r_exec::View*)*it)->get_oid(),VIEW_SLN,morphed_sln_change));
		}
		object->rel_views();
	}

	////////////////////////////////////////////////////////////////

	r_exec_dll r_exec::Mem<r_exec::LObject> *Run(const	char	*user_operator_library_path,
												uint64			(*time_base)(),
												const	char	*seed_path,
												const	char	*source_file_name)
	{
		r_exec::Init(user_operator_library_path,time_base,seed_path );

		srand(r_exec::Now());
		Random::Init();

		std::string	error;
		r_exec::Compile(source_file_name,error);

		r_exec::Mem<r_exec::LObject> *mem = new r_exec::Mem<r_exec::LObject>();

		r_code::vector<r_code::Code	*>	ram_objects;
		r_exec::Seed.get_objects(mem,ram_objects);

		mem->init(	100000,	// base period.
					3,		// reduction core count.
					1,		// time core count.
					0.9,	// mdl inertia sr thr.
					10,		// mdl inertia cnt thr.
					0.1,	// tpx_dsr_thr.
					25000,	// min_sim time horizon
					100000,	// max_sim time horizon
					0.3,	// sim time horizon
					500000,	// tpx time horizon
					250000,	// perf sampling period
					0.1,	// float tolerance.
					10000,	// time tolerance.
					3600000,// primary thz.
					7200000,// secondary thz.
					false,	// debug.
					1000,	// ntf marker resilience.
					1000,	// goal pred success resilience.
					2);		// probe level.

		uint32	stdin_oid;
		std::string	stdin_symbol("stdin");
		uint32	stdout_oid;
		std::string	stdout_symbol("stdout");
		uint32	self_oid;
		std::string	self_symbol("self");
		UNORDERED_MAP<uint32,std::string>::const_iterator	n;
		for(n=r_exec::Seed.object_names.symbols.begin();n!=r_exec::Seed.object_names.symbols.end();++n){

			if(n->second==stdin_symbol)
				stdin_oid=n->first;
			else	if(n->second==stdout_symbol)
				stdout_oid=n->first;
			else	if(n->second==self_symbol)
				self_oid=n->first;
		}

		mem->load(ram_objects.as_std(),stdin_oid,stdout_oid,self_oid);

		return mem;
	}

	void	_Mem::unpack_hlp(Code	*hlp)	const{	// produces a new object (featuring a set of pattern objects instread of a set of embedded pattern expressions) and add it as a hidden reference to the original (still packed) hlp.

		Code	*unpacked_hlp=new	LObject();	// will not be transmitted nor decompiled.

		for(uint16	i=0;i<hlp->code_size();++i)
			unpacked_hlp->code(i)=hlp->code(i);

		uint16	pattern_set_index=hlp->code(HLP_OBJS).asIndex();
		uint16	pattern_count=hlp->code(pattern_set_index).getAtomCount();
		for(uint16	i=1;i<=pattern_count;++i){	// init the new references with the facts; turn the exisitng i-ptrs into r-ptrs.

			Code	*fact=unpack_fact(hlp,hlp->code(pattern_set_index+i).asIndex());
			unpacked_hlp->add_reference(fact);
			unpacked_hlp->code(pattern_set_index+i)=Atom::RPointer(unpacked_hlp->references_size()-1);
		}

		uint16	group_set_index=hlp->code(HLP_OUT_GRPS).asIndex();
		uint16	group_count=hlp->code(group_set_index++).getAtomCount();
		for(uint16	i=0;i<group_count;++i){	// append the out_groups to the new references; adjust the exisitng r-ptrs.

			unpacked_hlp->add_reference(hlp->get_reference(hlp->code(group_set_index+i).asIndex()));
			unpacked_hlp->code(group_set_index+i)=Atom::RPointer(unpacked_hlp->references_size()-1);
		}

		uint16	invalid_point=pattern_set_index+pattern_count+1;	// index of what is after set of the patterns.
		uint16	valid_point=hlp->code(HLP_FWD_GUARDS).asIndex();	// index of the first atom that does not belong to the patterns.
		uint16	invalid_zone_length=valid_point-invalid_point;
		for(uint16	i=valid_point;i<hlp->code_size();++i){	//	shift the valid code upward; adjust i-ptrs.

			Atom	h_atom=hlp->code(i);
			switch(h_atom.getDescriptor()){
			case	Atom::I_PTR:
				unpacked_hlp->code(i-invalid_zone_length)=Atom::IPointer(h_atom.asIndex()-invalid_zone_length);
				break;
			case	Atom::ASSIGN_PTR:
				unpacked_hlp->code(i-invalid_zone_length)=Atom::AssignmentPointer(h_atom.asAssignmentIndex(),h_atom.asIndex()-invalid_zone_length);
				break;
			default:
				unpacked_hlp->code(i-invalid_zone_length)=h_atom;
				break;
			}
		}

		// adjust set indices.
		unpacked_hlp->code(HLP_FWD_GUARDS)=Atom::IPointer(hlp->code(CST_FWD_GUARDS).asIndex()-invalid_zone_length);
		unpacked_hlp->code(HLP_BWD_GUARDS)=Atom::IPointer(hlp->code(CST_BWD_GUARDS).asIndex()-invalid_zone_length);
		unpacked_hlp->code(HLP_OUT_GRPS)=Atom::IPointer(hlp->code(CST_OUT_GRPS).asIndex()-invalid_zone_length);

		uint16	unpacked_code_length=hlp->code_size()-invalid_zone_length;
		unpacked_hlp->resize_code(unpacked_code_length);
		hlp->add_reference(unpacked_hlp);
	}

	Code	*_Mem::unpack_fact(Code	*hlp,uint16	fact_index)	const{

		Code	*fact=new	LObject();
		Code	*fact_object;
		uint16	fact_size=hlp->code(fact_index).getAtomCount()+1;
		for(uint16	i=0;i<fact_size;++i){

			Atom	h_atom=hlp->code(fact_index+i);
			switch(h_atom.getDescriptor()){
			case	Atom::I_PTR:
				fact->code(i)=Atom::RPointer(fact->references_size());
				fact_object=unpack_fact_object(hlp,h_atom.asIndex());
				fact->add_reference(fact_object);
				break;
			case	Atom::R_PTR:	// case of a reference to an exisitng object.
				fact->code(i)=Atom::RPointer(fact->references_size());
				fact->add_reference(hlp->get_reference(h_atom.asIndex()));
				break;
			default:
				fact->code(i)=h_atom;
				break;
			}
		}

		return	fact;
	}

	Code	*_Mem::unpack_fact_object(Code	*hlp,uint16	fact_object_index)	const{

		Code	*fact_object=new	LObject();
		_unpack_code(hlp,fact_object_index,fact_object,fact_object_index);
		return	fact_object;
	}

	void	_Mem::_unpack_code(Code	*hlp,uint16	fact_object_index,Code	*fact_object,uint16	read_index)	const{

		Atom	h_atom=hlp->code(read_index);
		uint16	code_size=h_atom.getAtomCount()+1;
		uint16	write_index=read_index-fact_object_index;
		for(uint16	i=0;i<code_size;++i){

			switch(h_atom.getDescriptor()){
			case	Atom::R_PTR:
				fact_object->code(write_index+i)=Atom::RPointer(fact_object->references_size());
				fact_object->add_reference(hlp->get_reference(h_atom.asIndex()));
				break;
			case	Atom::I_PTR:
				fact_object->code(write_index+i)=Atom::IPointer(h_atom.asIndex()-fact_object_index);
				_unpack_code(hlp,fact_object_index,fact_object,h_atom.asIndex());
				break;
			default:
				fact_object->code(write_index+i)=h_atom;
				break;
			}

			h_atom=hlp->code(read_index+i+1);
		}
	}

	void	_Mem::pack_hlp(Code	*hlp)	const{	// produces a new object where a set of pattern objects is transformed into a packed set of pattern code.

		Code	*unpacked_hlp=clone(hlp);

		std::vector<Atom>	trailing_code;	//	copy of the original code (the latter will be overwritten by packed facts).
		uint16	trailing_code_index=hlp->code(HLP_FWD_GUARDS).asIndex();
		for(uint16	i=trailing_code_index;i<hlp->code_size();++i)
			trailing_code.push_back(hlp->code(i));

		uint16	group_set_index=hlp->code(HLP_OUT_GRPS).asIndex();
		uint16	group_count=hlp->code(group_set_index).getAtomCount();

		std::vector<P<Code>	>	references;

		uint16	pattern_set_index=hlp->code(HLP_OBJS).asIndex();
		uint16	pattern_count=hlp->code(pattern_set_index).getAtomCount();
		uint16	insertion_point=pattern_set_index+pattern_count+1;	// point from where compacted code is to be inserted.
		uint16	extent_index=insertion_point;
		for(uint16	i=0;i<pattern_count;++i){

			Code	*pattern_object=hlp->get_reference(i);
			hlp->code(pattern_set_index+i+1)=Atom::IPointer(extent_index);
			pack_fact(pattern_object,hlp,extent_index,&references);
		}

		uint16	inserted_zone_length=extent_index-insertion_point;

		for(uint16	i=0;i<trailing_code.size();++i){	// shift the trailing code downward; adjust i-ptrs.

			Atom	t_atom=trailing_code[i];
			switch(t_atom.getDescriptor()){
			case	Atom::I_PTR:
				hlp->code(i+extent_index)=Atom::IPointer(t_atom.asIndex()+inserted_zone_length);
				break;
			case	Atom::ASSIGN_PTR:
				hlp->code(i+extent_index)=Atom::AssignmentPointer(t_atom.asAssignmentIndex(),t_atom.asIndex()+inserted_zone_length);
				break;
			default:
				hlp->code(i+extent_index)=t_atom;
				break;
			}
		}

		// adjust set indices.
		hlp->code(CST_FWD_GUARDS)=Atom::IPointer(hlp->code(HLP_FWD_GUARDS).asIndex()+inserted_zone_length);
		hlp->code(CST_BWD_GUARDS)=Atom::IPointer(hlp->code(HLP_BWD_GUARDS).asIndex()+inserted_zone_length);
		hlp->code(CST_OUT_GRPS)=Atom::IPointer(hlp->code(HLP_OUT_GRPS).asIndex()+inserted_zone_length);

		group_set_index+=inserted_zone_length;
		for(uint16	i=1;i<=group_count;++i){	//	append the out_groups to the new references; adjust the exisitng r-ptrs.

			references.push_back(hlp->get_reference(hlp->code(group_set_index+i).asIndex()));
			hlp->code(group_set_index+i)=Atom::RPointer(references.size()-1);
		}

		hlp->set_references(references);

		hlp->add_reference(unpacked_hlp);	// hidden reference.
	}

	void	_Mem::pack_fact(Code	*fact,Code	*hlp,uint16	&write_index,std::vector<P<Code>	>	*references)	const{

		uint16	extent_index=write_index+fact->code_size();
		for(uint16	i=0;i<fact->code_size();++i){

			Atom	p_atom=fact->code(i);
			switch(p_atom.getDescriptor()){
			case	Atom::R_PTR:	//	transform into a i_ptr and pack the pointed object.
				hlp->code(write_index)=Atom::IPointer(extent_index);
				pack_fact_object(fact->get_reference(p_atom.asIndex()),hlp,extent_index,references);
				++write_index;
				break;
			default:
				hlp->code(write_index)=p_atom;
				++write_index;
				break;
			}
		}
		write_index=extent_index;
	}

	void	_Mem::pack_fact_object(Code	*fact_object,Code	*hlp,uint16	&write_index,std::vector<P<Code>	>	*references)	const{

		uint16	extent_index=write_index+fact_object->code_size();
		uint16	offset=write_index;
		for(uint16	i=0;i<fact_object->code_size();++i){

			Atom	p_atom=fact_object->code(i);
			switch(p_atom.getDescriptor()){
			case	Atom::R_PTR:{	//	append this reference to the hlp's if not already there.
				Code	*reference=fact_object->get_reference(p_atom.asIndex());
				bool	found=false;
				for(uint16	i=0;i<references->size();++i){

					if((*references)[i]==reference){

						hlp->code(write_index)=Atom::RPointer(i);
						found=true;
						break;
					}
				}
				if(!found){

					hlp->code(write_index)=Atom::RPointer(references->size());
					references->push_back(reference);
				}
				++write_index;
				break;
			}case	Atom::I_PTR:	//	offset the ptr by write_index. PB HERE.
				hlp->code(write_index)=Atom::IPointer(offset+p_atom.asIndex());
				++write_index;
				break;
			default:
				hlp->code(write_index)=p_atom;
				++write_index;
				break;
			}
		}
	}

	Code	*_Mem::clone(Code	*original)	const{	// shallow copy; oid not copied.

		Code	*_clone=build_object(original->code(0));
		uint16	opcode=original->code(0).asOpcode();
		if(opcode==Opcodes::Ont	||	opcode==Opcodes::Ent)
			return	original;

		for(uint16	i=0;i<original->code_size();++i)
			_clone->code(i)=original->code(i);
		for(uint16	i=0;i<original->references_size();++i)
			_clone->add_reference(original->get_reference(i));
		return	_clone;
	}
}