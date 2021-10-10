#ifndef REPLISTRUCT_H
#define REPLISTRUCT_H

#include <cstdint>        // for uint64_t, int64_t
#include <iosfwd>         // for ostream, istream
#include <list>           // for list
#include <string>         // for string, basic_string, allocator
#include <unordered_map>  // for unordered_map
#include <vector>         // for vector
#include <memory>

namespace r_comp
{

class RepliCondition;
class RepliMacro;

class RepliStruct
{
public:
    typedef std::shared_ptr<RepliStruct> Ptr;
    static std::unordered_map<std::string, std::shared_ptr<RepliMacro>> s_macros;
    static std::unordered_map<std::string, int64_t> s_counters;
    static std::list<RepliCondition *> s_conditions;
    static uint64_t GlobalLine;
    static std::string GlobalFilename;
    static void cleanup();

    enum Type {Root, Structure, Set, Atom, Directive, Condition, Development};
    Type type;
    std::string cmd;
    std::string tail;
    std::string label;
    std::string error;
    std::string fileName;
    uint64_t line;
    std::vector<std::shared_ptr<RepliStruct>> args;
    RepliStruct *parent;

    RepliStruct(r_comp::RepliStruct::Type type);
    ~RepliStruct();

    void reset(); // remove rags that are objects.

    uint64_t getIndent(std::istream *stream);
    int64_t parse(std::istream *stream, uint64_t &curIndent, uint64_t &prevIndent, int64_t paramExpect = 0);
    bool parseDirective(std::istream *stream, uint64_t &curIndent, uint64_t &prevIndent);
    int64_t process();

    RepliStruct::Ptr findAtom(const std::string &name);
    RepliStruct::Ptr loadReplicodeFile(const std::string &filename);

    RepliStruct::Ptr clone() const;
    std::string print() const;
    std::string printError() const;

    friend std::ostream& operator<<(std::ostream &os, const RepliStruct &structure);
    friend std::ostream& operator<<(std::ostream &os, RepliStruct::Ptr structure);
private:
    RepliStruct(const RepliStruct &) = delete;
    RepliStruct() = delete;
};

class RepliMacro
{
public:
    typedef std::shared_ptr<RepliMacro> Ptr;
    std::string name;
    RepliStruct::Ptr src;
    RepliStruct::Ptr dest;
    std::string error;

    RepliMacro(const std::string &name, RepliStruct::Ptr src, RepliStruct::Ptr dest);
    ~RepliMacro();

    uint64_t argCount();
    RepliStruct::Ptr expandMacro(RepliStruct *oldStruct);
};

class RepliCondition
{
public:
    std::string name;
    bool reversed;

    RepliCondition(const std::string &name, bool reversed);
    ~RepliCondition();
    bool reverse();
    bool isActive(std::unordered_map<std::string, RepliMacro::Ptr> &RepliMacros, std::unordered_map<std::string, int64_t> &Counters);
};


}

#endif//REPLISTRUCT_H
