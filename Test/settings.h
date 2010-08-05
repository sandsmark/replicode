#ifndef	settings_h
#define	settings_h

#include	<../CoreLibrary/xml_parser.h>


class	Settings{
public:
	uint64	base_period;
	uint32	reduction_core_count;
	uint32	time_core_count;
	uint32	notification_resilience;
	uint32	run_time;
	typedef	enum{
		TS_RELATIVE=0,
		TS_ABSOLUTE=1
	}TSMode;
	TSMode	decompile_timestamps;
	std::string	usr_operator_path;
	std::string	usr_class_path;
	std::string source_file_name;

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
			const	char	*_run_time=parameters.getAttribute("run_time");
			const	char	*_decompile_timestamps=parameters.getAttribute("decompile_timestamps");

			usr_operator_path=parameters.getAttribute("usr_operator_path");
			usr_class_path=parameters.getAttribute("usr_class_path");
			source_file_name=parameters.getAttribute("source_file_name");

			base_period=atoi(_base_period);
			reduction_core_count=atoi(_reduction_core_count);
			time_core_count=atoi(_time_core_count);
			notification_resilience=atoi(_notification_resilience);
			run_time=atoi(_run_time);
			if(strcmp(_decompile_timestamps,"relative")==0)
				decompile_timestamps=TS_RELATIVE;
			else
				decompile_timestamps=TS_ABSOLUTE;
		}else{

			std::cerr<<"> Error: Parameter section is unreadable"<<std::endl;
			return	false;
		}

		return	true;
	}
};


#endif