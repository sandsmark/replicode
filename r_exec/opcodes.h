#ifndef	opcodes_h
#define	opcodes_h

#include	"types.h"
#include	"dll.h"


using	namespace	core;

namespace	r_exec{

	//	Opcodes are initialized by Init().
	class	r_exec_dll	Opcodes{
	public:
		static	uint16	Group;

		static	uint16	PTN;
		static	uint16	AntiPTN;

		static	uint16	IPGM;
		static	uint16	PGM;
		static	uint16	AntiPGM;

		static	uint16	IGoal;
		static	uint16	Goal;
		static	uint16	AntiGoal;

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