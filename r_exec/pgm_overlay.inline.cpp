namespace	r_exec{

	inline	void	Overlay::kill(){
	}

	inline	InstantiatedProgram	*Overlay::getIPGM()	const{	
		
		return	controller->getIPGM();
	}
	
	inline	r_exec::View	*Overlay::getIPGMView()	const{	
		
		return	controller->getIPGMView();
	}
	
	inline	r_exec::Object	*Overlay::getInputObject(uint16	i)	const{	
		
		return	input_views[i]->object;
	}
	
	inline	r_exec::View	*Overlay::getInputView(uint16	i)	const{	
		
		return	(r_exec::View*)&input_views[i];
	}

	inline	void	Overlay::patch_code(uint16	index,Atom	value){

		pgm_code[index]=value;
		patch_indices.push_back(index);
	}

	////////////////////////////////////////////////////////////////

	inline	void	AntiOverlay::kill(){
		
		alive=false;
	}
	
	inline	bool	AntiOverlay::is_alive()	const{
		
		return	alive;
	}

	////////////////////////////////////////////////////////////////

	inline	bool	IPGMController::is_alive()	const{
		
		return	alive;
	}

	inline	r_exec::View	*IPGMController::getIPGMView()	const{
		
		return	(r_exec::View	*)ipgm_view;
	}
	
	inline	InstantiatedProgram	*IPGMController::getIPGM()	const{
		
		return	(InstantiatedProgram	*)ipgm_view->object;
	}
}