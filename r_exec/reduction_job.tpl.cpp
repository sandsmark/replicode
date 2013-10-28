#include "mem.h"

namespace	r_exec{

    template <class _P> bool ReductionJob<_P>::update(uint64	now){
        _Mem::Get()->register_reduction_job_latency(now-ijt);
        processor->reduce(input);
        return	true;
    }
    template<class	_P,class	T,class	C> bool BatchReductionJob<_P, T, C>::update(uint64	now){
        _Mem::Get()->register_reduction_job_latency(now-ijt);
        processor->reduce_batch(trigger,controller);
        return	true;
    }

}
