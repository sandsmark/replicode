#include	"Object.h"
#include	"MemImpl.h"
#include	"opcodes.h"

namespace	r_exec{

	Object* Object::create(std::vector<r_code::Atom> atoms, std::vector<Object*> references)
	{
		if (atoms[0] == opcodeRegister["grp"]) {
			MemImpl::GroupImpl* obj = new MemImpl::GroupImpl(atoms, references);
			return obj;
		} else {
			MemImpl::ObjectImpl* obj = new MemImpl::ObjectImpl();
			for (int i = 0; i < references.size(); ++i)
				obj->references.push_back( reinterpret_cast<MemImpl::ObjectBase*>(references[i]));
			obj->atoms = atoms;
			
			// HACK HACK HACK -- correct the code
			for (int i = 0; i < atoms.size(); ++i) {
				if (obj->atoms[i].atom == 0xc3003806) {
					printf("converting atom %d from 0x3003806 to 0x4003806\n", i);
					obj->atoms[i].atom = 0xc4003806;
				}
				if (obj->atoms[i].getDescriptor() == r_code::Atom::C_PTR) {
					for (int j = 2; j <= obj->atoms[i].getAtomCount(); ++j) {
						if (obj->atoms[i+j].getDescriptor() == r_code::Atom::I_PTR) {
							printf("converting from I_PTR to INDEX\n");
							obj->atoms[i+j] = r_code::Atom::Index(obj->atoms[i+j].asIndex());
						}
					}
				}
			}

			int opcode = atoms[0].asOpcode();
			if (opcode == OPCODE_IPGM)
				obj->type = MemImpl::ObjectBase::REACTIVE;
			else
				obj->type = MemImpl::ObjectBase::OBJECT;
			return obj;
		}
	}
}