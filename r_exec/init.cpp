#include	"init.h"
#include	"object.h"
#include	"operator.h"
#include	"opcodes.h"

#include	"../r_comp/preprocessor.h"


namespace	r_exec{

	dll_export	uint64	(*Now)();

	dll_export	r_comp::Metadata	Metadata;
	dll_export	r_comp::Image		Seed;

	UNORDERED_MAP<std::string,uint16>	Opcodes;

	dll_export	r_comp::Compiler		Compiler;
	r_exec_dll	r_comp::Preprocessor	Preprocessor;

	bool	Compile(std::istream	&source_code,std::string	&error,bool	compile_metadata){

		std::ostringstream	preprocessed_code_out;
		if(!r_exec::Preprocessor.process(&source_code,&preprocessed_code_out,error,compile_metadata?&Metadata:NULL))
			return	false;

		std::istringstream	preprocessed_code_in(preprocessed_code_out.str());

		if(!r_exec::Compiler.compile(&preprocessed_code_in,&Seed,&Metadata,error,false)){

			std::streampos	i=preprocessed_code_in.tellg();
			std::cerr.write(preprocessed_code_in.str().c_str(),i);
			std::cerr<<" <- "<<error<<std::endl;
			return	false;
		}

		return	true;
	}

	bool	Compile(const	char	*filename,std::string	&error,bool	compile_metadata){

		std::ifstream	source_code(filename);
		if(!source_code.good()){

			error="unable to load file ";
			error+=filename;
			return	false;
		}

		bool	r=Compile(source_code,error,compile_metadata);
		source_code.close();
		return	r;
	}

	bool	Compile(const	char	*filename,std::string	&error){

		return	Compile(filename,error,false);
	}

	bool	Compile(std::istream	&source_code,std::string	&error){

		return	Compile(source_code,error,false);
	}

