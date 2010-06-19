#ifndef	factory_h
#define	factory_h

#include	"object.h"
#include	"dll.h"


namespace	r_exec{
	namespace	factory{

		class	r_exec_dll	MkNew:
		public	LObject{
		public:
			MkNew(Code	*object);
		};

		class	r_exec_dll	MkLowRes:
		public	LObject{
		public:
			MkLowRes(Code	*object);
		};

		class	r_exec_dll	MkLowSln:
		public	LObject{
		public:
			MkLowSln(Code	*object);
		};

		class	r_exec_dll	MkHighSln:
		public	LObject{
		public:
			MkHighSln(Code	*object);
		};

		class	r_exec_dll	MkLowAct:
		public	LObject{
		public:
			MkLowAct(Code	*object);
		};

		class	r_exec_dll	MkHighAct:
		public	LObject{
		public:
			MkHighAct(Code	*object);
		};

		class	r_exec_dll	MkSlnChg:
		public	LObject{
		public:
			MkSlnChg(Code	*object,float32	value);
		};

		class	r_exec_dll	MkActChg:
		public	LObject{
		public:
			MkActChg(Code	*object,float32	value);
		};
	}
}


#endif