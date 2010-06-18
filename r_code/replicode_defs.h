//	replicode_defs.h
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2008, Eric Nivel
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

#ifndef	replicode_defs_h
#define	replicode_defs_h

#define	VIEW_CODE_MAX_SIZE	12	//	size of the code of the largest view (grp view).

#define	VIEW_OID		1
#define	VIEW_IJT		2
#define	VIEW_SLN		3
#define	VIEW_RES		4
#define	VIEW_HOST		5
#define	VIEW_ORG		6
#define	VIEW_ACT_VIS	7
#define	IPGM_VIEW_ACT	7
#define	GRP_VIEW_VIS	7
#define	GRP_VIEW_COV	8

#define	OBJECT_CLASS	0

#define	GRP_UPR				1
#define	GRP_SPR				2
#define	GRP_SLN_THR			3
#define	GRP_ACT_THR			4
#define	GRP_VIS_THR			5
#define	GRP_C_SLN			6
#define	GRP_C_SLN_THR		7
#define	GRP_C_ACT			8
#define	GRP_C_ACT_THR		9
#define	GRP_DCY_PER			10
#define	GRP_DCY_TGT			11
#define	GRP_DCY_PRD			12
#define	GRP_SLN_CHG_THR		13
#define	GRP_SLN_CHG_PRD		14
#define	GRP_ACT_CHG_THR		15
#define	GRP_ACT_CHG_PRD		16
#define	GRP_AVG_SLN			17
#define	GRP_HIGH_SLN		18
#define	GRP_LOW_SLN			19
#define	GRP_AVG_ACT			20
#define	GRP_HIGH_ACT		21
#define	GRP_LOW_ACT			22
#define	GRP_HIGH_SLN_THR	23
#define	GRP_LOW_SLN_THR		24
#define	GRP_SLN_NTF_PRD		25
#define	GRP_HIGH_ACT_THR	26
#define	GRP_LOW_ACT_THR		27
#define	GRP_ACT_NTF_PRD		28
#define	GRP_NTF_NEW			29
#define	GRP_LOW_RES_THR		30
#define	GRP_RES_NTF_PRD		31
#define	GRP_NTF_GRP			32

#define	IPGM_PGM	1
#define	IPGM_ARGS	2

#define	PGM_TPL_ARGS	1
#define	PGM_INPUTS		2
#define	PGM_PRODS		3
#define	PGM_TSC			4
#define	PGM_NFR			5

#define	IPGM_IGOL_ARITY	6
#define	MK_RDX_ARITY	7

#define	EXECUTIVE_DEVICE	0xA1000000

#define	CMD_DEVICE		1
#define	CMD_FUNCTION	2
#define	CMD_ARGS		3

#endif