#ifndef	init_h
#define	init_h

#include	"utils.h"

#include	"../r_comp/segments.h"
#include	"../r_comp/compiler.h"
#include	"../r_comp/preprocessor.h"


#if defined	EXECUTIVE_EXPORTS
	#define r_exec_dll	__declspec(dllexport)
#else
	#define r_exec_dll	__declspec(dllimport)
#endif

namespace	r_exec{

	//	Time base; either Time::Get or network-aware synced time.
	extern	r_exec_dll	uint64				(*Now)();

	//	Loaded once for all.
	//	Results from the compilation of user.classes.replicode.
	//	The latter contains all class definitions and all shared objects (e.g. ontology); does not contain any dynamic (res!=forever) objects.
	extern	r_exec_dll	r_comp::Metadata	Metadata;
	extern	r_exec_dll	r_comp::Image		Seed;

	//	A preprocessor and a compiler are maintained throughout the life of the dll to retain, respectively, macros and global references.
	//	Both functions add the compiled object to Seed.code_image.
	//	Source files: use ANSI encoding (not Unicode).
	bool	r_exec_dll	Compile(const	char	*filename,std::string	&error);
	bool	r_exec_dll	Compile(std::istream	&source_code,std::string	&error);
	
	//	Initialize Now, compile user.classes.replicode and builds the Seed and loads the user-defined operators.
	//	Return false in case of a problem (e.g. file not found).
	bool	r_exec_dll	Init(const	char	*user_operator_library_path,
							uint64			(*time_base)(),
							const	char	*seed_path);

	//	Alternate taking a ready-made metadata and seed (will be copied into Metadata and Seed).
	bool	r_exec_dll	Init(const	char				*user_operator_library_path,
							uint64						(*time_base)(),
							const	r_comp::Metadata	&metadata,
							const	r_comp::Image		&seed);

	uint16	r_exec_dll	GetOpcode(const	char	*class_name);
}


#endif