#include	"compiler.h"
#include	<string.h>


#define	NEWLINE	'\n'

#define	DEBUG

#ifdef	DEBUG
#define	OUTPUT	CompilerOutput()
#elif
#define	OUTPUT	NoStream()
#endif

#define	NO_PREPROCESSOR


namespace	replicode{

	static	bool	Output=true;

	class	NoStream:
	public	std::ostream{
	public:
		NoStream():std::ostream(NULL){}
		template<typename	T>	NoStream&	operator	<<(T	&t){
			return	*this;
		}
	};

	class	CompilerOutput:
	public	std::ostream{
	public:
		CompilerOutput():std::ostream(NULL){}
		template<typename	T>	std::ostream&	operator	<<(T	&t){
			if(Output)
				return	std::cout<<t;
			return	*this;
		}
	};

	Compiler::Compiler():out_stream(NULL){

		std::vector<Member>	r_expr;
		objects[std::string("expr")]=Pattern(RAtom::Object(0,0),"-generic-expr-",r_expr);	//	to read expression in sets, special case: see read_expression()

		uint16	object_opcode=1;	//	shared with sys_objects
		uint16	function_opcode=0;
		uint16	operator_opcode=0;

#ifdef	NO_PREPROCESSOR
		//	everything below shall be filled by the preprocessor
		
		//	objects
		//	ptn
		std::vector<Member>	r_expr_set_of_expr;
		r_expr_set_of_expr.push_back(Member(&Compiler::read_expression,"skel"));
		r_expr_set_of_expr.push_back(Member(&Compiler::read_set,"guards","expr",Member::READ_EXPRESSION));	//	reads a set of expressions

		object_names.resize(object_opcode+1);
		object_names[object_opcode]="ptn";
		objects_by_opcodes.resize(2);
		objects_by_opcodes[object_opcode]=objects[std::string("ptn")]=Pattern(RAtom::Object(object_opcode,2),"ptn",r_expr_set_of_expr);
		++object_opcode;

		//	in_sec
		std::vector<Member>	r_set_of_ptn_set_of_expr_set_of_expr;
		r_set_of_ptn_set_of_expr_set_of_expr.push_back(Member(&Compiler::read_set,"inputs","ptn",Member::READ_EXPRESSION));	//	reads a set of patterns
		r_set_of_ptn_set_of_expr_set_of_expr.push_back(Member(&Compiler::read_set,"timings","expr",Member::READ_EXPRESSION));
		r_set_of_ptn_set_of_expr_set_of_expr.push_back(Member(&Compiler::read_set,"guards","expr",Member::READ_EXPRESSION));
		objects_by_opcodes.resize(objects_by_opcodes.size()+1);
		objects_by_opcodes[object_opcode]=objects[std::string("in_sec")]=Pattern(RAtom::SetObject(object_opcode,3),"-in_sec-",r_set_of_ptn_set_of_expr_set_of_expr);
		++object_opcode;

		//	prod_sub_sec
		std::vector<Member>	r_set_of_expr_set_of_expr;
		r_set_of_expr_set_of_expr.push_back(Member(&Compiler::read_set,"guards","expr",Member::READ_EXPRESSION));
		r_set_of_expr_set_of_expr.push_back(Member(&Compiler::read_set,"prods","expr",Member::READ_EXPRESSION));

		objects_by_opcodes.resize(objects_by_opcodes.size()+1);
		objects_by_opcodes[object_opcode]=objects[std::string("prod_sub_sec")]=Pattern(RAtom::SetObject(object_opcode,2),"-prod_sub_sec-",r_set_of_expr_set_of_expr);
		++object_opcode;

		//	head
		std::vector<Member>	r_set_of_ptn_insec_set_of_expr;
		r_set_of_ptn_insec_set_of_expr.push_back(Member(&Compiler::read_set,"tpl","ptn",Member::READ_EXPRESSION));
		r_set_of_ptn_insec_set_of_expr.push_back(Member(&Compiler::read_set,"inputs","in_sec",Member::READ_PATTERN));
		r_set_of_ptn_insec_set_of_expr.push_back(Member(&Compiler::read_set,"prods","prod_sub_sec",Member::READ_SET));

		objects_by_opcodes.resize(objects_by_opcodes.size()+1);
		objects_by_opcodes[object_opcode]=objects[std::string("head")]=Pattern(RAtom::SetObject(object_opcode,3),"-head-",r_set_of_ptn_insec_set_of_expr);
		++object_opcode;

		//	cmd_head
		std::vector<Member>	r_fid_did_set;
		r_fid_did_set.push_back(Member(&Compiler::read_function,"function"));
		r_fid_did_set.push_back(Member(&Compiler::read_device,"device"));
		r_fid_did_set.push_back(Member(&Compiler::read_set,"args"));

		objects_by_opcodes.resize(objects_by_opcodes.size()+1);
		objects_by_opcodes[object_opcode]=objects[std::string("cmd_head")]=Pattern(RAtom::SetObject(object_opcode,3),"-cmd-head-",r_fid_did_set);
		++object_opcode;
		
		//	sys-objects
		//	pgm
		std::vector<Member>	r_pgm;
		r_pgm.push_back(Member(&Compiler::read_set,"head","head",Member::READ_PATTERN));
		r_pgm.push_back(Member(&Compiler::read_number,"act"));
		r_pgm.push_back(Member(&Compiler::read_timestamp,"tsc"));
		r_pgm.push_back(Member(&Compiler::read_number,"csm"));
		r_pgm.push_back(Member(&Compiler::read_number,"sig"));
		r_pgm.push_back(Member(&Compiler::read_number,"nfc"));
		r_pgm.push_back(Member(&Compiler::read_number,"nfr"));
		r_pgm.push_back(Member(&Compiler::read_number,"res"));
		r_pgm.push_back(Member(&Compiler::read_number,"sln"));
		r_pgm.push_back(Member(&Compiler::read_timestamp,"ijt"));
		r_pgm.push_back(Member(&Compiler::read_timestamp,"ejt"));
		r_pgm.push_back(Member(&Compiler::read_sub_system,"org"));
		r_pgm.push_back(Member(&Compiler::read_sub_system,"hst"));
		r_pgm.push_back(Member(&Compiler::read_set,"mks"));

		object_names.resize(object_opcode+1);
		object_names[object_opcode]="pgm";
		objects_by_opcodes.resize(objects_by_opcodes.size()+1);
		objects_by_opcodes[object_opcode]=sys_objects[std::string("pgm")]=Pattern(RAtom::Object(object_opcode,14),"pgm",r_pgm);
		++object_opcode;

		//	cmd
		std::vector<Member>	r_cmd;
		r_cmd.push_back(Member(&Compiler::read_set,"head","cmd_head",Member::READ_PATTERN));
		r_cmd.push_back(Member(&Compiler::read_number,"res"));
		r_cmd.push_back(Member(&Compiler::read_number,"sln"));
		r_cmd.push_back(Member(&Compiler::read_timestamp,"ijt"));
		r_cmd.push_back(Member(&Compiler::read_timestamp,"ejt"));
		r_cmd.push_back(Member(&Compiler::read_sub_system,"org"));
		r_cmd.push_back(Member(&Compiler::read_sub_system,"hst"));
		r_cmd.push_back(Member(&Compiler::read_set,"mks"));

		object_names.resize(object_opcode+1);
		object_names[object_opcode]="cmd";
		objects_by_opcodes.resize(objects_by_opcodes.size()+1);
		objects_by_opcodes[object_opcode]=sys_objects[std::string("cmd")]=Pattern(RAtom::Object(object_opcode,8),"cmd",r_cmd);
		++object_opcode;

		//	mk.position: non-standard (i.e. does not belong to std.replicode), only for testing
		std::vector<Member>	r_mk_position;
		r_mk_position.push_back(Member(&Compiler::read_any,"object"));
		r_mk_position.push_back(Member(&Compiler::read_number,"position"));
		r_mk_position.push_back(Member(&Compiler::read_number,"res"));
		r_mk_position.push_back(Member(&Compiler::read_number,"sln"));
		r_mk_position.push_back(Member(&Compiler::read_timestamp,"ijt"));
		r_mk_position.push_back(Member(&Compiler::read_timestamp,"ejt"));
		r_mk_position.push_back(Member(&Compiler::read_sub_system,"org"));
		r_mk_position.push_back(Member(&Compiler::read_sub_system,"hst"));
		r_mk_position.push_back(Member(&Compiler::read_set,"mks"));

		object_names.resize(object_opcode+1);
		object_names[object_opcode]="mk.position";
		objects_by_opcodes.resize(objects_by_opcodes.size()+1);
		objects_by_opcodes[object_opcode]=sys_objects[std::string("mk.position")]=Pattern(RAtom::Object(object_opcode,9),"mk.position",r_mk_position);
		++object_opcode;

		//	mk.last_known: non-standard
		std::vector<Member>	r_mk_last_known;
		r_mk_last_known.push_back(Member(&Compiler::read_any,"event"));
		r_mk_last_known.push_back(Member(&Compiler::read_number,"res"));
		r_mk_last_known.push_back(Member(&Compiler::read_number,"sln"));
		r_mk_last_known.push_back(Member(&Compiler::read_timestamp,"ijt"));
		r_mk_last_known.push_back(Member(&Compiler::read_timestamp,"ejt"));
		r_mk_last_known.push_back(Member(&Compiler::read_sub_system,"org"));
		r_mk_last_known.push_back(Member(&Compiler::read_sub_system,"hst"));
		r_mk_last_known.push_back(Member(&Compiler::read_set,"mks"));

		object_names.resize(object_opcode+1);
		object_names[object_opcode]="mk.last_known";
		objects_by_opcodes.resize(objects_by_opcodes.size()+1);
		objects_by_opcodes[object_opcode]=sys_objects[std::string("mk.last_known")]=Pattern(RAtom::Object(object_opcode,8),"mk.last_known",r_mk_last_known);
		++object_opcode;

		//	operators & related objects
		//	red_prod
		std::vector<Member>	r_set_of_ptn_set_of_expr;
		r_set_of_ptn_set_of_expr.push_back(Member(&Compiler::read_expression,"","ptn",Member::READ_PATTERN));	//	pattern
		r_set_of_ptn_set_of_expr.push_back(Member(&Compiler::read_set,"","expr",Member::READ_EXPRESSION));		//	productions

		objects_by_opcodes.resize(objects_by_opcodes.size()+1);
		objects_by_opcodes[object_opcode]=objects[std::string("red_prod")]=Pattern(RAtom::Object(object_opcode,2),"",r_set_of_ptn_set_of_expr);
		++object_opcode;

		//	utilities
		std::vector<Member>	r_any;
		r_any.push_back(Member(&Compiler::read_any,""));

		std::vector<Member>	r_any_any;
		r_any_any.push_back(Member(&Compiler::read_any,""));
		r_any_any.push_back(Member(&Compiler::read_any,""));

		std::vector<Member>	r_any_set;
		r_any_set.push_back(Member(&Compiler::read_any,""));
		r_any_set.push_back(Member(&Compiler::read_set,""));

		std::vector<Member>	r_nb_nb;
		r_nb_nb.push_back(Member(&Compiler::read_number,""));
		r_nb_nb.push_back(Member(&Compiler::read_number,""));

		std::vector<Member>	r_set_set_of_redprod;
		r_set_set_of_redprod.push_back(Member(&Compiler::read_set,""));
		r_set_set_of_redprod.push_back(Member(&Compiler::read_set,"","red_prod",Member::READ_SET));

		//	operators
		objects[std::string("add")]=Pattern(RAtom::Operator(operator_opcode++,2),"add",r_any_any,ANY);
		operator_names.push_back("add");
		objects[std::string("sub")]=Pattern(RAtom::Operator(operator_opcode++,2),"sub",r_any_any,ANY);
		operator_names.push_back("sub");
		objects[std::string("mul")]=Pattern(RAtom::Operator(operator_opcode++,2),"mul",r_nb_nb,NUMBER);
		operator_names.push_back("mul");
		objects[std::string("div")]=Pattern(RAtom::Operator(operator_opcode++,2),"div",r_nb_nb,NUMBER);
		operator_names.push_back("div");
		objects[std::string("gtr")]=Pattern(RAtom::Operator(operator_opcode++,2),"gtr",r_any_any,BOOLEAN);
		operator_names.push_back("gtr");
		objects[std::string("lse")]=Pattern(RAtom::Operator(operator_opcode++,2),"lse",r_any_any,BOOLEAN);
		operator_names.push_back("lse");
		objects[std::string("lsr")]=Pattern(RAtom::Operator(operator_opcode++,2),"lsr",r_any_any,BOOLEAN);
		operator_names.push_back("lsr");
		objects[std::string("gte")]=Pattern(RAtom::Operator(operator_opcode++,2),"gte",r_any_any,BOOLEAN);
		operator_names.push_back("gte");
		objects[std::string("equ")]=Pattern(RAtom::Operator(operator_opcode++,2),"equ",r_any_any,BOOLEAN);
		operator_names.push_back("equ");
		objects[std::string("neq")]=Pattern(RAtom::Operator(operator_opcode++,2),"neq",r_any_any,BOOLEAN);
		operator_names.push_back("neq");
		objects[std::string("syn")]=Pattern(RAtom::Operator(operator_opcode++,1),"syn",r_any,ANY);
		operator_names.push_back("syn");
		objects[std::string("red")]=Pattern(RAtom::Operator(operator_opcode++,2),"red",r_set_set_of_redprod,SET);
		operator_names.push_back("red");
		objects[std::string("ins")]=Pattern(RAtom::Operator(operator_opcode++,2),"ins",r_any_set,ANY);
		operator_names.push_back("ins");

		//	functions
		objects[std::string("_inj")]=Pattern(RAtom::DeviceFunction(function_opcode++),"_inj",r_any);
		function_names.push_back("_inj");
		objects[std::string("_mod")]=Pattern(RAtom::DeviceFunction(function_opcode++),"_mod",r_any);
		function_names.push_back("_mod");
		objects[std::string("_set")]=Pattern(RAtom::DeviceFunction(function_opcode++),"_set",r_any);
		function_names.push_back("_set");
#endif
	}

