#include	"../r_comp/decompiler.h"
#include	"../r_comp/compiler.h"
#include	"../r_comp/preprocessor.h"

//#include	"../r_exec/Mem.h"

#include	<iostream>


using	namespace	r_comp;

int32	main(int	argc,char	**argv){

	std::string		error;
	std::ifstream	source_code(argv[1]);	//	ANSI encoding (not Unicode)

	if(!source_code.good()){

		std::cout<<"error: unable to load file "<<argv[1]<<std::endl;
		return	1;
	}

	//	for decompiling
	std::ostringstream	decompiled_code;

	//	compile the source code and show the result
	DefinitionSegment		definition_segment;	//	preprocessor output
	HardCodedPreprocessor	preprocessor;
	std::ostringstream		preprocessed_code;
	if(!preprocessor.process(&definition_segment,&source_code,&preprocessed_code,&error)){

		std::cout<<error;
		return	2;
	}
	r_comp::Image	*_image=new	r_comp::Image(&definition_segment);	//	compiler input
	r_code::Image	*image;											//	compiler output, decompiler input, r_exec::Mem input
	Compiler		compiler;
	Decompiler		decompiler;
	if(!compiler.compile(&source_code,_image,image,&error)){

		std::cout<<error;
		source_code.close();
		delete	image;
		delete	_image;
		return	3;
	}else{

		image->trace();
		source_code.close();
#if 1
		//	Loading code from the image into memory then into the r_exec::Mem
		//	Instantiate objects and views
		r_code::vector<r_code::Object	*>	ram_objects;
		*_image>>ram_objects;
		
		// Translate the compiler's class table to give just an Atom for use by the Mem
		UNORDERED_MAP<std::string, r_code::Atom> classes;
		UNORDERED_MAP<std::string, r_comp::Class>::iterator it;
		for (it = _image->definition_segment->classes.begin(); it != _image->definition_segment->classes.end(); ++it) {
			classes.insert(make_pair(it->first, it->second.atom));
			printf("CLASS %s=0x%08x\n", it->first.c_str(), it->second.atom.atom);
		}
		for (it = _image->definition_segment->sys_classes.begin(); it != _image->definition_segment->sys_classes.end(); ++it) {
			classes.insert(make_pair(it->first, it->second.atom));
			printf("SYS %s=0x%08x\n", it->first.c_str(), it->second.atom.atom);
		}

		delete	_image;
		_image=new	r_comp::Image(&definition_segment);

		//	Create the mem with objects defined in ram_objects
#if 0
		r_exec::Mem* mem = r_exec::Mem::create(
			classes,
			*ram_objects.as_std(),
			0
		);
		sleep(3600);
#endif
		//	Loading code from memory to an r_comp::Image
		*_image<<ram_objects;	//	all at once; to load one object obj, use: *_image<<obj;	//	this recursively loads all obj dependencies
#endif
		//decompiler.decompile(image,&decompiled_code);	//	this is to be called when the image comes from disk/network: not ready yet

		decompiler.decompile(_image,&decompiled_code);	//	uses the direct output form the compiler (instead of the r_code::Image): we can do that since there's no streaming of the image (disk/network), and _image is available
		std::cout<<"\n\n DECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
		delete	image;
		delete	_image;
	}

	return	0;
}
