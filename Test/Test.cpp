#include	"decompiler.h"

#include	"mem.h"
#include	"init.h"

#include	"image_impl.h"


using	namespace	r_comp;

int32	main(int	argc,char	**argv){

	core::Time::Init(1000);

	r_exec::Init("C:/Work/Replicode/Debug/usr_operators.dll",Time::Get,"C:/Work/Replicode/Test/user.classes.replicode");

	std::string	error;
	if(!r_exec::Compile(argv[1],error)){

		std::cerr<<" <- "<<error<<std::endl;
		return	1;
	}else{

		r_exec::Mem<r_exec::LObject>	*mem=new	r_exec::Mem<r_exec::LObject>();

		r_code::vector<r_code::Code	*>	ram_objects;
		r_exec::Seed.getObjects(&r_exec::Metadata,mem,ram_objects);

		mem->init(100000,1,1);
		mem->load(ram_objects.as_std());
		mem->start();
		uint32	in;std::cout<<"Enter a number to stop the rMem:\n";std::cin>>in;
		mem->stop();

		delete	mem;

		Decompiler			decompiler;
		std::ostringstream	decompiled_code;
		decompiler.init(&r_exec::Metadata);
		decompiler.decompile(&r_exec::Seed,&decompiled_code);
		std::cout<<"\n\nDECOMPILATION\n\n"<<decompiled_code.str()<<std::endl;
	}

	return	0;
}
