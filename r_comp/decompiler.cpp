//	decompiler.cpp
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

#include	"decompiler.h"


namespace	r_comp{

	Decompiler::Decompiler():out_stream(NULL),current_object(NULL),metadata(NULL),image(NULL){
	}

	Decompiler::~Decompiler(){

		if(out_stream)
			delete	out_stream;
	}

	std::string	Decompiler::get_variable_name(uint16	index,bool	postfix){

		static	uint16	Last_variable_ID=0;
		std::string	s;
		UNORDERED_MAP<uint16,std::string>::iterator	it=variable_names.find(index);
		if(it==variable_names.end()){

			char	buffer[255];
			s="v";
			sprintf(buffer,"%d",Last_variable_ID++);
			s+=buffer;
			variable_names[index]=s;
			if(postfix)
				s+=':';
			out_stream->insert(index,s);
			if(postfix)
				return	s.substr(0,s.length()-1);
			return	s;
		}
		return	it->second;
	}

	std::string	Decompiler::get_object_name(uint16	index){

		std::string	s;
		UNORDERED_MAP<uint16,std::string>::iterator	it=object_names.find(index);
		if(it==object_names.end()){

			s="unknown-object";
			return	s;
		}
		return	it->second;
	}

	void	Decompiler::init(r_comp::Metadata	*metadata){

		this->metadata=metadata;
	}

	void	Decompiler::decompile(r_comp::Image		*image,std::ostringstream	*stream){

		out_stream=new	OutStream(stream);
		this->image=image;

		uint32		last_object_ID=0;
		char		buffer[255];
		std::string	s;

		closing_set=false;

		for(uint32	i=0;i<image->code_segment.objects.size();++i){

			current_object=image->code_segment.objects[i];
			SysObject	*sys_object=(SysObject	*)current_object;
			uint16	read_index=0;
			bool	after_tail_wildcard=false;
			indents=0;

			switch(i){
			case	0:	s="root";break;
			case	1:	s="stdin";break;
			case	2:	s="stdout";break;
			case	3:	s="self";break;
			default:
				sprintf(buffer,"%d",last_object_ID++);
				s=metadata->getClass(current_object->code[0].asOpcode())->str_opcode;
				s+=buffer;
				break;
			}
			object_names[i]=s;
			s+=":";
			*out_stream<<s;

			out_stream->push('(',read_index);
			write_expression_head(read_index);
			write_indent(0);
			write_expression_tail(read_index,true);
			*out_stream<<')';
			
			write_indent(0);
			write_indent(0);

			uint32	view_count=sys_object->views.size();
			if(view_count){	//	write the set of views

				*out_stream<<"[]; view set";
				for(uint32	i=0;i<view_count;++i){

					write_indent(3);
					current_object=sys_object->views[i];
					read_index=0;
					after_tail_wildcard=false;
					*out_stream<<"[";
					uint16	arity=current_object->code[0].getAtomCount();
					for(uint32	j=1;j<=arity;++j){

						write_any(read_index+j,after_tail_wildcard);
						if(j<arity)
							*out_stream<<" ";
					}
					*out_stream<<"]";
				}
			}else
				*out_stream<<"|[]; view set";
			write_indent(0);
			write_indent(0);
		}
	}

	void	Decompiler::write_indent(uint16	i){

		*out_stream<<NEWLINE;
		indents=i;
		for(uint16	j=0;j<indents;j++)
			*out_stream<<' ';
	}

	void	Decompiler::write_expression_head(uint16	read_index){

		switch(current_object->code[read_index].getDescriptor()){
		case	Atom::OPERATOR:
			*out_stream<<metadata->operator_names[current_object->code[read_index].asOpcode()];
			break;
		case	Atom::OBJECT:
		case	Atom::MARKER:
			*out_stream<<metadata->class_names[current_object->code[read_index].asOpcode()];
			break;
		case	Atom::INSTANTIATED_PROGRAM:
			*out_stream<<"ipgm";
			break;
		case	Atom::GROUP:
			*out_stream<<"grp";
			break;
		default:
			*out_stream<<"undefined-class";
			break;
		}
	}

	void	Decompiler::write_expression_tail(uint16	read_index,bool	vertical){	//	read_index points initially to the head.

		uint16	arity=current_object->code[read_index].getAtomCount();
		bool	after_tail_wildcard=false;
		for(uint16	i=0;i<arity;++i){

			if(after_tail_wildcard)
				write_any(++read_index,after_tail_wildcard);
			else{

				if(closing_set){

					closing_set=false;
					write_indent(indents);
				}else	if(!vertical)
					*out_stream<<' ';
				write_any(++read_index,after_tail_wildcard);
				if(!closing_set	&&	vertical)
					*out_stream<<NEWLINE;
			}
		}
	}

