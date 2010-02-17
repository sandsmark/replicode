#ifndef __MATCH_H
#define __MATCH_H
#include "ExecutionContext.h"
#include "Expression.h"

namespace r_exec {

bool match(Expression input, ExecutionContext pattern);

}
#endif
