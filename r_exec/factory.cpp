#include	"factory.h"


namespace	r_exec{
	namespace	factory{

		MkNew::MkNew(Code	*object):LObject(){

			uint32	write_index=0;
			code(write_index++)=r_code::Atom::Marker(Opcodes::MkNew,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
			references(0)=object;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkLowRes::MkLowRes(Code	*object):LObject(){

			uint32	write_index=0;
			code(write_index++)=r_code::Atom::Marker(Opcodes::MkLowRes,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
			references(0)=object;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkLowSln::MkLowSln(Code	*object):LObject(){

			uint32	write_index=0;
			code(write_index++)=r_code::Atom::Marker(Opcodes::MkLowSln,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
			references(0)=object;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkHighSln::MkHighSln(Code	*object):LObject(){

			uint32	write_index=0;
			code(write_index++)=r_code::Atom::Marker(Opcodes::MkHighSln,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
			references(0)=object;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkLowAct::MkLowAct(Code	*object):LObject(){

			uint32	write_index=0;
			code(write_index++)=r_code::Atom::Marker(Opcodes::MkLowAct,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
			references(0)=object;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkHighAct::MkHighAct(Code	*object):LObject(){

			uint32	write_index=0;
			code(write_index++)=r_code::Atom::Marker(Opcodes::MkHighAct,5);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
			references(0)=object;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkSlnChg::MkSlnChg(Code	*object,float32	value):LObject(){

			uint32	write_index=0;
			code(write_index++)=r_code::Atom::Marker(Opcodes::MkSlnChg,6);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::Float(value);	//	change.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
			references(0)=object;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		MkActChg::MkActChg(Code	*object,float32	value):LObject(){

			uint32	write_index=0;
			code(write_index++)=r_code::Atom::Marker(Opcodes::MkActChg,6);
			code(write_index++)=r_code::Atom::RPointer(0);	//	object.
			code(write_index++)=r_code::Atom::Float(value);	//	change.
			code(write_index++)=r_code::Atom::View();
			code(write_index++)=r_code::Atom::Mks();
			code(write_index++)=r_code::Atom::Vws();
			code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
			references(0)=object;
		}
	}
}