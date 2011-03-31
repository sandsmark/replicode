//	decompiler.cpp
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

#include	"decompiler.h"
#include	"../../CoreLibrary/trunk/CoreLibrary/utils.h"


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
		this->time_offset=0;

		ignore_ontology=false;

		//	Load the renderers;
		for(uint16	i=0;i<metadata->classes_by_opcodes.size();++i){

			Class	*c=metadata->getClass(i);
			std::string	class_name=c->str_opcode;
			size_t		p=class_name.find("mk.");
			if(p!=std::string::npos)
				renderers[i]=&Decompiler::write_marker;
			else	if(class_name=="grp")
				renderers[i]=&Decompiler::write_group;
			else	if(class_name=="ipgm"	||	class_name=="icpp_pgm")
				renderers[i]=&Decompiler::write_ipgm;
			else	if(class_name=="pgm"	||	class_name==" |pgm")
				renderers[i]=&Decompiler::write_pgm;
			else	if(class_name=="cmd")
				renderers[i]=&Decompiler::write_cmd;
			else
				renderers[i]=&Decompiler::write_expression;
		}
	}

	uint32	Decompiler::decompile(r_comp::Image		*image,std::ostringstream	*stream,uint64	time_offset,bool	ignore_ontology){

		this->ignore_ontology=ignore_ontology;

		uint32	object_count=decompile_references(image);

		for(uint16	i=0;i<image->code_segment.objects.size();++i)
			decompile_object(i,stream,time_offset);

		return	object_count;
	}

	uint32	Decompiler::decompile_references(r_comp::Image	*image){

		UNORDERED_MAP<const	Class	*,uint16>	object_ID_per_class;
		UNORDERED_MAP<std::string,Class>::const_iterator	it;
		for(it=metadata->sys_classes.begin();it!=metadata->sys_classes.end();++it)
			object_ID_per_class[&(it->second)]=0;

		char		buffer[255];
		std::string	s;

		this->image=image;

		//	populate object names first so they can be referenced in any order.
		Class	*c;
		uint16	last_object_ID;
		for(uint16	i=0;i<image->code_segment.objects.size();++i){

			SysObject	*sys_object=(SysObject	*)image->code_segment.objects[i];
			switch(sys_object->axiom){
			case	SysObject::ROOT_GRP:	s="root";break;
			case	SysObject::STDIN_GRP:	s="stdin";break;
			case	SysObject::STDOUT_GRP:	s="stdout";break;
			case	SysObject::SELF_ENT:	s="self";break;
			default:
				c=metadata->getClass(sys_object->code[0].asOpcode());
				last_object_ID=object_ID_per_class[c];
				object_ID_per_class[c]=last_object_ID+1;
				sprintf(buffer,"%d",last_object_ID);
				s=c->str_opcode;
				s+=buffer;
				break;
			}
			object_names[i]=s;
			object_indices[s]=i;
		}

		closing_set=false;

		return	image->code_segment.objects.size();
	}

	void	Decompiler::decompile_object(uint16	object_index,std::ostringstream	*stream,uint64	time_offset){

		if(!out_stream)
			out_stream=new	OutStream(stream);
		else	if(out_stream->stream!=stream){

			delete	out_stream;
			out_stream=new	OutStream(stream);
		}

		this->time_offset=time_offset;

		current_object=image->code_segment.objects[object_index];
		SysObject	*sys_object=(SysObject	*)current_object;
		uint16	read_index=0;
		bool	after_tail_wildcard=false;
		indents=0;

		if(ignore_ontology){

			std::string	ent="ent";
			std::string	ont="ont";
			if(	current_object->code[0]==metadata->getClass(ent)->atom	||
				current_object->code[0]==metadata->getClass(ont)->atom)
				return;
		}

		std::string	s=object_names[object_index];
		s+=":";
		*out_stream<<s;

		(this->*renderers[current_object->code[read_index].asOpcode()])(read_index);

		uint16	view_count=sys_object->views.size();
		if(view_count){	//	write the set of views

			*out_stream<<" []";
			for(uint16	i=0;i<view_count;++i){

				write_indent(3);
				current_object=sys_object->views[i];
				write_view(0,current_object->code[0].getAtomCount());
			}
		}else
			*out_stream<<" |[]";
		write_indent(0);
		write_indent(0);
	}

	void	Decompiler::decompile_object(const	std::string	object_name,std::ostringstream	*stream,uint64	time_offset){

		decompile_object(object_indices[object_name],stream,time_offset);
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
		case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
		case	Atom::INSTANTIATED_ANTI_PROGRAM:
			*out_stream<<"ipgm";
			break;
		case	Atom::INSTANTIATED_CPP_PROGRAM:
			*out_stream<<"icpp_pgm";
			break;
		case	Atom::COMPOSITE_STATE:
			*out_stream<<"cst";
			break;
		case	Atom::MODEL:
			*out_stream<<"mdl";
			break;
		case	Atom::GROUP:
			*out_stream<<"grp";
			break;
		default:
			*out_stream<<"undefined-class";
			break;
		}
	}

	void	Decompiler::write_expression_tail(uint16	read_index,bool	apply_time_offset,bool	vertical){	//	read_index points initially to the head.

		uint16	arity=current_object->code[read_index].getAtomCount();
		bool	after_tail_wildcard=false;

		for(uint16	i=0;i<arity;++i){

			if(after_tail_wildcard)
				write_any(++read_index,after_tail_wildcard,apply_time_offset);
			else{

				if(closing_set){

					closing_set=false;
					write_indent(indents);
				}else	if(!vertical)
					*out_stream<<' ';

				write_any(++read_index,after_tail_wildcard,apply_time_offset);

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
		write_expression_tail(read_index,true);
		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		*out_stream<<')';
	}

	void	Decompiler::write_group(uint16	read_index){

		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		out_stream->push('(',read_index);
		write_expression_head(read_index);
		write_expression_tail(read_index,false);
		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		*out_stream<<')';
	}

	void	Decompiler::write_marker(uint16	read_index){

		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		out_stream->push('(',read_index);
		write_expression_head(read_index);
		write_expression_tail(read_index,false);
		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		*out_stream<<')';
	}

	void	Decompiler::write_pgm(uint16	read_index){

		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		out_stream->push('(',read_index);
		write_expression_head(read_index);
		write_expression_tail(read_index,true);
		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		*out_stream<<')';
	}


	void	Decompiler::write_ipgm(uint16	read_index){

		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		out_stream->push('(',read_index);
		write_expression_head(read_index);
		write_expression_tail(read_index,false);
		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		*out_stream<<')';
	}

	void	Decompiler::write_cmd(uint16	read_index){

		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		out_stream->push('(',read_index);
		write_expression_head(read_index);
		//write_expression_tail(read_index,true);

		uint16	write_as_view_index=0;
		if(current_object->code[read_index+1].asOpcode()==metadata->classes.find("_inj")->second.atom.asOpcode()){

			uint16	arg_set_index=current_object->code[read_index+3].asIndex();	//	2 args for _inj; the view is the second.
			write_as_view_index=current_object->code[arg_set_index+2].asIndex();
		}

		uint16	arity=current_object->code[read_index].getAtomCount();
		bool	after_tail_wildcard=false;

		for(uint16	i=0;i<arity;++i){

			if(after_tail_wildcard)
				write_any(++read_index,after_tail_wildcard,true);
			else{

				if(closing_set){

					closing_set=false;
					write_indent(indents);
				}else
					*out_stream<<' ';

				write_any(++read_index,after_tail_wildcard,true,write_as_view_index);
			}
		}

		if(closing_set){

			closing_set=false;
			write_indent(indents);
		}
		*out_stream<<')';
	}

	void	Decompiler::write_view(uint16	read_index,uint16	arity){

		bool	after_tail_wildcard=false;
		*out_stream<<"[";
		for(uint16	j=1;j<=arity;++j){

			write_any(read_index+j,after_tail_wildcard,true);
			if(j<arity)
				*out_stream<<" ";
		}
		*out_stream<<"]";
	}

	void	Decompiler::write_set(uint16	read_index,bool	aply_time_offset,uint16	write_as_view_index){	//	read_index points to a set atom.

		uint16	arity=current_object->code[read_index].getAtomCount();
		bool	after_tail_wildcard=false;
		
		if(arity==1){	//	write [element]

			out_stream->push('[',read_index);
			write_any(++read_index,after_tail_wildcard,aply_time_offset);
			*out_stream<<']';
		}else	if(write_as_view_index>0	&&	write_as_view_index==read_index)
			write_view(read_index,arity);
		else{		//	write []+indented elements.

			out_stream->push("[]",read_index);
			indents+=3;
			for(uint16	i=0;i<arity;++i){

				if(after_tail_wildcard)
					write_any(++read_index,after_tail_wildcard,aply_time_offset);
				else{

					write_indent(indents);
					write_any(++read_index,after_tail_wildcard,aply_time_offset,write_as_view_index);
				}
			}
			closing_set=true;
			indents-=3;	//	don't call write_indents() here as the last set member can be a set.
		}
	}

	void	Decompiler::write_any(uint16	read_index,bool	&after_tail_wildcard,bool	apply_time_offset,uint16	write_as_view_index){	//	after_tail_wildcard meant to avoid printing ':' after "::".

		Atom	a=current_object->code[read_index];

		if(a.isFloat()){

			if(a.atom==0x3FFFFFFF)
				out_stream->push("|nb",read_index);
			else	if(a==Atom::PlusInfinity())
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
			case	Atom::OBJECT:{

				std::string	class_name=metadata->class_names[current_object->code[index].asOpcode()];
				if(class_name!="val_hld"){

					Atom	first=current_object->code[index+1];
					if(first.getDescriptor()==Atom::STRUCTURAL_VARIABLE){

						*out_stream<<class_name;
						out_stream->push("<",read_index);
						*out_stream<<first.getVariableID()<<",";
						*out_stream<<first.getMultiplier()<<">";
						break;
					}
				}
			}case	Atom::MARKER:
			case	Atom::GROUP:
			case	Atom::INSTANTIATED_PROGRAM:
			case	Atom::INSTANTIATED_INPUT_LESS_PROGRAM:
			case	Atom::INSTANTIATED_ANTI_PROGRAM:
			case	Atom::INSTANTIATED_CPP_PROGRAM:
			case	Atom::COMPOSITE_STATE:
			case	Atom::MODEL:
			case	Atom::OPERATOR:
				(this->*renderers[atom.asOpcode()])(index);
				break;
			case	Atom::SET:
			case	Atom::S_SET:
				if(atom.readsAsNil())
					out_stream->push("|[]",read_index);
				else
					write_set(index,apply_time_offset,write_as_view_index);
				break;
			case	Atom::STRING:
				if(atom.readsAsNil())
					out_stream->push("|st",read_index);
				else{

					Atom	first=current_object->code[index+1];
					if(first.getDescriptor()==Atom::STRUCTURAL_VARIABLE){

						out_stream->push("st<",read_index);
						*out_stream<<first.getVariableID()<<",";
						*out_stream<<first.getMultiplier()<<">";
					}else{

						std::string	s=Utils::GetString(&current_object->code[index]);
						*out_stream<<'\"'<<s<<'\"';
					}
				}
				break;
			case	Atom::TIMESTAMP:
				if(atom.readsAsNil())
					out_stream->push("|us",read_index);
				else{

					Atom	first=current_object->code[index+1];
					if(first.getDescriptor()==Atom::STRUCTURAL_VARIABLE){

						out_stream->push("us<",read_index);
						*out_stream<<first.getVariableID()<<",";
						*out_stream<<first.getMultiplier()<<">";
					}else{

						uint64	ts=Utils::GetTimestamp(&current_object->code[index]);
						if(ts>0	&&	apply_time_offset)
							ts-=time_offset;
						out_stream->push(Time::ToString_seconds(ts),read_index);
					}
				}
				break;
			case	Atom::C_PTR:{
				
				uint16	opcode;
				uint16	member_count=atom.getAtomCount();
				atom=current_object->code[index+1];	//	current_object->code[index] is the cptr; lead atom is at index+1; iptrs start at index+2.
				switch(atom.getDescriptor()){
				case	Atom::THIS:	//	this always refers to an instantiated reactive object.
					out_stream->push("this",read_index);
					opcode=metadata->sys_classes["ipgm"].atom.asOpcode();
					break;
				case	Atom::VL_PTR:{

					uint8	cast_opcode=atom.asCastOpcode();
					while(current_object->code[atom.asIndex()].getDescriptor()==Atom::I_PTR)	// position to a structure or an atomic value.	
						atom=current_object->code[atom.asIndex()];
					out_stream->push(get_variable_name(atom.asIndex(),current_object->code[atom.asIndex()].getDescriptor()!=Atom::WILDCARD),read_index);
					if(cast_opcode==0xFF){

						if(current_object->code[atom.asIndex()].getDescriptor()==Atom::WILDCARD)
							opcode=current_object->code[atom.asIndex()].asOpcode();
						else
							opcode=current_object->code[atom.asIndex()].asOpcode();
					}else
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

				Class	embedding_class=metadata->classes_by_opcodes[opcode];	//	class defining the members.
				for(uint16	i=2;i<=member_count;++i){	//	get the class of the pointed structure and retrieve the member name from i.

					std::string	member_name;
					atom=current_object->code[index+i];	//	atom is an iptr appearing after the leading atom in the cptr.
					switch(atom.getDescriptor()){
					case	Atom::VIEW:
						member_name="vw";
						break;
					case	Atom::MKS:
						member_name="mks";
						break;
					case	Atom::VWS:
						member_name="vws";
						break;
					default:
						member_name=embedding_class.get_member_name(atom.asIndex());
						if(i<member_count)	//	not the last member, get the next class.
							embedding_class=*embedding_class.get_member_class(metadata,member_name);
						break;
					}
					*out_stream<<'.'<<member_name;

					if(member_name=="vw"){	//	special case: no view structure in the code, vw is just a place holder; vw is the second to last member of the cptr: write the last member and exit.

						atom=current_object->code[index+member_count];	//	atom is the last internal pointer.
						Class	view_class;
						if(embedding_class.str_opcode=="grp")
							view_class=metadata->classes.find("grp_view")->second;
						else	if(embedding_class.str_opcode=="ipgm"		||
									embedding_class.str_opcode=="icpp_pgm"	||
									embedding_class.str_opcode=="mdl"		||
									embedding_class.str_opcode=="cst")
							view_class=metadata->classes.find("pgm_view")->second;
						else
							view_class=metadata->classes.find("view")->second;
						member_name=view_class.get_member_name(atom.asIndex());
						*out_stream<<'.'<<member_name;
						break;
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
			out_stream->push("vw",read_index);
			break;
		case	Atom::MKS:
			out_stream->push("mks",read_index);
			break;
		case	Atom::VWS:
			out_stream->push("vws",read_index);
			break;
		case	Atom::NUMERICAL_VARIABLE:
			out_stream->push("nb<",read_index);
			*out_stream<<a.getVariableID()<<",";
			*out_stream<<a.getMultiplier()<<">";
			break;
		case	Atom::STRUCTURAL_VARIABLE:
			out_stream->push("svar<",read_index);
			*out_stream<<a.getVariableID()<<",";
			*out_stream<<a.getMultiplier()<<">";
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