#include	"mem.h"
#include	"MemImpl.h"
#include	"opcodes.h"

namespace	r_exec{

	Mem* Mem::create(
		int64 resilienceUpdatePeriod,
		int64 baseUpdatePeriod,
		UNORDERED_MAP<std::string, r_code::Atom> classes,
		std::vector<r_code::Object*> objects,
		ObjectReceiver *r
	) {
		opcodeRegister = classes;
		return new MemImpl::Impl(r, resilienceUpdatePeriod, baseUpdatePeriod, objects);
	}
}