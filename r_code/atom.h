#ifndef	atom_h
#define	atom_h

#include	"types.h"


namespace	r_code{

	class	dll_export	Atom{
	private:	//	trace utilities
		static	uint32	Members_to_go;
		static	uint8	Timestamp_data;
		static	uint8	String_data;
		void	write_indents()	const;
	public:
		typedef	enum{
			NIL=0x80,
			BOOLEAN_=0x81,
			WILDCARD=0x82,
			T_WILDCARD=0x83,
			I_PTR=0x84,		// internal pointer
			R_PTR=0x85,		// reference pointer
			VL_PTR=0x86,	// value pointer
			INDEX=0x87,		// chain pointer index
			THIS=0x90,		// this pointer
			VIEW=0x91,
			MKS=0x92,
			VWS=0x93,
			NODE =0xA0,
			DEVICE=0xA1,
			DEVICE_FUNCTION=0xA2,
			C_PTR =0xC0, // chain pointer
			SET=0xC1,
			S_SET=0xC2, // structured set
			OBJECT=0xC3,
			MARKER=0xC4,
			OPERATOR=0xC5,
			STRING=0xC6,
			TIMESTAMP=0xC7
		}Type;
		// encoders
		static	Atom	Float(float32 f);
		static	Atom	UndefinedFloat();
		static	Atom	Nil();
		static	Atom	Boolean(bool value);
		static	Atom	UndefinedBoolean();
		static	Atom	Wildcard();
		static	Atom	TailWildcard();
		static	Atom	IPointer(uint16 index);
		static	Atom	VPointer(uint16 index);
		static	Atom	RPointer(uint16 index);
		static	Atom	VLPointer(uint16 index);
		static	Atom	This();
		static	Atom	View();
		static	Atom	Mks();
		static	Atom	Vws();
		static	Atom	Node(uint8 nodeID);
		static	Atom	UndefinedNode();
		static	Atom	Device(uint8 nodeID,uint8 classID,uint8 devID);
		static	Atom	UndefinedDevice();
		static	Atom	DeviceFunction(uint16 opcode);
		static	Atom	UndefinedDeviceFunction();
		static	Atom	CPointer(uint8 elementCount);
		static	Atom	SSet(uint16 opcode,uint16 elementCount);
		static	Atom	Set(uint16 elementCount);
		static	Atom	Object(uint16 opcode,uint8 arity);
		static	Atom	Marker(uint16 opcode,uint8 arity);
		static	Atom	Operator(uint16 opcode,uint8 arity);
		static	Atom	String(uint16 characterCount);
		static	Atom	UndefinedString();
		static	Atom	Timestamp();
		static	Atom	UndefinedTimestamp();
		static	Atom	Forever();

		Atom(uint32	a=0xFFFFFFFF);
		~Atom();

		Atom	&operator	=(const	Atom&	a);
		bool	operator	==(const	Atom&	a)	const;
		bool	operator	!=(const	Atom&	a)	const;

		uint32	atom;

		// decoders
		uint8	getDescriptor()	const;
		bool	isPointer()		const;	// returns true for all pointer types
										// incl. this and view
		bool	isStructural()	const;
		bool	isFloat()		const;
		bool	readsAsNil()	const;	// returns true for all undefined values
		float32	asFloat()		const;
		bool	asBoolean()		const;
		uint16	asIndex()		const;	// applicable to internal, view, reference,
										// and value pointers.
		uint16	asOpcode()		const;
		uint16	getAtomCount()	const;	// arity of operators and
									// objects/markers/structured sets,
									// number of atoms in pointers chains,
									// number of blocks of characters in
									// strings.
		uint16	getOpcode()		const;
		uint8	getNodeID()		const;	// applicable to nodes and devices.
		uint8	getClassID()	const;	// applicable to devices.
		uint8	getDeviceID()	const;	// applicable to devices.

		void	trace()	const;
	};
}


#endif
