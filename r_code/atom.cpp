//	atom.cpp
//
//	Author: Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Eric Nivel
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

	uint8	Atom::GetTolerance(float32	t){

		if(t<0)
			return	0;
		if(t>1)
			return	1;
		float32	f=t*127;
		uint8	u=f;
		if(f-u>0.5)
			return	u+1;
		return	u;
	}

	float32	Atom::getMultiplier()	const{

		switch(atom	&	0x0000007F){
		case    0:      return  0;
		case    1:      return  0.00787;
		case    2:      return  0.01575;
		case    3:      return  0.02362;
		case    4:      return  0.0315;
		case    5:      return  0.03937;
		case    6:      return  0.04724;
		case    7:      return  0.05512;
		case    8:      return  0.06299;
		case    9:      return  0.07087;
		case    10:     return  0.07874;
		case    11:     return  0.08661;
		case    12:     return  0.09449;
		case    13:     return  0.10236;
		case    14:     return  0.11024;
		case    15:     return  0.11811;
		case    16:     return  0.12598;
		case    17:     return  0.13386;
		case    18:     return  0.14173;
		case    19:     return  0.14961;
		case    20:     return  0.15748;
		case    21:     return  0.16535;
		case    22:     return  0.17323;
		case    23:     return  0.1811;
		case    24:     return  0.18898;
		case    25:     return  0.19685;
		case    26:     return  0.20472;
		case    27:     return  0.2126;
		case    28:     return  0.22047;
		case    29:     return  0.22835;
		case    30:     return  0.23622;
		case    31:     return  0.24409;
		case    32:     return  0.25197;
		case    33:     return  0.25984;
		case    34:     return  0.26772;
		case    35:     return  0.27559;
		case    36:     return  0.28346;
		case    37:     return  0.29134;
		case    38:     return  0.29921;
		case    39:     return  0.30709;
		case    40:     return  0.31496;
		case    41:     return  0.32283;
		case    42:     return  0.33071;
		case    43:     return  0.33858;
		case    44:     return  0.34646;
		case    45:     return  0.35433;
		case    46:     return  0.3622;
		case    47:     return  0.37008;
		case    48:     return  0.37795;
		case    49:     return  0.38583;
		case    50:     return  0.3937;
		case    51:     return  0.40157;
		case    52:     return  0.40945;
		case    53:     return  0.41732;
		case    54:     return  0.4252;
		case    55:     return  0.43307;
		case    56:     return  0.44094;
		case    57:     return  0.44882;
		case    58:     return  0.45669;
		case    59:     return  0.46457;
		case    60:     return  0.47244;
		case    61:     return  0.48031;
		case    62:     return  0.48819;
		case    63:     return  0.49606;
		case    64:     return  0.50394;
		case    65:     return  0.51181;
		case    66:     return  0.51969;
		case    67:     return  0.52756;
		case    68:     return  0.53543;
		case    69:     return  0.54331;
		case    70:     return  0.55118;
		case    71:     return  0.55906;
		case    72:     return  0.56693;
		case    73:     return  0.5748;
		case    74:     return  0.58268;
		case    75:     return  0.59055;
		case    76:     return  0.59843;
		case    77:     return  0.6063;
		case    78:     return  0.61417;
		case    79:     return  0.62205;
		case    80:     return  0.62992;
		case    81:     return  0.6378;
		case    82:     return  0.64567;
		case    83:     return  0.65354;
		case    84:     return  0.66142;
		case    85:     return  0.66929;
		case    86:     return  0.67717;
		case    87:     return  0.68504;
		case    88:     return  0.69291;
		case    89:     return  0.70079;
		case    90:     return  0.70866;
		case    91:     return  0.71654;
		case    92:     return  0.72441;
		case    93:     return  0.73228;
		case    94:     return  0.74016;
		case    95:     return  0.74803;
		case    96:     return  0.75591;
		case    97:     return  0.76378;
		case    98:     return  0.77165;
		case    99:     return  0.77953;
		case    100:    return  0.7874;
		case    101:    return  0.79528;
		case    102:    return  0.80315;
		case    103:    return  0.81102;
		case    104:    return  0.8189;
		case    105:    return  0.82677;
		case    106:    return  0.83465;
		case    107:    return  0.84252;
		case    108:    return  0.85039;
		case    109:    return  0.85827;
		case    110:    return  0.86614;
		case    111:    return  0.87402;
		case    112:    return  0.88189;
		case    113:    return  0.88976;
		case    114:    return  0.89764;
		case    115:    return  0.90551;
		case    116:    return  0.91339;
		case    117:    return  0.92126;
		case    118:    return  0.92913;
		case    119:    return  0.93701;
		case    120:    return  0.94488;
		case    121:    return  0.95276;
		case    122:    return  0.96063;
		case    123:    return  0.9685;
		case    124:    return  0.97638;
		case    125:    return  0.98425;
		case    126:    return  0.99213;
		case    127:    return  1;
		default:
			return	0;
		}
	}

	void	Atom::trace()	const{

		write_indents();
		switch(getDescriptor()){
		case	NIL:					std::cout<<"nil";return;
		case	BOOLEAN_:				std::cout<<"bl: "<<std::boolalpha<<asBoolean();return;
		case	WILDCARD:				std::cout<<":";return;
		case	T_WILDCARD:				std::cout<<"::";return;
		case	I_PTR:					std::cout<<"iptr: "<<std::dec<<asIndex();return;
		case	VL_PTR:					std::cout<<"vlptr: "<<std::dec<<asIndex();return;
		case	R_PTR:					std::cout<<"rptr: "<<std::dec<<asIndex();return;
		case	IPGM_PTR:				std::cout<<"ipgm_ptr: "<<std::dec<<asIndex();return;
		case	IN_OBJ_PTR:				std::cout<<"in_obj_ptr: "<<std::dec<<(uint32)asInputIndex()<<" "<<asIndex();return;
		case	D_IN_OBJ_PTR:			std::cout<<"d_in_obj_ptr: "<<std::dec<<(uint32)asRelativeIndex()<<" "<<asIndex();return;
		case	OUT_OBJ_PTR:			std::cout<<"out_obj_ptr: "<<std::dec<<asIndex();return;
		case	VALUE_PTR:				std::cout<<"value_ptr: "<<std::dec<<asIndex();return;
		case	PROD_PTR:				std::cout<<"prod_ptr: "<<std::dec<<asIndex();return;
		case	THIS:					std::cout<<"this";return;
		case	VIEW:					std::cout<<"view";return;
		case	MKS:					std::cout<<"mks";return;
		case	VWS:					std::cout<<"vws";return;
		case	NODE:					std::cout<<"nid: "<<std::dec<<(uint32)getNodeID();return;
		case	DEVICE:					std::cout<<"did: "<<std::dec<<(uint32)getNodeID()<<" "<<(uint32)getClassID()<<" "<<(uint32)getDeviceID();return;
		case	DEVICE_FUNCTION:		std::cout<<"fid: "<<std::dec<<asOpcode();return;
		case	C_PTR:					std::cout<<"cptr: "<<std::dec<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	SET:					std::cout<<"set: "<<std::dec<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	OBJECT:					std::cout<<"obj: "<<std::dec<<asOpcode()<<" "<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	S_SET:					std::cout<<"s_set: "<<std::dec<<asOpcode()<<" "<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	MARKER:					std::cout<<"mk: "<<std::dec<<asOpcode()<<" "<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	OPERATOR:				std::cout<<"op: "<<std::dec<<asOpcode()<<" "<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	STRING:					std::cout<<"st: "<<std::dec<<(uint16)getAtomCount();Members_to_go=String_data=getAtomCount();Char_count=(atom	&	0x000000FF);return;
		case	TIMESTAMP:				std::cout<<"us";Members_to_go=Timestamp_data=2;return;
		case	INSTANTIATED_PROGRAM:	std::cout<<"ipgm: "<<std::dec<<asOpcode()<<" "<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	GROUP:					std::cout<<"grp: "<<std::dec<<asOpcode()<<" "<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	REDUCTION_GROUP:		std::cout<<"rgrp: "<<std::dec<<asOpcode()<<" "<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	INSTANTIATED_CPP_PROGRAM:	std::cout<<"icpp_pgm: "<<std::dec<<asOpcode()<<" "<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	MODEL:					std::cout<<"mdl: "<<std::dec<<asOpcode()<<" "<<(uint16)getAtomCount();Members_to_go=getAtomCount();return;
		case	NUMERICAL_VARIABLE:		std::cout<<"num_var: "<<std::dec<<getVariableID()<<" "<<std::fixed<<getTolerance();return;
		case	STRUCTURAL_VARIABLE:	std::cout<<"struct_var: "<<std::dec<<getVariableID()<<" "<<std::fixed<<getTolerance();return;
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

	void	Atom::Trace(Atom	*base,uint16	count){

		std::cout<<"--------\n";
		for(uint16	i=0;i<count;++i){

			std::cout<<i<<"\t";
			base[i].trace();
			std::cout<<std::endl;
		}
	}
}
