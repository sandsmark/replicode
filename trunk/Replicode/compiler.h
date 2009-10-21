#ifndef	compiler_h
#define	compiler_h

#include	"array.h"
#include	<fstream>
#include	<sstream>
#include	"config.h"

#include	"rview.h"


using	namespace	mBrane;
using	namespace	mBrane::sdk;

namespace	replicode{

	class	dll_export	Compiler{
	private:
		//	compilation
		std::istream			*in_stream;
		sdk::Array<RView,16>	*storage;
		std::string				*error;

		RView	*out_view;	//	current view
		Code	*out_code;	//	write destination: view or object

		class	State{
		public:
			State():indents(0),
					right_indents_ahead(0),
					left_indents_ahead(0),
					pattern_lvl(0),
					no_arity_check(false){}
			State(Compiler	*c):indents(c->state.indents),
								right_indents_ahead(c->state.right_indents_ahead),
								left_indents_ahead(c->state.left_indents_ahead),
								pattern_lvl(c->state.pattern_lvl),
								no_arity_check(c->state.no_arity_check),
								stream_ptr(c->in_stream->tellg()){}
			uint16			indents;				//	1 indent = 3 char
			uint16			right_indents_ahead;	//	parsing right indents may unveil more 3-char groups than needed: that's some indents ahead. Avoids requiring a newline for each indent
			uint16			left_indents_ahead;		//	as above
			uint16			pattern_lvl;			//	add one when parsing skel in (ptn skel guards), sub one when done
			bool			no_arity_check;			//	set to true when a tail wildcard is encountered while parsing an expression, set back to false when done parsing the expression
			std::streampos	stream_ptr;
		};

		State	state;
		State	save_state();			//	called before trying to read an expression
		void	restore_state(State	s);	//	called after failing to read an expression

		void	set_error(std::string	s);

		typedef	enum{
			ANY=0,
			NUMBER=1,
			TIMESTAMP=2,
			SET=3,
			BOOLEAN=4,
			STRING=5,
			SUB_SYSTEM_ID=6,
			DEVICE_ID=7,
			FUNCTION_ID=8
		}ReturnType;

		class	Pattern;
		typedef	bool	(Compiler::*_Read)(bool	&,bool,const	Pattern	*,uint16,uint16	&,bool);	//	reads fromt he stream and writes in an r-view/r-object

		class	Member{
		public:
			typedef	enum{
				READ_PATTERN=0,		//	iterate using the pattern to enumerate elements
				READ_EXPRESSION=1,	//	iterate using the pattern in read_expression
				READ_SET=2			//	iterate using the pattern in read_set
			}Iteration;
		private:
			_Read		read;
			ReturnType	type;
			std::string	pattern;	//	when r==read_set or read_expression, pattern specifies the class of said set/expression if one is targeted in particular; otherwise pattern==""
			Iteration	iteration;	//	indicates how to use the pattern to read the elements of the set: as an enumeration of types, as a class of expression, or as an enumeration of types to use for reading subsets
		public:
			std::string	name;	//	unused for anything but set/object/marker classes
			Member(_Read				r,
					const	std::string	m,
					const	std::string	p="",
					const	Iteration	i=READ_PATTERN):read(r),
														name(m),
														pattern(p),
														iteration(i){
						if(read==&Compiler::read_number)		type=NUMBER;
				else	if(read==&Compiler::read_boolean)		type=BOOLEAN;
				else	if(read==&Compiler::read_string)		type=STRING;
				else	if(read==&Compiler::read_sub_system)	type=SUB_SYSTEM_ID;
				else	if(read==&Compiler::read_device)		type=DEVICE_ID;
				else	if(read==&Compiler::read_function)		type=FUNCTION_ID;
				else	if(read==&Compiler::read_expression)	type=ANY;
				else	if(read==&Compiler::read_set)			type=SET;
				else	if(read==&Compiler::read_timestamp)		type=TIMESTAMP;
			}
			Pattern		*get_pattern(Compiler	*c)	const{	return	pattern==""?NULL:&c->objects.find(pattern)->second;	}
			ReturnType	get_return_type()	const{	return	type;	}
			bool		operator	()(Compiler	*c,bool	&indented,bool	enforce,uint16	write_index,uint16	&extent_index,bool	write)	const{
				if(Pattern	*p=get_pattern(c)){
					p->use_as=iteration;
					return	(c->*read)(indented,enforce,p,write_index,extent_index,write);
				}
				return	(c->*read)(indented,enforce,NULL,write_index,extent_index,write);
			}
			bool	used_as_expression()	const{	return	iteration==READ_EXPRESSION;	}
		};

