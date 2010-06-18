#include	"object.h"
#include	"replicode_defs.h"
#include	"../r_code/utils.h"
#include	"mem.h"
#include	"opcodes.h"
#include	<math.h>


namespace	r_exec{

	ObjectType	Object::getType()	const{

		return	OTHER;
	}

	bool	Object::isIPGM()	const{

		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	InstantiatedProgram::InstantiatedProgram():Object(){
	}

	InstantiatedProgram::InstantiatedProgram(r_code::SysObject	*source,Mem	*m):Object(source,m){
	}

	InstantiatedProgram::~InstantiatedProgram(){
	}

	ObjectType	InstantiatedProgram::getType()	const{

		if(getPGM()->code(0).asOpcode()==Opcodes::AntiPGM)
			return	ANTI_IPGM;
		if(getPGM()->code(PGM_INPUTS).getAtomCount()>0)
			return	IPGM;
		return	INPUT_LESS_IPGM;
	}

	bool	InstantiatedProgram::isIPGM()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Marker::Marker():Object(){
	}

	Marker::Marker(r_code::SysObject	*source,Mem	*m):Object(source,m){
	}

	Marker::~Marker(){
	}

	ObjectType	Marker::getType()	const{

		return	MARKER;
	}

	bool	Marker::isNotification()	const{

		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkNew::MkNew(Object	*object):Marker(){

		uint32	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkNew,5);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::View();
		code(write_index++)=r_code::Atom::Mks();
		code(write_index++)=r_code::Atom::Vws();
		code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
		references(0)=object;
	}

	bool	MkNew::isNotification()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkLowRes::MkLowRes(Object	*object):Marker(){

		uint32	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkLowRes,5);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::View();
		code(write_index++)=r_code::Atom::Mks();
		code(write_index++)=r_code::Atom::Vws();
		code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
		references(0)=object;
	}

	bool	MkLowRes::isNotification()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkLowSln::MkLowSln(Object	*object):Marker(){

		uint32	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkLowSln,5);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::View();
		code(write_index++)=r_code::Atom::Mks();
		code(write_index++)=r_code::Atom::Vws();
		code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
		references(0)=object;
	}

	bool	MkLowSln::isNotification()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkHighSln::MkHighSln(Object	*object):Marker(){

		uint32	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkHighSln,5);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::View();
		code(write_index++)=r_code::Atom::Mks();
		code(write_index++)=r_code::Atom::Vws();
		code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
		references(0)=object;
	}

	bool	MkHighSln::isNotification()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkLowAct::MkLowAct(Object	*object):Marker(){

		uint32	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkLowAct,5);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::View();
		code(write_index++)=r_code::Atom::Mks();
		code(write_index++)=r_code::Atom::Vws();
		code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
		references(0)=object;
	}

	bool	MkLowAct::isNotification()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkHighAct::MkHighAct(Object	*object):Marker(){

		uint32	write_index=0;
		code(write_index++)=r_code::Atom::Marker(Opcodes::MkHighAct,5);
		code(write_index++)=r_code::Atom::RPointer(0);	//	object.
		code(write_index++)=r_code::Atom::View();
		code(write_index++)=r_code::Atom::Mks();
		code(write_index++)=r_code::Atom::Vws();
		code(write_index++)=r_code::Atom::Float(1);		//	psln_thr.
		references(0)=object;
	}

	bool	MkHighAct::isNotification()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkSlnChg::MkSlnChg(Object	*object,float32	value):Marker(){

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

	bool	MkSlnChg::isNotification()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkActChg::MkActChg(Object	*object,float32	value):Marker(){

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

	bool	MkActChg::isNotification()	const{

		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MkRdx::MkRdx():Marker(){
	}

	bool	MkRdx::isNotification()	const{

		return	true;
	}
}