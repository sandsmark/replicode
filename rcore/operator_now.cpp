#include "ExecutionContext.h"

using namespace r_code;

void operator_now(ExecutionContext& context)
{
	context.setResultTimestamp( context.now() );
}
