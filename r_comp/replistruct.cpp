#include "replistruct.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace r_comp {

std::unordered_map<std::string, RepliMacro *> RepliStruct::RepliMacros;
std::unordered_map<std::string, int64_t> RepliStruct::Counters;
std::list<RepliCondition *> RepliStruct::Conditions;
uint64_t RepliStruct::GlobalLine = 1;
std::string RepliStruct::GlobalFilename;

RepliStruct::RepliStruct(RepliStruct::Type type) {
//    std::cout << "creating strucT: " << GlobalFilename << GlobalLine << std::endl;
    this->type = type;
    this->fileName = GlobalFilename;
    line = GlobalLine;
    parent = NULL;
}

RepliStruct::~RepliStruct() {
    parent = NULL;
}

void RepliStruct::reset() {
    GlobalLine = 1;
    std::vector<RepliStruct*>::iterator arg;
    for (arg = args.begin(); arg != args.end();) {

        switch ((*arg)->type) {
        case RepliStruct::Atom:
        case RepliStruct::Structure:
        case RepliStruct::Development:
        case RepliStruct::Set:
            arg = args.erase(arg);
            break;
        default:
            ++arg;
        }
    }
}

uint64_t RepliStruct::getIndent(std::istream *stream) {

    uint64_t count = 0;
    while (!stream->eof()) {
        switch (stream->get()) {
        case '\n':
            GlobalLine++;
        case '\r':
            stream->seekg(-1, std::ios_base::cur);
            return count / 3;
        case ' ':
            count++;
            break;
        default:
            stream->seekg(-1, std::ios_base::cur);
            return count / 3;
        }
    }
    return count / 3;
}

