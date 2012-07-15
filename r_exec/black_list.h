//	black_list.h
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

#ifndef	black_list_h
#define	black_list_h
/* DERECATED
#include	"factory.h"


namespace	r_exec{

	class	_Mem;

	// TPX guess models: this list is meant for TPXs to avoid re-guessing known failed models.
	// Suspicious models are weak models predicting in the secondary group.
	// Failed models are suspicious models which died.
	// Suspicious models are silent: their predictions are unbeknowst to the A/F.
	// Asynchronous control: say, M0:[A -> B] is suspicious and from an a predicts a b: pred(b) is silent, unbeknownst to TPX, which can in the meantime build a M1 (equal to M0) before M0 dies and proves itself not worth repeating.
	// Then, M1 will live the life of M0, and the TPX will replicate M1 endlessly.
	// The black list is meant for the TPX to wait for the outcome of pred(b) produced by M0 (M0 may die from the negative outcomer of pred(b)) before checking if M1 is to be avoided as a past counter-evidence (i.e. M0 turns out to be a failure and M1 to be equal to M0).
	class	BlackList{
	friend	class	_Mem;
	private:
		static	BlackList	*Singleton;

		BlackList();
	public:
		static	BlackList	*Get();

		~BlackList();

		void	suspect(Code	*mdl,Code	*prediction);	// a suspicious model attempts to predict.
		void	blacklist(Code	*mdl,Code	*prediction);	// a suspicious model dies upon its last bad prediction.
		bool	contains(Code	*mdl);						// return true if a model is a known failure.
		void	clear(Code	*prediction);					// a strong (i.e. unsupsicious) model can make the prediction: clear the black list for said prediction (i.e. override suspicion).
	};
}
*/

#endif