	Compiler::~Compiler(){

		if(out_stream)
			delete	out_stream;
	}

	Compiler::State	Compiler::save_state(){

		State	s(this);
		return	s;
	}

	void	Compiler::restore_state(State	s){

		in_stream->seekg(s.stream_ptr);
		state=s;
	}

	void	Compiler::set_error(std::string	s){

		if(!err	&&	Output){

			err=true;
			*error=s;
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	bool	Compiler::compile(std::istream	*stream,sdk::Array<RView,16>	*storage,std::string	*error){

		this->in_stream=stream;
		this->storage=storage;
		this->error=error;
		this->out_view=0;
		this->err=false;

		while(!in_stream->eof()){

			switch(in_stream->peek()){
			case	'!':
				if(!interpretDirective())
					return	false;
				break;
			default:
				if(in_stream->eof())
					return	true;
				if(!compileObject())
					return	false;
				break;
			}
			char	c=(char)in_stream->get();
			if(c!=-1)
				in_stream->putback(c);
		}
		return	true;
	}

	bool	Compiler::interpretDirective(){

		return	true;	//	TODO
	}

	bool	Compiler::compileObject(){

		static	uint16	ViewIndex=0;

		out_view=&(*storage)[ViewIndex++];

		local_references.clear();

		Pattern	p;
		bool	indented=false;
		bool	lbl=false;
		std::string	l;
		while(indent(false));
		if(label(l)){

			lbl=true;
			OUTPUT<<"lbl_"<<l<<":";
		}
		if(!expression_begin(indented)){

			if(lbl)
				set_error(" error: label not followed by an expression");
			else
				set_error("syntax error: missing expression opening");
			return	false;
		}
		indent(false);
		if(!sys_object(p)){

			set_error("error: undefined sys-object");
			return	false;
		}
		out_view->setResilienceIndex(p.get_member_index(this,"res"));
		out_view->setSaliencyIndex(p.get_member_index(this,"sln"));
		if(lbl)
			global_references[l]=GlobalReference(out_view,p);
		OUTPUT<<p.str_opcode;
		out_view->setSID(p.atom);
		RObject	*object=NULL;
		if(p.atom.getDescriptor()!=RAtom::MARKER){

			out_view->setObject(object=new	RObject());
			out_code=object->code();
		}else
			out_code=out_view->code();
		_this=p;
		out_code->data[0]=p.atom;
		if(p.atom.getAtomCount()){

			if(!right_indent(true)){

				if(!separator(false)){

					set_error("syntax error: missing separator/right_indent after head");
					return	false;
				}
			}else
				OUTPUT<<" ";
			uint16	extent_index=2;	//	first element of an object must be a structure (translating into one iptr)
			if(!expression_tail(indented,p,1,extent_index,true,p.atom.getDescriptor()!=RAtom::MARKER))
				return	false;
		}
		OUTPUT<<"\n\n";
#ifdef	DEBUG
		if(object){

			OUTPUT<<"OBJECT\n";
			object->code()->trace();
			OUTPUT<<"\n";
		}
		OUTPUT<<"VIEW\n";
		out_view->code()->trace();
		OUTPUT<<"\n\n";
#endif
		return	true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	bool	Compiler::comment(){

		std::streampos	i=in_stream->tellg();
		bool	started=false;
		bool	continuation=false;	//	continuation mark detected
		bool	period=false;		//	to detect 2 subsequent '.'
		while(!in_stream->eof()){

			switch(char	c=(char)in_stream->get()){
			case	';':
				if(!started)
					started=true;
				break;
			case	'.':
				if(!started)
					goto	return_false;
				if(continuation){
				
					set_error(" syntax error: ...");
					goto	return_false;
				}
				if(period)
					continuation=true;
				period=true;
				break;
			case	NEWLINE:
				if(!continuation){

					in_stream->putback(c);
					return	true;
				}
				continuation=period=false;
				break;
			default:
				if(!started)
					goto	return_false;
				period=false;
				break;
			}
		}
return_false:
		in_stream->seekg(i);
		in_stream->clear();
		return	false;
	}

	bool	Compiler::indent(bool	pushback){

		comment();
		std::string	s;
		s+=NEWLINE;
		for(uint16	j=0;j<3*state.indents;j++)
			s+=' ';
		return	match_symbol(s.c_str(),pushback);
	}

	bool	Compiler::right_indent(bool	pushback){	//	no look ahead when pushback==true

		comment();
		if(pushback){

			if(state.right_indents_ahead)
				return	true;
			std::string	s;
			s+=NEWLINE;
			for(uint16	j=0;j<3*(state.indents+1);j++)
				s+=' ';
			return	match_symbol(s.c_str(),true);
		}
		if(state.right_indents_ahead){

			state.indents++;
			state.right_indents_ahead--;
			return	true;
		}
		std::string	s;
		s+=NEWLINE;
		for(uint16	j=0;j<3*(state.indents+1);j++)
			s+=' ';
		if(!match_symbol(s.c_str(),false))
			return	false;
		state.indents++;
		s="   ";
		while(match_symbol(s.c_str(),false))	//	look ahead for more indents
			state.right_indents_ahead++;
		return	true;
	}

	bool	Compiler::left_indent(bool	pushback){	//	no look ahead when pushback==true

		comment();
		if(indent(true))
			return	false;
		if(pushback){

			if(state.left_indents_ahead)
				return	true;
			std::string	s;
			s+=NEWLINE;
			for(uint16	j=0;j<3*(state.indents-1);j++)
				s+=' ';
			return	match_symbol(s.c_str(),true);
		}
		if(state.left_indents_ahead){

			if(state.indents)
				state.indents--;
			state.left_indents_ahead--;
			return	true;
		}
		std::string	s;
		s+=NEWLINE;
		if(!match_symbol(s.c_str(),false))
			return	false;
		uint16	expected=state.indents-1;
		if(expected<=0){

			if(state.indents)
				state.indents--;
			return	true;
		}
		state.left_indents_ahead=expected;	//	look ahead for more indents
		s="   ";
		for(uint16	j=0;j<expected;j++){

			if(match_symbol(s.c_str(),false))
				state.left_indents_ahead--;
		}
		if(state.indents)
			state.indents--;
		return	true;
	}

	bool	Compiler::separator(bool	pushback){

		if(indent(pushback)){

			if(!pushback)
				OUTPUT<<" ";
			return	true;
		}
		char	c=(char)in_stream->get();
		if(c==' '){

			if(pushback)
				in_stream->putback(c);
			else
				OUTPUT<<" ";
			return	true;
		}
		in_stream->clear();
		in_stream->putback(c);
		return	false;
	}

	bool	Compiler::symbol_expr(std::string	&s){

		std::streampos	i=in_stream->tellg();
		uint16	count=0;
		while(!in_stream->eof()){

			switch(char	c=(char)in_stream->get()){
			case	':':
			case	' ':
			case	NEWLINE:
			case	';':
			case	')':
				if(count){

					in_stream->putback(c);
					return	true;
				}
			case	'(':
			case	'[':
			case	']':
			case	'.':
				in_stream->seekg(i);
				return	false;
			default:
				count++;
				s+=c;
				break;
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::symbol_expr_set(std::string	&s){

		std::streampos	i=in_stream->tellg();
		uint16	count=0;
		while(!in_stream->eof()){

			switch(char	c=(char)in_stream->get()){
			case	' ':
			case	NEWLINE:
			case	';':
			case	')':
			case	']':
				if(count){

					in_stream->putback(c);
					return	true;
				}
			case	'(':
			case	'[':
			case	'.':
			case	':':
				in_stream->seekg(i);
				return	false;
			default:
				count++;
				s+=c;
				break;
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::match_symbol_separator(const	char	*symbol,bool	pushback){

		if(match_symbol(symbol,pushback)){

			if(separator(true)	||	right_indent(true)	||	left_indent(true))
				return	true;
			char	c=(char)in_stream->peek();
			if(c==')'	||	c==']')					
				return	true;
		}
		return	false;
	}

	bool	Compiler::match_symbol(const	char	*symbol,bool	pushback){

		std::streampos	i=in_stream->tellg();
		for(uint32	j=0;j<strlen(symbol);j++){
			
			if(in_stream->eof()	||	((char)in_stream->get())!=symbol[j]){

				in_stream->clear();
				in_stream->seekg(i);
				return	false;
			}
		}
		if(pushback)
			in_stream->seekg(i);
		return	true;
	}

	bool	Compiler::member(std::string	&s){

		std::streampos	i=in_stream->tellg();
		s="";
		uint16	count=0;
		while(!in_stream->eof()){

			switch(char	c=(char)in_stream->get()){
			case	' ':
			case	NEWLINE:
			case	';':
			case	')':
			case	']':
			case	'.':
				if(count){

					in_stream->putback(c);
					return	true;
				}
			case	'(':
			case	'[':
			case	':':
				in_stream->seekg(i);
				return	false;
			default:
				count++;
				s+=c;
				break;
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::expression_begin(bool	&indented){

		if(right_indent(false)){

			indented=true;
			return	true;
		}
		std::streampos	i=in_stream->tellg();
		if(indent(false)){

			char	c=(char)in_stream->get();
			if(c=='(')
				return	true;
			in_stream->clear();
		}
		in_stream->seekg(i);
		char	c=(char)in_stream->get();
		if(c=='(')
			return	true;
		in_stream->clear();
		in_stream->putback(c);
		return	false;
	}

	bool	Compiler::expression_end(bool	indented){

		if(indented)
			return	left_indent(false);
		std::streampos	i=in_stream->tellg();
		if(indent(false)){

			char	c=(char)in_stream->get();
			if(c==')')
				return	true;
			in_stream->clear();
		}
		in_stream->seekg(i);
		char	c=(char)in_stream->get();
		if(c==')')
			return	true;
		in_stream->clear();
		in_stream->putback(c);
		return	false;
	}

	bool	Compiler::set_begin(bool	&indented){

		std::streampos	i=in_stream->tellg();
		if(match_symbol("[]",false)){

			if(right_indent(false)){

				indented=true;
				return	true;
			}else{

				set_error(" syntax error: [] not followed by indent");
				return	false;
			}
		}
		in_stream->seekg(i);
		if(indent(false)){

			char	c=(char)in_stream->get();
			if(c=='[')
				return	true;
			in_stream->clear();
		}
		in_stream->seekg(i);
		char	c=(char)in_stream->get();
		if(c=='[')
			return	true;
		in_stream->clear();
		in_stream->putback(c);
		return	false;
	}

	bool	Compiler::set_end(bool	indented){

		if(indented)
			return	left_indent(false);
		std::streampos	i=in_stream->tellg();
		if(indent(false)){

			char	c=(char)in_stream->get();
			if(c==']')
				return	true;
			in_stream->clear();
		}
		in_stream->seekg(i);
		char	c=(char)in_stream->get();
		if(c==']')
			return	true;
		in_stream->clear();
		in_stream->putback(c);
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	bool	Compiler::nil(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("nil",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::nil_nb(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("|nb",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::nil_ms(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("|ms",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::nil_sid(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("|sid",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::nil_did(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("|did",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::nil_fid(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("|fid",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::nil_bl(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("|bl",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::nil_st(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("|st",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::label(std::string	&l){

		std::streampos	i=in_stream->tellg();
		if(symbol_expr(l)	&&	(char)in_stream->get()==':')
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::variable(std::string	&l){

		std::streampos	i=in_stream->tellg();
		if(symbol_expr(l)	&&	(char)in_stream->get()==':'){

			in_stream->seekg(i);
			std::string	_l=l+':';
			if(match_symbol_separator(_l.c_str(),false))
				return	true;
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::this_(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("this",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::self(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("self",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::local_reference(uint16	&index,const	ReturnType	t){

		std::streampos	i=in_stream->tellg();
		std::string	r;
		if(symbol_expr_set(r)){

			UNORDERED_MAP<std::string,LocalReference>::iterator	it=local_references.find(r);
			if(it!=local_references.end()	&&	(t==ANY	||	(t!=ANY	&&	it->second.pattern.type==t))){

				index=it->second.index;
				return	true;
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::global_reference(uint16	&index,const	ReturnType	t){

		std::streampos	i=in_stream->tellg();
		std::string	r;
		if(symbol_expr_set(r)){

			UNORDERED_MAP<std::string,GlobalReference>::iterator	it=global_references.find(r);
			if(it!=global_references.end()	&&	(t==ANY	||	(t!=ANY	&&	it->second.pattern.type==t))){

				for(uint16	j=0;j<out_code->pointers.count();++j)
					if(out_code->pointers[j]==it->second.view){	//	the view has already been referenced

						index=j;
						return	true;
					}
				out_code->pointers.pushBack(it->second.view);	//	add new reference to the view
				index=out_code->pointers.count()-1;
				return	true;
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::this_indirection(std::vector<uint16>	&v,const	ReturnType	t){

		std::streampos	i=in_stream->tellg();
		if(match_symbol("this.",false)){

			Pattern		*p=&_this;
			Pattern		*_p;
			std::string	m;
			uint16		index;
			ReturnType	type;
			while(member(m)){

				if(!p->get_member_index(this,m,index,_p)){

					set_error(" error: "+m+" is not a member of "+p->str_opcode);
					break;
				}
				type=p->get_member_type(index);
				v.push_back(index);
				char	c=(char)in_stream->get();
				if(c=='.'){

					if(!_p){

						set_error(" error: "+m+" is not a structure");
						break;
					}
					p=_p;
				}else{

					if(t==ANY	||	(t!=ANY	&&	type==t)){
					
						in_stream->putback(c);
						return	true;
					}
					break;
				}
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::local_indirection(std::vector<uint16>	&v,const	ReturnType	t){

		std::streampos	i=in_stream->tellg();
		std::string	m;
		std::string	path="";
		Pattern		*p;
		if(member(m)	&&	(char)in_stream->get()=='.'){	//	first m is a reference to a label or a variable

			uint16		index;
			ReturnType	type;
			UNORDERED_MAP<std::string,LocalReference>::iterator	it=local_references.find(m);
			if(it!=local_references.end()){

				index=it->second.index;
				v.push_back(index);
				p=&it->second.pattern;
				Pattern	*_p;
				while(member(m)){

					if(!p->get_member_index(this,m,index,_p)){

						set_error(" error: "+m+" is not a member of "+p->str_opcode);
						break;
					}
					type=p->get_member_type(index);
					v.push_back(index);
					path+='.';
					path+=m;
					char	c=(char)in_stream->get();
					if(c=='.'){

						if(!_p){

							set_error(" error: "+path+" is not an addressable structure");
							break;
						}
						p=_p;
					}else{

						if(t==ANY	||	(t!=ANY	&&	type==t)){
						
							in_stream->putback(c);
							return	true;
						}
						break;
					}
				}
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::global_indirection(std::vector<uint16>	&v,const	ReturnType	t){

		std::streampos	i=in_stream->tellg();
		std::string	m;
		Pattern		*p;
		if(member(m)	&&	(char)in_stream->get()=='.'){	//	first m is a reference

			uint16		index;
			ReturnType	type;
			UNORDERED_MAP<std::string,GlobalReference>::iterator	it=global_references.find(m);
			if(it!=global_references.end()){

				uint16	j;
				for(j=0;j<out_code->pointers.count();++j)
					if(out_code->pointers[j]==it->second.view){	//	the view has already been referenced

						index=j;
						break;
					}
				if(j==out_code->pointers.count()){
						
					out_code->pointers.pushBack(it->second.view);	//	add new reference to the view
					index=out_code->pointers.count()-1;
				}
				v.push_back(index);
				p=&it->second.pattern;
				Pattern	*_p;
				bool	first_member=true;
				while(member(m)){

					if(!p->get_member_index(this,m,index,_p)){

						set_error(" error: "+m+" is not a member of "+p->str_opcode);
						break;
					}
					type=p->get_member_type(index);
					if(first_member	&&	index==0)	//	indicates the first member; store in the RObject, after the leading atom, hence index=1
						index=1;
					first_member=false;
					v.push_back(index);
					char	c=(char)in_stream->get();
					if(c=='.'){

						if(!_p){

							set_error(" error: "+m+" is not a structure");
							break;
						}
						p=_p;
					}else{

						if(t==ANY	||	(t!=ANY	&&	type==t)){
						
							in_stream->putback(c);
							return	true;
						}
						break;
					}
				}
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::wildcard(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator(":",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::tail_wildcard(){

		std::streampos	i=in_stream->tellg();
		if(match_symbol("::",false)){
			
			if(left_indent(true)){

				state.no_arity_check=true;
				return	true;
			}
			char	c=(char)in_stream->peek();
			if(c==')'	||	c==']'){

				state.no_arity_check=true;
				return	true;
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::number(float32	&n){

		std::streampos	i=in_stream->tellg();
		if(match_symbol("0x",true)){

			in_stream->clear();
			in_stream->seekg(i);
			return	false;
		}
		*in_stream>>std::dec>>n;
		if(in_stream->fail()	||	in_stream->eof()){

			in_stream->clear();
			in_stream->seekg(i);
			return	false;
		}
		return	true;
	}

	bool	Compiler::hex(uint32	&h){

		std::streampos	i=in_stream->tellg();
		if(!match_symbol("0x",false)){

			in_stream->clear();
			in_stream->seekg(i);
			return	false;
		}
		*in_stream>>std::hex>>h;
		if(in_stream->fail()	||	in_stream->eof()){

			in_stream->clear();
			in_stream->seekg(i);
			return	false;
		}
		return	true;
	}

	bool	Compiler::boolean(bool	&b){

		std::streampos	i=in_stream->tellg();
		if(match_symbol_separator("true",false))
			return	true;
		if(match_symbol_separator("false",false))
			return	true;
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::timestamp(uint64	&ts){

		std::streampos	i=in_stream->tellg();
		if(match_symbol("0x",true)){

			in_stream->clear();
			in_stream->seekg(i);
			return	false;
		}
		*in_stream>>std::dec>>ts;
		if(in_stream->fail()	||	in_stream->eof()){

			in_stream->clear();
			in_stream->seekg(i);
			return	false;
		}
		return	true;
	}

	bool	Compiler::str(std::string	&s){

		std::streampos	i=in_stream->tellg();
		uint16	count=0;
		bool	started=false;
		while(!in_stream->eof()){

			switch(char	c=(char)in_stream->get()){
			case	'"':
				if(!count){

					started=true;
					break;
				}
				return	true;
			default:
				if(!started){

					in_stream->clear();
					in_stream->seekg(i);
					return	false;
				}
				count++;
				s+=c;
				break;
			}
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::object(Pattern	&p){

		if(sys_object(p))
			return	true;
		std::streampos	i=in_stream->tellg();
		std::string	s;
		if(!symbol_expr(s)){

			in_stream->seekg(i);
			return	false;
		}
		UNORDERED_MAP<std::string,Pattern>::const_iterator	it=objects.find(s);
		if(it==objects.end()){

			in_stream->seekg(i);
			return	false;
		}
		p=it->second;
		return	true;
	}

	bool	Compiler::object(const	Pattern	&p){

		if(sys_object(p))
			return	true;
		std::streampos	i=in_stream->tellg();
		if(!match_symbol_separator(p.str_opcode.c_str(),false)){

			in_stream->seekg(i);
			return	false;
		}
		return	true;
	}

	bool	Compiler::sys_object(Pattern	&p){

		std::streampos	i=in_stream->tellg();
		std::string	s;
		if(!symbol_expr(s)){

			in_stream->seekg(i);
			return	false;
		}
		UNORDERED_MAP<std::string,Pattern>::const_iterator	it=sys_objects.find(s);
		if(it==sys_objects.end()){

			in_stream->seekg(i);
			return	false;
		}
		p=it->second;
		return	true;
	}

	bool	Compiler::sys_object(const	Pattern	&p){

		std::streampos	i=in_stream->tellg();
		if(!match_symbol_separator(p.str_opcode.c_str(),false)){

			in_stream->seekg(i);
			return	false;
		}
		return	true;
	}

	bool	Compiler::marker(Pattern	&p){

		std::streampos	i=in_stream->tellg();
		if(!match_symbol("mk.",false)){

			in_stream->seekg(i);
			return	false;
		}
		std::string	s;
		if(!symbol_expr(s)){

			in_stream->seekg(i);
			return	false;
		}
		UNORDERED_MAP<std::string,Pattern>::const_iterator	it=sys_objects.find("mk."+s);
		if(it==sys_objects.end()){

			in_stream->seekg(i);
			return	false;
		}
		p=it->second;
		return	true;
	}

	bool	Compiler::op(Pattern	&p,const	ReturnType	t){	//	return true if type matches t or ANY

		std::streampos	i=in_stream->tellg();
		std::string	s;
		if(!symbol_expr(s)){

			in_stream->seekg(i);
			return	false;
		}
		UNORDERED_MAP<std::string,Pattern>::const_iterator	it=objects.find(s);
		if(it==objects.end()	|| (t!=ANY	&&	it->second.type!=ANY	&&	it->second.type!=t)){

			in_stream->seekg(i);
			return	false;
		}
		p=it->second;
		return	true;
	}

	bool	Compiler::op(const	Pattern	&p){

		std::streampos	i=in_stream->tellg();
		if(!match_symbol_separator(p.str_opcode.c_str(),false)){

			in_stream->seekg(i);
			return	false;
		}
		return	true;
	}

	bool	Compiler::function(Pattern	&p){

		std::streampos	i=in_stream->tellg();
		std::string	s;
		if(!symbol_expr(s)){

			in_stream->seekg(i);
			return	false;
		}
		UNORDERED_MAP<std::string,Pattern>::const_iterator	it=objects.find(s);
		if(it==objects.end()){

			in_stream->seekg(i);
			return	false;
		}
		p=it->second;
		return	true;
	}

	bool	Compiler::expression_head(Pattern	&p,const	ReturnType	t){
		
		indent(false);
		if(t==ANY){

			if(!object(p))
				if(!marker(p))
					if(!op(p,ANY))
						return	false;
		}else	if(!op(p,t))
			return	false;
		OUTPUT<<"("<<p.str_opcode;
		if(p.atom.getAtomCount()){

			if(!right_indent(true)){

				if(!separator(false)){

					set_error("syntax error: missing separator/right_indent after head");
					return	false;
				}
			}else
				OUTPUT<<" ";
		}
		return	true;
	}

	bool	Compiler::expression_head(const	Pattern	&p){
		
		indent(false);
		if(!object(p))
			if(!op(p))
				return	false;
		OUTPUT<<"("<<p.str_opcode;
		if(p.atom.getAtomCount()){

			if(!right_indent(true)){

				if(!separator(false)){

					set_error("syntax error: missing separator/right_indent after head");
					return	false;
				}
			}else
				OUTPUT<<" ";
		}
		return	true;
	}

	bool	Compiler::expression_tail(bool	indented,const	Pattern	&p,uint16	write_index,uint16	&extent_index,bool	write,bool	switch_to_view){	//	arity>0

		uint16	count=0;
		bool	_indented=false;
		bool	entered_pattern=p.is_pattern(this);
		if(write	&&	state.pattern_lvl)	//	fill up with wildcards that will be overwritten up to ::
			for(uint16	j=write_index;j<write_index+p.atom.getAtomCount();++j)
				out_code->data[j]=RAtom::Wildcard();
		std::streampos	i=in_stream->tellg();
		while(!in_stream->eof()){

			if(expression_end(indented)){

				if(state.no_arity_check){

					state.no_arity_check=false;
					return	true;
				}
				if(count==p.atom.getAtomCount())
					return	true;
				goto	return_arity_error;
			}
			if(count>=p.atom.getAtomCount())
				goto	return_arity_error;
			if(count){
				
				if(!_indented){

					if(!right_indent(true)){

						if(!separator(false)){

							set_error("syntax error: missing separator between 2 elements");
							return	false;
						}
					}else
						OUTPUT<<" ";
				}else{

					_indented=false;
					OUTPUT<<" ";
				}
			}
			if(entered_pattern	&&	count==0)	//	pattern skeleton begin
				++state.pattern_lvl;
			if(!p.things_to_read[count](this,_indented,true,write_index+count,extent_index,write)){

				set_error(" error: parsing element in expression");
				return	false;
			}
			if(entered_pattern	&&	count==0)	//	pattern skeleton end
				--state.pattern_lvl;
			if(++count==1	&&	switch_to_view){

				extent_index=p.atom.getAtomCount();
				write_index=0;	
				out_code=out_view->code();
				if(write)
					out_code->data[write_index]=RAtom::RAtom();	//	first member is stored in the object, set this spot undefined
			}
		}
		return	false;
return_arity_error:
		char	buffer[255];
		std::string	s="error: got ";
		sprintf(buffer,"%d",count);
		s+=buffer;
		s+=" elements, expected ";
		sprintf(buffer,"%d",p.atom.getAtomCount());
		s+=buffer;
		set_error(s);
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	bool	Compiler::expression(bool	&indented,const	ReturnType	t,uint16	write_index,uint16	&extent_index,bool	write){

		bool	lbl=false;
		std::streampos	i=in_stream->tellg();
		std::string	l;
		if(label(l)){

			lbl=true;
			OUTPUT<<"lbl_"<<l<<":";
		}
		if(!expression_begin(indented)){

			if(lbl)
				set_error(" error: label not followed by an expression");
			return	false;
		}
		Pattern	p;
		if(!expression_head(p,t)){

			in_stream->seekg(i);
			return	false;
		}
		if(lbl)
			local_references[l]=LocalReference(write_index,p);
		uint16	tail_write_index=0;
		if(write){

			out_code->data[write_index]=RAtom::IPointer(extent_index);
			out_code->data[extent_index++]=p.atom;
			tail_write_index=extent_index;
			extent_index+=p.atom.getAtomCount();
		}
		if(!expression_tail(indented,p,tail_write_index,extent_index,write))
			return	false;
		OUTPUT<<")";
		return	true;
	}

	bool	Compiler::expression(bool	&indented,const	Pattern	&p,uint16	write_index,uint16	&extent_index,bool	write){
		
		bool	lbl=false;
		std::streampos	i=in_stream->tellg();
		std::string	l;
		if(label(l)){

			lbl=true;
			OUTPUT<<"lbl_"<<l<<":";
		}
		if(!expression_begin(indented)){

			if(lbl)
				set_error(" error: label not followed by an expression");
			return	false;
		}
		if(!expression_head(p)){

			in_stream->seekg(i);
			return	false;
		}
		if(lbl)
			local_references[l]=LocalReference(write_index,p);
		uint16	tail_write_index=0;
		if(write){

			out_code->data[write_index]=RAtom::IPointer(extent_index);
			out_code->data[extent_index++]=p.atom;
			tail_write_index=extent_index;
			extent_index+=p.atom.getAtomCount();
		}
		if(!expression_tail(indented,p,tail_write_index,extent_index,write))
			return	false;
		OUTPUT<<")";
		return	true;
	}

	bool	Compiler::set(bool	&indented,uint16	write_index,uint16	&extent_index,bool	write){	//	[ ] illegal; use |[] instead, or [nil]

		std::streampos	i=in_stream->tellg();
		bool	lbl=false;
		std::string	l;
		if(label(l)){

			lbl=true;
			OUTPUT<<"lbl_"<<l<<":";
		}
		if(!set_begin(indented)){

			if(lbl)
				set_error(" error: label not followed by a structure");
			return	false;
		}
		if(lbl)
			local_references[l]=LocalReference(write_index,Pattern(SET));
		indent(false);
		OUTPUT<<"[";
		uint16	content_write_index=0;
		if(write){

			out_code->data[write_index]=RAtom::IPointer(extent_index);
			uint16	element_count=set_element_count(indented);
			out_code->data[extent_index++]=RAtom::Set(element_count);
			content_write_index=extent_index;
			extent_index+=element_count;
		}
		uint16	count=0;
		bool	_indented=false;
		while(!in_stream->eof()){

			if(set_end(indented)){

				OUTPUT<<"]";
				if(!count){

					set_error(" syntax error: use |[] for empty sets");
					return	false;
				}
				return	true;
			}
			if(count){
				
				if(!_indented){

					if(!right_indent(true)){

						if(!separator(false)){

							set_error("syntax error: missing separator between 2 elements");
							return	false;
						}
					}else
						OUTPUT<<" ";
				}else{

					_indented=false;
					OUTPUT<<" ";
				}
			}
			if(!read_any(_indented,false,NULL,content_write_index+count,extent_index,write)){
			
				set_error(" error: illegal element in set");
				return	false;
			}
			count++;
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::set(bool	&indented,const	Pattern	&p,uint16	write_index,uint16	&extent_index,bool	write){	//	for class defs like member-name:[member-list] or !class (name[] member-list)
		
		std::streampos	i=in_stream->tellg();
		bool	lbl=false;
		std::string	l;
		if(label(l)){

			lbl=true;
			OUTPUT<<"lbl_"<<l<<":";
		}
		if(!set_begin(indented)){

			if(lbl)
				set_error(" error: label not followed by a structure");
			return	false;
		}
		if(lbl)
			local_references[l]=LocalReference(write_index,p);
		indent(false);
		OUTPUT<<"[";
		uint16	content_write_index=0;
		if(write){

			out_code->data[write_index]=RAtom::IPointer(extent_index);
			uint16	element_count;
			if(p.atom.getDescriptor()==RAtom::SET_OBJECT	&&	p.use_as!=Member::READ_SET){

				element_count=p.atom.getAtomCount();
				out_code->data[extent_index++]=p.atom;
			}else{

				element_count=set_element_count(indented);
				out_code->data[extent_index++]=RAtom::Set(element_count);
			}
			content_write_index=extent_index;
			extent_index+=element_count;
		}
		uint16	count=0;
		bool	_indented=false;
		uint16	arity=p.use_as==Member::READ_PATTERN?p.atom.getAtomCount():0xFFFF;	//	undefined arity for sets of expressions or sets of sets
		while(!in_stream->eof()){

			if(set_end(indented)){

				if(!count){

					set_error(" syntax error: use |[] for empty sets");
					return	false;
				}else
					OUTPUT<<"]";
				if(count==arity	||	arity==0xFFFF)
					return	true;
				goto	return_arity_error;
			}
			if(count>=arity)
				goto	return_arity_error;
			if(count){
				
				if(!_indented){

					if(!right_indent(true)){

						if(!separator(false)){

							set_error("syntax error: missing separator between 2 elements");
							return	false;
						}
					}else
						OUTPUT<<" ";
				}else{

					_indented=false;
					OUTPUT<<" ";
				}
			}
			bool	r;
			switch(p.use_as){
			case	Member::READ_EXPRESSION:
				r=read_expression(_indented,true,&p,content_write_index+count,extent_index,write);
				break;
			case	Member::READ_SET:
				{
				Pattern	_p=p;
				_p.use_as=Member::READ_PATTERN;
				r=read_set(_indented,true,&_p,content_write_index+count,extent_index,write);
				break;
				}
			case	Member::READ_PATTERN:
				r=p.things_to_read[count](this,_indented,true,content_write_index+count,extent_index,write);
				break;
			}
			if(!r){
						
				set_error(" error: illegal element in set");
				return	false;
			}
			count++;
		}
		in_stream->clear();
		in_stream->seekg(i);
		return	false;
return_arity_error:
		char	buffer[255];
		std::string	s="error: got ";
		sprintf(buffer,"%d",count);
		s+=buffer;
		s+=" elements, expected ";
		sprintf(buffer,"%d",arity);
		s+=buffer;
		set_error(s);
		return	false;
	}

	uint16	Compiler::set_element_count(bool	indented){	//	error checking is done in set(). This is a naive implementation: basically it parses the whole set depth-first. That's very slow and shall be replaced by a more clever design (avoiding deliving in the depths of the elements of the set).

		Output=false;
		uint16	count=0;
		State	s=save_state();
		indent(false);
		bool	_indented=false;
		uint16	unused_index=0;
		while(!in_stream->eof()){

			if(set_end(indented))
				break;
			if(count){
				
				if(!_indented){

					if(!right_indent(true)){

						if(!separator(false))
							break;
					}
				}else
					_indented=false;
			}
			if(!read_any(_indented,false,NULL,0,unused_index,false))
				break;
			count++;
		}
		in_stream->clear();
		restore_state(s);
		Output=true;
		return	count;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	bool	Compiler::read_any(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){	//	enforce always false, p always NULL

		indented=false;
		if(read_number(indented,false,NULL,write_index,extent_index,write))
			return	true;
		if(err)
			return	false;
		if(read_timestamp(indented,false,NULL,write_index,extent_index,write))
			return	true;
		if(err)
			return	false;
		if(read_string(indented,false,NULL,write_index,extent_index,write))
			return	true;
		if(err)
			return	false;
		if(read_function(indented,false,NULL,write_index,extent_index,write))
			return	true;
		if(err)
			return	false;
		if(read_entity(indented,false,NULL,write_index,extent_index,write))
			return	true;
		if(err)
			return	false;
		if(read_sub_system(indented,false,NULL,write_index,extent_index,write))
			return	true;
		if(err)
			return	false;
		if(read_device(indented,false,NULL,write_index,extent_index,write))
			return	true;
		if(err)
			return	false;
		if(read_expression(indented,false,NULL,write_index,extent_index,write))
			return	true;
		if(err)
			return	false;
		if(read_set(indented,false,NULL,write_index,extent_index,write))
			return	true;
		if(write)
			set_error(" error: expecting something");
		return	false;
	}

	bool	Compiler::read_number(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){	//	p always NULL

		if(read_nil_nb(write_index,extent_index,write))
			return	true;
		if(read_variable(write_index,extent_index,write,NUMBER))
			return	true;
		if(read_reference(write_index,extent_index,write,NUMBER))
			return	true;
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;

		float32	n;
		if(number(n)){

			OUTPUT<<n;
			if(write)
				out_code->data[write_index]=RAtom::Float(n);
			return	true;
		}
		State	s=save_state();
		if(expression(indented,NUMBER,write_index,extent_index,write))
			return	true;
		restore_state(s);
		if(enforce){

			set_error(" error: expected a number or an expr evaluating to a number");
			return	false;
		}
		return	false;
	}

	bool	Compiler::read_boolean(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){	//	p always NULL

		if(read_nil_bl(write_index,extent_index,write))
			return	true;
		if(read_variable(write_index,extent_index,write,BOOLEAN))
			return	true;
		if(read_reference(write_index,extent_index,write,BOOLEAN))
			return	true;
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;

		bool	b;
		if(boolean(b)){

			OUTPUT<<b;
			if(write)
				out_code->data[write_index]=RAtom::Boolean(b);
			return	true;
		}
		State	s=save_state();
		if(expression(indented,BOOLEAN,write_index,extent_index,write))
			return	true;
		restore_state(s);
		if(enforce){

			set_error(" error: expected a Boolean or an expr evaluating to a Boolean");
			return	false;
		}
		return	false;
	}

	bool	Compiler::read_timestamp(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){	//	p always NULL

		if(read_nil_ms(write_index,extent_index,write))
			return	true;
		if(read_variable(write_index,extent_index,write,TIMESTAMP))
			return	true;
		if(read_reference(write_index,extent_index,write,TIMESTAMP))
			return	true;
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;

		uint64	ts;
		if(timestamp(ts)){

			OUTPUT<<ts;
			if(write){

				out_code->data[write_index]=RAtom::IPointer(extent_index);
				out_code->data[extent_index++]=RAtom::Timestamp();
				out_code->data[extent_index++]=ts>>32;
				out_code->data[extent_index++]=(ts	&	0x00000000FFFFFFFF);
			}
			return	true;
		}
		State	s=save_state();
		if(expression(indented,TIMESTAMP,write_index,extent_index,write))
			return	true;
		restore_state(s);
		if(enforce){

			set_error(" error: expected a timestamp or an expr evaluating to a timestamp");
			return	false;
		}
		return	false;
	}

	bool	Compiler::read_string(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){	//	p always NULL

		if(read_nil_st(write_index,extent_index,write))
			return	true;
		if(read_variable(write_index,extent_index,write,STRING))
			return	true;
		if(read_reference(write_index,extent_index,write,STRING))
			return	true;
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;

		std::string	st;
		if(str(st)){

			OUTPUT<<"\""<<st<<"\"";
			if(write){

				uint16	l=(uint16)st.length();
				out_code->data[write_index]=RAtom::String(l);
				out_code->data.ensure(extent_index+l/4+1);
				uint8	*_s=out_code->data.asBytes(extent_index);
				for(uint16	i=0;i<l;++i){
					
					_s[i]=st[i];
					if(i%4==0)
						++extent_index;
				}
			}
			return	true;
		}
		State	s=save_state();
		if(expression(indented,STRING,write_index,extent_index,write))
			return	true;
		restore_state(s);
		if(enforce){

			set_error(" error: expected a string");
			return	false;
		}
		return	false;
	}

	bool	Compiler::read_entity(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){	//	p always NULL

		if(read_nil(write_index,extent_index,write))
			return	true;
		if(read_variable(write_index,extent_index,write,ANY))
			return	true;
		if(read_reference(write_index,extent_index,write,ANY))
			return	true;
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;

		if(self()){

			OUTPUT<<"self";
			if(write)
				out_code->data[write_index]=RAtom::Self();
			return	true;
		}
		State	s=save_state();
		if(expression(indented,ANY,write_index,extent_index,write))
			return	true;
		restore_state(s);
		if(enforce){

			set_error(" error: expected an entity");
			return	false;
		}
		return	false;
	}

	bool	Compiler::read_sub_system(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){	//	p always NULL

		if(read_nil_sid(write_index,extent_index,write))
			return	true;
		if(read_variable(write_index,extent_index,write,SUB_SYSTEM_ID))
			return	true;
		if(read_reference(write_index,extent_index,write,SUB_SYSTEM_ID))
			return	true;
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;

		std::streampos	i=in_stream->tellg();
		uint32	h;
		if(hex(h)	&&	RAtom(h).getDescriptor()==RAtom::SUB_SYSTEM){

			OUTPUT<<std::hex<<h;
			if(write)
				out_code->data[write_index]=RAtom::RAtom(h);
			return	true;
		}
		in_stream->seekg(i);
		State	s=save_state();
		if(expression(indented,SUB_SYSTEM_ID,write_index,extent_index,write))
			return	true;
		restore_state(s);
		if(enforce){

			set_error(" error: expected a sub-system id");
			return	false;
		}
		return	false;
	}

	bool	Compiler::read_device(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){	//	p always NULL

		if(read_nil_did(write_index,extent_index,write))
			return	true;
		if(read_variable(write_index,extent_index,write,DEVICE_ID))
			return	true;
		if(read_reference(write_index,extent_index,write,DEVICE_ID))
			return	true;
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;

		std::streampos	i=in_stream->tellg();
		uint32	h;
		if(hex(h)	&&	RAtom(h).getDescriptor()==RAtom::DEVICE){

			OUTPUT<<std::hex<<h;
			if(write)
				out_code->data[write_index]=RAtom::RAtom(h);
			return	true;
		}
		in_stream->seekg(i);
		State	s=save_state();
		if(expression(indented,DEVICE_ID,write_index,extent_index,write))
			return	true;
		restore_state(s);
		if(enforce){

			set_error(" error: expected a device id");
			return	false;
		}
		return	false;
	}

	bool	Compiler::read_function(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){	//	p always NULL

		if(read_nil_fid(write_index,extent_index,write))
			return	true;
		if(read_variable(write_index,extent_index,write,FUNCTION_ID))
			return	true;
		if(read_reference(write_index,extent_index,write,FUNCTION_ID))
			return	true;
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;

		Pattern	_p;
		if(function(_p)){	//	TODO: _p shall be used to parse the args in the embedding expression

			OUTPUT<<_p.str_opcode;
			if(write)
				out_code->data[write_index]=_p.atom;
			return	true;
		}
		State	s=save_state();
		if(expression(indented,FUNCTION_ID,write_index,extent_index,write))
			return	true;
		restore_state(s);
		if(enforce){

			set_error(" error: expected a device function");
			return	false;
		}
		return	false;
	}

	bool	Compiler::read_expression(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){

		if(read_nil(write_index,extent_index,write))
			return	true;
		if(p	&&	p->str_opcode!="-generic-expr-"){

			if(read_variable(write_index,extent_index,write,*p))
				return	true;
			if(read_reference(write_index,extent_index,write,p->type))
				return	true;
		}else{

			if(read_variable(write_index,extent_index,write,Pattern()))
				return	true;
			if(read_reference(write_index,extent_index,write,ANY))
				return	true;
		}
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;
		
		indented=false;
		if(p	&&	p->str_opcode!="-generic-expr-"){
			
			if(expression(indented,*p,write_index,extent_index,write))
				return	true;
		}else	if(expression(indented,ANY,write_index,extent_index,write))
				return	true;
		if(enforce){

			std::string	s=" error: expected an expression";
			if(p){

				s+=" of type: ";
				s+=p->str_opcode;
			}
			set_error(s);
			return	false;
		}
		return	false;
	}

	bool	Compiler::read_set(bool	&indented,bool	enforce,const	Pattern	*p,uint16	write_index,uint16	&extent_index,bool	write){

		if(read_nil_set(write_index,extent_index,write))
			return	true;
		if(read_variable(write_index,extent_index,write,Pattern(SET)))
			return	true;
		if(read_reference(write_index,extent_index,write,SET))
			return	true;
		if(read_wildcard(write_index,extent_index,write))
			return	true;
		if(read_tail_wildcard(write_index,extent_index,write))
			return	true;
		
		indented=false;
		if(p){
			
			if(set(indented,*p,write_index,extent_index,write))
				return	true;
		}else	if(set(indented,write_index,extent_index,write))
			return	true;
		if(enforce){

			set_error(" error: expected a set");
			return	false;
		}
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	bool	Compiler::read_nil(uint16	write_index,uint16	&extent_index,bool	write){

		if(nil()){

			OUTPUT<<"nil";
			if(write)
				out_code->data[write_index]=RAtom::Nil();
			return	true;
		}
		return	false;
	}

	bool	Compiler::read_nil_set(uint16	write_index,uint16	&extent_index,bool	write){

		std::streampos	i=in_stream->tellg();
		if(match_symbol("|[]",false)){

			OUTPUT<<"|[]";
			if(write){

				out_code->data[write_index]=RAtom::IPointer(extent_index);
				out_code->data[extent_index++]=RAtom::Set(0);
			}
			return	true;
		}
		in_stream->seekg(i);
		return	false;
	}

	bool	Compiler::read_nil_nb(uint16	write_index,uint16	&extent_index,bool	write){

		if(nil_nb()){

			OUTPUT<<"|nb";
			if(write)
				out_code->data[write_index]=RAtom::UndefinedFloat();
			return	true;
		}
		return	false;
	}

	bool	Compiler::read_nil_ms(uint16	write_index,uint16	&extent_index,bool	write){

		if(nil_ms()){

			OUTPUT<<"|ms";
			if(write){

				out_code->data[write_index]=RAtom::IPointer(extent_index);
				out_code->data[extent_index++]=RAtom::UndefinedTimestamp();
				out_code->data[extent_index++]=0xFFFFFFFF;
				out_code->data[extent_index++]=0xFFFFFFFF;
			}
			return	true;
		}
		return	false;
	}

	bool	Compiler::read_nil_sid(uint16	write_index,uint16	&extent_index,bool	write){

		if(nil_sid()){

			OUTPUT<<"|sid";
			if(write)
				out_code->data[write_index]=RAtom::UndefinedSubSystem();
			return	true;
		}
		return	false;
	}

	bool	Compiler::read_nil_did(uint16	write_index,uint16	&extent_index,bool	write){

		if(nil_did()){

			OUTPUT<<"|did";
			if(write)
				out_code->data[write_index]=RAtom::UndefinedDevice();
			return	true;
		}
		return	false;
	}

	bool	Compiler::read_nil_fid(uint16	write_index,uint16	&extent_index,bool	write){

		if(nil_fid()){

			OUTPUT<<"|fid";
			if(write)
				out_code->data[write_index]=RAtom::UndefinedDeviceFunction();
			return	true;
		}
		return	false;
	}

	bool	Compiler::read_nil_bl(uint16	write_index,uint16	&extent_index,bool	write){

		if(nil_bl()){

			OUTPUT<<"|bl";
			if(write)
				out_code->data[write_index]=RAtom::UndefinedBoolean();
			return	true;
		}
		return	false;
	}

	bool	Compiler::read_nil_st(uint16	write_index,uint16	&extent_index,bool	write){

		if(nil_st()){

			OUTPUT<<"|st";
			if(write)
				out_code->data[write_index]=RAtom::UndefinedString();
			return	true;
		}
		return	false;
	}

	bool	Compiler::read_variable(uint16	write_index,uint16	&extent_index,bool	write,const	Pattern	p){

		std::string	v;
		if(variable(v)){

			if(state.pattern_lvl){
			
				OUTPUT<<"var_"<<v<<":";
				local_references[v]=LocalReference(write_index,p);
				if(write)
					out_code->data[write_index]=RAtom::Wildcard();	//	useless in skeleton expressions (already filled up in expression_head); usefull when the skeleton itself is a variable
				return	true;
			}else{

				set_error(" error: no variables allowed outside a pattern skeleton");
				return	false;
			}
		}
		return	false;
	}

	bool	Compiler::read_reference(uint16	write_index,uint16	&extent_index,bool	write,const	ReturnType	t){

		uint16	index;
		if((t==ANY	||	(t!=ANY	&&	_this.type==t))	&&	this_()){

			if(write)
				out_code->data[write_index]=RAtom::This();
			return	true;
		}
		if(local_reference(index,t)){

			if(write)
				out_code->data[write_index]=RAtom::VLPointer(index);	//	local references are always pointing to the value array
			return	true;
		}
		if(global_reference(index,t)){

			if(write)
				out_code->data[write_index]=RAtom::VPointer(index);
			return	true;
		}
		std::vector<uint16>	v;
		if(this_indirection(v,t)){

			if(write){

				out_code->data[write_index]=RAtom::IPointer(extent_index);
				out_code->data[extent_index++]=RAtom::CPointer(v.size()+1);
				out_code->data[extent_index++]=RAtom::This();
				for(uint16	i=0;i<v.size();++i)
					out_code->data[extent_index++]=RAtom::IPointer(v[i]);
			}
			return	true;
		}
		if(local_indirection(v,t)){

			if(write){

				out_code->data[write_index]=RAtom::IPointer(extent_index);
				out_code->data[extent_index++]=RAtom::CPointer(v.size());
				out_code->data[extent_index++]=RAtom::VLPointer(v[0]);
				for(uint16	i=1;i<v.size();++i)
					out_code->data[extent_index++]=RAtom::IPointer(v[i]);
			}
			return	true;
		}
		if(global_indirection(v,t)){

			if(write){

				out_code->data[write_index]=RAtom::IPointer(extent_index);
				out_code->data[extent_index++]=RAtom::CPointer(v.size());
				out_code->data[extent_index++]=RAtom::VPointer(v[0]);
				for(uint16	i=1;i<v.size();++i)
					out_code->data[extent_index++]=RAtom::IPointer(v[i]);
			}
			return	true;
		}
		return	false;
	}

	bool	Compiler::read_wildcard(uint16	write_index,uint16	&extent_index,bool	write){

		if(wildcard()){

			if(state.pattern_lvl){

				OUTPUT<<":";
				if(write)
					out_code->data[write_index]=RAtom::Wildcard();
				return	true;
			}else{

				set_error(" error: no wildcards allowed outside a pattern skeleton");
				return	false;
			}
		}
		return	false;
	}

	bool	Compiler::read_tail_wildcard(uint16	write_index,uint16	&extent_index,bool	write){

		if(tail_wildcard()){

			if(state.pattern_lvl){

				OUTPUT<<"::";
				if(write)
					out_code->data[write_index]=RAtom::TailWildcard();
				return	true;
			}else{

				set_error(" error: no wildcards allowed outside a pattern skeleton");
				return	false;
			}
		}
		return	false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////

	std::string	Compiler::get_variable_name(uint16	index,bool	postfix){

		std::string	s;
		UNORDERED_MAP<uint16,std::string>::iterator	it=variable_names.find(index);
		if(it==variable_names.end()){

			char	buffer[255];
			s="v";
			sprintf(buffer,"%d",last_variable_ID++);
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

	void	Compiler::decompile(RView	*view,std::ostringstream	*stream){

		last_variable_ID=0;
		variable_names.clear();
		out_stream=new	OutStream(stream);
		in_view=view;
		indents=0;
		uint16	read_index=0;
		if(in_view->code()->data[0].atom==0xFFFFFFFF){	//	non-marker

			in_code=in_view->getObject()->code();
			out_stream->code_indexes_to_stream_indexes.resize(in_code->data.count());
			out_stream->push('(',0);
			write_expression_head(read_index);
			write_expression_tail(read_index,true);
		}else{											//	marker

			in_code=in_view->code();
			out_stream->code_indexes_to_stream_indexes.resize(in_code->data.count());
			write_expression(read_index);
		}
		return;
	}

	void	Compiler::write_indent(uint16	i){

		*out_stream<<NEWLINE;
		indents=i;
		for(uint16	j=0;j<indents;j++)
			*out_stream<<' ';
	}

	void	Compiler::write_expression_head(uint16	read_index){

		switch(in_code->data[read_index].getDescriptor()){
		case	RAtom::OPERATOR:
			*out_stream<<operator_names[in_code->data[read_index].asOpcode()];
			break;
		case	RAtom::OBJECT:
			*out_stream<<object_names[in_code->data[read_index].asOpcode()];
			break;
		default:
			*out_stream<<"undefined-head";
			break;
		}
	}

	void	Compiler::write_expression_tail(uint16	read_index,bool	switch_to_view){	//	read_index points initially to the head

		uint16	arity=in_code->data[read_index].getAtomCount();
		bool	after_tail_wildcard=false;
		bool	follow_indents=true;
		for(uint16	i=0;i<arity;++i){

			if(!after_tail_wildcard){

				if(closing_set){
					
					closing_set=false;
					write_indent(indents);
				}else
					*out_stream<<' ';
			}
			write_any(++read_index,after_tail_wildcard);
			if(i==0	&&	switch_to_view){	//	non-marker case

				in_code=in_view->code();
				read_index=0;	//	first atom in the view is unused
			}
		}
		if(closing_set){
					
			closing_set=false;
			write_indent(indents);
		}
		*out_stream<<')';
	}

	void	Compiler::write_expression(uint16	read_index){

		out_stream->push('(',read_index);
		write_expression_head(read_index);
		write_expression_tail(read_index);
	}

	void	Compiler::write_set(uint16	read_index){	//	read_index points initially to set atom

		uint16	arity=in_code->data[read_index].getAtomCount();
		bool	after_tail_wildcard=false;
		if(arity==1){	//	write [element]

			out_stream->push('[',read_index);
			write_indent(indents);
			write_any(++read_index,after_tail_wildcard);
			write_indent(indents);
			*out_stream<<']';
		}else{		//	write []+indented elements

			out_stream->push("[]",read_index);
			write_indent(indents+3);
			for(uint16	i=0;i<arity;++i){

				if(i>0)
					write_indent(indents);
				write_any(++read_index,after_tail_wildcard);
			}
			closing_set=true;
			indents-=3;
		}
	}

	void	Compiler::write_any(uint16	read_index,bool	&after_tail_wildcard){	//	after_tail_wildcard meant to avoid printing ':' after "::"

		RAtom	a=in_code->data[read_index];

		if(a.isFloat()){

			if(a.atom==0x3FFFFFFF)
				out_stream->push("|nb",read_index);
			else{

				*out_stream<<std::dec;
				out_stream->push(in_code->data[read_index].asFloat(),read_index);
			}
			return;
		}

		RAtom	atom;
		uint16	index;
		switch(a.getDescriptor()){
		case	RAtom::I_PTR:
		case	RAtom::VL_PTR:
			index=a.asIndex();
			atom=in_code->data[index];
			while(atom.getDescriptor()==RAtom::I_PTR){

				index=atom.asIndex();
				atom=in_code->data[index];
			}
			if(index<read_index){	//	reference to a label or variable

				std::string	s=get_variable_name(index,atom.getDescriptor()!=RAtom::WILDCARD); // post-fix labels with ':' (no need for variables since they are inserted just before wildcards)
				out_stream->push(s,read_index);
				break;
			}
			switch(atom.getDescriptor()){	//	structures
			case	RAtom::OBJECT:
			case	RAtom::MARKER:
			case	RAtom::OPERATOR:
				write_expression(index);
				break;
			case	RAtom::SET:
			case	RAtom::SET_OBJECT:
				if(atom.readsAsNil())
					out_stream->push("|[]",read_index);
				else
					write_set(index);
				break;
			case	RAtom::STRING:
				if(atom.readsAsNil())
					out_stream->push("|st",read_index);
				else{

					uint8	*chars=in_code->data.asBytes(index+1);
					out_stream->push(chars[0],read_index);
					for(uint16	i=1;i<atom.getAtomCount();++i)
						*out_stream<<chars[i];
				}
				break;
			case	RAtom::TIMESTAMP:
				if(atom.readsAsNil())
					out_stream->push("|ms",read_index);
				else{

					uint64	ts=in_code->data[index+1].atom<<32	|	in_code->data[index+2].atom;
					out_stream->push(ts,read_index);
				}
				break;
			case	RAtom::C_PTR:{
				uint16	member_count=atom.getAtomCount();
				uint16	opcode;
				uint16	structure_index=0;
				for(uint16	i=0;i<member_count;++i){

					atom=in_code->data[index+1+i];	//	in_code->data[index] is the cptr
					switch(atom.getDescriptor()){
					case	RAtom::THIS:	//	first child
						out_stream->push("this",read_index);
						opcode=in_view->getObject()->code()->data[0].asOpcode();
						break;
					case	RAtom::V_PTR:	//	first child
						out_stream->push("object of class ",read_index);
						*out_stream<<in_view->code()->pointers[atom.asIndex()]->getObject()->code()->data[0].asOpcode();
						break;
					case	RAtom::VL_PTR:	//	first child
						while(in_code->data[atom.asIndex()].getDescriptor()==RAtom::I_PTR)
							atom=in_code->data[atom.asIndex()];
						out_stream->push(get_variable_name(atom.asIndex(),true),read_index);
						opcode=in_code->data[atom.asIndex()].asOpcode();
						break;
					case	RAtom::I_PTR:	//	from second child on
						*out_stream<<'.'<<objects_by_opcodes[opcode].get_member_name(atom.asIndex());
						if(i<member_count-1){	//	not the last member, therefore must have a structure: set the opcode and structure index accordingly for the next members
						
							uint16	_target_index=structure_index+atom.asIndex();
							while(in_code->data[_target_index].getDescriptor()==RAtom::I_PTR){

								atom=in_code->data[_target_index];
								_target_index=atom.asIndex();
							}
							opcode=in_code->data[atom.asIndex()].asOpcode();
							structure_index=atom.asIndex()+1;
						}
						break;
					default:
						out_stream->push("unknown-cptr-child-type",read_index);
						break;
					}
				}
				break;
			}default:
				out_stream->push("undefined-structural-atom-or-reference",read_index);
				break;
			}
			break;
		case	RAtom::V_PTR:	//	global ref
			//	TODO: look in pointer array and get the view; name it if no vptr has already referenced it
			break;
		case	RAtom::B_PTR:	//	ref in view/object from value array
			break;
		case	RAtom::THIS:
			out_stream->push("this",read_index);
			break;
		case	RAtom::NIL:
			out_stream->push("nil",read_index);
			break;
		case	RAtom::BOOLEAN_:
			if(a.readsAsNil())
				out_stream->push("|bl",read_index);
			else{

				*out_stream<<std::boolalpha;
				out_stream->push(a.asBoolean(),read_index);
			}
			break;
		case	RAtom::WILDCARD:
			if(after_tail_wildcard)
				out_stream->push();
			else
				out_stream->push(':',read_index);
			break;
		case	RAtom::T_WILDCARD:
			out_stream->push("::",read_index);
			after_tail_wildcard=true;
			break;
		case	RAtom::ENTITY:
			break;
		case	RAtom::SUB_SYSTEM:
			if(a.readsAsNil())
				out_stream->push("|sid",read_index);
			else{

				out_stream->push("0x",read_index);
				*out_stream<<std::hex;
				*out_stream<<a.atom;
			}
			break;
		case	RAtom::DEVICE:
			if(a.readsAsNil())
				out_stream->push("|did",read_index);
			else{

				out_stream->push("0x",read_index);
				*out_stream<<std::hex;
				*out_stream<<a.atom;
			}
			break;
		case	RAtom::DEVICE_FUNCTION:
			if(a.readsAsNil())
				out_stream->push("|fid",read_index);
			else
				out_stream->push(function_names[a.asOpcode()],read_index);
			break;
		case	RAtom::SELF:
			out_stream->push("self",read_index);
			break;
		case	RAtom::HOST:
			break;
		default:
			out_stream->push("undefined-atom",read_index);
			break;
		}
	}
}