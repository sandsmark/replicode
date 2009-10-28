#ifndef __R_CODE_H
#define __R_CODE_H
#include "types.h"
#include <vector>

namespace r_code{
class r_atom{
public:
   explicit r_atom(uint32 atom_) :atom(atom_) {}
   r_atom() :atom(UNDEFINED) {}
   uint32 atom;
   
   uint8 getDescriptor();
   bool  isPointer(); // returns true for all 5 pointer types.
   bool  isStructural();
   bool  isFloat();
   bool  readsAsNil(); // returns true for all undefined values 
                       // and nil.

   typedef enum{
      NIL=0x80,
      BOOLEAN_=0x81,
      WILDCARD=0x82,
      T_WILDCARD=0x83,
      I_PTR=0x84, // internal pointer
      V_PTR=0x85, // view pointer
      VL_PTR=0x86, // value pointer
      THIS=0x87, // this pointer
      ENTITY=0x88,
      SUB_SYSTEM=0x89,
      DEVICE=0x8A,
      DEVICE_FUNCTION=0x8B,
      SELF=0x8C,
      HOST=0x8D,
	  UNDEFINED=0x8F,
      C_PTR =0xC0, // chain pointer
      SET=0xC1,
      OBJECT=0xC2,
      MARKER=0xC3,
      OPERATOR=0xC4,
      STRING=0xC5,
      TIMESTAMP=0xC6
   }Type;

   // encoders
   static r_atom Undefined();
   static r_atom Float(float32 f,bool mediated=false);
   static r_atom UndefinedFloat();
   static r_atom Nil();
   static r_atom Boolean(bool value); 
   static r_atom UndefinedBoolean(); 
   static r_atom Wildcard(); 
   static r_atom TailWildcard(); 
   static r_atom IPointer(int16 index); 
   static r_atom VPointer(int16 index); 
   static r_atom VLPointer(int16 index); 
   static r_atom This(); 
   static r_atom Set(uint32 elementCount); 
   static r_atom CPointer(uint8 elementCount); 
   static r_atom Object(uint16 opcode,uint8 arity); 
   static r_atom Marker(uint16 opcode,uint8 arity); 
   static r_atom Operator(uint16 opcode,uint8 arity); 
   static r_atom Entity(); 
   static r_atom SubSystem(uint8 nodeID,uint16 sysID); 
   static r_atom UndefinedSubSystem(); 
   static r_atom Device(uint8 nodeID,uint8 classID,uint8 devID); 
   static r_atom UndefinedDevice(); 
   static r_atom DeviceFunction(uint16 opcode); 
   static r_atom UndefinedDeviceFunction(); 
   static r_atom String(uint16 characterCount); 
   static r_atom Timestamp(bool mediated=false); 
   static r_atom UndefinedTimestamp(); 
   static r_atom Self(); 
   static r_atom Host(); 
 
   // decoders 
   float32 asFloat(); 
   int16  asIndex(); // for internal, view, and value pointers 
   uint16 getAtomCount(); // arity of operators and  
                           // objects/markers, number of atoms in  
                           // pointers chains, number of blocks of 
                           // characters in strings. 
   uint16 asOpcode(); 
   uint8  getNodeID(); // devices and sub-systems 
   uint8  getClassID(); // devices 
   uint8  getDevID(); 
   uint16 getSysID(); 
}; 

inline bool operator==(const r_atom& x, const r_atom& y) { return x.atom == y.atom; }
inline bool operator!=(const r_atom& x, const r_atom& y) { return x.atom != y.atom; }

inline uint8 r_atom::getDescriptor() { return atom >> 24; }
inline bool r_atom::isFloat() { return (atom & 0x80000000) == 0; }
inline float32 r_atom::asFloat() { uint32 x = atom << 1; return *reinterpret_cast<float32*>(&x); }
inline r_atom r_atom::Float(float32 f, bool mediated) {
	uint32 x = *reinterpret_cast<uint32*>(&f);
	return r_atom(x >> 1);
}
inline bool r_atom::isStructural() { return (atom & 0xC0000000) == 0xC0000000; }
inline uint16 r_atom::getAtomCount()
{
	if (isStructural()) {
		if (getDescriptor() == SET)
			return atom & 0xFFFF;
		else if (getDescriptor() == OPERATOR)
			return atom & 0x7;
		else
			return atom & 0xFF;
	} else {
		return 0;
	}
} 

inline r_atom r_atom::Timestamp(bool mediated) {
	return r_atom((r_atom::TIMESTAMP << 24) + (int(mediated) << 8) + 2);
}

inline r_atom r_atom::IPointer(int16 index) {
	return r_atom((r_atom::I_PTR << 24) + index);
}

inline r_atom r_atom::VLPointer(int16 index) {
	return r_atom((r_atom::VL_PTR << 24) + index);
}

inline r_atom r_atom::Wildcard() { return r_atom(r_atom::WILDCARD << 24); }
inline r_atom r_atom::TailWildcard() { return r_atom(r_atom::T_WILDCARD << 24); }

inline r_atom r_atom::Undefined() { return r_atom(r_atom::UNDEFINED << 24); }
inline r_atom r_atom::Nil() { return r_atom(r_atom::NIL << 24); }
inline r_atom r_atom::Set(uint32 elementCount) { return r_atom((r_atom::SET << 24) + elementCount); }


inline uint16 r_atom::asOpcode() { return (atom >> 8) & 0xFFFF; }

inline int16 r_atom::asIndex() { return atom & 0xFFFF; }

inline r_atom r_atom::Boolean(bool value) { return r_atom((r_atom::BOOLEAN_ << 24) + value); }

inline bool r_atom::readsAsNil() { return atom == static_cast<uint32>((r_atom::NIL) << 24) || atom == static_cast<uint32>((r_atom::BOOLEAN_ << 24)); }
inline bool r_atom::isPointer() {
	switch(getDescriptor()) {
		case I_PTR:
		case V_PTR:
		case VL_PTR:
		case THIS:
		case C_PTR:
			return true;
		default:
			return false;
	}
}

}
#endif // __R_CODE_H
