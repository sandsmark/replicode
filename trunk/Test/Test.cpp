#include	"compiler.h"
#include	"precompiler.h"

#include	<iostream>


using	namespace	replicode;

int	main(int	argc,char	**argv){

	//	for compiling
	mBrane::sdk::Array<RView,16>	storage;
	std::string						error;
	std::ifstream					source(argv[1]);
/*
	//	for precompiling
	std::ostringstream	preprocessor_output;

	PreCompiler prec;
	if(!prec.preCompile(&source,&preprocessor_output,&error)){

		std::cout<<error;
		source.close();
		return	0;
	}else
		std::cout<<"PRE-PROCESSING\n"<<preprocessor_output.str()<<std::endl;

	std::istringstream	preprocessed_source;

	preprocessed_source.str(preprocessor_output.str());
*/
	std::ostringstream	decompiler_output;

	//	compile the source code and show the result
	Compiler	c;
	//if(!c.compile(&preprocessed_source,&storage,&error))
	if(!c.compile(&source,&storage,&error))
		std::cout<<error;
	else{

		c.decompile(storage.get(0),&decompiler_output);
		std::cout<<"FIRST DECOMPILATION\n"<<decompiler_output.str()<<std::endl;
	}

	source.close();

	//	compile the result of the decompilation and show the result
	std::istringstream	decompiled_source;
	decompiled_source.str(decompiler_output.str());
	Compiler	cc;
	if(!cc.compile(&decompiled_source,&storage,&error))
		std::cout<<error;
	else{

		std::ostringstream	decompiler_output;
		cc.decompile(storage.get(0),&decompiler_output);
		std::cout<<"SECOND DECOMPILATION\n"<<decompiler_output.str()<<std::endl;
	}

	return	0;
}