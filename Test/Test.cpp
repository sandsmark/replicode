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
