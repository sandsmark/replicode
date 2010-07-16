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

#ifndef	opcodes_h
#define	opcodes_h

#include	"types.h"
#include	"dll.h"


using	namespace	core;

namespace	r_exec{

	//	Opcodes are initialized by Init().
	class	r_exec_dll	Opcodes{
	public:
		static	uint16	View;
		static	uint16	ReactView;
		static	uint16	GroupView;

		static	uint16	Entity;

		static	uint16	Group;

		static	uint16	PTN;
		static	uint16	AntiPTN;

		static	uint16	IPGM;
		static	uint16	PGM;
		static	uint16	AntiPGM;

		static	uint16	IGoal;
		static	uint16	Goal;
		static	uint16	AntiGoal;

		static	uint16	Cmd;

		static	uint16	MkRdx;
		static	uint16	MkAntiRdx;

		static	uint16	MkNew;

		static	uint16	MkLowRes;
		static	uint16	MkLowSln;
		static	uint16	MkHighSln;
		static	uint16	MkLowAct;
		static	uint16	MkHighAct;
		static	uint16	MkSlnChg;
		static	uint16	MkActChg;

		static	uint16	Inject;
		static	uint16	Eject;
		static	uint16	Mod;
		static	uint16	Set;
		static	uint16	NewClass;
		static	uint16	DelClass;
		static	uint16	LDC;
		static	uint16	Swap;
		static	uint16	NewDev;
		static	uint16	DelDev;
		static	uint16	Suspend;
		static	uint16	Resume;
		static	uint16	Stop;
	};
}


#endif