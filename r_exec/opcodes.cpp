//	opcodes.cpp
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

#include "opcodes.h"


namespace r_exec
{

uint16_t Opcodes::View;
uint16_t Opcodes::PgmView;
uint16_t Opcodes::GrpView;

uint16_t Opcodes::Ent;
uint16_t Opcodes::Ont;
uint16_t Opcodes::MkVal;

uint16_t Opcodes::Grp;

uint16_t Opcodes::Ptn;
uint16_t Opcodes::AntiPtn;

uint16_t Opcodes::IPgm;
uint16_t Opcodes::ICppPgm;

uint16_t Opcodes::Pgm;
uint16_t Opcodes::AntiPgm;

uint16_t Opcodes::ICmd;
uint16_t Opcodes::Cmd;

uint16_t Opcodes::Fact;
uint16_t Opcodes::AntiFact;

uint16_t Opcodes::Cst;
uint16_t Opcodes::Mdl;

uint16_t Opcodes::ICst;
uint16_t Opcodes::IMdl;

uint16_t Opcodes::Pred;
uint16_t Opcodes::Goal;

uint16_t Opcodes::Success;

uint16_t Opcodes::MkGrpPair;

uint16_t Opcodes::MkRdx;
uint16_t Opcodes::Perf;

uint16_t Opcodes::MkNew;

uint16_t Opcodes::MkLowRes;
uint16_t Opcodes::MkLowSln;
uint16_t Opcodes::MkHighSln;
uint16_t Opcodes::MkLowAct;
uint16_t Opcodes::MkHighAct;
uint16_t Opcodes::MkSlnChg;
uint16_t Opcodes::MkActChg;

uint16_t Opcodes::Inject;
uint16_t Opcodes::Eject;
uint16_t Opcodes::Mod;
uint16_t Opcodes::Set;
uint16_t Opcodes::NewClass;
uint16_t Opcodes::DelClass;
uint16_t Opcodes::LDC;
uint16_t Opcodes::Swap;
uint16_t Opcodes::Prb;
uint16_t Opcodes::Stop;

uint16_t Opcodes::Add;
uint16_t Opcodes::Sub;
uint16_t Opcodes::Mul;
uint16_t Opcodes::Div;
}