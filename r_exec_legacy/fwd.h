/*
 *  fwd.h
 *  rsystem
 *
 *  Created by Nathaniel Thurston on 15/12/2009.
 *
 */

#ifndef __FWD_H
#define __FWD_H

namespace r_exec {

// forward declarations of interfaces
struct System;
class Mem;
struct Core;

// forward declarations of public RMem classes
class Group;
class Object;

// forward declarations of RCore classes
class ReductionInstance;
class ExecutionContext;
class Expression;

}

#endif // __FWD_H
