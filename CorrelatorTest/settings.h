//	settings.h
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

#ifndef	test_correlator_settings_h
#define	test_correlator_settings_h

#include	<../CoreLibrary/xml_parser.h>


class	CorrelatorTestSettings{
public:
	typedef	enum{
		TS_RELATIVE=0,
		TS_ABSOLUTE=1
	}TSMode;
	TSMode		decompile_timestamps;
	std::string	usr_operator_path;
	std::string	usr_class_path;
	std::string	use_case_path;
	std::string	use_case_name;
	uint32		episode_count;

	bool	load(const	char	*file_name){

		XMLNode	mainNode=XMLNode::openFileHelper(file_name,"TestConfiguration");
		if(!mainNode){

			std::cerr<<"> Error: TestConfiguration is unreadable"<<std::endl;
			return	false;
		}
		
		XMLNode	parameters=mainNode.getChildNode("Parameters");
		if(!!parameters){

			const	char	*_decompile_timestamps=parameters.getAttribute("decompile_timestamps");
			const	char	*_episode_count=parameters.getAttribute("episode_count");

			usr_operator_path=parameters.getAttribute("usr_operator_path");
			usr_class_path=parameters.getAttribute("usr_class_path");

			use_case_path=parameters.getAttribute("use_case_path");
			use_case_name=parameters.getAttribute("use_case_name");

			if(strcmp(_decompile_timestamps,"relative")==0)
				decompile_timestamps=TS_RELATIVE;
			else
				decompile_timestamps=TS_ABSOLUTE;

			episode_count=atoi(_episode_count);
		}else{

			std::cerr<<"> Error: Parameter section is unreadable"<<std::endl;
			return	false;
		}

		return	true;
	}
};


#endif