int64_t RepliStruct::parse(std::istream *stream, uint64_t &curIndent, uint64_t &prevIndent, int64_t paramExpect) {

    char c = 0, lastc = 0, lastcc, tc;
    std::string str, label;
    RepliStruct* subStruct;

    int64_t paramCount = 0;
    int64_t returnIndent = 0;

    bool inComment = false;
    bool expectSet = false;

    while (!stream->eof()) {
        lastcc = lastc;
        lastc = c;
        c = stream->get();
// printf("%c", c);
        switch (c) {
        case '\t':
            if (inComment) continue; // allow tabs in comments, does not matter anyway
            line = GlobalLine;
            error += "Tabs chars are not permitted. ";
            return -1;
        case '!':
            if (inComment) continue;
            if (this->type == Root) {
                subStruct = new RepliStruct(Directive);
                subStruct->parent = this;
                args.push_back(subStruct);
                if (!subStruct->parseDirective(stream, curIndent, prevIndent))
                    return -1;
            }
            else {
                error += "Directive not allowed inside a structure. ";
                return -1;
            }
            break;
        case '\n':
            GlobalLine++;
        case '\r':
// remain inComment?
            if (inComment) {
                if ((lastc == ' ') || ((lastc == '.') && (lastcc == '.')))
                    continue; // continue comment on next line
                else
                    inComment = false; // end comment
            }
// skip all CR and LF
            while ((!stream->eof()) && (stream->peek() < 32))
                if (stream->get() == '\n')
                    GlobalLine++;
// get indents
            prevIndent = curIndent;
            curIndent = getIndent(stream);
// Are we in a parenthesis or set
            if (curIndent > prevIndent) {
// Are we in a set
                if (expectSet) {
// Add new sub structure
                    subStruct = new RepliStruct(Set);
                    subStruct->parent = this;
                    subStruct->label = label;
                    label = "";
                    args.push_back(subStruct);
                    returnIndent = subStruct->parse(stream, curIndent, prevIndent);
                    expectSet = false;
                    if (returnIndent < 0)
                        return -1;
                    if ((paramExpect > 0) && (++paramCount == paramExpect))
                        return 0;
                    if (returnIndent > 0)
                        return (returnIndent - 1);
                }
// or a parenthesis
                else {
                    subStruct = new RepliStruct(Structure);
                    args.push_back(subStruct);
                    returnIndent = subStruct->parse(stream, curIndent, prevIndent);
                    expectSet = false;
                    if (returnIndent < 0)
                        return -1;
                    if ((paramExpect > 0) && (++paramCount == paramExpect))
                        return 0;
                    if (returnIndent > 0)
                        return (returnIndent - 1);
                }
            }
            else if (curIndent < prevIndent) {
                if (str.size() > 0) {
                    if ((cmd.size() > 0) || (type == Set)) {
                        subStruct = new RepliStruct(Atom);
                        subStruct->parent = this;
                        args.push_back(subStruct);
                        subStruct->cmd = str;
                        str = "";
                        if ((paramExpect > 0) && (++paramCount == paramExpect))
                            return 0;
                    }
                    else {
                        cmd = str;
                        str = "";
                    }
                }
// current structure or set is complete
                return prevIndent - curIndent - 1;
            }
            else {
// act as if we met a space
                if (str.size() > 0) {
                    if ((cmd.size() > 0) || (type == Set) || (type == Root)) {

                        subStruct = new RepliStruct(Atom);
                        subStruct->parent = this;
                        args.push_back(subStruct);
                        subStruct->cmd = str;
                        str = "";
                        if ((paramExpect > 0) && (++paramCount == paramExpect))
                            return 0;
                    }
                    else {
                        cmd = str;
                        str = "";
                    }
                }
            }
            break;
        case ';':
            inComment = true;
            break;
        case ' ':
            if (inComment) continue;
// next string is ready
            if (str.size() > 0) {
                if ((cmd.size() > 0) || (type == Set) || (type == Development)) { // Modification from Eric to have the Development treat tpl vars as atoms instead of a Development.cmd
                    // Check if we need to read more tokens for the string
                    if (str[0] == '"' && str[str.length() - 1] != '"') {
                        str += ' ';
                        while (!stream->eof()) {
                            c = stream->get();
                            str += c;
                            if (c == '"') {
                                break;
                            }
                        }
                    }
                    subStruct = new RepliStruct(Atom);
                    subStruct->parent = this;
                    args.push_back(subStruct);
                    subStruct->cmd = str;
                    str = "";
                    if ((paramExpect > 0) && (++paramCount == paramExpect))
                        return 0;
                }
                else {
                    cmd = str;
                    str = "";
                }
            }
            break;
        case '(':
            if (inComment) continue;
// Check for scenario 'xxx('
            if (str.size() > 0) {
                if (lastc == ':') { // label:(xxx)
                    label = str;
                    str = "";
                }
                else if ((cmd.size() > 0) || (type == Set)) {
                    subStruct = new RepliStruct(Atom);
                    subStruct->parent = this;
                    args.push_back(subStruct);
                    subStruct->cmd = str;
                    str = "";
                    if ((paramExpect > 0) && (++paramCount == paramExpect))
                        return 0;
                }
                else {
                    cmd = str;
                    str = "";
                }
            }
            subStruct = new RepliStruct(Structure);
            subStruct->label = label;
            label = "";
            subStruct->parent = this;
            args.push_back(subStruct);
            returnIndent = subStruct->parse(stream, curIndent, prevIndent);
            if (returnIndent < 0)
                return -1;
            if ((paramExpect > 0) && (++paramCount == paramExpect))
                return 0;
            if (returnIndent > 0)
                return (returnIndent - 1);
            break;
        case ')':
            if (inComment) continue;
// Check for directive use of xxx):xxx or xxx):
            if (stream->peek() == ':') {
// expect ':' or ':xxx'
                while ((!stream->eof()) && ((c = stream->get()) > 32))
                    tail += c;
            }
// We met a boundary, act as ' '
            if (str.size() > 0) {
                if ((cmd.size() > 0) || (type == Set)) {
                    subStruct = new RepliStruct(Atom);
                    subStruct->parent = this;
                    args.push_back(subStruct);
                    subStruct->cmd = str;
                    str = "";
                    if ((paramExpect > 0) && (++paramCount == paramExpect))
                        return 0;
                }
                else {
                    cmd = str;
                    str = "";
                }
            }
            return 0;
        case '{':
            if (inComment) continue;
// Check for scenario 'xxx{'
            if (str.size() > 0) {
                if (lastc == ':') { // label:{xxx}
                    label = str;
                    str = "";
                }
                else if ((cmd.size() > 0) || (type == Set)) {
                    subStruct = new RepliStruct(Atom);
                    subStruct->parent = this;
                    args.push_back(subStruct);
                    subStruct->cmd = str;
                    str = "";
                    if ((paramExpect > 0) && (++paramCount == paramExpect))
                        return 0;
                }
                else {
                    cmd = str;
                    str = "";
                }
            }
            subStruct = new RepliStruct(Development);
            subStruct->label = label;
            label = "";
            subStruct->parent = this;
            args.push_back(subStruct);
            returnIndent = subStruct->parse(stream, curIndent, prevIndent);
            if (returnIndent < 0)
                return -1;
            if ((paramExpect > 0) && (++paramCount == paramExpect))
                return 0;
            if (returnIndent > 0)
                return (returnIndent - 1);
            break;
        case '}':
            if (inComment) continue;
// Check for directive use of xxx):xxx or xxx):
            if (stream->peek() == ':') {
// expect ':' or ':xxx'
                while ((!stream->eof()) && ((c = stream->get()) > 32))
                    tail += c;
            }
// We met a boundary, act as ' '
            if (str.size() > 0) {
                if ((cmd.size() > 0) || (type == Set) || (type == Development)) { // Modification from Eric to have the Development treat tpl vars as atoms instead of a Development.cmd
                    subStruct = new RepliStruct(Atom);
                    subStruct->parent = this;
                    args.push_back(subStruct);
                    subStruct->cmd = str;
                    str = "";
                    if ((paramExpect > 0) && (++paramCount == paramExpect))
                        return 0;
                }
                else {
                    cmd = str;
                    str = "";
                }
            }
            return 0;
        case '|':
            if (inComment) continue;
            if (stream->peek() == '[') {
                stream->get(); // read the [
                stream->get(); // read the ]
                str += "|[]";
            }
            else
                str += c;
            break;
        case '[': // set start
            if (inComment) continue;
            if (lastc == ':') { // label:[xxx]
                label = str;
                str = "";
            }
            if (stream->peek() == ']') {
                stream->get(); // read the ]
// if we have a <CR> or <LF> next we may have a line indent set
                if (((tc = stream->peek()) < 32) || (tc == ';'))
                    expectSet = true;
                else {
// this could be a xxx:[] or xxx[]
                    if ((lastc != ':') && (lastc > 32)) { // label[]
// act as if [] is part of the string and continue
                        str += "[]";
                        continue;
                    }
// create empty set
                    subStruct = new RepliStruct(Set);
                    subStruct->parent = this;
                    subStruct->label = label;
                    args.push_back(subStruct);
                }
            }
            else {
// Check for scenario 'xxx['
                if (str.size() > 0) {
                    if ((cmd.size() > 0) || (type == Set)) {
                        subStruct = new RepliStruct(Atom);
                        subStruct->parent = this;
                        args.push_back(subStruct);
                        subStruct->cmd = str;
                        str = "";
                        if ((paramExpect > 0) && (++paramCount == paramExpect))
                            return 0;
                    }
                    else {
                        cmd = str;
                        str = "";
                    }
                }
                subStruct = new RepliStruct(Set);
                subStruct->parent = this;
                subStruct->label = label;
                label = "";
                args.push_back(subStruct);
                returnIndent = subStruct->parse(stream, curIndent, prevIndent);
                if (returnIndent < 0)
                    return -1;
                if ((paramExpect > 0) && (++paramCount == paramExpect))
                    return 0;
                if (returnIndent > 0)
                    return (returnIndent - 1);
            }
            break;
        case ']':
            if (inComment) continue;
// We met a boundary, act as ' '
            if (str.size() > 0) {
                if ((cmd.size() > 0) || (type == Set)) {
                    subStruct = new RepliStruct(Atom);
                    subStruct->parent = this;
                    args.push_back(subStruct);
                    subStruct->cmd = str;
                    str = "";
                    if ((paramExpect > 0) && (++paramCount == paramExpect))
                        return 0;
                }
                else {
                    cmd = str;
                    str = "";
                }
            }
            return 0;
        default:
            if (inComment) continue;
            str += c;
            break;
        }
    }
    return 0;
}


