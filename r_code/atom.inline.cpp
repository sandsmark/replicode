#include <iostream>
#include "atom.h"

namespace r_code {

inline Atom Atom::Float(double f)
{
    uint64_t a = *reinterpret_cast<uint64_t *>(&f);
    return Atom(a >> 1);
}

inline Atom Atom::PlusInfinity()
{
    return Atom(0x7FF0000000000000);
}

inline Atom Atom::MinusInfinity()
{
    return Atom(0xFFF0000000000000);
}

inline Atom Atom::UndefinedFloat()
{
    return Atom(0xFFFFFFFFFFFFFFFF);
}

inline Atom Atom::Nil()
{
    return Atom(NIL << 48);
}

inline Atom Atom::Boolean(bool value)
{
    return Atom((BOOLEAN_ << 48) + value);
}

inline Atom Atom::UndefinedBoolean()
{
    return Atom(0x81FFFFFF);
}

inline Atom Atom::Wildcard(uint32_t opcode)
{
    return Atom((WILDCARD << 48) + ((opcode & 0x0FFFFFF) << 8));
}

inline Atom Atom::TailWildcard()
{
    return Atom(T_WILDCARD << 48);
}

inline Atom Atom::IPointer(uint32_t index)
{
    return Atom((I_PTR << 48) + (index & 0x0FFFFFFF));
}

inline Atom Atom::VLPointer(uint32_t index, uint32_t cast_opcode)
{
    return Atom((VL_PTR << 48) + ((cast_opcode & 0x0FFFFFFF) << 24) + (index & 0x0FFFFFFF));
}

inline Atom Atom::RPointer(uint32_t index)
{
    return Atom((R_PTR << 48) + (index & 0x0FFFFFFF));
}

inline Atom Atom::IPGMPointer(uint32_t index)
{
    return Atom((IPGM_PTR << 48) + (index & 0x0FFFFFFF));
}

inline Atom Atom::InObjPointer(uint8 inputIndex, uint32_t index)
{
    return Atom((IN_OBJ_PTR << 48) + (inputIndex << 24) + (index & 0x0FFFFFFF));
}

inline Atom Atom::DInObjPointer(uint8 relativeIndex, uint32_t index)
{
    return Atom((D_IN_OBJ_PTR << 48) + (relativeIndex << 24) + (index & 0x0FFFFFFF));
}

inline Atom Atom::OutObjPointer(uint32_t index)
{
    return Atom((OUT_OBJ_PTR << 48) + (index & 0x0FFFFFFF));
}

inline Atom Atom::ValuePointer(uint32_t index)
{
    return Atom((VALUE_PTR << 48) + (index & 0x0FFFFFFF));
}

inline Atom Atom::ProductionPointer(uint32_t index)
{
    return Atom((PROD_PTR << 48) + (index & 0x0FFFFFFF));
}

inline Atom Atom::AssignmentPointer(uint8 variable_index, uint32_t index)
{
    return Atom((ASSIGN_PTR << 48) + (variable_index << 16) + (index & 0x0FFFFFFF));
}

inline Atom Atom::This()
{
    return Atom(THIS << 48);
}

inline Atom Atom::View()
{
    return Atom(VIEW << 48);
}

inline Atom Atom::Mks()
{
    return Atom(MKS << 48);
}

inline Atom Atom::Vws()
{
    return Atom(VWS << 48);
}

inline Atom Atom::SSet(uint32_t opcode, uint8 elementCount)
{
    return Atom((S_SET << 48) + ((opcode & 0x0FFFFFFF) << 8) + elementCount);
}

inline Atom Atom::Set(uint8 elementCount)
{
    return Atom((SET << 48) + elementCount);
}

inline Atom Atom::CPointer(uint8 elementCount)
{
    return Atom((C_PTR << 48) + elementCount);
}


inline Atom Atom::Object(uint32_t opcode, uint8 arity)
{
    return Atom((OBJECT << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::Marker(uint32_t opcode, uint8 arity)
{
    return Atom((MARKER << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::Operator(uint32_t opcode, uint8 arity)
{
    return Atom((OPERATOR << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::Node(uint8 nodeID)
{
    return Atom((NODE << 48) + (nodeID << 8));
}

inline Atom Atom::UndefinedNode()
{
    return Atom(0xA0FFFFFFFFFF);
}

inline Atom Atom::Device(uint8 nodeID, uint8 classID, uint8 devID)
{
    return Atom((DEVICE << 48) + (nodeID << 16) + (classID << 8) + devID);
}

inline Atom Atom::UndefinedDevice()
{
    return Atom(0xA1FFFFFFFFFF);
}

inline Atom Atom::DeviceFunction(uint32_t opcode)
{
    return Atom((DEVICE_FUNCTION << 48) + (opcode << 8));
}

inline Atom Atom::UndefinedDeviceFunction()
{
    return Atom(0xA2FFFFFFFFFF);
}

inline Atom Atom::String(uint8 characterCount)
{
    uint8 blocks = characterCount / 4;
    if (characterCount % 4)
        ++blocks;
    return Atom((STRING << 48) + (blocks << 8) + characterCount);
}

inline Atom Atom::UndefinedString()
{
    return Atom(0xC6FFFFFFFFFFFFF);
}

inline Atom Atom::Timestamp()
{
    return Atom(TIMESTAMP << 48);
}

inline Atom Atom::UndefinedTimestamp()
{
    return Atom(0xC7FFFFFFFFFFFFFF);
}

inline Atom Atom::InstantiatedProgram(uint32_t opcode, uint8 arity)
{
    return Atom((INSTANTIATED_PROGRAM << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::Group(uint32_t opcode, uint8 arity)
{
    return Atom((GROUP << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::InstantiatedCPPProgram(uint32_t opcode, uint8 arity)
{
    return Atom((INSTANTIATED_CPP_PROGRAM << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::InstantiatedAntiProgram(uint32_t opcode, uint8 arity)
{
    return Atom((INSTANTIATED_ANTI_PROGRAM << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::InstantiatedInputLessProgram(uint32_t opcode, uint8 arity)
{
    return Atom((INSTANTIATED_INPUT_LESS_PROGRAM << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::CompositeState(uint32_t opcode, uint8 arity)
{
    return Atom((COMPOSITE_STATE << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::Model(uint32_t opcode, uint8 arity)
{
    return Atom((MODEL << 48) + ((opcode & 0x0FFFFFFF) << 8) + arity);
}

inline Atom Atom::NullProgram(bool take_past_inputs)
{
    return Atom((NULL_PROGRAM << 48) + (take_past_inputs ? 1 : 0));
}

inline Atom Atom::RawPointer(void *pointer)
{
    return Atom((uint64_t)pointer);
}

inline Atom::Atom(uint64_t a): atom(a)
{
}

inline Atom::~Atom()
{
}

inline Atom &Atom::operator =(const Atom& a)
{
    atom = a.atom;
    return *this;
}

inline bool Atom::operator ==(const Atom& a) const
{
    return atom == a.atom;
}

inline bool Atom::operator !=(const Atom& a) const
{
    return atom != a.atom;
}

inline bool Atom::operator !() const
{
    return isUndefined();
}

inline Atom::operator size_t () const
{
    return (size_t)atom;
}

inline bool Atom::isUndefined() const
{
    return atom == UINT64_MAX;
}

inline uint8 Atom::getDescriptor() const
{
    return atom >> 48;
}

inline bool Atom::isStructural() const
{
    return ((atom & 0xC000000000000000) == 0xC000000000000000 || (atom & 0xD000000000000000) == 0xD000000000000000);
}

inline bool Atom::isFloat() const
{
    return atom >> 63 == 0;
}

inline bool Atom::readsAsNil() const
{
    return atom == 0x8000000000000000 ||
           atom == 0x3FFFFFFFFFFFFFFF ||
           atom == 0x81FFFFFFFFFFFFFF ||
           atom == 0xC100000000000000 ||
           atom == 0xA0FFFFFFFFFFFFFF ||
           atom == 0xA1FFFFFFFFFFFFFF ||
           atom == 0xA2FFFFFFFFFFFFFF ||
           atom == 0xC6FFFFFFFFFFFFFF;
}

inline double Atom::asDouble() const
{
    uint64_t _f = atom << 1;
    return *reinterpret_cast<const double *>(&_f);
}

inline bool Atom::asBoolean() const
{
    return atom & 0x00000000000000FF;
}

inline uint32_t Atom::asIndex() const
{
    return atom & 0x000000000FFFFFFF;
}

inline uint8 Atom::asInputIndex() const
{
    return (uint8)((atom & 0x000FF0000000) >> 24);
}

inline uint8 Atom::asRelativeIndex() const
{
    return (uint8)((atom & 0x000FF0000000) >> 24);
}

inline uint32_t Atom::asOpcode() const
{
    return (atom >> 8) & 0x000000000FFFFFFF;
}

inline uint32_t Atom::asCastOpcode() const
{
    return (uint32_t)((atom & 0x00FFFFFFF000) >> 24);
}

inline uint8 Atom::getNodeID() const
{
    return (uint8)((atom & 0x00FF00000000) >> 16);
}

inline uint8 Atom::getClassID() const
{
    return (atom & 0x0000FF00) >> 8;
}

inline uint8 Atom::getDeviceID() const
{
    return atom & 0x00000000000000FF;
}

inline uint8 Atom::asAssignmentIndex() const
{
    return (atom & 0x00FF0000) >> 16;
}

inline uint8 Atom::getAtomCount() const
{
    switch (getDescriptor()) {
    case SET:
    case OBJECT:
    case MARKER:
    case C_PTR:
    case OPERATOR:
    case INSTANTIATED_PROGRAM:
    case INSTANTIATED_CPP_PROGRAM:
    case INSTANTIATED_INPUT_LESS_PROGRAM:
    case INSTANTIATED_ANTI_PROGRAM:
    case COMPOSITE_STATE:
    case MODEL:
    case GROUP:
    case S_SET: return atom & 0x0000000000FF;
    case STRING: return (atom & 0x00000000FF00) >> 8;
    default:
        return 0;
    }
}

inline bool Atom::takesPastInputs() const
{
    return atom & 0x0000000000000001;
}

}
