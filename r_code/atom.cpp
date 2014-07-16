//	atom.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Eric Nivel nor the
//     names of their contributors may be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
//	THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
//	EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//	DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
//	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "atom.h"

#include <iostream>
#include <limits>


namespace r_code {

Atom Atom::Float(double f)
{
    uint64_t a = *reinterpret_cast<uint64_t *>(&f);
    return Atom(a >> 1);
}

Atom Atom::PlusInfinity()
{
    return Atom::Float(std::numeric_limits<double>::infinity());
}

Atom Atom::MinusInfinity()
{
    return Atom::Float(-std::numeric_limits<double>::infinity());
}

Atom Atom::UndefinedFloat()
{
    return Atom(0x0FFFFFFFFFFFFFFF);
}

Atom Atom::Nil()
{
    return Atom(NIL << 56);
}

Atom Atom::Boolean(bool value)
{
    return Atom((BOOLEAN_ << 56) + value);
}

Atom Atom::UndefinedBoolean()
{
    return Atom(0x81FFFFFF);
}

Atom Atom::Wildcard(uint32_t opcode)
{
    return Atom((WILDCARD << 56) + ((uint64_t)(opcode & 0x0FFFFFF) << 8));
}

Atom Atom::TailWildcard()
{
    return Atom(T_WILDCARD << 56);
}

Atom Atom::IPointer(uint32_t index)
{
    return Atom((I_PTR << 56) + (index & 0x0FFFFFFF));
}

Atom Atom::VLPointer(uint32_t index, uint32_t cast_opcode)
{
    return Atom((VL_PTR << 56) + ((uint64_t)(cast_opcode & 0x0FFFFFFF) << 28) + (index & 0x0FFFFFFF));
}

Atom Atom::RPointer(uint32_t index)
{
    return Atom((R_PTR << 56) + (index & 0x0FFFFFFF));
}

Atom Atom::IPGMPointer(uint32_t index)
{
    return Atom((IPGM_PTR << 56) + (index & 0x0FFFFFFF));
}

Atom Atom::InObjPointer(uint8_t inputIndex, uint32_t index)
{
    return Atom((IN_OBJ_PTR << 56) + ((uint64_t)inputIndex << 28) + (index & 0x0FFFFFFF));
}

Atom Atom::DInObjPointer(uint8_t relativeIndex, uint32_t index)
{
    return Atom((D_IN_OBJ_PTR << 56) + ((uint64_t)relativeIndex << 28) + (index & 0x0FFFFFFF));
}

Atom Atom::OutObjPointer(uint32_t index)
{
    return Atom((OUT_OBJ_PTR << 56) + (index & 0x0FFFFFFF));
}

Atom Atom::ValuePointer(uint32_t index)
{
    return Atom((VALUE_PTR << 56) + (index & 0x0FFFFFFF));
}

Atom Atom::ProductionPointer(uint32_t index)
{
    return Atom((PROD_PTR << 56) + (index & 0x0FFFFFFF));
}

Atom Atom::AssignmentPointer(uint8_t variable_index, uint32_t index)
{
    return Atom((ASSIGN_PTR << 56) + ((uint64_t)variable_index << 28) + (index & 0x0FFFFFFF));
}

Atom Atom::This()
{
    return Atom(THIS << 56);
}

Atom Atom::View()
{
    return Atom(VIEW << 56);
}

Atom Atom::Mks()
{
    return Atom(MKS << 56);
}

Atom Atom::Vws()
{
    return Atom(VWS << 56);
}

Atom Atom::SSet(uint32_t opcode, uint8_t elementCount)
{
    return Atom((S_SET << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + elementCount);
}

Atom Atom::Set(uint8_t elementCount)
{
    return Atom((SET << 56) + elementCount);
}

Atom Atom::CPointer(uint8_t elementCount)
{
    return Atom((C_PTR << 56) + elementCount);
}


Atom Atom::Object(uint32_t opcode, uint8_t arity)
{
    return Atom((OBJECT << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::Marker(uint32_t opcode, uint8_t arity)
{
    return Atom((MARKER << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::Operator(uint32_t opcode, uint8_t arity)
{
    return Atom((OPERATOR << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::Node(uint8_t nodeID)
{
    return Atom((NODE << 56) + ((uint64_t)nodeID << 8));
}

Atom Atom::UndefinedNode()
{
    return Atom(0xA0FFFFFFFFFFFFFF);
}

Atom Atom::Device(uint8_t nodeID, uint8_t classID, uint8_t devID)
{
    return Atom((DEVICE << 56) + ((uint64_t)nodeID << 16) + ((uint64_t)classID << 8) + devID);
}

Atom Atom::UndefinedDevice()
{
    return Atom(0xA1FFFFFFFFFFFFFF);
}

Atom Atom::DeviceFunction(uint32_t opcode)
{
    return Atom((DEVICE_FUNCTION << 56) + (opcode << 8));
}

Atom Atom::UndefinedDeviceFunction()
{
    return Atom(0xA2FFFFFFFFFFFFFF);
}

Atom Atom::String(uint8_t characterCount)
{
    uint8_t blocks = characterCount / 4;
    if (characterCount % 4)
        ++blocks;
    return Atom((STRING << 56) + ((uint64_t)blocks << 8) + characterCount);
}

Atom Atom::UndefinedString()
{
    return Atom(0xC6FFFFFFFFFFFFFF);
}

Atom Atom::Timestamp()
{
    return Atom(TIMESTAMP << 56);
}

Atom Atom::UndefinedTimestamp()
{
    return Atom(0xC7FFFFFFFFFFFFFF);
}

Atom Atom::InstantiatedProgram(uint32_t opcode, uint8_t arity)
{
    return Atom((INSTANTIATED_PROGRAM << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::Group(uint32_t opcode, uint8_t arity)
{
    return Atom((GROUP << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::InstantiatedCPPProgram(uint32_t opcode, uint8_t arity)
{
    return Atom((INSTANTIATED_CPP_PROGRAM << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::InstantiatedAntiProgram(uint32_t opcode, uint8_t arity)
{
    return Atom((INSTANTIATED_ANTI_PROGRAM << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::InstantiatedInputLessProgram(uint32_t opcode, uint8_t arity)
{
    return Atom((INSTANTIATED_INPUT_LESS_PROGRAM << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::CompositeState(uint32_t opcode, uint8_t arity)
{
    return Atom((COMPOSITE_STATE << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::Model(uint32_t opcode, uint8_t arity)
{
    return Atom((MODEL << 56) + ((uint64_t)(opcode & 0x0FFFFFFF) << 8) + arity);
}

Atom Atom::NullProgram(bool take_past_inputs)
{
    return Atom((NULL_PROGRAM << 56) + (take_past_inputs ? 1 : 0));
}

Atom Atom::RawPointer(void *pointer)
{
    return Atom((uint64_t)pointer);
}

Atom::Atom(uint64_t a): atom(a)
{
}

Atom::~Atom()
{
}

Atom &Atom::operator =(const Atom& a)
{
    atom = a.atom;
    return *this;
}

bool Atom::operator ==(const Atom& a) const
{
    return atom == a.atom;
}

bool Atom::operator !=(const Atom& a) const
{
    return atom != a.atom;
}

bool Atom::operator !() const
{
    return isUndefined();
}

Atom::operator size_t () const
{
    return (size_t)atom;
}

bool Atom::isUndefined() const
{
    return atom == UINT64_MAX;
}

uint8_t Atom::getDescriptor() const
{
    return atom >> 56;
}

bool Atom::isStructural() const
{
    return ((atom & 0xC000000000000000) == 0xC000000000000000 || (atom & 0xD000000000000000) == 0xD000000000000000);
}

bool Atom::isFloat() const
{
    return atom >> 63 == 0;
}

bool Atom::readsAsNil() const
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

double Atom::asDouble() const
{
    uint64_t _f = atom << 1;
    return *reinterpret_cast<const double *>(&_f);
}

bool Atom::asBoolean() const
{
    return atom & 0x00000000000000FF;
}

uint32_t Atom::asIndex() const
{
    return atom & 0x000000000FFFFFFF;
}

uint8_t Atom::asInputIndex() const
{
    return (uint8_t)((atom & 0x000FF0000000) >> 28);
}

uint8_t Atom::asRelativeIndex() const
{
    return (uint8_t)((atom & 0x000FF0000000) >> 28);
}

uint32_t Atom::asOpcode() const
{
    return (atom >> 8) & 0x000000000FFFFFFF;
}

uint32_t Atom::asCastOpcode() const
{
    return (uint32_t)((atom & 0x00FFFFFFF000) >> 28);
}

uint8_t Atom::getNodeID() const
{
    return (uint8_t)((atom & 0x0000000000FF0000) >> 16);
}

uint8_t Atom::getClassID() const
{
    return (atom & 0x0000FF00) >> 8;
}

uint8_t Atom::getDeviceID() const
{
    return atom & 0x00000000000000FF;
}

uint8_t Atom::asAssignmentIndex() const
{
    return (atom & 0x00FFF0000000) >> 28;
}

uint8_t Atom::getAtomCount() const
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

bool Atom::takesPastInputs() const
{
    return atom & 0x0000000000000001;
}

/*
 * The rest is for debugging
 */

uint8_t Atom::Members_to_go = 0;
uint8_t Atom::Timestamp_data = 0;
uint8_t Atom::String_data = 0;
uint8_t Atom::Char_count = 0;
void Atom::trace() const {

    write_indents();
    switch (getDescriptor()) {
    case NIL: std::cout << "nil"; return;
    case BOOLEAN_: std::cout << "bl: " << std::boolalpha << asBoolean(); return;
    case WILDCARD: std::cout << ":"; return;
    case T_WILDCARD: std::cout << "::"; return;
    case I_PTR: std::cout << "iptr: " << std::dec << asIndex(); return;
    case VL_PTR: std::cout << "vlptr: " << std::dec << asIndex(); return;
    case R_PTR: std::cout << "rptr: " << std::dec << asIndex(); return;
    case IPGM_PTR: std::cout << "ipgm_ptr: " << std::dec << asIndex(); return;
    case IN_OBJ_PTR: std::cout << "in_obj_ptr: " << std::dec << (uint64_t)asInputIndex() << " " << asIndex(); return;
    case D_IN_OBJ_PTR: std::cout << "d_in_obj_ptr: " << std::dec << (uint64_t)asRelativeIndex() << " " << asIndex(); return;
    case OUT_OBJ_PTR: std::cout << "out_obj_ptr: " << std::dec << asIndex(); return;
    case VALUE_PTR: std::cout << "value_ptr: " << std::dec << asIndex(); return;
    case PROD_PTR: std::cout << "prod_ptr: " << std::dec << asIndex(); return;
    case ASSIGN_PTR: std::cout << "assign_ptr: " << std::dec << (uint16_t)asAssignmentIndex() << " " << asIndex(); return;
    case THIS: std::cout << "this"; return;
    case VIEW: std::cout << "view"; return;
    case MKS: std::cout << "mks"; return;
    case VWS: std::cout << "vws"; return;
    case NODE: std::cout << "nid: " << std::dec << (uint16_t)getNodeID(); return;
    case DEVICE: std::cout << "did: " << std::dec << (uint16_t)getNodeID() << " " << (uint16_t)getClassID() << " " << (uint16_t)getDeviceID(); return;
    case DEVICE_FUNCTION: std::cout << "fid: " << std::dec << asOpcode(); return;
    case C_PTR: std::cout << "cptr: " << std::dec << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case SET: std::cout << "set: " << std::dec << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case OBJECT: std::cout << "obj: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case S_SET: std::cout << "s_set: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case MARKER: std::cout << "mk: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case OPERATOR: std::cout << "op: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case STRING: std::cout << "st: " << std::dec << (uint16_t)getAtomCount(); Members_to_go = String_data = getAtomCount(); Char_count = (atom & 0x000000FF); return;
    case TIMESTAMP: std::cout << "us"; Members_to_go = Timestamp_data = 1; return;
    case GROUP: std::cout << "grp: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case INSTANTIATED_PROGRAM:
    case INSTANTIATED_ANTI_PROGRAM:
    case INSTANTIATED_INPUT_LESS_PROGRAM:
        std::cout << "ipgm: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case COMPOSITE_STATE: std::cout << "cst: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case MODEL: std::cout << "mdl: " << std::dec << asOpcode() << " " << (uint16_t)getAtomCount(); Members_to_go = getAtomCount(); return;
    case NULL_PROGRAM: std::cout << "null pgm " << (takesPastInputs() ? "all inputs" : "new inputs"); return;
    default:
        if (Timestamp_data) {

            --Timestamp_data;
            std::cout << atom;
        } else if (String_data) {

            --String_data;
            std::string s;
            char *content = (char *)&atom;
            for (uint8_t i = 0; i < 4; ++i) {

                if (Char_count-- > 0)
                    s += content[i];
                else
                    break;
            }
            std::cout << s.c_str();
        } else if (isFloat()) {
            std::cout << "nb: " << std::scientific << asDouble();
            return;
        } else
            std::cout << "undef";
        return;
    }
}

void Atom::write_indents() const {

    if (Members_to_go) {

        std::cout << " ";
        --Members_to_go;
    }
}

void Atom::Trace(Atom *base, uint16_t count) {

    std::cout << "--------\n";
    for (uint16_t i = 0; i < count; ++i) {

        std::cout << i << "\t";
        base[i].trace();
        std::cout << std::endl;
    }
}
}
