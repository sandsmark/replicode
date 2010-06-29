#include	<iostream>
namespace	r_code{

	inline	Atom	Atom::Float(float32	f){

		float32	_f=f;
		uint32	a=*reinterpret_cast<uint32	*>(&_f);
		return	Atom(a>>1);
	}

	inline	Atom	Atom::PlusInfinity(){

		return	Atom(0x3FC00000);
	}

	inline	Atom	Atom::MinusInfinity(){

		return	Atom(0x7FC00000);
	}

	inline	Atom	Atom::UndefinedFloat(){
		
		return	Atom(0xFFFFFFF);
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
		
		return	Atom((I_PTR<<24)+(index	&	0x0FFF));
	}

	inline	Atom	Atom::VLPointer(uint16	index,uint16	cast_opcode){
		
		uint32	a=(VL_PTR<<24);
		a+=cast_opcode<<16;
		a+=index;
		return	Atom((VL_PTR<<24)+((cast_opcode	&	0x0FFF)<<12)+(index	&	0x0FFF));
	}

	inline	Atom	Atom::RPointer(uint16	index){
		
		return	Atom((R_PTR<<24)+(index	&	0x0FFF));
	}

	inline	Atom	Atom::IPGMPointer(uint16	index){
		
		return	Atom((IPGM_PTR<<24)+(index	&	0x0FFF));
	}

	inline	Atom	Atom::InObjPointer(uint8	inputIndex,uint16	index){
		
		return	Atom((IN_OBJ_PTR<<24)+(inputIndex<<12)+(index	&	0x0FFF));
	}

	inline	Atom	Atom::InVwPointer(uint8	inputIndex,uint16	index){
		
		return	Atom((IN_VW_PTR<<24)+(inputIndex<<12)+(index	&	0x0FFF));
	}

	inline	Atom	Atom::ValuePointer(uint16	index){
		
		return	Atom((VALUE_PTR<<24)+(index	&	0x0FFF));
	}

	inline	Atom	Atom::ProductionPointer(uint16	index){
		
		return	Atom((PROD_PTR<<24)+(index	&	0x0FFF));
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

	inline	Atom	Atom::SSet(uint16 opcode,uint8	elementCount){
		
		return	Atom((S_SET<<24)+((opcode	&	0x0FFF)<<8)+elementCount);
	}

	inline	Atom	Atom::Set(uint8	elementCount){
		
		return	Atom((SET<<24)+elementCount);
	}

	inline	Atom	Atom::CPointer(uint8	elementCount){
		
		return	Atom((C_PTR<<24)+elementCount);
	}

	inline	Atom	Atom::Object(uint16	opcode,uint8	arity){
		
		return	Atom((OBJECT<<24)+((opcode	&	0x0FFF)<<8)+arity);
	}

	inline	Atom	Atom::Marker(uint16	opcode,uint8	arity){
		
		return	Atom((MARKER<<24)+((opcode	&	0x0FFF)<<8)+arity);
	}

	inline	Atom	Atom::Operator(uint16	opcode,uint8	arity){
		
		return	Atom((OPERATOR<<24)+((opcode	&	0x0FFF)<<8)+arity);
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

	inline	Atom	Atom::String(uint8	characterCount){
		
		uint8	blocks=characterCount/4;
		if(characterCount%4)
			++blocks;
		return	Atom((STRING<<24)+(blocks<<8)+characterCount);
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

	inline	Atom	Atom::InstantiatedProgram(uint16 opcode,uint8 arity){

		return	Atom((INSTANTIATED_PROGRAM<<24)+((opcode	&	0x0FFF)<<8)+arity);
	}

	inline	Atom	Atom::Group(uint16 opcode,uint8 arity){

		return	Atom((GROUP<<24)+((opcode	&	0x0FFF)<<8)+arity);
	}

	inline	Atom::Atom(uint32	a):atom(a){
	}

	inline	Atom::~Atom(){
	}

	inline		Atom	&Atom::operator	=(const	Atom&	a){
		
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

	inline	bool	Atom::isStructural()	const{

		return	(atom	&	0xC0000000)==0xC0000000;
	}

	inline	bool	Atom::isFloat()	const{

		return	atom>>31==0;
	}

	inline	bool	Atom::readsAsNil()	const{

		return	atom==0x80000000	||
				atom==0x3FFFFFFF	||
				atom==0x81FFFFFF	||
				atom==0xC1000000	||
				atom==0xA0FFFFFF	||
				atom==0xA1FFFFFF	||
				atom==0xA2FFFFFF	||
				atom==0xC6FFFFFF;
	}

	inline	float32	Atom::asFloat()	const{

		uint32	_f=atom<<1;
		return	*reinterpret_cast<const	float32	*>(&_f);
	}

	inline	bool	Atom::asBoolean()	const{

		return	atom	&	0x000000FF;
	}

	inline	uint16	Atom::asIndex()	const{

		return	atom	&	0x00000FFF;
	}

	inline	uint8	Atom::asViewIndex()	const{

		return	(uint8)((atom	&	0x000FF000)>>12);
	}

	inline	uint16	Atom::asOpcode()	const{

		return	(atom>>8)	&	0x00000FFF;
	}

	inline	uint8	Atom::asCastOpcode()	const{

		return	(uint8)((atom	&	0x00FFF000)>>12);
	}

	inline	uint8	Atom::getNodeID()	const{

		return	(uint8)((atom	&	0x00FF0000)>>16);
	}

	inline	uint8	Atom::getClassID()	const{

		return	(atom	&	0x0000FF00)>>8;
	}

	inline	uint8	Atom::getDeviceID()	const{

		return	atom	&	0x000000FF;
	}

	inline	bool	Atom::isPointer()	const{

		switch(getDescriptor()){
		case	I_PTR:
		case	VL_PTR:
		case	R_PTR:
		case	THIS:
		case	C_PTR:
		case	IPGM_PTR:
		case	IN_OBJ_PTR:
		case	IN_VW_PTR:
		case	VALUE_PTR:
		case	PROD_PTR:
			return true;
		default:
			return false;
		}
	}

	inline	uint8	Atom::getAtomCount()	const{
		
		switch(getDescriptor()){
		case	SET:
		case	OBJECT:
		case	MARKER:
		case	C_PTR:
		case	OPERATOR:
		case	INSTANTIATED_PROGRAM:
		case	GROUP:
		case	S_SET:	return	atom	&	0x000000FF;
		case	STRING:	return	(atom	&	0x0000FF00)>>8;
		case	TIMESTAMP: return 2;
		default:
			return	0;
		}
	}
}