#ifndef	r_exec_object_h
#define	r_exec_object_h

#include	"../CoreLibrary/utils.h"
#include	"../r_code/object.h"
#include	"view.h"


namespace	r_exec{

	class	Mem;

	typedef	enum{
		GROUP=0,
		MARKER=1,
		IPGM=2,
		ANTI_IPGM=3,
		INPUT_LESS_IPGM=4,
		OTHER=5
	}ObjectType;

	//	Shared resources:
	//		view_map: accessed by Mem::injectNow (via various sub calls) and Mem::update.
	//		psln_thr: accessed by reduction cores (via overlay mod/set).
	//		marker_set: accessed by Mem::injectNow ans Mem::_initiate_sln_propagation.
	class	Object:
	public	r_code::Object{
	private:
		uint32	OID;
		size_t	hash_value;
		FastSemaphore	*psln_thr_sem;
		FastSemaphore	*views_sem;
		FastSemaphore	*marker_set_sem;
	protected:
		Mem		*mem;
	public:
		UNORDERED_SET<View	*,View::Hash,View::Equal>	views;
	
		Object();
		Object(uint32	OID,Mem	*m);
		Object(r_code::SysObject	*source,Mem	*m);
		virtual	~Object();	//	un-registers from the rMem's object-map and object_io_map.

		void	computeHashValue();

		virtual	ObjectType	getType()	const;
		virtual	bool		isIPGM()	const;

		float32	get_psln_thr();

		uint32	getOID()	const;

		void	acq_views()			const{	views_sem->acquire();	}
		void	rel_views()			const{	views_sem->release();	}
		void	acq_marker_set()	const{	marker_set_sem->acquire();	}
		void	rel_marker_set()	const{	marker_set_sem->release();	}

		//	Target psln_thr only.
		virtual	void	set(uint16	member_index,float32	value);
		virtual	void	mod(uint16	member_index,float32	value);

		class	Hash{
		public:
			size_t	operator	()(Object	*o)	const{
				if(o->hash_value==0)
					o->computeHashValue();
				return	o->hash_value;
			}
		};

		class	Equal{
		public:
			bool	operator	()(const	Object	*lhs,const	Object	*rhs)	const{
				if(lhs->references_size()!=rhs->references_size())
					return	false;
				if(lhs->code_size()!=rhs->code_size())
					return	false;
				uint32	i;
				for(i=0;i<lhs->references_size();++i)
					if(lhs->references(i)!=rhs->references(i))
						return	false;
				for(i=0;i<lhs->code_size();++i)
					if(lhs->code(i)!=rhs->code(i))
						return	false;
				return	true;
			}
		};
	};

	class	InstantiatedProgram:
	public	Object{
	public:
		InstantiatedProgram();
		InstantiatedProgram(r_code::SysObject	*source,Mem	*m);
		~InstantiatedProgram();

		ObjectType	getType()	const;
		bool		isIPGM()	const;

		Object	*getPGM()	const;
		uint64	get_tsc()	const;
	};

	class	Marker:
	public	Object{
	protected:
		Marker();
	public:
		Marker(r_code::SysObject	*source,Mem	*m);
		~Marker();

		ObjectType	getType()	const;
		virtual	bool	isNotification()	const;
	};

	class	MkNew:
	public	Marker{
	public:
		MkNew(Object	*object);

		bool	isNotification()	const;
	};

	class	MkLowRes:
	public	Marker{
	public:
		MkLowRes(Object	*object);

		bool	isNotification()	const;
	};

	class	MkLowSln:
	public	Marker{
	public:
		MkLowSln(Object	*object);

		bool	isNotification()	const;
	};

	class	MkHighSln:
	public	Marker{
	public:
		MkHighSln(Object	*object);

		bool	isNotification()	const;
	};

	class	MkLowAct:
	public	Marker{
	public:
		MkLowAct(Object	*object);

		bool	isNotification()	const;
	};

	class	MkHighAct:
	public	Marker{
	public:
		MkHighAct(Object	*object);

		bool	isNotification()	const;
	};

	class	MkSlnChg:
	public	Marker{
	public:
		MkSlnChg(Object	*object,float32	value);

		bool	isNotification()	const;
	};

	class	MkActChg:
	public	Marker{
	public:
		MkActChg(Object	*object,float32	value);

		bool	isNotification()	const;
	};

	class	MkRdx:
	public	Marker{
	public:
		MkRdx();

		bool	isNotification()	const;
	};
}


#endif