#include "hash_containers"
#include <string>
#include "../r_code/atom.h"

namespace r_exec {

extern UNORDERED_MAP<std::string, r_code::Atom> opcodeRegister;

#define OPCODE_NOW opcodeRegister["now"].asOpcode()
#define OPCODE_EQU opcodeRegister["equ"].asOpcode()
#define OPCODE_NEQ opcodeRegister["neq"].asOpcode()
#define OPCODE_GTR opcodeRegister["gtr"].asOpcode()
#define OPCODE_LSR opcodeRegister["lsr"].asOpcode()
#define OPCODE_GTE opcodeRegister["gte"].asOpcode()
#define OPCODE_LSE opcodeRegister["lse"].asOpcode()
#define OPCODE_ADD opcodeRegister["add"].asOpcode()
#define OPCODE_SUB opcodeRegister["sub"].asOpcode()
#define OPCODE_MUL opcodeRegister["mul"].asOpcode()
#define OPCODE_DIV opcodeRegister["div"].asOpcode()
#define OPCODE_SYN opcodeRegister["syn"].asOpcode()
#define OPCODE_RED opcodeRegister["red"].asOpcode()
#define OPCODE_NOTRED opcodeRegister["|red"].asOpcode()
#define MARKER_REDUCTION opcodeRegister["mk.rdx"].asOpcode()
#define MARKER_ANTIREDUCTION opcodeRegister["mk.|rdx"].asOpcode()
#define MARKER_SALIENCY_CHANGE opcodeRegister["mk.sln_chg"].asOpcode()
#define MARKER_SALIENCY_LOW_VALUE opcodeRegister["mk.low_sln"].asOpcode()
#define MARKER_SALIENCY_HIGH_VALUE opcodeRegister["mk.high_sln"].asOpcode()
#define MARKER_RESILIENCE_LOW_VALUE opcodeRegister["mk.low_res"].asOpcode()
#define MARKER_RESILIENCE_HIGH_VALUE opcodeRegister["mk.high_res"].asOpcode()
#define MARKER_ACTIVATION_CHANGE opcodeRegister["mk.act_chg"].asOpcode()
#define MARKER_ACTIVATION_LOW_VALUE opcodeRegister["mk.low_res"].asOpcode()
#define MARKER_ACTIVATION_HIGH_VALUE opcodeRegister["mk.high_res"].asOpcode()
#define OPCODE_GROUP opcodeRegister["grp"].asOpcode()
#define ARITY_GROUP opcodeRegister["grp"].getAtomCount()
#define OPCODE_PROGRAM opcodeRegister["pgm"].asOpcode()
#define OPCODE_ANGIPROGRAM opcodeRegister["|pgm"].asOpcode()
#define OPCODE_IPGM opcodeRegister["ipgm"].asOpcode()

}
