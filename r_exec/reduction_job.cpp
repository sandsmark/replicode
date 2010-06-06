#include	"reduction_job.h"


namespace	r_exec{

	ReductionJob::ReductionJob():input(NULL),overlay(NULL),deadline(0){
	}

	ReductionJob::ReductionJob(View	*input,Overlay	*overlay,uint64	deadline):input(input),overlay(overlay),deadline(deadline){
	}
}