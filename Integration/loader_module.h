#ifndef	loader_module_h
#define loader_module_h

#include	"integration.h"
#include	"module_node.h"
#include	"mem.h"

#define	N		module::Node
#define	NODE	module::Node::Get()
#define	OUTPUT	NODE->trace(N::APPLICATION)


MODULE_CLASS_BEGIN(Loader,Module<Loader>)
private:
	P<ImageMessage>	image;
	void	initialize();
	void	finalize();
	void	compile(std::string		&filename);
public:
	void	start(){
		initialize();
	}
	void	stop(){
		finalize();
		OUTPUT<<"Loader "<<"stopped"<<std::endl;
	}
	template<class	T>	Decision	decide(T	*p){
		return	WAIT;
	}
	template<class	T>	void	react(T	*p){
		OUTPUT<<"Loader "<<"got message"<<std::endl;
	}
	void	react(SystemReady	*p){
		OUTPUT<<"Loader "<<"got SysReady"<<std::endl;
		std::string		filename("c:/work/replicode/test/test.4.replicode");
		/*std::cout<<" file to compile: ";
		std::cin>>filename;*/
		compile(filename);
		OUTPUT<<"Loader "<<"started"<<std::endl;
		if(image!=NULL)
			NODE->send(this,image,N::PRIMARY);
	}
MODULE_CLASS_END(Loader)


#endif
