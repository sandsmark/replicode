//	atom.cpp
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

#include	"atom.h"

#include	<iostream>


namespace	r_code{

	uint8	Atom::Members_to_go=0;
	uint8	Atom::Timestamp_data=0;
	uint8	Atom::String_data=0;
	uint8	Atom::Char_count=0;

	void	Atom::trace()	const{

		write_indents();
		switch(getDescriptor()){
		case	NIL:				std::cout<<"nil";return;
		case	BOOLEAN_:			std::cout<<"bl: "<<std::boolalpha<<asBoolean();return;
		case	WILDCARD:			std::cout<<":";return;
		case	T_WILDCARD:			std::cout<<"::";return;
		case	I_PTR:				std::cout<<"iptr: "<<std::dec<<asIndex();return;
		case	VL_PTR:				std::cout<<"vlptr: "<<std::dec<<asIndex();return;
		case	R_PTR:				std::cout<<"rptr: "<<std::dec<<asIndex();return;
		case	IPGM_PTR:			std::cout<<"ipgm_ptr: "<<std::dec<<asIndex();return;
		case	IN_OBJ_PTR:			std::cout<<"in_obj_ptr: "<<std::dec<<asViewIndex()<<" "<<asIndex();return;
		case	IN_VW_PTR:			std::cout<<"in_vw_ptr: "<<std::dec<<asViewIndex()<<" "<<asIndex();return;
		case	VALUE_PTR:			std::cout<<"value_ptr: "<<std::dec<<asIndex();return;
		case	PROD_PTR:			std::cout<<"prod_ptr: "<<std::dec<<asIndex();return;
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
		case	STRING:				std::cout<<"st: "<<std::dec<<getAtomCount();Members_to_go=String_data=getAtomCount();Char_count=(atom	&	0x000000FF);return;
		case	TIMESTAMP:			std::cout<<"us";Members_to_go=Timestamp_data=2;return;
		default:
			if(Timestamp_data){
				
				--Timestamp_data;
				std::cout<<atom;
			}else	if(String_data){

				--String_data;
				std::string	s;
				char	*content=(char	*)&atom;
				for(uint8	i=0;i<4;++i){

					if(Char_count-->0)
						s+=content[i];
					else
						break;
				}
				std::cout<<s.c_str();
			}else	if(isFloat()){

				std::cout<<"nb: "<<std::scientific<<asFloat();
				return;
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
