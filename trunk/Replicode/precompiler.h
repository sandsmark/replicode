#ifndef	precompiler_h
#define	precompiler_h

#include	"types.h"
#include	<istream>
#include	<sstream>
#include	"config.h"
#include	"StringUtils.h"
#include	<fstream>

namespace	replicode{

//	!def now (_now)
//	!def (inj args) (CALL _inj exe args)
//	!def (macro arg1 arg2 arg3) (CALL _inj exe arg1 bla arg2 bla arg3)


	

class RepliMacro;
class RepliCondition;
class RepliStruct {
public:
	// Global members for all instances
	static UNORDERED_MAP<std::string,RepliMacro*> repliMacros;
	static UNORDERED_MAP<std::string,int> counters;
	static std::list<RepliCondition*> conditions;
	static unsigned int globalLine;

	enum Type {Root, Structure, Set, Atom, Directive, Condition, Development};
	Type type;
	std::string cmd;
	std::string tail;
	std::string label;
	std::string error;
	unsigned int line;
	std::list<RepliStruct*> args;
	RepliStruct* parent;

	RepliStruct(RepliStruct::Type type);
	~RepliStruct();

	unsigned int getIndent(std::istream *stream);
	int parse(std::istream *stream, unsigned int &curIndent, unsigned int &prevIndent, int paramExpect = 0);
	bool parseDirective(std::istream *stream, unsigned int &curIndent, unsigned int &prevIndent);
	int process();

	RepliStruct* findAtom(const std::string &name);
	RepliStruct* loadReplicodeFile(const std::string &filename);

	RepliStruct* clone() const;
	std::string print() const;
	std::string printError() const;

	friend std::ostream& operator<<(std::ostream& os, const RepliStruct& structure);
	friend std::ostream& operator<<(std::ostream& os, RepliStruct* structure);
};

class RepliMacro {
public:
	std::string name;
	RepliStruct* src;
	RepliStruct* dest;
	std::string error;

	RepliMacro(const std::string &name, RepliStruct* src, RepliStruct* dest);
	~RepliMacro();

	unsigned int argCount();
	RepliStruct* expandMacro(RepliStruct* oldStruct);

};


class RepliCondition {
public:
	std::string name;
	bool reversed;

	RepliCondition(const std::string &name, bool reversed);
	~RepliCondition();
	bool reverse();
	bool isActive(UNORDERED_MAP<std::string,RepliMacro*> &repliMacros, UNORDERED_MAP<std::string,int> &counters);
};



class dll_export PreCompiler {
private:
	
public:
	RepliStruct* root;

	PreCompiler();
	~PreCompiler();

	bool preCompile(std::istream			*stream,	//	if an ifstream, stream must be open
					std::ostringstream		*outstream,	//	empty stream to be filled by function
					std::string				*error);	//	set when function fails, e.g. returns false
};

}


#endif
