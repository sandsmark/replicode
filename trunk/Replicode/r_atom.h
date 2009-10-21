#ifndef	r_atom_h
#define	r_atom_h

#include	"message.h"


using	namespace	mBrane;
using	namespace	mBrane::sdk;

namespace	replicode{

	class	dll_export	RAtom{
	public:
		typedef	enum{
			NIL=0x80,
			BOOLEAN_=0x81,
			WILDCARD=0x82,
			T_WILDCARD=0x83,
			I_PTR=0x84,		// internal pointer
			V_PTR=0x85,		// view pointer
			VL_PTR=0x86,	// value pointer
			B_PTR=0x87,		// back pointer
			THIS=0x88,		// this pointer
			ENTITY=0x89,
			SUB_SYSTEM=0x8A,
			DEVICE=0x8B,
			DEVICE_FUNCTION=0x8C,
			SELF=0x8D,
			HOST=0x8E,
			C_PTR=0xC0,		// chain pointer
			SET=0xC1,
			OBJECT=0xC2,
			MARKER=0xC3,
			OPERATOR=0xC4,
			STRING=0xC5,
			TIMESTAMP=0xC6,
			SET_OBJECT=0xC7
		}Type;
		// encoders
		static	RAtom	Float(float32	f);
		static	RAtom	UndefinedFloat();
		static	RAtom	Nil();
		static	RAtom	Boolean(bool	value);
		static	RAtom	UndefinedBoolean();
		static	RAtom	Wildcard();
		static	RAtom	TailWildcard();
		static	RAtom	IPointer(uint16	index);
		static	RAtom	VPointer(uint16	index);
		static	RAtom	VLPointer(uint16	index);
		static	RAtom	BPointer(uint16	index);
		static	RAtom	This();
		static	RAtom	Set(uint16	elementCount);
		static	RAtom	CPointer(uint8	elementCount);
		static	RAtom	Object(uint16	opcode,uint8	arity);
		static	RAtom	Marker(uint16	opcode,uint8	arity);
		static	RAtom	Operator(uint16	opcode,uint8	arity);
		static	RAtom	Entity();
		static	RAtom	SubSystem(uint8	nodeID,uint16	sysID);
		static	RAtom	UndefinedSubSystem();
		static	RAtom	Device(uint8	nodeID,uint8	classID,uint8	devID);
		static	RAtom	UndefinedDevice();
		static	RAtom	DeviceFunction(uint16	opcode);
		static	RAtom	UndefinedDeviceFunction();
		static	RAtom	String(uint16	characterCount);
		static	RAtom	UndefinedString();
		static	RAtom	Timestamp();
		static	RAtom	UndefinedTimestamp();
		static	RAtom	Self();
		static	RAtom	Host();
		static	RAtom	SetObject(uint16	opcode,uint8	arity);

		RAtom(uint32	a=0xFFFFFFFF);
		~RAtom();

		RAtom	&operator	=(const	RAtom&	a);
		bool	operator	==(const	RAtom&	a)	const;
		bool	operator	!=(const	RAtom&	a)	const;

		uint32	atom;

		// decoders
		uint8	getDescriptor()	const;
		bool	isPointer()	const;		// returns true for all 6 pointer types.
		bool	isStructural()	const;
		bool	isFloat()	const;
		bool	readsAsNil()	const;	// returns true for all undefined values and nil.
		float32	asFloat()	const;
		bool	asBoolean()	const;
		int16	asIndex()	const;		// for internal, view, and value pointers
		uint16	getAtomCount()	const;	// arity of operators and objects/markers, number of atoms in pointers chains, number of blocks of characters in strings.
		uint16	asOpcode()	const;
		uint8	getNodeID()	const;		// devices and sub-systems
		uint8	getClassID()	const;	// devices
		uint8	getDevID()	const;
		uint16	getSysID()	const;

		void	trace();
	};
}


#endif