	void	Decompiler::write_expression(uint16	read_index){

		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		out_stream->push('(',read_index);
		write_expression_head(read_index);
		write_expression_tail(read_index);
		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		*out_stream<<')';
	}

	void	Decompiler::write_set(uint16	read_index){	//	read_index points initially to set atom.

		uint16	arity=current_object->code[read_index].getAtomCount();
		bool	after_tail_wildcard=false;
		if(arity==1){	//	write [element]

			out_stream->push('[',read_index);
			write_any(++read_index,after_tail_wildcard);
			*out_stream<<']';
		}else{		//	write []+indented elements.

			out_stream->push("[]",read_index);
			indents+=3;
			for(uint16	i=0;i<arity;++i){

				if(after_tail_wildcard)
					write_any(++read_index,after_tail_wildcard);
				else{

					write_indent(indents);
					write_any(++read_index,after_tail_wildcard);
				}
			}
			closing_set=true;
			indents-=3;	//	don't call write_indents() here as the last set member can be a set.
		}
	}

	void	Decompiler::write_any(uint16	read_index,bool	&after_tail_wildcard){	//	after_tail_wildcard meant to avoid printing ':' after "::".

		Atom	a=current_object->code[read_index];

		if(a.isFloat()){

			if(a.atom==0x3FFFFFFF)
				out_stream->push("|nb",read_index);
			else	if(a.atom==0x3FC00000)
				out_stream->push("forever",read_index);
			else{

				*out_stream<<std::dec;
				out_stream->push(a.asFloat(),read_index);
			}
			return;
		}

		Atom	atom;
		uint16	index;
		switch(a.getDescriptor()){
		case	Atom::I_PTR:
		case	Atom::VL_PTR:
			index=a.asIndex();
			atom=current_object->code[index];
			while(atom.getDescriptor()==Atom::I_PTR){

				index=atom.asIndex();
				atom=current_object->code[index];
			}
			if(index<read_index){	//	reference to a label or variable.

				std::string	s=get_variable_name(index,atom.getDescriptor()!=Atom::WILDCARD); // post-fix labels with ':' (no need for variables since they are inserted just before wildcards).
				out_stream->push(s,read_index);
				break;
			}
			switch(atom.getDescriptor()){	//	structures.
			case	Atom::OBJECT:
			case	Atom::MARKER:
			case	Atom::OPERATOR:
				write_expression(index);
				break;
			case	Atom::SET:
			case	Atom::S_SET:
				if(atom.readsAsNil())
					out_stream->push("|[]",read_index);
				else
					write_set(index);
				break;
			case	Atom::STRING:
				if(atom.readsAsNil())
					out_stream->push("|st",read_index);
				else{

					uint16		char_count=atom.atom	&	0x0000FFFF;
					uint8		block_offset=1;
					std::string	s;
					s+=(char	*)&current_object->code[index+block_offset].atom;
					*out_stream<<'\"'<<s<<'\"';
				}
				break;
			case	Atom::TIMESTAMP:
				if(atom.readsAsNil())
					out_stream->push("|us",read_index);
				else{
					uint64	ts=((uint64)(current_object->code[index+1].atom))<<32	|	((uint64)(current_object->code[index+2].atom));
					out_stream->push(ts,read_index);
					*out_stream<<"us";
				}
				break;
			case	Atom::C_PTR:{
				uint16	opcode;
				uint16	member_count=/*current_object->code[0].getAtomCount()*/atom.getAtomCount()-1;	//	-1: the leading atom is the CPtr.
				atom=current_object->code[index+1];	//	current_object->code[index] is the cptr; members start at 1.
				Class	_class;
				switch(atom.getDescriptor()){
				case	Atom::THIS:
					out_stream->push("this",read_index);
					opcode=current_object->code[0].asOpcode();
					//	for reactive objects, this refers to the instantiated reactive object.
					_class=metadata->classes_by_opcodes[opcode];
					if(_class.str_opcode=="pgm")
						opcode=metadata->sys_classes["ipgm"].atom.asOpcode();
					else	if(_class.str_opcode=="fmd")
						opcode=metadata->sys_classes["ifmd"].atom.asOpcode();
					else	if(_class.str_opcode=="imd")
						opcode=metadata->sys_classes["iimd"].atom.asOpcode();
					break;
				case	Atom::VL_PTR:{

					uint8	cast_opcode=atom.asCastOpcode();
					while(current_object->code[atom.asIndex()].getDescriptor()==Atom::I_PTR)	// position to a structure or an atomic value.	
						atom=current_object->code[atom.asIndex()];
					out_stream->push(get_variable_name(atom.asIndex(),true),read_index);
					if(cast_opcode==0xFF)
						opcode=current_object->code[atom.asIndex()].asOpcode();
					else
						opcode=cast_opcode;
					break;
				}case	Atom::R_PTR:{
					uint32	object_index=current_object->references[atom.asIndex()];
					out_stream->push(get_object_name(object_index),read_index);
					opcode=image->code_segment.objects[object_index]->code[0].asOpcode();
					break;
				}default:
					out_stream->push("unknown-cptr-lead-type",read_index);
					break;
				}
				uint16	structure_index=0;
				for(uint16	i=1;i<=member_count;++i){	//	get the opcode of the pointed structure and retrieve the member name from i.

					std::string	member_name;
					atom=current_object->code[index+1+i];	//	atom is an I_PTR appearing after the leading atom.
					Class	embedding_class=metadata->classes_by_opcodes[opcode];	//	class defining the member.
					member_name=embedding_class.get_member_name(atom.asIndex());
					*out_stream<<'.'<<member_name;	
					if(i<member_count){	//	not the last member, point to next structure
					
						if(member_name=="vw"){	//	special case: no view structure in the code, vw is just a place holder; vw is the second to last member: write the last member and exit.

							atom=current_object->code[index+i+2];	//	atom is the last internal pointer.
							member_name=embedding_class.get_member_class(metadata,"vw")->get_member_name(atom.asIndex());
							*out_stream<<'.'<<member_name;
							break;
						}else{	//	regular case: the member points to a structure embedded in the code.

							uint16	_target_index=structure_index+atom.asIndex();
							while(current_object->code[_target_index].getDescriptor()==Atom::I_PTR){

								atom=current_object->code[_target_index];
								_target_index=atom.asIndex();
							}
							opcode=current_object->code[atom.asIndex()].asOpcode();
							structure_index=atom.asIndex()+1;
						}
					}
				}
				break;
			}default:
				out_stream->push("undefined-structural-atom-or-reference",read_index);
				break;
			}
			break;
		case	Atom::R_PTR:
			out_stream->push(get_object_name(current_object->references[a.asIndex()]),read_index);
			break;
		case	Atom::THIS:
			out_stream->push("this",read_index);
			break;
		case	Atom::NIL:
			out_stream->push("nil",read_index);
			break;
		case	Atom::BOOLEAN_:
			if(a.readsAsNil())
				out_stream->push("|bl",read_index);
			else{

				*out_stream<<std::boolalpha;
				out_stream->push(a.asBoolean(),read_index);
			}
			break;
		case	Atom::WILDCARD:
			if(after_tail_wildcard)
				out_stream->push();
			else
				out_stream->push(':',read_index);
			break;
		case	Atom::T_WILDCARD:
			out_stream->push("::",read_index);
			after_tail_wildcard=true;
			break;
		case	Atom::NODE:
			if(a.readsAsNil())
				out_stream->push("|sid",read_index);
			else{

				out_stream->push("0x",read_index);
				*out_stream<<std::hex;
				*out_stream<<a.atom;
			}
			break;
		case	Atom::DEVICE:
			if(a.readsAsNil())
				out_stream->push("|did",read_index);
			else{

				out_stream->push("0x",read_index);
				*out_stream<<std::hex;
				*out_stream<<a.atom;
			}
			break;
		case	Atom::DEVICE_FUNCTION:
			if(a.readsAsNil())
				out_stream->push("|fid",read_index);
			else
				out_stream->push(metadata->function_names[a.asOpcode()],read_index);
			break;
		case	Atom::VIEW:
			out_stream->push("vw",read_index);	//	to be consistent with hand-crafted source code, shall be written |[].
			break;
		case	Atom::MKS:
			out_stream->push("mks",read_index);
			break;
		case	Atom::VWS:
			out_stream->push("vws",read_index);
			break;
		default:
			//	out_stream->push("undefined-atom",read_index).
			out_stream->push("0x",read_index);
			*out_stream<<std::hex;
			*out_stream<<a.atom;
			break;
		}
	}
}