		class	Pattern{
		private:
			bool	has_offset()	const{
				switch(atom.getDescriptor()){
				case	RAtom::OBJECT:
				case	RAtom::MARKER:	return	true;
				default:				return	false;
				}
			}
		public:
			Pattern(ReturnType	t=ANY):type(t){}
			Pattern(RAtom				atom,
					std::string			str_opcode,
					std::vector<Member>	r,
					ReturnType			t=ANY):atom(atom),
											   str_opcode(str_opcode),
											   things_to_read(r),
											   type(t),
											   use_as(Member::READ_PATTERN){}
			bool	is_pattern(Compiler	*c)	const{	return	c->objects.find("ptn")->second.atom==atom;	}
			bool	get_member_index(Compiler	*c,const	std::string	&m,uint16	&index,Pattern	*&p)	const{
				for(uint16	i=0;i<things_to_read.size();++i)
					if(things_to_read[i].name==m){
						index=(has_offset()?i+1:i);	//	i+1: in expressions the lead r-atom is at 0, members start a t 1
						if(things_to_read[i].used_as_expression())	//	the pattern is: [::a-class]
							p=NULL;
						else
							p=things_to_read[i].get_pattern(c);
						return	true;
					}
				return	false;
			}
			std::string	get_member_name(uint32	index){	return	things_to_read[has_offset()?index-1:index].name;	}	//	for decompilation
			uint16		get_member_index(Compiler	*c,const	std::string	&m)	const{	//	used to retrieve sln and res indexes
				for(uint16	i=0;i<things_to_read.size();++i)
					if(things_to_read[i].name==m){	return	(has_offset()?i+1:i);	}
				return	0;	//	0: unused in a view
			}
			ReturnType	get_member_type(const	uint16	index){	return	things_to_read[has_offset()?index-1:index].get_return_type();	}
			RAtom				atom;
			std::string			str_opcode;			//	unused for anything but objects, markers and operators
			std::vector<Member>	things_to_read;
			ReturnType			type;				//	ANY for non-operators
			Member::Iteration	use_as;
		};

		Pattern	_this;	//	specification of the sys-object currently parsed

		//	filled by the preprocessor
		UNORDERED_MAP<std::string,Pattern>	objects;
		UNORDERED_MAP<std::string,Pattern>	sys_objects;

