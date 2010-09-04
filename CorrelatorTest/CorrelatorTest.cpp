//	CorrelatorTest.cpp
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

#include	<set>

#include	"decompiler.h"
#include	"init.h"
#include	"opcodes.h"
#include	"../r_code/image_impl.h"

#include	"settings.h"
#include	"correlator.h"


void	decompile(r_comp::Decompiler	&decompiler,r_comp::Image	*image,uint64	time_offset){

#ifdef	DECOMPILE_ONE_BY_ONE
	uint32	object_count=decompiler.decompile_references(image);
	std::cout<<object_count<<" objects in the image\n";
	while(1){

		std::cout<<"Which object (-1 to exit)?\n";
		int32	index;std::cin>>index;
		if(index==-1)
			break;
		if(index>=object_count){

			std::cout<<"there is only "<<object_count<<" objects\n";
			continue;
		}
		std::ostringstream	decompiled_code;
		decompiler.decompile_object(index,&decompiled_code,time_offset);
		std::cout<<"... done\n";
		std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
	}
#else
	std::cout<<"\ndecompiling ...\n";
	std::ostringstream	decompiled_code;
	uint32	object_count=decompiler.decompile(image,&decompiled_code,time_offset);
	std::cout<<"... done\n";
	std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
	std::cout<<"Image taken at: "<<Time::ToString_year(image->timestamp)<<std::endl<<std::endl;
	std::cout<<object_count<<" objects\n";
#endif
}

int32	main(int	argc,char	**argv){

	core::Time::Init(1000);

	CorrelatorTestSettings	settings;
	if(!settings.load(argv[1]))
		return	1;

	std::cout<<"compiling ...\n";
	r_exec::Init(settings.usr_operator_path.c_str(),Time::Get,settings.usr_class_path.c_str());
	std::cout<<"... done\n";

	r_comp::Decompiler	decompiler;
	decompiler.init(&r_exec::Metadata);

	Correlator	correlator;

	for(uint32	i=0;i<settings.episode_count;++i){

		std::string	image_path=settings.use_case_path;
		image_path+="/";
		image_path+=settings.use_case_name;
		image_path+=".";
		image_path+=core::String::Int2String(i);
		image_path+=".replicode.image";

		ifstream	input(image_path.c_str(),ios::binary|ios::in);
		if(!input.good())
			return	1;

		r_code::Image<ImageImpl>	*img=(r_code::Image<ImageImpl>	*)r_code::Image<ImageImpl>::Read(input);
		input.close();

		r_code::vector<Code	*>	objects;
		r_comp::Image			*_i=new	r_comp::Image();
		_i->load(img);
		_i->getObjects<r_code::LObject>(objects);

		decompile(decompiler,_i,0);
		delete	_i;

		delete	img;

		//	Filter objects: retain only those which are actual inputs in stdin and store them in a time-ordered list.
		std::set<r_code::View	*,r_code::View::Less>	correlator_inputs;
		for(uint32	i=0;i<objects.size();++i){

			Code	*object=objects[i];
			if(object->code(0).asOpcode()==r_exec::Opcodes::IPGM)
				continue;

			UNORDERED_SET<View	*,View::Hash,View::Equal>::const_iterator	v;
			for(v=object->views.begin();v!=object->views.end();++v){

				if((*v)->references[0]->get_axiom()==r_code::SysObject::STDIN_GRP){

					correlator_inputs.insert(*v);
					break;
				}
			}
		}

		//	Feed the Correlator with one episode.
		std::set<r_code::View	*,r_code::View::Less>::const_iterator	v;
		for(v=correlator_inputs.begin();v!=correlator_inputs.end();++v)
			correlator.take_input(*v);

		//	Get the result.
		float32	error;
		CorrelatorOutput	*output=correlator.get_output(error);
		output->trace();
		delete	output;
	}

	return	0;
}