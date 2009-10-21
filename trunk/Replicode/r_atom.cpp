#include	"r_atom.h"


namespace	replicode{

	RAtom	RAtom::Float(float32	f){

		uint32	_f=*reinterpret_cast<uint32*>(&f);
		return	RAtom(_f>>1);
	}

	RAtom	RAtom::UndefinedFloat(){
		
		return	RAtom(0x3FFFFFFF);
	}

	RAtom	RAtom::Nil(){
		
		return	RAtom(NIL<<24);
	}

	RAtom	RAtom::Boolean(bool	value){
		
		return	RAtom((BOOLEAN_<<24)+value);
	}

	RAtom	RAtom::UndefinedBoolean(){
		
		return	RAtom(0x81FFFFFF);
	}

	RAtom	RAtom::Wildcard(){
		
		return	RAtom(WILDCARD<<24);
	}

	RAtom	RAtom::TailWildcard(){
		
		return	RAtom(T_WILDCARD<<24);
	}

	RAtom	RAtom::IPointer(uint16	index){
		
		return	RAtom((I_PTR<<24)+index);
	}

	RAtom	RAtom::VPointer(uint16	index){
		
		return	RAtom((V_PTR<<24)+index);
	}

	RAtom	RAtom::VLPointer(uint16	index){
		
		return	RAtom((VL_PTR<<24)+index);
	}

	RAtom	RAtom::BPointer(uint16	index){
		
		return	RAtom((B_PTR<<24)+index);
	}

	RAtom	RAtom::This(){
		
		return	RAtom(THIS<<24);
	}

	RAtom	RAtom::Set(uint16	elementCount){
		
		return	RAtom((SET<<24)+elementCount);
	}

	RAtom	RAtom::CPointer(uint8	elementCount){
		
		return	RAtom((C_PTR<<24)+elementCount);
	}

	RAtom	RAtom::Object(uint16	opcode,uint8	arity){
		
		return	RAtom((OBJECT<<24)+(opcode<<8)+arity);
	}

	RAtom	RAtom::Marker(uint16	opcode,uint8	arity){
		
		return	RAtom((MARKER<<24)+(opcode<<8)+arity);
	}

	RAtom	RAtom::Operator(uint16	opcode,uint8	arity){
		
		return	RAtom((OPERATOR<<24)+(opcode<<8)+arity);
	}

	RAtom	RAtom::Entity(){
		
		return	RAtom(ENTITY<<24);
	}

	RAtom	RAtom::SubSystem(uint8	nodeID,uint16	sysID){
		
		return	RAtom((SUB_SYSTEM<<24)+(nodeID<<16)+sysID);
	}

	RAtom	RAtom::UndefinedSubSystem(){
		
		return	RAtom(0x8AFFFFFF);
	}

	RAtom	RAtom::Device(uint8	nodeID,uint8	classID,uint8	devID){
		
		return	RAtom((DEVICE<<24)+(nodeID<<16)+(classID<<8)+devID);
	}

	RAtom	RAtom::UndefinedDevice(){
		
		return	RAtom(0x8BFFFFFF);
	}

	RAtom	RAtom::DeviceFunction(uint16	opcode){
		
		return	RAtom((DEVICE_FUNCTION<<24)+(opcode<<8));
	}

	RAtom	RAtom::UndefinedDeviceFunction(){
		
		return	RAtom(0x8CFFFFFF);
	}

	RAtom	RAtom::String(uint16	characterCount){
		
		uint8	blocks=characterCount/4;
		if(characterCount%4)
			++blocks;
		return	RAtom((STRING<<24)+(blocks<<16)+characterCount);
	}

	RAtom	RAtom::UndefinedString(){
		
		return	RAtom(0xC5FFFFFF);
	}

	RAtom	RAtom::Timestamp(){
		
		return	RAtom(TIMESTAMP<<24);
	}

	RAtom	RAtom::UndefinedTimestamp(){
		
		return	RAtom(0xC6FFFFFF);
	}

	RAtom	RAtom::Self(){
		
		return	SELF<<24;
	}

	RAtom	RAtom::Host(){

		return	HOST<<24;
	}

	RAtom	RAtom::SetObject(uint16	opcode,uint8	arity){
		
		return	RAtom((SET_OBJECT<<24)+(opcode<<8)+arity);
	}

	RAtom::RAtom(uint32	a):atom(a){
	}

	RAtom::~RAtom(){
	}

	RAtom	&RAtom::operator	=(const	RAtom&	a){
		
		atom=a.atom;
		return	*this;
	}

	bool	RAtom::operator	==(const	RAtom&	a)	const{
		
		return	atom==a.atom;
	}

	bool	RAtom::operator	!=(const	RAtom&	a)	const{
		
		return	atom!=a.atom;
	}


	inline	uint8	RAtom::getDescriptor()	const{

		return	atom>>24;
	}

