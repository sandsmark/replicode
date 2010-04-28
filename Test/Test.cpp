#include	"../r_comp/decompiler.h"
#include	"../r_comp/compiler.h"
#include	"../r_comp/preprocessor.h"

#include	"../r_exec/Mem.h"

#include	"../r_code/utils.h"

#include	<iostream>


using	namespace	r_comp;

int32	main(int	argc,char	**argv){

	std::string		error;
	std::ifstream	source_code(argv[1]);	//	ANSI encoding (not Unicode)

	r_comp::Image		*_image=new	r_comp::Image();	//	compiler input; definition segment filled by preprocessor

	if(!source_code.good()){

		std::cout<<"error: unable to load file "<<argv[1]<<std::endl;
		return	1;
	}

	std::ostringstream		preprocessed_code_out;
	Preprocessor			preprocessor;
	if(!preprocessor.process(&_image->definition_segment,&source_code,&preprocessed_code_out,&error)){

		std::cout<<error;
		return	2;
	}
	source_code.close();

	std::istringstream	preprocessed_code_in(preprocessed_code_out.str());
	std::cout<<preprocessed_code_in.str()<<std::endl;
	r_code::Image		*image;		//	compiler output, decompiler input, r_exec::Mem input
	Compiler			compiler;
	if(!compiler.compile(&preprocessed_code_in,_image,image,&error)){

		std::streampos	i=preprocessed_code_in.tellg();
		std::cout.write(preprocessed_code_in.str().c_str(),i);
		std::cout<<" <- "<<error<<std::endl;
		delete	image;
		delete	_image;
		return	3;
	}else{

		//image->trace();
		source_code.close();
#if 1
		//	Loading code from the image into memory then into the r_exec::Mem
		//	Instantiate objects and views
		r_code::vector<r_code::Object	*>	ram_objects;
		*_image>>ram_objects;
		
		// Translate the compiler's class table to give just an Atom for use by the Mem
		UNORDERED_MAP<std::string, r_code::Atom> classes;
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
		_image=new	r_comp::Image();
		*_image<<image;				//	this stores the ram_objects in the _image
		_image->removeObjects();	//	remove these objects, to keep only the definiton segment
#define	USE_MEM
#ifdef USE_MEM
		//	Create the mem with objects defined in ram_objects
		r_exec::Mem* mem = r_exec::Mem::create(
			10000, // resilience update period: 10ms
			10000, // base update period: 10ms
			classes,
			*ram_objects.as_std(),
			0
		);
		r_code::Thread::Sleep(3600);
#endif
		//	Loading code from memory to an r_comp::Image
		*_image<<ram_objects;	//	all at once; to load one object obj, use: *_image<<obj;	//	this recursively loads all obj dependencies

		Decompiler			decompiler;
		std::ostringstream	decompiled_code;
		decompiler.decompile(_image,&decompiled_code);
		std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
#endif
		delete	image;
		delete	_image;
	}

	return	0;
}
