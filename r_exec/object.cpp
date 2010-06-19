#include	"object.h"
#include	"opcodes.h"


namespace	r_exec{

	bool	IsNotification(Code	*object){

		return	object->code(0).getDescriptor()==Atom::MARKER	&&
				(object->code(0).asOpcode()==Opcodes::MkActChg	||
				object->code(0).asOpcode()==Opcodes::MkAntiRdx	||
				object->code(0).asOpcode()==Opcodes::MkHighAct	||
				object->code(0).asOpcode()==Opcodes::MkHighSln	||
				object->code(0).asOpcode()==Opcodes::MkLowAct	||
				object->code(0).asOpcode()==Opcodes::MkLowRes	||
				object->code(0).asOpcode()==Opcodes::MkLowSln	||
				object->code(0).asOpcode()==Opcodes::MkNew		||
				object->code(0).asOpcode()==Opcodes::MkRdx		||
				object->code(0).asOpcode()==Opcodes::MkSlnChg);
	}

	ObjectType	GetType(Code	*object){

		switch(object->code(0).getDescriptor()){
		case	Atom::INSTANTIATED_PROGRAM:
			if(object->references(0)->code(0).asOpcode()==Opcodes::PGM){

				if(object->references(0)->code(object->references(0)->code(PGM_INPUTS).asIndex()).getAtomCount()>0)
					return	IPGM;
				else
					return	ANTI_IPGM;
			}else
				return	ANTI_IPGM;
		case	Atom::OBJECT:
			return	OBJECT;
		case	Atom::MARKER:
			return	MARKER;
		case	Atom::GROUP:
			return	GROUP;
		}
	}
}