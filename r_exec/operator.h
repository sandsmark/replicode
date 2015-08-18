//	operator.h
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

#ifndef operator_h
#define operator_h

#include "../r_code/object.h"

#include "_context.h"


namespace r_exec
{

// Wrapper class for evaluation contexts.
// Template operator functions is not an option since some operators are defined in usr_operators.dll.
class dll_export Context
{
private:
    _Context *implementation;
public:
    Context(_Context *implementation): implementation(implementation) {}
    virtual ~Context()
    {
        delete implementation;
    }

    _Context *get_implementation() const
    {
        return implementation;
    }

    uint16_t getChildrenCount() const
    {
        return implementation->getChildrenCount();
    }
    Context getChild(uint16_t index) const
    {
        return Context(implementation->_getChild(index));
    }

    Context operator *() const
    {
        return Context(implementation->dereference());
    }
    Context &operator =(const Context &c)
    {
        delete implementation;
        implementation = implementation->assign(c.get_implementation());
        return *this;
    }

    bool operator ==(const Context &c) const
    {
        return implementation->equal(c.get_implementation());
    }
    bool operator !=(const Context &c) const
    {
        return !implementation->equal(c.get_implementation());
    }

    Atom &operator [](uint16_t i) const
    {
        return implementation->get_atom(i);
    }

    uint16_t setAtomicResult(Atom a) const
    {
        return implementation->setAtomicResult(a);
    }
    uint16_t setTimestampResult(uint64_t t) const
    {
        return implementation->setTimestampResult(t);
    }
    uint16_t setCompoundResultHead(Atom a) const
    {
        return implementation->setCompoundResultHead(a);
    }
    uint16_t addCompoundResultPart(Atom a) const
    {
        return implementation->addCompoundResultPart(a);
    }

    void trace() const
    {
        return implementation->trace();
    }
};

bool red(const Context &context, uint16_t &index); // executive-dependent.

bool syn(const Context &context, uint16_t &index);

class Operator
{
private:
    static r_code::vector<Operator> Operators; // indexed by opcodes.

    bool (*_operator)(const Context &, uint16_t &);
    bool (*_overload)(const Context &, uint16_t &);
public:
    static void Register(uint16_t opcode, bool (*op)(const Context &, uint16_t &)); // first, register std operators; next register user-defined operators (may be registered as overloads).
    static Operator Get(uint16_t opcode)
    {
        return Operators[opcode];
    }
    Operator(): _operator(NULL), _overload(NULL) {}
    Operator(bool (*o)(const Context &, uint16_t &)): _operator(o), _overload(NULL) {}
    ~Operator() {}

    void setOverload(bool (*o)(const Context &, uint16_t &))
    {
        _overload = o;
    }

    bool operator()(const Context &context, uint16_t &index) const
    {
        if (_operator(context, index)) {
            return true;
        }

        if (_overload) {
            return _overload(context, index);
        }

        return false;
    }

    bool is_red() const
    {
        return _operator == red;
    }
    bool is_syn() const
    {
        return _operator == syn;
    }
};

// std operators ////////////////////////////////////////

bool now(const Context &context, uint16_t &index);

bool rnd(const Context &context, uint16_t &index);

bool equ(const Context &context, uint16_t &index);
bool neq(const Context &context, uint16_t &index);
bool gtr(const Context &context, uint16_t &index);
bool lsr(const Context &context, uint16_t &index);
bool gte(const Context &context, uint16_t &index);
bool lse(const Context &context, uint16_t &index);

bool add(const Context &context, uint16_t &index);
bool sub(const Context &context, uint16_t &index);
bool mul(const Context &context, uint16_t &index);
bool div(const Context &context, uint16_t &index);

bool dis(const Context &context, uint16_t &index);

bool ln(const Context &context, uint16_t &index);
bool exp(const Context &context, uint16_t &index);
bool log(const Context &context, uint16_t &index);
bool e10(const Context &context, uint16_t &index);

bool ins(const Context &context, uint16_t &index); // executive-dependent.

bool fvw(const Context &context, uint16_t &index); // executive-dependent.
}


#endif
