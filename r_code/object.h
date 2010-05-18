#ifndef	r_code_object_h
#define	r_code_object_h

#include	"atom.h"
#include	"vector.h"
#include	"base.h"


namespace	r_code{

	//	I/O from/to r_code::Image ////////////////////////////////////////////////////////////////////////

	class	dll_export	ImageObject{
	public:
		r_code::vector<Atom>	code;
		r_code::vector<uint32>	reference_set;	//	for views: 0, 1 or 2 elements; these are indexes in the relocation segment for grp (exception: not for root) and possibly org
												//	for sys-objects: any number
		virtual	void	write(word32	*data)=0;
		virtual	void	read(word32		*data)=0;
		virtual	void	trace()=0;
	};

	class	View;

	class	dll_export	SysView:
	public	ImageObject{
	public:
		SysView();
		SysView(View	*source);

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	getSize()	const;
		void	trace();
	};

	class	Object;

	class	dll_export	SysObject:
	public	ImageObject{
	public:
		r_code::vector<uint32>		marker_set;		//	indexes in the relocation segment
		r_code::vector<SysView	*>	view_set;

		SysObject();
		SysObject(Object	*source);
		~SysObject();

		void	write(word32	*data);
		void	read(word32		*data);
		uint32	getSize();
		void	trace();
	};

	// Interfaces for r_exec classes ////////////////////////////////////////////////////////////////////////

	class	View;

	class	dll_export	Object:
	public	_Object{
	public:
		r_code::vector<Atom>		code;
		r_code::vector<P<Object> >	marker_set;
		r_code::vector<P<Object> >	reference_set;
		r_code::vector<View	*>		view_set;

		Object();
		Object(SysObject	*source);
		virtual	~Object();
	};

	class	dll_export	View:
	public	_Object{
	public:
		P<Object>					object;
		r_code::vector<Atom>		code;
		r_code::vector<Object	*>	reference_set;

		View();
		View(SysView	*source,Object	*object);
		virtual	~View();
	};

	class	Mem{
	public:
		virtual	Object	*buildObject(SysObject	*source)=0;
		virtual	Object	*buildGroup(SysObject	*source)=0;
	};
}


#endif