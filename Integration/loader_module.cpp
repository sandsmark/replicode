#include "loader_module.h"

#include	"image.h"
#include	"preprocessor.h"
#include	"compiler.h"


using	namespace	r_comp;

LOAD_MODULE(Loader)

void	Loader::initialize(){

	image=NULL;
}

void	Loader::finalize(){
}

void	Loader::compile(std::string		&filename){

	std::string		error;
	std::ifstream	source_code(filename.c_str());	//	ANSI encoding (not Unicode)

	r_comp::Image		*_image=new	r_comp::Image();	//	compiler input; definition segment filled by preprocessor

	if(!source_code.good()){

		std::cout<<"error: unable to load file "<<filename<<std::endl;
		return;
	}

	std::ostringstream		preprocessed_code_out;
	Preprocessor			preprocessor;
	if(!preprocessor.process(&_image->definition_segment,&source_code,&preprocessed_code_out,&error)){

		std::cout<<error;
		return;
	}
	source_code.close();

	std::istringstream	preprocessed_code_in(preprocessed_code_out.str());

	Compiler	compiler;
	if(!compiler.compile(&preprocessed_code_in,_image,&error,false)){

		std::streampos	i=preprocessed_code_in.tellg();
		std::cout.write(preprocessed_code_in.str().c_str(),i);
		std::cout<<" <- "<<error<<std::endl;
		delete	_image;
		return;
	}else{

		image=_image->serialize<ImageMessage>();	
		//image->trace();
		source_code.close();
	}
}