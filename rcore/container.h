#ifndef __CONTAINER_H
#define __CONTAINER_H
#include "r_code.h"
#ifdef MBRANE_CONTAINERS
#include "../mbrane/Core/payload_utils.h"
typedef mBrane::sdk::payloads::Array<r_code::r_atom, 16> AtomArray;

#else // MBRANE_CONTAINERS

#include "r_code.h"
#include <vector>
typedef std::vector<r_code::r_atom> AtomArray;

#endif // MBRANE_CONTAINERS
#endif
