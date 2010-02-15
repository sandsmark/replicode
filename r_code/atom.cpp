#include	"atom.h"

#include	<iostream>


namespace	r_code{

	uint32	Atom::Members_to_go=0;
	uint8	Atom::Timestamp_data=0;
	uint8	Atom::String_data=0;

	inline	Atom	Atom::Float(float32	f){

		uint32	_f=*reinterpret_cast<uint32*>(&f);
		return	Atom(_f>>1);
	}

	inline	Atom	Atom::UndefinedFloat(){
		
		return	Atom(0x3FFFFFFF);
	}

	inline	Atom	Atom::Nil(){
		
		return	Atom(NIL<<24);
	}

	inline	Atom	Atom::Boolean(bool	value){
		
		return	Atom((BOOLEAN_<<24)+value);
	}

	inline	Atom	Atom::UndefinedBoolean(){
		
		return	Atom(0x81FFFFFF);
	}

	inline	Atom	Atom::Wildcard(){
		
		return	Atom(WILDCARD<<24);
	}

	inline	Atom	Atom::TailWildcard(){
		
		return	Atom(T_WILDCARD<<24);
	}

	inline	Atom	Atom::IPointer(uint16	index){
		
		return	Atom((I_PTR<<24)+index);
	}

	inline	Atom	Atom::VLPointer(uint16	index){
		
		return	Atom((VL_PTR<<24)+index);
	}

	inline	Atom	Atom::RPointer(uint16	index){
		
		return	Atom((R_PTR<<24)+index);
	}

	inline	Atom	Atom::This(){
		
		return	Atom(THIS<<24);
	}

	inline	Atom	Atom::View(){
		
		return	Atom(VIEW<<24);
	}

	inline	Atom	Atom::Mks(){
		
		return	Atom(MKS<<24);
	}

	inline	Atom	Atom::Vws(){
		
		return	Atom(VWS<<24);
	}

	inline	Atom	Atom::SSet(uint16 opcode,uint16	elementCount){
		
		return	Atom((S_SET<<24)+(opcode<<8)+elementCount);
	}

	inline	Atom	Atom::Set(uint16	elementCount){
		
		return	Atom((SET<<24)+elementCount);
	}

	inline	Atom	Atom::CPointer(uint8	elementCount){
		
		return	Atom((C_PTR<<24)+elementCount);
	}

	inline	Atom	Atom::Object(uint16	opcode,uint8	arity){
		
		return	Atom((OBJECT<<24)+(opcode<<8)+arity);
	}

	inline	Atom	Atom::Marker(uint16	opcode,uint8	arity){
		
		return	Atom((MARKER<<24)+(opcode<<8)+arity);
	}

	inline	Atom	Atom::Operator(uint16	opcode,uint8	arity){
		
		return	Atom((OPERATOR<<24)+(opcode<<8)+arity);
	}

	inline	Atom	Atom::Node(uint8	nodeID){
		
		return	Atom((NODE<<24)+(nodeID<<8));
	}

	inline	Atom	Atom::UndefinedNode(){
		
		return	Atom(0xA0FFFFFF);
	}

	inline	Atom	Atom::Device(uint8	nodeID,uint8	classID,uint8	devID){
		
		return	Atom((DEVICE<<24)+(nodeID<<16)+(classID<<8)+devID);
	}

	inline	Atom	Atom::UndefinedDevice(){
		
		return	Atom(0xA1FFFFFF);
	}

	inline	Atom	Atom::DeviceFunction(uint16	opcode){
		
		return	Atom((DEVICE_FUNCTION<<24)+(opcode<<8));
	}

	inline	Atom	Atom::UndefinedDeviceFunction(){
		
		return	Atom(0xA2FFFFFF);
	}

	inline	Atom	Atom::String(uint16	characterCount){
		
		uint8	blocks=characterCount/4;
		if(characterCount%4)
			++blocks;
		return	Atom((STRING<<24)+(blocks<<16)+characterCount);
	}

	inline	Atom	Atom::UndefinedString(){
		
		return	Atom(0xC6FFFFFF);
	}

	inline	Atom	Atom::Timestamp(){
		
		return	Atom(TIMESTAMP<<24);
	}

	inline	Atom	Atom::UndefinedTimestamp(){
		
		return	Atom(0xC7FFFFFF);
	}

	inline	Atom	Atom::Forever(){

		return	Atom(0xC7FFFFF0);
	}

	inline	Atom::Atom(uint32	a):atom(a){
	}

	inline	Atom::~Atom(){
	}

	inline	Atom	&Atom::operator	=(const	Atom&	a){
		
		atom=a.atom;
		return	*this;
	}

	inline	bool	Atom::operator	==(const	Atom&	a)	const{
		
		return	atom==a.atom;
	}

	inline	bool	Atom::operator	!=(const	Atom&	a)	const{
		
		return	atom!=a.atom;
	}