bool RepliStruct::parseDirective(std::istream *stream, uint64_t &curIndent, uint64_t &prevIndent) {

    std::string str = "!";
// RepliStruct* subStruct;
    char c;

// We know that parent read a '!', so first find out which directive
    while ((!stream->eof()) && ((c = stream->peek()) > 32))
        str += stream->get();
    if (stream->eof()) {
        error += "Error in directive formatting, end of file reached unexpectedly. ";
        return false;
    }

    unsigned int paramCount = 0;

    if (str.compare("!def") == 0) // () ()
        paramCount = 2;
    else if (str.compare("!counter") == 0) // xxx val
        paramCount = 2;
    else if (str.compare("!undef") == 0) // xxx
        paramCount = 1;
    else if (str.compare("!ifdef") == 0) { // name
        this->type = Condition;
        paramCount = 1;
    }
    else if (str.compare("!ifundef") == 0) { // name
        this->type = Condition;
        paramCount = 1;
    }
    else if (str.compare("!else") == 0) { //
        this->type = Condition;
        paramCount = 0;
    }
    else if (str.compare("!endif") == 0) { //
        this->type = Condition;
        paramCount = 0;
    }
    else if (str.compare("!class") == 0) // ()
        paramCount = 1;
    else if (str.compare("!op") == 0) // ():xxx
        paramCount = 1;
    else if (str.compare("!dfn") == 0) // ()
        paramCount = 1;
    else if (str.compare("!load") == 0) // xxx
        paramCount = 1;
    else {
        error += "Unknown directive: '" + str + "'. ";
        return false;
    }
    cmd = str;

    if (paramCount == 0) {
// read until end of line, including any comments
        while ((!stream->eof()) && (stream->peek() > 13))
            stream->get();
// read the end of line too
        while ((!stream->eof()) && (stream->peek() < 32))
            if (stream->get() == '\n')
                GlobalLine++;
        return true;
    }

    if (parse(stream, curIndent, prevIndent, paramCount) != 0) {
        error += "Error parsing the arguments for directive '" + cmd + "'. ";
        return false;
    }
    else
        return true;
}