		//	read functions; always try to read nil (typed), a variable, a wildcrad or a tail wildcard first; then try to read the lexical unit; then try to read an expression returning the appropriate type
		//	indented: flag indicating if an indent has been found, meaning that a matching indent will have to be enforced
		//	enforce: set to true when the stream content has to conform with the type xxx in read_xxx
		//	pattern: specifies the elements that shall compose a structure (expression or set)
		//	write_index: the index where the r-atom shall be written (atomic data), or where an internal pointer to a structure shall be written (structural data)
		//	extent_index: the index where to write data belonging to a structure (the internal pointer is written at write_index)
		//	write: when false, no writing in code->data is performed (needed by set_element_count())
		bool	read_any(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);		//	calls all of the functions below
		bool	read_number(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_timestamp(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_boolean(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_string(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_entity(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_sub_system(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_device(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_function(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_expression(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_set(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write);

		//	utility
		bool	read_nil(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_set(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_nb(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_ms(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_sid(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_did(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_fid(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_bl(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_nil_st(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_variable(uint16	write_index,uint16	&extent_index,bool	write,const	Pattern	p);
		bool	read_reference(uint16	write_index,uint16	&extent_index,bool	write,const	ReturnType	t);
		bool	read_wildcard(uint16	write_index,uint16	&extent_index,bool	write);
		bool	read_tail_wildcard(uint16	write_index,uint16	&extent_index,bool	write);

		class	LocalReference{
		public:
			LocalReference(){}
			LocalReference(uint16	i,Pattern	p):index(i),pattern(p){}
			uint16	index;
			Pattern	pattern;
		};

		class	GlobalReference{
		public:
			GlobalReference(){}
			GlobalReference(RView	*v,Pattern	p):view(v),pattern(p){}
			RView	*view;
			Pattern	pattern;
		};

		UNORDERED_MAP<std::string,uint16>	global_reference_indexes;	//	indexes in the pointer array, accessed by labels

		UNORDERED_MAP<std::string,LocalReference>	local_references;	//	labels and variables declared inside objects (cleared before parsing each sys-object)
		UNORDERED_MAP<std::string,GlobalReference>	global_references;	//	labels declared outside sys-objects

		bool	err;	//	set to true when parsing fails in the functions below
		//	all functions below return false (a) upon eof or, (b) when the pattern is not occurring; in both cases, characters are pushed back
		//	sub-lexical units
		bool	comment();															//	pull comments out of the stream
		bool	separator(bool	pushback);											//	blank space or indent
		bool	right_indent(bool	pushback);										//	newline + 3 blank spaces wrt indents.top()
		bool	left_indent(bool	pushback);										//	newline - 3 blank spaces wrt indents.top()
		bool	indent(bool	pushback);												//	newline + same number of 3 blank spaces as given by indents.top()
		bool	expression_begin(bool	&indented);									//	( or right_indent
		bool	expression_end(bool	indented);										//	) or left_indent
		bool	set_begin(bool	&indented);											//	[ or []+right_indent
		bool	set_end(bool	indented);											//	] or left_indent
		bool	symbol_expr(std::string	&s);										//	finds any symbol s; detects trailing blanks, newline and )
		bool	symbol_expr_set(std::string	&s);									//	finds any symbol s; detects trailing blanks, newline, ) and ]
		bool	match_symbol_separator(const	char	*symbol,bool	pushback);	//	matches a symbol followed by a separator/left/right indent; separator/left/right indent is pushed back
		bool	match_symbol(const	char	*symbol,bool	pushback);				//	matches a symbol regardless of what follows
		bool	member(std::string	&s);											//	finds a string possibly followed by ., blanks, newline, ) and ]

		//	lexical units
		bool	nil();
		bool	nil_nb();
		bool	nil_ms();
		bool	nil_sid();
		bool	nil_did();
		bool	nil_fid();
		bool	nil_bl();
		bool	nil_st();
		bool	label(std::string	&l);
		bool	variable(std::string	&v);
		bool	this_();
		bool	self();
		bool	local_reference(uint16	&index,const	ReturnType	t);				//	must conform to t; indicates if the ref is to ba valuated in the value array (in_pattern set to true)
		bool	global_reference(uint16	&index,const	ReturnType	t);				//	no conformance: return type==ANY
		bool	this_indirection(std::vector<uint16>	&v,const	ReturnType	t);	//	ex: this.res
		bool	local_indirection(std::vector<uint16>	&v,const	ReturnType	t);	//	ex: p.res where p is a label/variable declared within the object
		bool	global_indirection(std::vector<uint16>	&v,const	ReturnType	t);	//	ex: p.res where p is a label/variable declared outside the object
		bool	wildcard();
		bool	tail_wildcard();
		bool	timestamp(uint64	&ts);
		bool	str(std::string	&s);
		bool	number(float32	&n);
		bool	hex(uint32	&h);
		bool	boolean(bool	&b);
		bool	object(Pattern	&p);					//	looks first in sys_objects, then in objects
		bool	object(const	Pattern	&p);			//	must conform to p
		bool	sys_object(Pattern	&p);				//	looks only in sys_objects
		bool	sys_object(const	Pattern	&p);		//	must conform to p
		bool	marker(Pattern	&p);
		bool	op(Pattern	&p,const	ReturnType	t);	//	operator; must conform to t
		bool	op(const	Pattern	&p);				//	must conform to p
		bool	function(Pattern	&p);				//	device function
		bool	expression_head(Pattern	&p,const	ReturnType	t);																						//	starts from the first element; arity does not count the head; must conform to t
		bool	expression_head(const	Pattern	&p);																									//	starts from the first element; arity does not count the head; must conform to p
		bool	expression_tail(bool	indented,const	Pattern	&p,uint16	write_index,uint16	&extent_index,bool	write,bool	switch_to_view=false);	//	starts from the second element; must conform to p
		
		//	structural units; check for heading labels
		bool	expression(bool	&indented,const	ReturnType	t,uint16	write_index,uint16	&extent_index,bool	write);	//	must conform to t
		bool	expression(bool	&indented,const	Pattern	&p,uint16	write_index,uint16	&extent_index,bool	write);		//	must conform to p
		bool	set(bool	&indented,uint16	write_index,uint16	&extent_index,bool	write);							//	no conformance, i.e. set of anything
		bool	set(bool	&indented,const	Pattern	&p,uint16	write_index,uint16	&extent_index,bool	write);			//	must conform to p
		
		uint16	set_element_count(bool	indented);	//	returns the number of elements in a set; parses the stream (write set to false) until it finds the end of the set and rewinds (write set back to true)

		//	decompilation
		class	OutStream{	//	allows inserting data at a specified index and right shifting the current content; ex: labels and variables, i.e. when iptrs are discovered and these hold indexes are < read_index and do not point to variables 
		public:
			std::vector<uint16>	code_indexes_to_stream_indexes;
			uint16	code_index;
			std::vector<std::streampos>	positions;
			OutStream(std::ostringstream	*s):stream(s){}
			std::ostringstream	*stream;
			template<typename	T>	OutStream	&push(const	T	&t,uint16	code_index){
				positions.push_back(stream->tellp());
				code_indexes_to_stream_indexes[code_index]=positions.size()-1;
				return	*this<<t;
			}
			OutStream	&push(){	//	to keep adding entries in v without outputing anything (e.g. for wildcards after ::)
				std::streampos	p;
				positions.push_back(p);
				return	*this;
			}
			template<typename	T>	OutStream	&operator	<<(const	T	&t){
				*stream<<t;
				return	*this;
			}
			template<typename	T>	OutStream	&insert(uint32	index,const	T	&t){	//	inserts before code_indexes_to_stream_indexes[index]
				uint16	stream_index=code_indexes_to_stream_indexes[index];
				stream->seekp(positions[stream_index]);
				std::string	s=stream->str().substr(positions[stream_index]);
				*stream<<t;
				std::streamoff	offset=stream->tellp()-positions[stream_index];
				*stream<<s;
				for(uint16	i=stream_index+1;i<positions.size();++i)	//	right-shift
					positions[i]+=offset;
				return	*this;
			}
		};
		OutStream	*out_stream;
		RView		*in_view;
		Code		*in_code;
		uint16		indents;		//	in chars
		bool		closing_set;	//	set after writing the last element of a set: any element in an expression finding closing_set will indent and set closing_set to false
		uint16								last_variable_ID;		//	an integer representing the order of referencing of a variable/label in the code
		UNORDERED_MAP<uint16,std::string>	variable_names;			//	in the form vxxx where xxx is a variable_ID
		std::string							get_variable_name(uint16	index,bool	postfix);	//	associates iptr/vptr indexes to names; inserts them in out_stream if necessary; when postfix==true, a trailing ':' is added

		//	filled by the preprocessor
		std::vector<std::string>	object_names;		//	objects and sys-objects; does not include set classes
		std::vector<std::string>	operator_names;
		std::vector<std::string>	function_names;
		std::vector<Pattern>		objects_by_opcodes;	//	patterns indexed by opcodes; used to retrieve member names; registers all classes (incl. set classes)

		void	write_indent(uint16	i);

		void	write_expression_head(uint16	read_index);							//	decodes the leading atom of an expression
		void	write_expression_tail(uint16	read_index,bool	switch_to_view=false);	//	decodes the elements of an expression following the head
		void	write_expression(uint16	read_index);
		void	write_set(uint16	read_index);
		void	write_any(uint16	read_index,bool	&after_tail_wildcard);				//	decodes any element in an expression or a set

		//	main functions; return false when there is an error
		bool	interpretDirective();	//	preprocessor
		bool	compileObject();		//	compiler
	public:
		Compiler();
		~Compiler();
		bool	compile(std::istream			*stream,	//	if an ifstream, stream must be open
						sdk::Array<RView,16>	*storage,	//	shall be an image instead
						std::string				*error);	//	set when compile() fails, e.g. returns false
		void	decompile(RView					*view,
							std::ostringstream	*stream);	//	we also need an overload that decompiles an image instead of a single view
	};
}


#endif