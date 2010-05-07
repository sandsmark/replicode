#ifndef	r_mem_module_h
#define r_mem_module_h

#include	"integration.h"
#include	"module_node.h"
#include	"mem.h"
#include	"segments.h"

#define	N		module::Node
#define	NODE	module::Node::Get()
#define	OUTPUT	NODE->trace(N::APPLICATION)


MODULE_CLASS_BEGIN(RMem,Module<RMem>)
private:
	r_exec::Mem	*mem;
	void	initialize();
	void	finalize();
	void	decompile(r_comp::Image*image);
	void	load(r_comp::Image*image);
public:
	void	start(){
		initialize();
		OUTPUT<<"RMem "<<"started"<<std::endl;
	}
	void	stop(){
		finalize();
		OUTPUT<<"RMem "<<"stopped"<<std::endl;
	}
	template<class	T>	Decision	decide(T	*p){
		return	WAIT;
	}
	template<class	T>	void	react(T	*p){
		OUTPUT<<"RMem "<<"got message"<<std::endl;
	}
	void	react(SystemReady	*p){
		OUTPUT<<"RMem "<<"got SysReady"<<std::endl;
	}
	void	react(ImageMessage	*p){
		OUTPUT<<"RMem "<<"got an image"<<std::endl;
		r_comp::Image	*image=new	r_comp::Image();
		image->load<ImageMessage>(p);
		decompile(image);	//	for debugging
		load(image);		//	load received objects in the rMem
		delete	image;
	}
MODULE_CLASS_END(RMem)


#endif