	bool	Init(const	char	*user_operator_library_path,
				uint64			(*time_base)()){

		UNORDERED_MAP<std::string,r_comp::Class>::iterator it;
		for(it=Metadata.classes.begin();it!=Metadata.classes.end();++it){

			Opcodes[it->first]=it->second.atom.asOpcode();
			std::cout<<it->first<<":"<<it->second.atom.asOpcode()<<std::endl;
		}
		for(it=Metadata.sys_classes.begin();it!=Metadata.sys_classes.end();++it){

			Opcodes[it->first]=it->second.atom.asOpcode();
			std::cout<<it->first<<":"<<it->second.atom.asOpcode()<<std::endl;
		}

		//	load class Opcodes.
		View::ViewOpcode=Opcodes.find("view")->second;

		Opcodes::Group=Opcodes.find("grp")->second;

		Opcodes::PTN=Opcodes.find("ptn")->second;
		Opcodes::AntiPTN=Opcodes.find("|ptn")->second;

		Opcodes::IPGM=Opcodes.find("ipgm")->second;
		Opcodes::PGM=Opcodes.find("pgm")->second;
		Opcodes::AntiPGM=Opcodes.find("|pgm")->second;

		Opcodes::IGoal=Opcodes.find("igol")->second;
		Opcodes::Goal=Opcodes.find("gol")->second;
		Opcodes::AntiGoal=Opcodes.find("|gol")->second;

		Opcodes::MkRdx=Opcodes.find("mk.rdx")->second;
		Opcodes::MkAntiRdx=Opcodes.find("mk.|rdx")->second;

		Opcodes::MkNew=Opcodes.find("mk.new")->second;

		Opcodes::MkLowRes=Opcodes.find("mk.low_res")->second;
		Opcodes::MkLowSln=Opcodes.find("mk.low_sln")->second;
		Opcodes::MkHighSln=Opcodes.find("mk.high_sln")->second;
		Opcodes::MkLowAct=Opcodes.find("mk.low_act")->second;
		Opcodes::MkHighAct=Opcodes.find("mk.high_act")->second;
		Opcodes::MkSlnChg=Opcodes.find("mk.sln_chg")->second;
		Opcodes::MkActChg=Opcodes.find("mk.act_chg")->second;

		//	load executive function Opcodes.
		Opcodes::Inject=Opcodes.find("_inj")->second;
		Opcodes::Eject=Opcodes.find("_eje")->second;
		Opcodes::Mod=Opcodes.find("_eje")->second;
		Opcodes::Set=Opcodes.find("_eje")->second;
		Opcodes::NewClass=Opcodes.find("_new_class")->second;
		Opcodes::DelClass=Opcodes.find("_del_class")->second;
		Opcodes::LDC=Opcodes.find("_ldc")->second;
		Opcodes::Swap=Opcodes.find("_swp")->second;
		Opcodes::NewDev=Opcodes.find("_new_dev")->second;
		Opcodes::DelDev=Opcodes.find("_del_dev")->second;
		Opcodes::Suspend=Opcodes.find("_suspend")->second;
		Opcodes::Resume=Opcodes.find("_resume")->second;
		Opcodes::Stop=Opcodes.find("_stop")->second;

		//	load std operators.
		uint16	operator_opcode=0;
		Operator::Register(operator_opcode++,now);
		Operator::Register(operator_opcode++,equ);
		Operator::Register(operator_opcode++,neq);
		Operator::Register(operator_opcode++,gtr);
		Operator::Register(operator_opcode++,lsr);
		Operator::Register(operator_opcode++,gte);
		Operator::Register(operator_opcode++,lse);
		Operator::Register(operator_opcode++,add);
		Operator::Register(operator_opcode++,sub);
		Operator::Register(operator_opcode++,mul);
		Operator::Register(operator_opcode++,div);
		Operator::Register(operator_opcode++,dis);
		Operator::Register(operator_opcode++,ln);
		Operator::Register(operator_opcode++,exp);
		Operator::Register(operator_opcode++,log);
		Operator::Register(operator_opcode++,e10);
		Operator::Register(operator_opcode++,syn);
		Operator::Register(operator_opcode++,ins);
		Operator::Register(operator_opcode++,at);
		Operator::Register(operator_opcode++,red);
		Operator::Register(operator_opcode++,com);
		Operator::Register(operator_opcode++,spl);
		Operator::Register(operator_opcode++,mrg);
		Operator::Register(operator_opcode++,ptc);
		Operator::Register(operator_opcode++,fvw);

		//	load usr operators.
		SharedLibrary	userOperatorLibrary;
		if(!(userOperatorLibrary.load(user_operator_library_path)))
			exit(-1);

		typedef	void	(*UserInit)(UNORDERED_MAP<std::string,uint16>	&);
		UserInit	Init=userOperatorLibrary.getFunction<UserInit>("Init");
		if(!Init)
			return	false;

		typedef	bool	(*UserOperator)(const	Context	&,uint16	&);
		typedef	uint16	(*UserGetOperatorCount)();
		UserGetOperatorCount	GetOperatorCount=userOperatorLibrary.getFunction<UserGetOperatorCount>("GetOperatorCount");
		if(!GetOperatorCount)
			return	false;

		typedef	void	(*UserGetOperator)(UserOperator	&,std::string	&);
		UserGetOperator	GetOperator=userOperatorLibrary.getFunction<UserGetOperator>("GetOperator");
		if(!GetOperator)
			return	false;

		std::cout<<"> User-defined operator library "<<user_operator_library_path<<" loaded"<<std::endl;

		Init(Opcodes);
		uint16	operatorCount=GetOperatorCount();
		for(uint16	i=0;i<operatorCount;++i){

			UserOperator	op;
			std::string		op_name;
			GetOperator(op,op_name);

			UNORDERED_MAP<std::string,uint16>::iterator	it=Opcodes.find(op_name);
			if(it==Opcodes.end()){

				std::cerr<<"Operator "<<op_name<<" is undefined"<<std::endl;
				exit(-1);
			}
			Operator::Register(it->second,op);
		}

		return	true;
	}

	bool	Init(const	char	*user_operator_library_path,
				uint64			(*time_base)(),
				const	char	*seed_path){

		Now=time_base;

		std::string	error;
		if(!Compile(seed_path,error,true)){

			std::cerr<<error<<std::endl;
			return	false;
		}

		return	Init(user_operator_library_path,time_base);
	}

	bool	Init(const	char				*user_operator_library_path,
				uint64						(*time_base)(),
				const	r_comp::Metadata	&metadata,
				const	r_comp::Image		&seed){

		Metadata=metadata;
		Seed=seed;

		return	Init(user_operator_library_path,time_base);
	}

	uint16	GetOpcode(const	char	*name){

		UNORDERED_MAP<std::string,uint16>::iterator it=Opcodes.find(name);
		if(it==Opcodes.end())
			return	0xFFFF;
		return	it->second;
	}
}