int64_t RepliStruct::process() {
    int64_t changes = 0, count;
    RepliStruct *structure, *newStruct, *tempStruct;
    RepliMacro* macro;
    RepliCondition* cond;
    std::string loadError;

// expand Counters in all structures
    if (Counters.find(cmd) != Counters.end()) {
// expand the counter
        cmd = std::to_string(Counters[cmd]++);
        changes++;
    }
// expand Macros in all structures
    if (RepliMacros.find(cmd) != RepliMacros.end()) {
// expand the macro
        macro = RepliMacros[cmd];
        newStruct = macro->expandMacro(this);
        if (newStruct != NULL) {
            *this = *newStruct;
            delete(newStruct);
            changes++;
        }
        else {
            error = macro->error;
            macro->error = "";
            return -1;
        }
    }

    if (args.size() == 0)
        return changes;

    for (std::vector<RepliStruct*>::iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter) {
        structure = (*iter);

// printf("Processing %s with %d args...\n", structure->cmd.c_str(), structure->args.size());
        if (structure->type == Condition) {
            if (structure->cmd.compare("!ifdef") == 0) {
                cond = new RepliCondition(structure->args.front()->cmd, false);
                Conditions.push_back(cond);
            }
            else if (structure->cmd.compare("!ifundef") == 0) {
                cond = new RepliCondition(structure->args.front()->cmd, true);
                Conditions.push_back(cond);
            }
            else if (structure->cmd.compare("!else") == 0) {
// reverse the current condition
                Conditions.back()->reverse();
            }
            else if (structure->cmd.compare("!endif") == 0) {
                Conditions.pop_back();
            }
            return 0;
        }

// Check Conditions to see if we are active at the moment
        for (std::list<RepliCondition*>::const_iterator iCon(Conditions.begin()), iConEnd(Conditions.end()); iCon != iConEnd; ++iCon) {
// if just one active condition is not active we will ignore the current line
// until we get an !else or !endif
            if (!((*iCon)->isActive(RepliMacros, Counters)))
                return 0;
        }

        if (structure->type == Directive) {
            if (structure->cmd.compare("!counter") == 0) {
// register the counter
                if (structure->args.size() > 1)
                    Counters[structure->args.front()->cmd] = atoi(structure->args.back()->cmd.c_str());
                else
                    Counters[structure->args.front()->cmd] = 0;
            }
            else if (structure->cmd.compare("!def") == 0) {
// check second sub structure only containing macros
                while ((count = structure->args.back()->process()) > 0)
                    changes += count;
                if (count < 0)
                    return -1;
// register the macro
                macro = new RepliMacro(structure->args.front()->cmd, structure->args.front(), structure->args.back());
                RepliMacros[macro->name] = macro;
            }
            else if (structure->cmd.compare("!undef") == 0) {
// remove the counter or macro
                RepliMacros.erase(RepliMacros.find(structure->args.front()->cmd));
                Counters.erase(Counters.find(structure->args.front()->cmd));
            }
            else if (structure->cmd.compare("!load") == 0) {
// Check for a load directive...
                newStruct = loadReplicodeFile(structure->args.front()->cmd);
                if (newStruct == NULL) {
                    structure->error += "Load: File '" + structure->args.front()->cmd + "' cannot be read! ";
                    return -1;
                }
                else if ((loadError = newStruct->printError()).size() > 0) {
                    structure->error = loadError;
                    delete(newStruct);
                    return -1;
                }
// Insert new data into current args
// save location
                tempStruct = (*iter);
// insert new structures
                args.insert(++iter, newStruct->args.begin(), newStruct->args.end());
// reinit iterator and find location again
                iter = args.begin();
                iterEnd = args.end();
                while ((*iter) != tempStruct) iter++;
// we want to remove the !load line, so get the next line
                iter++;
                args.erase(std::find(args.begin(), args.end(), tempStruct));
                //args.erase(std::remove(args.begin(), args.end(), tempStruct), args.end());
// and because we changed the list, repeat
                tempStruct = (*iter);
                iter = args.begin();
                iterEnd = args.end();
                while ((*iter) != tempStruct) iter++;
// now we have replaced the !load line with the loaded lines
                changes++;
            }
        }
        else { // a Structure, Set, Atom or Development
            if (RepliMacros.find(structure->cmd) != RepliMacros.end()) {
// expand the macro
                macro = RepliMacros[structure->cmd];
                newStruct = macro->expandMacro(structure);
                if (newStruct != NULL) {
                    *structure = *newStruct;
                    delete(newStruct);
                    changes++;
                }
                else {
                    structure->error = macro->error;
                    macro->error = "";
                    return -1;
                }
            }

// check for sub structures containing macros
            for (std::vector<RepliStruct*>::iterator iter2(structure->args.begin()), iter2End(structure->args.end()); iter2 != iter2End; ++iter2) {
                if ((count = (*iter2)->process()) > 0)
                    changes += count;
                else if (count < 0)
                    return -1;
            }
        }

// expand Counters in all structures
        if (Counters.find(structure->cmd) != Counters.end()) {
// expand the counter
            structure->cmd = std::to_string(Counters[structure->cmd]++);
            changes++;
        }
    }

    return changes;
}

