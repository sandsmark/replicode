#ifndef	correlator_h
#define	correlator_h

#include	<../r_code/object.h>


class	CorrelatorOutput{
public:
	void	trace(){
	}
};

class	Correlator{
public:
	void	take_input(r_code::View	*input){
	}

	CorrelatorOutput	*get_output(float32	&error){

		return	new	CorrelatorOutput();
	}
};


#endif