	inline	bool	RAtom::isPointer()	const{

		switch(getDescriptor()){
		case I_PTR:
		case V_PTR:
		case VL_PTR:
		case B_PTR:
		case THIS:
		case C_PTR:
			return true;
		default:
			return false;
		}
	}

	inline	bool	RAtom::isStructural()	const{

		return	(atom	&	0xC0000000)==0xC0000000;
	}

	inline	bool	RAtom::isFloat()	const{

		return	atom>>31==0;
	}

	bool	RAtom::readsAsNil()	const{

		return	atom==0xC6FFFFFF	||
				atom==0x8CFFFFFF	||
				atom==0x8BFFFFFF	||
				atom==0x8AFFFFFF	||
				atom==0x81FFFFFFFF	||
				atom==0x3FFFFFFF	||
				atom==0xC5FFFFFF	||
				atom==0xC1000000	||
				atom==0xC7000000;
	}

	inline	float32	RAtom::asFloat()	const{

		int32	a=atom<<1;
		return	*reinterpret_cast<float32	*>(&a);
	}

	inline	bool	RAtom::asBoolean()	const{

		return	atom	&	0x000000FF;
	}

	inline	int16	RAtom::asIndex()	const{

		return	atom	&	0x0000FFFF;
	}

	inline	uint16	RAtom::getAtomCount()	const{

		if(isStructural()){
		
			switch(getDescriptor()){
			case	SET:		return	atom	&	0x0000FFFF;
			case	OBJECT:
			case	MARKER:
			case	C_PTR:
			case	OPERATOR:
			case	SET_OBJECT:	return	atom	&	0x000000FF;
			case	STRING:		return	atom	&	0x00FF0000;
			}
		}else
			return	0;
	}

	inline	uint16	RAtom::asOpcode()	const{

		return	(atom>>8)	&	0x0000FFFF;
	}

	inline	uint8	RAtom::getNodeID()	const{

		return	(atom	&	0x00FF0000)>>16;
	}

	inline	uint8	RAtom::getClassID()	const{

		return	(atom	&	0x0000FF00)>>8;
	}

	inline	uint8	RAtom::getDevID()	const{

		return	atom	&	0x000000FF;
	}

	inline	uint16	RAtom::getSysID()	const{

		return	atom	&	0x0000FFFF;
	}

	void	RAtom::trace(){

		if(isFloat()){

			std::cout<<"nb: "<<std::scientific<<asFloat();
			return;
		}
		switch(getDescriptor()){
		case	NIL:				std::cout<<"nil";return;
		case	BOOLEAN_:			std::cout<<"bl: "<<std::boolalpha<<asBoolean();return;
		case	WILDCARD:			std::cout<<":";return;
		case	T_WILDCARD:			std::cout<<"::";return;
		case	I_PTR:				std::cout<<"iptr: "<<std::dec<<asIndex();return;
		case	V_PTR:				std::cout<<"vptr: "<<std::dec<<asIndex();return;
		case	VL_PTR:				std::cout<<"vlptr: "<<std::dec<<asIndex();return;
		case	B_PTR:				std::cout<<"bptr: "<<std::dec<<asIndex();return;
		case	THIS:				std::cout<<"this";return;
		case	ENTITY:				std::cout<<"ent";return;
		case	SUB_SYSTEM:			std::cout<<"sid: "<<std::dec<<(uint32)getNodeID()<<" "<<(uint32)getSysID();return;
		case	DEVICE:				std::cout<<"did: "<<std::dec<<(uint32)getNodeID()<<" "<<(uint32)getClassID()<<" "<<(uint32)getDevID();return;
		case	DEVICE_FUNCTION:	std::cout<<"fid: "<<std::dec<<asOpcode();return;
		case	SELF:				std::cout<<"self";return;
		case	HOST:				std::cout<<"host";return;
		case	C_PTR:				std::cout<<"cptr: "<<std::dec<<getAtomCount();return;
		case	SET:				std::cout<<"set: "<<std::dec<<getAtomCount();return;
		case	OBJECT:				std::cout<<"obj: "<<std::dec<<asOpcode()<<" "<<getAtomCount();return;
		case	SET_OBJECT:			std::cout<<"set_obj: "<<std::dec<<asOpcode()<<" "<<getAtomCount();return;
		case	MARKER:				std::cout<<"mk: "<<std::dec<<asOpcode()<<" "<<getAtomCount();return;
		case	OPERATOR:			std::cout<<"op: "<<std::dec<<asOpcode()<<" "<<getAtomCount();return;
		case	STRING:				std::cout<<"st: "<<std::dec<<((atom	&	0x00FF0000)>>16)<<" "<<getAtomCount();return;
		case	TIMESTAMP:			std::cout<<"ms";return;
		default:	std::cout<<"undef";return;
		}
	}
}
