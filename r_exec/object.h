#ifndef	r_exec_object_h
#define	r_exec_object_h

#include	"../CoreLibrary/utils.h"
#include	"../r_code/object.h"
#include	"view.h"
#include	"opcodes.h"


namespace	r_exec{

	typedef	enum{
		IPGM=0,
		INPUT_LESS_IPGM=1,
		ANTI_IPGM=2,
		OBJECT=4,
		MARKER=5,
		GROUP=6
	}ObjectType;

	r_exec_dll	bool	IsNotification(Code	*object);
	r_exec_dll	ObjectType	GetType(Code	*object);

	//	Shared resources:
	//		views: accessed by Mem::injectNow (via various sub calls) and Mem::update.
	//		psln_thr: accessed by reduction cores (via overlay mod/set).
	//		marker_set: accessed by Mem::injectNow ans Mem::_initiate_sln_propagation.
	template<class	C>	class	Object:
	public	C{
	private:
		size_t	hash_value;

		FastSemaphore	*psln_thr_sem;
		FastSemaphore	*views_sem;
		FastSemaphore	*marker_set_sem;
	protected:
		r_code::Mem	*mem;

		Object();
		Object(r_code::Mem	*mem);
	public:
		UNORDERED_SET<View	*,View::Hash,View::Equal>	views;

		virtual	~Object();	//	un-registers from the rMem's object_register.

		void	compute_hash_value();

		float32	get_psln_thr();

		void	acq_views()			const{	views_sem->acquire();	}
		void	rel_views()			const{	views_sem->release();	}
		void	acq_marker_set()	const{	marker_set_sem->acquire();	}
		void	rel_marker_set()	const{	marker_set_sem->release();	}

		//	Target psln_thr only.
		virtual	void	set(uint16	member_index,float32	value);
		virtual	void	mod(uint16	member_index,float32	value);

		View	*find_view(Code	*group);

		class	Hash{
		public:
			size_t	operator	()(Object	*o)	const{
				if(o->hash_value==0)
					o->compute_hash_value();
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

	//	Local object.
	//	Used for r-code that does not travel across networks.
	//	Used for construction.
	//	If the mem is network-aware, instances will be packed into a suitable form.
	class	r_exec_dll	LObject:
	public	Object<r_code::Object>{
	public:
		static	bool	RequiresPacking(){	return	false;	}
		static	LObject	*Pack(Code	*object){	return	(LObject	*)object;	}	//	object is always a LObject (local operation).

		LObject():Object<r_code::Object>(){}
		LObject(r_code::SysObject	*source,r_code::Mem	*m):Object<r_code::Object>(mem){
		
			load(source);
			build_views<r_exec::View>(source);
		}
		~LObject(){}
	};
}


#include	"object.tpl.cpp"


#endif