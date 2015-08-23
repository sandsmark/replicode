//	opcodes.h
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

#ifndef opcodes_h
#define opcodes_h

#include "CoreLibrary/dll.h"

#include <cstdint>

namespace r_exec
{

// Opcodes are initialized by Init().
class REPLICODE_EXPORT Opcodes
{
public:
    static uint16_t View;
    static uint16_t PgmView;
    static uint16_t GrpView;

    static uint16_t Ent;
    static uint16_t Ont;
    static uint16_t MkVal;

    static uint16_t Grp;

    static uint16_t Ptn;
    static uint16_t AntiPtn;

    static uint16_t IPgm;
    static uint16_t ICppPgm;

    static uint16_t Pgm;
    static uint16_t AntiPgm;

    static uint16_t ICmd;
    static uint16_t Cmd;

    static uint16_t Fact;
    static uint16_t AntiFact;

    static uint16_t Mdl;
    static uint16_t Cst;

    static uint16_t ICst;
    static uint16_t IMdl;

    static uint16_t Pred;
    static uint16_t Goal;

    static uint16_t Success;

    static uint16_t MkGrpPair;

    static uint16_t MkRdx;
    static uint16_t Perf;

    static uint16_t MkNew;

    static uint16_t MkLowRes;
    static uint16_t MkLowSln;
    static uint16_t MkHighSln;
    static uint16_t MkLowAct;
    static uint16_t MkHighAct;
    static uint16_t MkSlnChg;
    static uint16_t MkActChg;

    static uint16_t Inject;
    static uint16_t Eject;
    static uint16_t Mod;
    static uint16_t Set;
    static uint16_t NewClass;
    static uint16_t DelClass;
    static uint16_t LDC;
    static uint16_t Swap;
    static uint16_t Prb;
    static uint16_t Stop;

    static uint16_t Add;
    static uint16_t Sub;
    static uint16_t Mul;
    static uint16_t Div;
};
}


#endif
