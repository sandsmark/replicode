#ifndef	sample_io_module_h
#define sample_io_module_h

#include	"integration.h"
#include	"module_node.h"
#include	"mem.h"

#define	N		module::Node
#define	NODE	module::Node::Get()
#define	OUTPUT	NODE->trace(N::APPLICATION)


MODULE_CLASS_BEGIN(SampleIO,Module<SampleIO>)
private:
	r_exec::Mem	*mem;
	void	initialize();
	void	finalize();
public:
	void	start(){
		initialize();
		OUTPUT<<"SampleIO "<<"started"<<std::endl;
	}
	void	stop(){
		finalize();
		OUTPUT<<"SampleIO "<<"stopped"<<std::endl;
	}
	template<class	T>	Decision	decide(T	*p){
		return	WAIT;
	}
	template<class	T>	void	react(T	*p){
		OUTPUT<<"SampleIO "<<"got message"<<std::endl;
	}
	void	react(SystemReady	*p){
		OUTPUT<<"SampleIO "<<"got SysReady"<<std::endl;
	}
MODULE_CLASS_END(SampleIO)


#endif