RepliStruct *RepliStruct::loadReplicodeFile(const std::string &filename) {
    int oldLine = GlobalLine;
    std::string oldFile = GlobalFilename;
    GlobalLine = 1;
    GlobalFilename = filename;

    RepliStruct* newRoot = new RepliStruct(Root);
    std::ifstream loadStream(filename.c_str());
    if (loadStream.bad() || loadStream.fail() || loadStream.eof()) {
        newRoot->error += "Load: File '" + filename + "' cannot be read! ";
        loadStream.close();
        return newRoot;
    }
// create new Root structure
    uint64_t a = 0, b = 0;
    if (newRoot->parse(&loadStream, a, b) < 0) {
// error is already recorded in newRoot
    }
    if (!loadStream.eof())
        newRoot->error = "Code structure error: Unmatched ) or ].\n";
    loadStream.close();
    GlobalLine = oldLine;
    GlobalFilename = oldFile;
    return newRoot;
}


std::string RepliStruct::print() const {

    std::string str;
    switch (type) {
    case Atom:
        return cmd;
    case Structure:
    case Development:
    case Directive:
    case Condition:
        str = cmd;
        for (std::vector<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
            str += " " + (*iter)->print();
        if (type == Structure)
            return label + "(" + str + ")" + tail;
        else if (type == Development)
            return label + "{" + str + "}" + tail;
        else
            return str;
    case Set:
        for (std::vector<RepliStruct*>::const_iterator iter(args.begin()), last(args.end()), iterEnd(last--); iter != iterEnd; ++iter) {
            str += (*iter)->print();
            if (iter != last)
                str += " ";
        }
        return label + "[" + str + "]" + tail;
    case Root:
        for (std::vector<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
            str += (*iter)->print();
        return str;
    default:
        break;
    }
    return str;
}

std::ostream &operator<<(std::ostream &os, RepliStruct *structure) {

    return operator<<(os, *structure);
}
int indentationLevel = 0;
std::ostream &operator<<(std::ostream &os, const RepliStruct &structure) {
    indentationLevel++;
    std::cout << std::endl;
    for (int i=0; i<indentationLevel; i++) {
        std::cout << ' ';
    }
//    std::cout << "cmd: " << structure.cmd << " tail: '" << structure.tail << "' type: ";
    switch (structure.type) {
        case RepliStruct::Atom:
            std::cout << "atom";
            break;
        case RepliStruct::Directive:
            std::cout << "directive";
            break;
        case RepliStruct::Condition:
            std::cout << "condition";
            break;
        case RepliStruct::Structure:
            std::cout << "structure";
            break;
        case RepliStruct::Development:
            std::cout << "development";
            break;
        case RepliStruct::Set:
            std::cout << "set";
            break;
        case RepliStruct::Root:
            std::cout << "root";
            break;
    }
    std::cout << ' ';
 //   std::cout << std::endl;


    switch (structure.type) {
    case RepliStruct::Atom:
        indentationLevel--;
        return os << structure.cmd;
    case RepliStruct::Directive:
    case RepliStruct::Condition:
        indentationLevel--;
        return os;
    case RepliStruct::Structure:
    case RepliStruct::Development:
        if (structure.type == RepliStruct::Structure)
            os << structure.label << "(";
        else if (structure.type == RepliStruct::Development)
            os << structure.label << "{";
        os << structure.cmd;
        for (std::vector<RepliStruct*>::const_iterator iter(structure.args.begin()), iterEnd(structure.args.end()); iter != iterEnd; ++iter)
            os << " " << (*iter);
        if (structure.type == RepliStruct::Structure)
            os << ")" << structure.tail;
        else if (structure.type == RepliStruct::Development)
            os << "}" << structure.tail;
        break;
    case RepliStruct::Set:
        os << structure.label << "[";
        if (structure.args.size() > 0) {
            for (std::vector<RepliStruct*>::const_iterator iter(structure.args.begin()), last(structure.args.end()), iterEnd(last--); iter != iterEnd; ++iter) {
                os << (*iter);
                if (iter != last)
                    os << " ";
            }
        }
        os << "]" << structure.tail;
        break;
    case RepliStruct::Root:
        for (std::vector<RepliStruct*>::const_iterator iter(structure.args.begin()), iterEnd(structure.args.end()); iter != iterEnd; ++iter)
            os << (*iter);// << ";" << structure.fileName << ":" << structure.line << std::endl;
        break;
    default:
        break;
    }

    indentationLevel--;
    return os;
}

RepliStruct *RepliStruct::clone() const {

    RepliStruct* newStruct = new RepliStruct(type);
    newStruct->cmd = cmd;
    newStruct->label = label;
    newStruct->tail = tail;
    newStruct->parent = parent;
    newStruct->error = error;
    newStruct->line = line;
    newStruct->fileName = fileName;
    for (std::vector<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
        newStruct->args.push_back((*iter)->clone());
    return newStruct;
}

std::string RepliStruct::printError() const {

    std::stringstream strError;
    if (error.size() > 0) {
        std::string com = cmd;
        RepliStruct* structure = parent;
        while ((cmd.size() == 0) && (structure != NULL)) {
            com = structure->cmd;
            structure = structure->parent;
        }
        if (com.size() == 0)
            strError << "Error";
        else
            strError << "Error in structure '" << com << "'";
        if (line > 0)
            strError << " line " << line;
        strError << ": " << error << std::endl;
    }
    for (std::vector<RepliStruct*>::const_iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter)
        strError << (*iter)->printError();
    return strError.str();
}

RepliMacro::RepliMacro(const std::string &name, RepliStruct *src, RepliStruct *dest) {

    this->name = name;
    this->src = src;
    this->dest = dest;
}

RepliMacro::~RepliMacro() {

    name = "";
    src = NULL;
    dest = NULL;
}

uint64_t RepliMacro::argCount() {

    if (src == NULL)
        return 0;
    return src->args.size();
}

RepliStruct *RepliMacro::expandMacro(RepliStruct *oldStruct) {

    if (src == NULL) {
        error += "Macro '" + name + "' source not defined. ";
        return NULL;
    }
    if (dest == NULL) {
        error += "Macro '" + name + "' destination not defined. ";
        return NULL;
    }
    if (oldStruct == NULL) {
        error += "Macro '" + name + "' cannot expand empty structure. ";
        return NULL;
    }
    if (oldStruct->cmd.compare(this->name) != 0) {
        error += "Macro '" + name + "' cannot expand structure with different name '" + oldStruct->cmd + "'. ";
        return NULL;
    }

    if ((src->args.size() > 0) && (src->args.size() != oldStruct->args.size())) {
        error += "Macro '" + name + "' requires " + std::to_string(src->args.size()) + " arguments, cannot expand structure with " + std::to_string(oldStruct->args.size()) + " arguments. ";
        return NULL;
    }

    RepliStruct* newStruct;

// Special case of macros without args, just copy in the args from oldStruct
    if ((src->args.size() == 0) && (oldStruct->args.size() > 0)) {
        newStruct = oldStruct->clone();
        newStruct->cmd = dest->cmd;
        newStruct->label = oldStruct->label;
        return newStruct;
    } else {

        newStruct = dest->clone();
        newStruct->label = oldStruct->label;
    }

    RepliStruct *findStruct;

    std::vector<RepliStruct*>::const_iterator iOld(oldStruct->args.begin());
    for (std::vector<RepliStruct*>::const_iterator iSrc(src->args.begin()), iSrcEnd(src->args.end()); iSrc != iSrcEnd; ++iSrc, ++iOld) {
// printf("looking for '%s'\n", (*iSrc)->cmd.c_str());
// find the Atom inside newStruct with the name of iSrc->cmd
        findStruct = newStruct->findAtom((*iSrc)->cmd);
        if (findStruct != NULL) {
// overwrite data in findStruct with the matching one from old
            *findStruct = *(*iOld);
        }
    }

    return newStruct;
}

RepliStruct *RepliStruct::findAtom(const std::string &name) {

    RepliStruct* structure;
    for (std::vector<RepliStruct*>::iterator iter(args.begin()), iterEnd(args.end()); iter != iterEnd; ++iter) {
        switch ((*iter)->type) {
        case Atom:
            if ((*iter)->cmd.compare(name) == 0)
                return (*iter);
            break;
        case Structure:
        case Set:
        case Development:
            structure = (*iter)->findAtom(name);
            if (structure != NULL)
                return structure;
            break;
        default:
            break;
        }
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RepliCondition::RepliCondition(const std::string &name, bool reversed) {

    this->name = name;
    this->reversed = reversed;
}

RepliCondition::~RepliCondition() {
}

bool RepliCondition::reverse() {

    reversed = !reversed;
    return true;
}

bool RepliCondition::isActive(std::unordered_map<std::string, RepliMacro *> &RepliMacros, std::unordered_map<std::string, int64_t> &Counters) {

    bool foundIt = (RepliMacros.find(name) != RepliMacros.end());

    if (!foundIt)
        foundIt = (Counters.find(name) != Counters.end());

    if (reversed)
        return (!foundIt);
    else
        return foundIt;
}


}
