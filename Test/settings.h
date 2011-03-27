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

#ifndef	settings_h
#define	settings_h

#include	<../../CoreLibrary/trunk/CoreLibrary/xml_parser.h>


class	Settings{
public:
	uint64		base_period;
	uint32		reduction_core_count;
	uint32		time_core_count;
	uint32		notification_resilience;
	uint32		goal_resilience;
	uint32		assumption_resilience;
	uint32		simulation_resilience;
	float32		float_tolerance;
	float32		time_tolerance;
	uint32		run_time;
	uint32		probe_level;
	std::string	usr_operator_path;
	std::string	usr_class_path;
	std::string source_file_name;
	bool		decompile_image;
	bool		write_image;
	std::string	image_path;
	bool		test_image;

	bool	load(const	char	*file_name){

		XMLNode	mainNode=XMLNode::openFileHelper(file_name,"TestConfiguration");
		if(!mainNode){

			std::cerr<<"> Error: TestConfiguration is unreadable"<<std::endl;
			return	false;
		}
		
		XMLNode	parameters=mainNode.getChildNode("Parameters");
		if(!!parameters){

			const	char	*_base_period=parameters.getAttribute("base_period");
			const	char	*_reduction_core_count=parameters.getAttribute("reduction_core_count");
			const	char	*_time_core_count=parameters.getAttribute("time_core_count");
			const	char	*_notification_resilience=parameters.getAttribute("notification_resilience");
			const	char	*_goal_resilience=parameters.getAttribute("goal_resilience");
			const	char	*_assumption_resilience=parameters.getAttribute("assumption_resilience");
			const	char	*_simulation_resilience=parameters.getAttribute("simulation_resilience");
			const	char	*_float_tolerance=parameters.getAttribute("float_tolerance");
			const	char	*_time_tolerance=parameters.getAttribute("time_tolerance");
			const	char	*_run_time=parameters.getAttribute("run_time");
			const	char	*_probe_level=parameters.getAttribute("probe_level");
			const	char	*_decompile_image=parameters.getAttribute("decompile_image");
			const	char	*_write_image=parameters.getAttribute("write_image");
			const	char	*_test_image=parameters.getAttribute("test_image");

			usr_operator_path=parameters.getAttribute("usr_operator_path");
			usr_class_path=parameters.getAttribute("usr_class_path");
			source_file_name=parameters.getAttribute("source_file_name");

			base_period=atoi(_base_period);
			reduction_core_count=atoi(_reduction_core_count);
			time_core_count=atoi(_time_core_count);
			notification_resilience=atoi(_notification_resilience);
			goal_resilience=atoi(_goal_resilience);
			assumption_resilience=atoi(_assumption_resilience);
			simulation_resilience=atoi(_simulation_resilience);
			float_tolerance=atof(_float_tolerance);
			time_tolerance=atof(_time_tolerance);
			run_time=atoi(_run_time);
			probe_level=atoi(_probe_level);
			decompile_image=(strcmp(_decompile_image,"yes")==0);
			write_image=(strcmp(_write_image,"yes")==0);
			if(write_image){

				image_path=parameters.getAttribute("image_path");
				test_image=(strcmp(_test_image,"yes")==0);
			}
		}else{

			std::cerr<<"> Error: Parameter section is unreadable"<<std::endl;
			return	false;
		}

		return	true;
	}
};


#endif