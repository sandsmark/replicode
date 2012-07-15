//	model_base.h
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

#ifndef	model_base_h
#define	model_base_h

#include	"factory.h"


namespace	r_exec{

	class	_Mem;

	// TPX guess models: this list is meant for TPXs to (a) avoid re-guessing known failed models and,
	// (b) avoid producing the same models in case they run concurrently.
	// The black list contains bad models (models that were killed). This list is trimmed down on a time basis (black_thz).
	// Each bad model is tagged with the last time it was successfully compared to. GC is performed by comparing this time to the thz.
	// The white list contains models that are still alive. As for the black list, the white list is trimmed down on a time basis (white_thz).
	class	ModelBase{
	friend	class	_Mem;
	private:
		static	ModelBase	*Singleton;

		CriticalSection	mdlCS;

		uint64	white_thz;	// set to primary_thz;
		uint64	black_thz;	// set to secondary_thz;

		class	MEntry{
		public:
			static	uint32	ComputeHashCode(Code	*mdl);

			MEntry();
			MEntry(Code	*mdl);
			~MEntry();

			P<Code>	mdl;
			uint64	touch_time;	// last time the mdl was successfully compared to.
			uint32	hash_code;	// 32 bits format: [number of tpl args (4)|lhs type(2)|rhs type (2)|sum of lhs and rhs opcodes(16)|number of lhs and rhs references(8)]. Type is fact (0) or anti fact (1) and imdl (1) or no imdl (0).

			bool	match(const	MEntry	&e)	const;

			class	Hash{
			public:
				size_t	operator	()(MEntry	e)	const{	return	e.hash_code;	}
			};

			class	Equal{
			public:
				bool	operator	()(const	MEntry	lhs,const	MEntry	rhs)	const{	return	lhs.match(rhs);	}
			};
		};

		typedef	UNORDERED_SET<MEntry,typename	MEntry::Hash,typename	MEntry::Equal> MdlSet;

		MdlSet	black_list;
		MdlSet	white_list;

		void	set_thz(uint64	white_thz,uint64	black_thz){	this->white_thz=white_thz;	this->black_thz=black_thz;	}	// called by _Mem::start().

		ModelBase();
	public:
		static	ModelBase	*Get();

		~ModelBase();

		bool	register_mdl(Code	*mdl);	// return true if the model already exist.
		void	register_mdl_failure(Code	*mdl);	// moves the mdl from the white to the black list; happens to bad models.
	};
}


#endif