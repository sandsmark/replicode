namespace	r_exec{

	template<class	C>	Object<C>::Object():C(),mem(NULL),hash_value(0){

		psln_thr_sem=new	FastSemaphore(1,1);
		views_sem=new	FastSemaphore(1,1);
		marker_set_sem=new	FastSemaphore(1,1);
	}

	template<class	C>	Object<C>::Object(r_code::Mem	*mem):C(),mem(mem),hash_value(0){

		psln_thr_sem=new	FastSemaphore(1,1);
		views_sem=new	FastSemaphore(1,1);
		marker_set_sem=new	FastSemaphore(1,1);
	}

	template<class	C>	Object<C>::~Object(){

		if(mem)
			mem->deleteObject(this);
		delete	psln_thr_sem;
		delete	views_sem;
		delete	marker_set_sem;
	}

	template<class	C>	void	Object<C>::compute_hash_value(){

		hash_value=code(0).asOpcode()<<20;				//	12 bits for the opcode.
		hash_value|=(code_size()	&	0x00000FFF)<<8;	//	12 bits for the code size.
		hash_value|=references_size()	&	0x000000FF;	//	8 bits for the reference set size.
	}

	template<class	C>	float32	Object<C>::get_psln_thr(){

		psln_thr_sem->acquire();
		float32	r=code(code_size()-1).asFloat();	//	psln is always the lat atom in code.
		psln_thr_sem->release();
		return	r;
	}

	template<class	C>	void	Object<C>::mod(uint16	member_index,float32	value){

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

	template<class	C>	void	Object<C>::set(uint16	member_index,float32	value){

		if(member_index!=code_size()-1)
			return;

		psln_thr_sem->acquire();
		code(member_index)=Atom::Float(value);
		psln_thr_sem->release();
	}

	template<class	C>	View	*Object<C>::find_view(Code	*group){

		return	(View	*)mem->find_view(this,group);
	}
}