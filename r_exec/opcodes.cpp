#include	"opcodes.h"


namespace	r_exec{

	uint16	Opcodes::Group;

	uint16	Opcodes::PTN;
	uint16	Opcodes::AntiPTN;

	uint16	Opcodes::IPGM;
	uint16	Opcodes::PGM;
	uint16	Opcodes::AntiPGM;

	uint16	Opcodes::IGoal;
	uint16	Opcodes::Goal;
	uint16	Opcodes::AntiGoal;

	uint16	Opcodes::MkRdx;
	uint16	Opcodes::MkAntiRdx;

	uint16	Opcodes::MkNew;

	uint16	Opcodes::MkLowRes;
	uint16	Opcodes::MkLowSln;
	uint16	Opcodes::MkHighSln;
	uint16	Opcodes::MkLowAct;
	uint16	Opcodes::MkHighAct;
	uint16	Opcodes::MkSlnChg;
	uint16	Opcodes::MkActChg;

	uint16	Opcodes::Inject;
	uint16	Opcodes::Eject;
	uint16	Opcodes::Mod;
	uint16	Opcodes::Set;
	uint16	Opcodes::NewClass;
	uint16	Opcodes::DelClass;
	uint16	Opcodes::LDC;
	uint16	Opcodes::Swap;
	uint16	Opcodes::NewDev;
	uint16	Opcodes::DelDev;
	uint16	Opcodes::Suspend;
	uint16	Opcodes::Resume;
	uint16	Opcodes::Stop;
}