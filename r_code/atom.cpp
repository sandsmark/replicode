#include	"atom.h"

#include	<iostream>


namespace	r_code{

	uint32	Atom::Members_to_go=0;
	uint8	Atom::Timestamp_data=0;
	uint8	Atom::String_data=0;

		Atom	Atom::Float(float32	f){

		uint32	_f=*reinterpret_cast<uint32*>(&f);
		return	Atom(_f>>1);
	}

		Atom	Atom::UndefinedFloat(){
		
		return	Atom(0x3FFFFFFF);
	}

		Atom	Atom::Nil(){
		
		return	Atom(NIL<<24);
	}

		Atom	Atom::Boolean(bool	value){
		
		return	Atom((BOOLEAN_<<24)+value);
	}

		Atom	Atom::UndefinedBoolean(){
		
		return	Atom(0x81FFFFFF);
	}

		Atom	Atom::Wildcard(){
		
		return	Atom(WILDCARD<<24);
	}

		Atom	Atom::TailWildcard(){
		
		return	Atom(T_WILDCARD<<24);
	}

		Atom	Atom::IPointer(uint16	index){
		
		return	Atom((I_PTR<<24)+index);
	}

		Atom	Atom::VLPointer(uint16	index){
		
		return	Atom((VL_PTR<<24)+index);
	}

		Atom	Atom::RPointer(uint16	index){
		
		return	Atom((R_PTR<<24)+index);
	}

		Atom	Atom::This(){
		
		return	Atom(THIS<<24);
	}

		Atom	Atom::View(){
		
		return	Atom(VIEW<<24);
	}

		Atom	Atom::Mks(){
		
		return	Atom(MKS<<24);
	}

		Atom	Atom::Vws(){
		
		return	Atom(VWS<<24);
	}

		Atom	Atom::SSet(uint16 opcode,uint16	elementCount){
		
		return	Atom((S_SET<<24)+(opcode<<8)+elementCount);
	}

		Atom	Atom::Set(uint16	elementCount){
		
		return	Atom((SET<<24)+elementCount);
	}

		Atom	Atom::CPointer(uint8	elementCount){
		
		return	Atom((C_PTR<<24)+elementCount);
	}

		Atom	Atom::Object(uint16	opcode,uint8	arity){
		
		return	Atom((OBJECT<<24)+(opcode<<8)+arity);
	}

		Atom	Atom::Marker(uint16	opcode,uint8	arity){
		
		return	Atom((MARKER<<24)+(opcode<<8)+arity);
	}

		Atom	Atom::Operator(uint16	opcode,uint8	arity){
		
		return	Atom((OPERATOR<<24)+(opcode<<8)+arity);
	}

		Atom	Atom::Node(uint8	nodeID){
		
		return	Atom((NODE<<24)+(nodeID<<8));
	}

		Atom	Atom::UndefinedNode(){
		
		return	Atom(0xA0FFFFFF);
	}

		Atom	Atom::Device(uint8	nodeID,uint8	classID,uint8	devID){
		
		return	Atom((DEVICE<<24)+(nodeID<<16)+(classID<<8)+devID);
	}

		Atom	Atom::UndefinedDevice(){
		
		return	Atom(0xA1FFFFFF);
	}

		Atom	Atom::DeviceFunction(uint16	opcode){
		
		return	Atom((DEVICE_FUNCTION<<24)+(opcode<<8));
	}

		Atom	Atom::UndefinedDeviceFunction(){
		
		return	Atom(0xA2FFFFFF);
	}

		Atom	Atom::String(uint16	characterCount){
		
		uint8	blocks=characterCount/4;
		if(characterCount%4)
			++blocks;
		return	Atom((STRING<<24)+(blocks<<16)+characterCount);
	}

		Atom	Atom::UndefinedString(){
		
		return	Atom(0xC6FFFFFF);
	}

		Atom	Atom::Timestamp(){
		
		return	Atom(TIMESTAMP<<24);
	}

		Atom	Atom::UndefinedTimestamp(){
		
		return	Atom(0xC7FFFFFF);
	}

		Atom	Atom::Forever(){

		return	Atom(0xC7FFFFF0);
	}

		Atom::Atom(uint32	a):atom(a){
	}

		Atom::~Atom(){
	}

		Atom	&Atom::operator	=(const	Atom&	a){
		
		atom=a.atom;
		return	*this;
	}

		bool	Atom::operator	==(const	Atom&	a)	const{
		
		return	atom==a.atom;
	}

		bool	Atom::operator	!=(const	Atom&	a)	const{
		
		return	atom!=a.atom;
	}


		uint8	Atom::getDescriptor()	const{

		return	atom>>24;
	}

		bool	Atom::isPointer()	const{

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

		bool	Atom::isStructural()	const{

		return	(atom	&	0xC0000000)==0xC0000000;
	}

		bool	Atom::isFloat()	const{

		return	atom>>31==0;
	}

		bool	Atom::readsAsNil()	const{

		return	atom==0x3FFFFFFF	||
				atom==0x81FFFFFF	||
				atom==0xC1000000	||
				atom==0xA0FFFFFF	||
				atom==0xA1FFFFFF	||
				atom==0xA2FFFFFF	||
				atom==0xC6FFFFFF	||
				atom==0xC7FFFFFF;
	}

		float32	Atom::asFloat()	const{

		int32	a=atom<<1;
		return	*reinterpret_cast<float32	*>(&a);
	}

		bool	Atom::asBoolean()	const{

		return	atom	&	0x000000FF;
	}

		uint16	Atom::asIndex()	const{

		return	atom	&	0x0000FFFF;
	}

		uint16	Atom::getAtomCount()	const{

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

		uint16	Atom::asOpcode()	const{

		return	(atom>>8)	&	0x0000FFFF;
	}

		uint8	Atom::getNodeID()	const{

		return	(atom	&	0x00FF0000)>>16;
	}

		uint8	Atom::getClassID()	const{

		return	(atom	&	0x0000FF00)>>8;
	}

		uint8	Atom::getDeviceID()	const{

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
				std::string	s;
				char	*content=(char	*)&atom;
				for(uint32	i=0;i<4;++i){

					if(content[i]!='\0')
						s+=content[i];
					else
						break;
				}
				std::cout<<s.c_str();
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
