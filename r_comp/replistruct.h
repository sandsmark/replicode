#ifndef REPLISTRUCT_H
#define REPLISTRUCT_H

#include "CoreLibrary/types.h"
#include <list>

namespace r_comp {

class RepliMacro;
class RepliCondition;
class RepliStruct {
public:
    static UNORDERED_MAP<std::string, RepliMacro *> RepliMacros;
    static UNORDERED_MAP<std::string, int32_t> Counters;
    static std::list<RepliCondition *> Conditions;
    static uint32_t GlobalLine;
    static std::string GlobalFilename;

    enum Type {Root, Structure, Set, Atom, Directive, Condition, Development};
    Type type;
    std::string cmd;
    std::string tail;
    std::string label;
    std::string error;
    std::string fileName;
    uint32_t line;
    std::list<RepliStruct *> args;
    RepliStruct *parent;

    RepliStruct(r_comp::RepliStruct::Type type);
    ~RepliStruct();

    void reset(); // remove rags that are objects.

    uint32_t getIndent(std::istream *stream);
    int32_t parse(std::istream *stream, uint32_t &curIndent, uint32_t &prevIndent, int32_t paramExpect = 0);
    bool parseDirective(std::istream *stream, uint32_t &curIndent, uint32_t &prevIndent);
    int32_t process();

    RepliStruct *findAtom(const std::string &name);
    RepliStruct *loadReplicodeFile(const std::string &filename);

    RepliStruct *clone() const;
    std::string print() const;
    std::string printError() const;

    friend std::ostream& operator<<(std::ostream &os, const RepliStruct &structure);
    friend std::ostream& operator<<(std::ostream &os, RepliStruct *structure);
private:
    RepliStruct(const RepliStruct &);
    RepliStruct();
};

class RepliMacro {
public:
    std::string name;
    RepliStruct *src;
    RepliStruct *dest;
    std::string error;

    RepliMacro(const std::string &name, RepliStruct *src, RepliStruct *dest);
    ~RepliMacro();

    uint32_t argCount();
    RepliStruct *expandMacro(RepliStruct *oldStruct);
};

class RepliCondition {
public:
    std::string name;
    bool reversed;

    RepliCondition(const std::string &name, bool reversed);
    ~RepliCondition();
    bool reverse();
    bool isActive(UNORDERED_MAP<std::string, RepliMacro*> &RepliMacros, UNORDERED_MAP<std::string, int32_t> &Counters);
};


}

#endif//REPLISTRUCT_H
