//	test.cpp
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

#include	"decompiler.h"
#include	"mem.h"
#include	"init.h"
#include	"image_impl.h"
#include	"settings.h"


//#define	DECOMPILE_ONE_BY_ONE

using	namespace	r_comp;

/*r_exec::View	*build_view(uint64	time, Code* rstdin){	// this is application dependent WRT view->sync.

	r_exec::View	*view=new	r_exec::View();
	const	uint32	arity=VIEW_ARITY;	// reminder: opcode not included in the arity.
	uint16	write_index=0;
	uint16	extent_index=arity+1;

	view->code(VIEW_OPCODE)=Atom::SSet(r_exec::View::ViewOpcode,arity);
	view->code(VIEW_SYNC)=Atom::Float(View::SYNC_ONCE);		// sync on front.
	view->code(VIEW_IJT)=Atom::IPointer(extent_index);		// iptr to injection time.
	view->code(VIEW_SLN)=Atom::Float(1.0);					// sln.
	view->code(VIEW_RES)=Atom::Float(1);					// res is set to 1 upr of the destination group.
	view->code(VIEW_HOST)=Atom::RPointer(0);				// stdin/stdout is the only reference.
	view->code(VIEW_ORG)=Atom::Nil();				// org.

	Utils::SetTimestamp(&view->code(extent_index),time);

	view->references[0]=rstdin;

	return	view;
}

Code	*make_object(r_exec::Mem<r_exec::LObject>	*mem, Code* rstdin, float32 i){

	Code	*object=new	r_exec::LObject(mem);
	object->code(0)=Atom::Marker(r_exec::GetOpcode("mk.val"),4);	//	Caveat: arity does not include the opcode.
	//object->code(0)=Atom::Marker(r_exec::Opcodes::MkVal,4);	//	Caveat: arity does not include the opcode.
	object->code(1)=Atom::RPointer(0);
	object->code(2)=Atom::RPointer(1);
	object->code(3)=Atom::Float(i);
	object->code(4)=Atom::Float(1);		//	psln_thr.

	object->set_reference(0,rstdin);
	object->set_reference(1,rstdin);

	return	object;
}

void test_injection(r_exec::Mem<r_exec::LObject>	*mem, float32 n) {

	Code* rstdin = mem->get_stdin();

	int64 tt1 = 0;
    int64 tt2 = 0;
    int64 tt3 = 0;
    int64 tt4 = 0;

    uint64    t0=r_exec::Now();

    for(float32 i = 0; i < n; ++i) {
        tt1 -= (int64) r_exec::Now();
        Code* object = make_object(mem, rstdin, i);
        tt1 += (int64) r_exec::Now();

        uint64    now=r_exec::Now();

        // Build a fact.
        tt2 -= (int64) r_exec::Now();
        Code    *fact=new    r_exec::Fact(object,now,now,1,1);
        tt2 += (int64) r_exec::Now();

        // Build a default view for the fact.
        tt3 -= (int64) r_exec::Now();
        r_exec::View    *view=build_view(now, rstdin);
        tt3 += (int64) r_exec::Now();

        // Inject the view.
        tt4 -= (int64) r_exec::Now();
        view->set_object(fact);
        mem->inject(view);
        tt4 += (int64) r_exec::Now();
    }

    uint64  t1=r_exec::Now();
    uint64	t2=t1-t0;
    uint32  samples=mem->timings_report.size();
    uint64  acc=0;
    for(uint32    i=0;i<samples;++i){
        acc+=mem->timings_report[i];
std::cout<<mem->timings_report[i]<<std::endl;}
    std::cout<<"total time: "<<acc<< std::endl;
    std::cout<<"for-loop total time: "<<t2<< std::endl;
    std::cout<<"for-loop accumelated time 1: "<<tt1<< std::endl;
    std::cout<<"for-loop accumelated time 2: "<<tt2<< std::endl;
    std::cout<<"for-loop accumelated time 3: "<<tt3<< std::endl;
    std::cout<<"for-loop accumelated time 4: "<<tt4<< std::endl; 
}*/

void	decompile(Decompiler	&decompiler,r_comp::Image	*image,uint64	time_offset,bool	ignore_named_objects){

#ifdef	DECOMPILE_ONE_BY_ONE
	uint32	object_count=decompiler.decompile_references(image);
	std::cout<<object_count<<" objects in the image\n";
	while(1){

		std::cout<<"> which object (-1 to exit)?\n";
		int32	index;std::cin>>index;
		if(index==-1)
			break;
		if(index>=object_count){

			std::cout<<"> there is only "<<object_count<<" objects\n";
			continue;
		}
		std::ostringstream	decompiled_code;
		decompiler.decompile_object(index,&decompiled_code,time_offset);
		std::cout<<"\n\n> DECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
	}
#else
	std::ostringstream	decompiled_code;
	uint32	object_count=decompiler.decompile(image,&decompiled_code,time_offset,ignore_named_objects);
	//uint32	object_count=image->code_segment.objects.size();
	std::cout<<"\n\n> DECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
	std::cout<<"> image taken at: "<<Time::ToString_year(image->timestamp)<<std::endl;
	std::cout<<"> "<<object_count<<" objects\n";
#endif
}

