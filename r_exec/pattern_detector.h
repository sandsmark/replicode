//	pattern_detector.h
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

#ifndef	pattern_detector_h
#define	pattern_detector_h

#include	"../../CoreLibrary/trunk/CoreLibrary/base.h"
#include	"../../CoreLibrary/trunk/CoreLibrary/utils.h"
#include	"../r_code/object.h"
#include	"dll.h"
#include	"binding_map.h"
#include	"group.h"


namespace	r_exec{

	class	PatternDetector;

	class	PatternDetectorOverlay:
	public	_Object{
	private:
		PatternDetector	*detector;
	public:
		PatternDetectorOverlay(PatternDetector	*detector);
		~PatternDetectorOverlay();

		void	reduce(Code	*input);
	};

	//	One instance per input source (e.g. architecture module).
	//	Detects patterns from input sequences: performs as a monitor (goal regression on target).
	//	Input sequences are closed when one of the targets is matched.
	//	This implies overlays - functioning similar to pgms'.
	//	Targets are:
	//		- goals.
	//		- predictions.
	//		- instantiations of high-level patterns (isct and imdl).
	//		- success/failure of goals/predictions.
	//		- reductions performed by high-level patterns (mk.rdx on icst and imdl).
	//	Excludes targets that perform well (e.g. goals that are matched very often, mdls that succeed with a good rate, etc.).
	//	Input objects are abstracted first (BindingMap for a whole sequence), then
	//	internal attention control kicks in in two phases:
	//		- phase 1 - filtering - retain:
	//			- objects sharing common variables, stemming from variables referred to by targets.
	//			- markers (that may not be present in the input sequence) pointing to the objects above.
	//			- all objects and markers referring to the targets and not in the sequence.
	//			- given the level of abstraction of the target, objects from above that are above or at the same level of abstraction
	//			(level of abstraction is lower for objects composing a mdl/cst)
	//			(ex: target=same_pos => discard positions for the objects involved in the target).
	//		- phase 2 - exploitation - detect:
	//			- synchronicity and recurrence (e.g. h1 is a hand, belongs to actor 1: yields a cst; e.g. same position, link, etc.).
	//			- change in state (e.g. |icst -> event -> icst).
	//			- functional values (e.g. y==f(x)) (TODO).
	//			- functional relations (e.g. o1 and o2 in the same relation wrt F as o3 and o3; relation: in or out; e.g. y==F(x), z==f(x) then as above with the constraint y==g(z) or vice-versa) (TODO).
	//
	//	Proxy in usr_operators.dll.
	//	Implementation class in pattern_detector.cpp.
	class	r_exec_dll	PatternDetector:
	public	_Object{
	protected:
		PatternDetector();
	public:
		static	PatternDetector	*New(Group	*storage_group,Group	**output_groups,uint16	output_group_count);
		virtual	~PatternDetector();
		virtual	void	take_input(Code	*input)=0;
	};
}


#endif