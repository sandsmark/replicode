#include	"replicode_defs.h"
#include	"../r_code/utils.h"


namespace	r_exec{

	inline	Object::Object():r_code::Object(),mem(NULL),hash_value(0){

		psln_thr_sem=new	FastSemaphore(1,1);
		views_sem=new	FastSemaphore(1,1);
		marker_set_sem=new	FastSemaphore(1,1);
	}

	inline	Object::Object(r_code::SysObject	*source,Mem	*m):r_code::Object(source),mem(m),hash_value(0){

		computeHashValue();
		psln_thr_sem=new	FastSemaphore(1,1);
		views_sem=new	FastSemaphore(1,1);
		marker_set_sem=new	FastSemaphore(1,1);
	}

	inline	Object::Object(uint32	OID,Mem	*m):r_code::Object(),mem(mem),hash_value(0),OID(OID){

		psln_thr_sem=new	FastSemaphore(1,1);
		views_sem=new	FastSemaphore(1,1);
		marker_set_sem=new	FastSemaphore(1,1);
	}

	inline	Object::~Object(){

		if(mem)
			mem->removeObject(this);
		delete	psln_thr_sem;
		delete	views_sem;
		delete	marker_set_sem;
	}

	inline	uint32	Object::getOID()	const{

		return	OID;
	}

	inline	void	Object::computeHashValue(){	//	assuming hash_value==0.

		hash_value=code(0).asOpcode()<<20;							//	12 bits for the opcode.
		hash_value|=(code_size()	&	0x00000FFF)<<8;		//	12 bits for the code size.
		hash_value|=references_size()	&	0x000000FF;	//	8 bits for the reference set size.
	}

	inline	float32	Object::get_psln_thr(){

		psln_thr_sem->acquire();
		float32	r=code(code_size()-1).asFloat();	//	psln is always the lat atom in code.
		psln_thr_sem->release();
		return	r;
	}

	inline	void	Object::mod(uint16	member_index,float32	value){

		if(member_index!=code_size()-1)
			return;
		float32	v=code(member_index).asFloat()+value;
		if(v<0)
			v=0;
		else	if(v>1)
			v=1;

		psln_thr_sem->acquire();
		code(member_index)=Atom::Float(v);
		psln_thr_sem->release();
	}

	inline	void	Object::set(uint16	member_index,float32	value){

		if(member_index!=code_size()-1)
			return;

		psln_thr_sem->acquire();
		code(member_index)=Atom::Float(value);
		psln_thr_sem->release();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	inline	Object	*InstantiatedProgram::getPGM()	const{

		uint32	index=code(IPGM_PGM).asIndex();
		return	references(index);
	}

	inline	uint64	InstantiatedProgram::get_tsc()	const{

		return	r_code::Timestamp::Get(&getPGM()->code(PGM_TSC));
	}
}