void	write_to_file(r_comp::Image	*image,std::string	&image_path,Decompiler	*decompiler,uint64	time_offset){

	ofstream	output(image_path.c_str(),ios::binary|ios::out);
	r_code::Image<r_code::ImageImpl>	*i=image->serialize<r_code::Image<r_code::ImageImpl> >();
	r_code::Image<r_code::ImageImpl>::Write(i,output);
	output.close();
	delete	i;

	if(decompiler){

		ifstream	input(image_path.c_str(),ios::binary|ios::in);
		if(!input.good())
			return;

		r_code::Image<r_code::ImageImpl>	*img=(r_code::Image<r_code::ImageImpl>	*)r_code::Image<r_code::ImageImpl>::Read(input);
		input.close();

		r_code::vector<Code	*>	objects;
		r_comp::Image			*_i=new	r_comp::Image();
		_i->load(img);

		decompile(*decompiler,_i,time_offset,false);
		delete	_i;

		delete	img;
	}
}

int32	main(int	argc,char	**argv){

	core::Time::Init(1000);

	Settings	settings;
	if(!settings.load(argv[1]))
		return	1;

	std::cout<<"> compiling ...\n";
	if(!r_exec::Init(settings.usr_operator_path.c_str(),Time::Get,settings.usr_class_path.c_str()))
		return	2;

	srand(r_exec::Now());
	Random::Init();

	std::string	error;
	if(!r_exec::Compile(settings.source_file_name.c_str(),error)){

		std::cerr<<" <- "<<error<<std::endl;
		return	3;
	}else{

		std::cout<<"> ... done\n";

		r_exec::PipeOStream::Open(1);

		Decompiler	decompiler;
		decompiler.init(&r_exec::Metadata);

		r_comp::Image	*image;

		r_exec::Mem<r_exec::LObject>	*mem=new	r_exec::Mem<r_exec::LObject>();

		r_code::vector<r_code::Code	*>	ram_objects;
		r_exec::Seed.get_objects(mem,ram_objects);

		mem->init(	settings.base_period,
					settings.reduction_core_count,
					settings.time_core_count,
					settings.mdl_inertia_sr_thr,
					settings.mdl_inertia_cnt_thr,
					settings.tpx_dsr_thr,
					settings.min_sim_time_horizon,
					settings.max_sim_time_horizon,
					settings.sim_time_horizon,
					settings.tpx_time_horizon,
					settings.perf_sampling_period,
					settings.float_tolerance,
					settings.time_tolerance,
					settings.primary_thz,
					settings.secondary_thz,
					settings.debug,
					settings.ntf_mk_resilience,
					settings.goal_pred_success_resilience,
					settings.probe_level);

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

		if(!mem->load(ram_objects.as_std(),stdin_oid,stdout_oid,self_oid))
			return	4;
		uint64	starting_time=mem->start();
		
		std::cout<<"> running for "<<settings.run_time<<" ms\n\n";
		Thread::Sleep(settings.run_time);

		/*Thread::Sleep(settings.run_time/2);
		test_injection(mem, 66);
		Thread::Sleep(settings.run_time/2);*/

		std::cout<<"\n> shutting rMem down...\n";
		mem->stop();

		//TimeProbe	probe;
		//probe.set();
		image=mem->get_image();
		//probe.check();
		image->object_names.symbols=r_exec::Seed.object_names.symbols;
		
		if(settings.write_image)
			write_to_file(image,settings.image_path,settings.test_image?&decompiler:NULL,starting_time);

		if(settings.decompile_image	&&	(!settings.write_image	||	!settings.test_image)){
            
			if(argc>2){	// argv[2] is a file to redirect the decompiled code to.

				std::ofstream	outfile;
				outfile.open(argv[2],std::ios_base::trunc);
				std::streambuf	*coutbuf=std::cout.rdbuf(outfile.rdbuf()); 

				decompile(decompiler,image,starting_time,settings.ignore_named_objects);
				
				std::cout.rdbuf(coutbuf);
                outfile.close(); 
			}else
				decompile(decompiler,image,starting_time,settings.ignore_named_objects);
        }
		//uint32	w;std::cin>>w;
		delete	image;
		delete	mem;

		//std::cout<<"getImage(): "<<probe.us()<<"us"<<std::endl;

		r_exec::PipeOStream::Close();
	}

	return	0;
}
