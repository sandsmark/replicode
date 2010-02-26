#ifndef	decompiler_h
#define	decompiler_h

#include	<fstream>
#include	<sstream>

#include	"out_stream.h"
#include	"segments.h"


namespace	r_comp{

	class	dll_export	Decompiler{
	private:
		OutStream	*out_stream;
		uint16		indents;		//	in chars
		bool		closing_set;	//	set after writing the last element of a set: any element in an expression finding closeing_set will indent and set closing_set to false
		
		ImageObject	*current_object;

		r_comp::Image	*_image;

		UNORDERED_MAP<uint16,std::string>	variable_names;				//	in the form vxxx where xxx is an integer representing the order of referencing of the variable/label in the code
		std::string	get_variable_name(uint16	index,bool	postfix);	//	associates iptr/vptr indexes to names; inserts them in out_stream if necessary; when postfix==true, a trailing ':' is added

		UNORDERED_MAP<uint16,std::string>	object_names;				//	in the form class_namexxx where xxx is an integer representing the order of appearence of the object in the image; N.B.: root:0 self:1 stdin:2 stdout:3
		std::string	get_object_name(uint16	index);			//	retrieves the name of an object

		void	write_indent(uint16	i);
		void	write_expression_head(uint16	read_index);						//	decodes the leading atom of an expression
		void	write_expression_tail(uint16	read_index,bool	vertical=false);	//	decodes the elements of an expression following the head
		void	write_expression(uint16	read_index);
		void	write_set(uint16	read_index);
		void	write_any(uint16	read_index,bool	&after_tail_wildcard);	//	decodes any element in an expression or a set
	public:
		Decompiler();
		~Decompiler();
		void	decompile(r_comp::Image		*image,std::ostringstream	*stream);
	};
}


#endif