	inline	uint8	Atom::getDescriptor()	const{

		return	atom>>24;
	}

	inline	bool	Atom::isPointer()	const{

		switch(getDescriptor()){
		case I_PTR:
		case VL_PTR:
		case R_PTR:
		case THIS:
		case C_PTR:
			return true;
		default:
			return false;
		}
	}

	inline	bool	Atom::isStructural()	const{

		return	(atom	&	0xC0000000)==0xC0000000;
	}

	inline	bool	Atom::isFloat()	const{

		return	atom>>31==0;
	}

	inline	bool	Atom::readsAsNil()	const{

		return	atom==0x3FFFFFFF	||
				atom==0x81FFFFFF	||
				atom==0xC1000000	||
				atom==0xA0FFFFFF	||
				atom==0xA1FFFFFF	||
				atom==0xA2FFFFFF	||
				atom==0xC6FFFFFF	||
				atom==0xC7FFFFFF;
	}

	inline	float32	Atom::asFloat()	const{

		int32	a=atom<<1;
		return	*reinterpret_cast<float32	*>(&a);
	}

	inline	bool	Atom::asBoolean()	const{

		return	atom	&	0x000000FF;
	}

	inline	uint16	Atom::asIndex()	const{

		return	atom	&	0x0000FFFF;
	}

	inline	uint16	Atom::getAtomCount()	const{

		if(isStructural()){
		
			switch(getDescriptor()){
			case	SET:	return	atom	&	0x0000FFFF;
			case	OBJECT:
			case	MARKER:
			case	C_PTR:
			case	OPERATOR:
			case	S_SET:	return	atom	&	0x000000FF;
			case	STRING:	return	(atom	&	0x00FF0000)>>16;
			}
		}else
			return	0;
	}

	inline	uint16	Atom::asOpcode()	const{

		return	(atom>>8)	&	0x0000FFFF;
	}

	inline	uint8	Atom::getNodeID()	const{

		return	(atom	&	0x00FF0000)>>16;
	}

	inline	uint8	Atom::getClassID()	const{

		return	(atom	&	0x0000FF00)>>8;
	}

	inline	uint8	Atom::getDeviceID()	const{

		return	atom	&	0x000000FF;
	}

	void	Atom::trace()	const{

		write_indents();
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
		case	VL_PTR:				std::cout<<"vlptr: "<<std::dec<<asIndex();return;
		case	R_PTR:				std::cout<<"rptr: "<<std::dec<<asIndex();return;
		case	THIS:				std::cout<<"this";return;
		case	VIEW:				std::cout<<"view";return;
		case	MKS:				std::cout<<"mks";return;
		case	VWS:				std::cout<<"vws";return;
		case	NODE:				std::cout<<"nid: "<<std::dec<<(uint32)getNodeID();return;
		case	DEVICE:				std::cout<<"did: "<<std::dec<<(uint32)getNodeID()<<" "<<(uint32)getClassID()<<" "<<(uint32)getDeviceID();return;
		case	DEVICE_FUNCTION:	std::cout<<"fid: "<<std::dec<<asOpcode();return;
		case	C_PTR:				std::cout<<"cptr: "<<std::dec<<getAtomCount();Members_to_go=getAtomCount();return;
		case	SET:				std::cout<<"set: "<<std::dec<<getAtomCount();Members_to_go=getAtomCount();return;
		case	OBJECT:				std::cout<<"obj: "<<std::dec<<asOpcode()<<" "<<getAtomCount();Members_to_go=getAtomCount();return;
		case	S_SET:				std::cout<<"s_set: "<<std::dec<<asOpcode()<<" "<<getAtomCount();Members_to_go=getAtomCount();return;
		case	MARKER:				std::cout<<"mk: "<<std::dec<<asOpcode()<<" "<<getAtomCount();Members_to_go=getAtomCount();return;
		case	OPERATOR:			std::cout<<"op: "<<std::dec<<asOpcode()<<" "<<getAtomCount();Members_to_go=getAtomCount();return;
		case	STRING:				std::cout<<"st: "<<std::dec<<getAtomCount();Members_to_go=String_data=getAtomCount();return;
		case	TIMESTAMP:			std::cout<<"us";Members_to_go=Timestamp_data=2;return;
		default:
			if(Timestamp_data){
				
				--Timestamp_data;
				std::cout<<atom;
			}else	if(String_data){

				--String_data;
				std::cout<<(char)(atom>>24);
				std::cout<<(char)(atom>>16	&&	0x000000FF);
				std::cout<<(char)(atom>>8	&&	0x000000FF);
				std::cout<<(char)(atom>>24	&&	0x000000FF);
			}else
				std::cout<<"undef";
			return;
		}
	}

	void	Atom::write_indents()	const{

		if(Members_to_go){

			std::cout<<"   ";
			--Members_to_go;
		}
	}
}