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
	// Load.
	std::string	usr_operator_path;
	std::string	usr_class_path;
	std::string source_file_name;

	// Init.
	core::uint32	base_period;
	core::uint32	reduction_core_count;
	core::uint32	time_core_count;

	// System.
	core::float32	mdl_inertia_sr_thr;
	core::uint32	mdl_inertia_cnt_thr;
	core::float32	tpx_dsr_thr;
	core::uint32	min_sim_time_horizon;
	core::uint32	max_sim_time_horizon;
	core::float32	sim_time_horizon;
	core::uint32	tpx_time_horizon;
	core::uint32	perf_sampling_period;
	core::float32	float_tolerance;
	core::uint32	time_tolerance;
	core::uint64	primary_thz;
	core::uint64	secondary_thz;

	// Debug.
	bool			debug;
	core::uint32	ntf_mk_resilience;
	core::uint32	goal_pred_success_resilience;

	//Run.
	core::uint32	run_time;
	core::uint32	probe_level;
	bool			decompile_image;
	bool			ignore_named_objects;
	bool			write_image;
	std::string		image_path;
	bool			test_image;

	bool	load(const	char	*file_name){

		core::XMLNode	mainNode=core::XMLNode::openFileHelper(file_name,"TestConfiguration");
		if(!mainNode){

			std::cerr<<"> Error: TestConfiguration is unreadable"<<std::endl;
			return	false;
		}
		
		core::XMLNode	load=mainNode.getChildNode("Load");
		if(!!load){

			usr_operator_path=load.getAttribute("usr_operator_path");
			usr_class_path=load.getAttribute("usr_class_path");
			source_file_name=load.getAttribute("source_file_name");
		}else{

			std::cerr<<"> Error: Load section is unreadable"<<std::endl;
			return	false;
		}

		core::XMLNode	init=mainNode.getChildNode("Init");
		if(!!init){

			const	char	*_base_period=init.getAttribute("base_period");
			const	char	*_reduction_core_count=init.getAttribute("reduction_core_count");
			const	char	*_time_core_count=init.getAttribute("time_core_count");
			
			base_period=atoi(_base_period);
			reduction_core_count=atoi(_reduction_core_count);
			time_core_count=atoi(_time_core_count);
		}else{

			std::cerr<<"> Error: Init section is unreadable"<<std::endl;
			return	false;
		}

		core::XMLNode	system=mainNode.getChildNode("System");
		if(!!system){

			const	char	*_mdl_inertia_sr_thr=system.getAttribute("mdl_inertia_sr_thr");
			const	char	*_mdl_inertia_cnt_thr=system.getAttribute("mdl_inertia_cnt_thr");
			const	char	*_tpx_dsr_thr=system.getAttribute("tpx_dsr_thr");
			const	char	*_min_sim_time_horizon=system.getAttribute("min_sim_time_horizon");
			const	char	*_max_sim_time_horizon=system.getAttribute("max_sim_time_horizon");
			const	char	*_sim_time_horizon=system.getAttribute("sim_time_horizon");
			const	char	*_tpx_time_horizon=system.getAttribute("tpx_time_horizon");
			const	char	*_perf_sampling_period=system.getAttribute("perf_sampling_period");
			const	char	*_float_tolerance=system.getAttribute("float_tolerance");
			const	char	*_time_tolerance=system.getAttribute("time_tolerance");
			const	char	*_primary_thz=system.getAttribute("primary_thz");
			const	char	*_secondary_thz=system.getAttribute("secondary_thz");

			mdl_inertia_sr_thr=atof(_mdl_inertia_sr_thr);
			mdl_inertia_cnt_thr=atoi(_mdl_inertia_cnt_thr);
			tpx_dsr_thr=atof(_tpx_dsr_thr);
			min_sim_time_horizon=atoi(_min_sim_time_horizon);
			max_sim_time_horizon=atoi(_max_sim_time_horizon);
			sim_time_horizon=atof(_sim_time_horizon);
			tpx_time_horizon=atoi(_tpx_time_horizon);
			perf_sampling_period=atoi(_perf_sampling_period);
			float_tolerance=atof(_float_tolerance);
			time_tolerance=atoi(_time_tolerance);
			primary_thz=atoi(_primary_thz);
			secondary_thz=atoi(_secondary_thz);
		}else{

			std::cerr<<"> Error: System section is unreadable"<<std::endl;
			return	false;
		}

		core::XMLNode	debug=mainNode.getChildNode("Debug");
		if(!!debug){

			const	char	*_debug=debug.getAttribute("debug");
			this->debug=(strcmp(_debug,"yes")==0);

			core::XMLNode	resilience=debug.getChildNode("Resilience");
			if(!!resilience){

				const	char	*_ntf_mk_resilience=resilience.getAttribute("ntf_mk_resilience");
				const	char	*_goal_pred_success_resilience=resilience.getAttribute("goal_pred_success_resilience");

				ntf_mk_resilience=atoi(_ntf_mk_resilience);
				goal_pred_success_resilience=atoi(_goal_pred_success_resilience);
			}else{

				std::cerr<<"> Error: Debug/Resilience section is unreadable"<<std::endl;
				return	false;
			}
		}else{

			std::cerr<<"> Error: Debug section is unreadable"<<std::endl;
			return	false;
		}

		core::XMLNode	run=mainNode.getChildNode("Run");
		if(!!run){

			const	char	*_run_time=run.getAttribute("run_time");
			const	char	*_probe_level=run.getAttribute("probe_level");
			const	char	*_decompile_image=run.getAttribute("decompile_image");
			const	char	*_ignore_named_objects=run.getAttribute("ignore_named_objects");
			const	char	*_write_image=run.getAttribute("write_image");
			const	char	*_test_image=run.getAttribute("test_image");

			run_time=atoi(_run_time);
			probe_level=atoi(_probe_level);
			decompile_image=(strcmp(_decompile_image,"yes")==0);
			ignore_named_objects=(strcmp(_ignore_named_objects,"yes")==0);
			write_image=(strcmp(_write_image,"yes")==0);
			if(write_image){

				image_path=run.getAttribute("image_path");
				test_image=(strcmp(_test_image,"yes")==0);
			}
		}else{

			std::cerr<<"> Error: Run section is unreadable"<<std::endl;
			return	false;
		}

		return	true;
	}
};


#endif