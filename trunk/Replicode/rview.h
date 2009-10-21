#ifndef	rview_h
#define	rview_h

#include	"message.h"
#include	"payload_utils.h"

#include	"r_atom.h"


using	namespace	mBrane;
using	namespace	mBrane::sdk;
using	namespace	mBrane::sdk::payloads;

namespace	replicode{

	class	RView;

	class	dll_export	Code{
	public:
		Code();
		payloads::Array<RAtom>		data;
		payloads::Array<P<RView> >	pointers;
		void	trace();
	};

	class	dll_export	RObject:
	public	RPayload<RObject,StaticData,Memory>{
	private:
		static	RView	*Self;
		Code			_code;
	public:
		RObject();
		~RObject();
		Code	*code();
		uint8		ptrCount();
		__Payload	*getPtr(uint16 i);
		void		setPtr(uint16	i,__Payload	*p);
	};

	class	dll_export	RView:
	public	StreamData<RView,StaticData,Memory>{
	private:
		static	RView	*Self;
		P<RObject>		object;
		Code			_code;
		uint16			res_index;
		uint16			sln_index;
		RView			*host;	//	won't be transmitted
	public:
		RView(RObject	*object=NULL);
		~RView();
		void	setSID(RAtom	a);
		RObject	*getObject()	const;
		void	setObject(RObject	*o);
		Code	*code();
		void	setResilienceIndex(uint16	i);
		void	setSaliencyIndex(uint16	i);
		int32	getResilience();
		void	setResilience(int32	res);
		float32	getSaliency();
		void	setSaliency(float32	sln);
		uint8		ptrCount();
		__Payload	*getPtr(uint16 i);
		void		setPtr(uint16	i,__Payload	*p);
	};
};


#endif