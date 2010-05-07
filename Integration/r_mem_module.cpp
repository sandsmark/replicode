#include "r_mem_module.h"

#include	"image.h"
#include	"preprocessor.h"
#include	"decompiler.h"


LOAD_MODULE(RMem)

void	RMem::initialize(){

	mem=NULL;

	r_code::vector<r_code::Object	*>			initial_objects;	//	empty
	UNORDERED_MAP<std::string, r_code::Atom>	classes;			//	opcodes retrieved by name

	//	load classes from the preprocessing std.replicode
	std::ifstream	source_code("C:/Work/Replicode/Test/std.replicode");	//	TODO: make the path a parameter
	if(!source_code.good()){

		std::cout<<"error: unable to load std.replicode"<<std::endl;
		return;
	}

	r_comp::Image			*_image=new	r_comp::Image();
	std::ostringstream		preprocessed_code_out;
	r_comp::Preprocessor	preprocessor;
	std::string				error;
	if(!preprocessor.process(&_image->definition_segment,&source_code,&preprocessed_code_out,&error)){

		std::cout<<error;
		delete	_image;
		return;
	}
	source_code.close();

	UNORDERED_MAP<std::string, r_comp::Class>::iterator it;
	for (it = _image->definition_segment.classes.begin(); it != _image->definition_segment.classes.end(); ++it) {
		classes.insert(make_pair(it->first, it->second.atom));
		printf("CLASS %s=0x%08x\n", it->first.c_str(), it->second.atom.atom);
	}
	for (it = _image->definition_segment.sys_classes.begin(); it != _image->definition_segment.sys_classes.end(); ++it) {
		classes.insert(make_pair(it->first, it->second.atom));
		printf("SYS %s=0x%08x\n", it->first.c_str(), it->second.atom.atom);
	}

	delete	_image;

	//	build the rMem
	mem=r_exec::Mem::create(
		10000, // resilience update period: 10ms
		10000, // base update period: 10ms
		classes,
		*initial_objects.as_std(),
		NULL
	);
}

void	RMem::finalize(){

	if(mem)
		delete	mem;
}

void	RMem::decompile(r_comp::Image	*image){

	r_comp::Decompiler	decompiler;
	std::ostringstream	decompiled_code;
	decompiler.decompile(image,&decompiled_code);
	std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
}

void	RMem::load(r_comp::Image*image){

}