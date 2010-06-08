#include	"decompiler.h"
#include	"compiler.h"
#include	"preprocessor.h"

#include	"Mem.h"

#include	"../CoreLibrary/utils.h"

#include	"image_impl.h"

#include	<iostream>

//#define PREPROCESSOR_TEST
//#define	R_MEM_TEST

uint64	(*r_exec::Mem::_Now)()=Time::Get;

using	namespace	r_comp;
/*
class	Y{
public:
	Y():ref(0){};
	int32	ref;
	void	dec(){
		if(--ref==0)
			delete	this;
	}
	void	inc(){
		++ref;
	}
};
class	X{
public:
	Y	*data;
	X():data(NULL){}
	X(Y	*y):data(y){
		if(data)
			data->inc();
	}
	~X(){
		if(data)
			data->dec();
	}
	X(const	X	&x){
		data=x.data;
		if(data)
			data->inc();
	}
	X&	operator	=(const	X	&x){
		if(data)
			data->dec();
		data=x.data;
		if(data)
			data->inc();
		return	*this;
	}
	X&	operator	=(Y	*y){
		if(data)
			data->dec();
		data=y;
		if(data)
			data->inc();
		return	*this;
	}
};
*/
int32	main(int	argc,char	**argv){
/*
	Y	*y=new	Y();
	std::vector<X>	*v=new	std::vector<X>();
	v->resize(2);
	(*v)[0]=y;
	(*v)[1]=y;
	delete	v;
*/

	core::Time::Init(1000);

	std::string		error;
	std::ifstream	source_code(argv[1]);	//	WARNING: ANSI encoding (not Unicode).

	r_comp::Image	*_image=new	r_comp::Image();	//	compiler input; class_image filled by preprocessor.

	if(!source_code.good()){

		std::cerr<<"error: unable to load file "<<argv[1]<<std::endl;
		return	1;
	}

	std::ostringstream		preprocessed_code_out;
	Preprocessor			preprocessor;
	if(!preprocessor.process(&_image->class_image,&source_code,&preprocessed_code_out,&error)){

		std::cerr<<error;
		return	2;
	}
	source_code.close();

	std::istringstream	preprocessed_code_in(preprocessed_code_out.str());

	#ifdef PREPROCESSOR_TEST
		std::cout<<preprocessed_code_in.str()<<std::endl;
		return 0;
	#endif

	Compiler	compiler;
	if(!compiler.compile(&preprocessed_code_in,_image,&error,true)){

		std::streampos	i=preprocessed_code_in.tellg();
		std::cerr.write(preprocessed_code_in.str().c_str(),i);
		std::cerr<<" <- "<<error<<std::endl;
		delete	_image;
		return	3;
	}else{

		source_code.close();

		r_code::Image<ImageImpl>	*image;
		image=_image->serialize<r_code::Image<ImageImpl> >();	//	flat structure that contains only objects; that's the mem input. We retain _image->class_image for the class definitions.
		//image->trace();

		r_comp::CodeImage	*_code_image=new	r_comp::CodeImage();
		_code_image->load<r_code::Image<ImageImpl> >(image);

		r_code::vector<r_code::Object	*>	ram_objects;
		_code_image->getObjects(&_image->class_image,ram_objects);

#ifdef	R_MEM_TEST
		r_exec::Mem::Init(&_image->class_image,Time::Get,"C:/Work/Replicode/Debug/usr_operators.dll");
		r_exec::Mem	*mem=new	r_exec::Mem(100000,8,8,NULL);
		mem->init(ram_objects.as_std());
		mem->start();
		mem->stop();

		delete	_code_image;
		_code_image=mem->getCodeImage();
		ram_objects.as_std()->clear();
		_code_image->getObjects(&_image->class_image,ram_objects);

		delete	mem;
#endif
		Decompiler			decompiler;
		std::ostringstream	decompiled_code;
		decompiler.init(&_image->class_image);
		decompiler.decompile(_code_image,&decompiled_code);
		std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;

		delete	_image;
		delete	image;
	}

	return	0;
}
