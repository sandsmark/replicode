//	pattern_detector.cpp
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

#include	"pattern_detector.h"
#include	"../r_exec/mem.h"


namespace	r_exec{

	PatternDetectorOverlay::PatternDetectorOverlay(PatternDetector	*detector):_Object(){

		this->detector=detector;
	}
	
	PatternDetectorOverlay::~PatternDetectorOverlay(){
	}

	void	PatternDetectorOverlay::reduce(Code	*input){

	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class	_PatternDetector:
	public	PatternDetector{
	private:
		CriticalSection							reductionCS;
		std::list<P<PatternDetectorOverlay> >	overlays;

		P<Group>				storage_group;
		std::vector<P<Group> >	output_groups;
	public:
		_PatternDetector(Group	*storage_group,Group	**output_groups,uint16	output_group_count):PatternDetector(){

			this->storage_group=storage_group;
			for(uint16	i=0;i<output_group_count;++i)
				this->output_groups.push_back(output_groups[i]);
			delete[]	output_groups;	//	output_groups allocated by the proxy (avoids passing stl containers across dlls).
		}

		~_PatternDetector(){
		}

		void	take_input(Code	*input){

			if(input->code(0).asOpcode()!=Opcodes::Fact	&&	input->code(0).asOpcode()!=Opcodes::AntiFact)
				return;

			reductionCS.enter();
			std::list<P<PatternDetectorOverlay> >::const_iterator	o;
			for(o=overlays.begin();o!=overlays.end();++o){

				(*o)->reduce(input);
			}
			reductionCS.leave();
		}
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PatternDetector	*PatternDetector::New(Group	*storage_group,Group	**output_groups,uint16	output_group_count){

		return	new	_PatternDetector(storage_group,output_groups,output_group_count);
	}

	PatternDetector::PatternDetector():_Object(){
	}

	PatternDetector::~PatternDetector(){
	}
}