//	preprocessor.cpp
//
//	Author: Thor List, Eric Nivel
//
//	BSD license:
//	Copyright (c) 2010, Thor List, Eric Nivel
//	All rights reserved.
//	Redistribution and use in source and binary forms, with or without
//	modification, are permitted provided that the following conditions are met:
//
//   - Redistributions of source code must retain the above copyright
//     notice, this list of conditions and the following disclaimer.
//   - Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   - Neither the name of Thor List or Eric Nivel nor the
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

#include "preprocessor.h"
#include "compiler.h"
#include "CoreLibrary/utils.h"
#include <string>
#include <list>


namespace r_comp {
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Preprocessor::Preprocessor() {

    root = new RepliStruct(RepliStruct::Root);
}

Preprocessor::~Preprocessor() {

    delete root;
}

RepliStruct *Preprocessor::process(const char* file, string& error, r_comp::Metadata* metadata)
{
    RepliStruct::GlobalFilename = file;

    std::ifstream stream(file);
    if (!stream.good()) {
        error = "unable to load file ";
        error += file;
        return 0;
    }

    root->reset(); // trims root from previously preprocessed objects.

    uint64_t a = 0, b = 0;
    if (root->parse(&stream, a, b) < 0) {
        error = root->printError();
        stream.close();
        return 0;
    }
    if (!stream.eof()) {
        error = "Code structure error: Unmatched ) or ].\n";
        stream.close();
        return 0;
    }

    int64_t pass = 0, total = 0, count;
    while ((count = root->process()) > 0) {

        total += count;
        pass++;
    }
    if (count < 0) {

        error = root->printError();
        stream.close();
        return 0;
    }

    if (metadata)
        initialize(metadata);

    error = root->printError();
    stream.close();
    if (error.size() > 0) {
        return 0;
    }

    return root;
}

bool Preprocessor::isTemplateClass(RepliStruct *s) {

    for (std::vector<RepliStruct *>::iterator j(s->args.begin()); j != s->args.end(); ++j) {
        std::string name;
        std::string type;
        switch ((*j)->type) {
        case RepliStruct::Atom:
            if ((*j)->cmd == ":~")
                return true;
            break;
        case RepliStruct::Structure: // template instantiation; args are the actual parameters.
        case RepliStruct::Development: // actual template arg as a list of args.
        case RepliStruct::Set: // sets can contain tpl args.
            if (isTemplateClass(*j))
                return true;
            break;
        default:
            break;
        }
    }
    return false;
}

bool Preprocessor::isSet(std::string class_name)
{
    for (std::vector<RepliStruct *>::iterator i(root->args.begin()); i != root->args.end(); ++i) {
        if ((*i)->type != RepliStruct::Directive || (*i)->cmd != "!class")
            continue;
        RepliStruct *s = *(*i)->args.begin();
        if (s->cmd == class_name) // set classes are written class_name[].
            return false;
        if (s->cmd == class_name + "[]")
            return true;
    }
    return false;
}

void Preprocessor::instantiateClass(RepliStruct *tpl_class, std::vector<RepliStruct *> &tpl_args, std::string &instantiated_class_name) {

    static uint64_t LastClassID = 0;
// remove the trailing [].
    std::string sset = "[]";
    instantiated_class_name = tpl_class->cmd;
    instantiated_class_name = instantiated_class_name.substr(0, instantiated_class_name.length() - sset.length());
// append an ID to the tpl class name.
    char buffer[255];
    sprintf(buffer, "%lu", LastClassID++);
    instantiated_class_name += buffer;

    std::vector<StructureMember> members;
    std::list<RepliStruct *> _tpl_args;
    for (std::vector<RepliStruct *>::reverse_iterator i = tpl_args.rbegin(); i != tpl_args.rend(); ++i)
        _tpl_args.push_back(*i);
    getMembers(tpl_class, members, _tpl_args, true);

    metadata->class_names[class_opcode] = instantiated_class_name;
    metadata->classes_by_opcodes[class_opcode] = metadata->classes[instantiated_class_name] = Class(Atom::SSet(class_opcode, members.size()), instantiated_class_name, members);
    ++class_opcode;
}

void Preprocessor::getMember(std::vector<StructureMember> &members, RepliStruct *m, std::list<RepliStruct *> &tpl_args, bool instantiate) {

    size_t p;
    std::string name;
    std::string type;
    switch (m->type) {
    case RepliStruct::Set:
        name = m->label.substr(0, m->label.length() - 1);
        if (m->args.size() == 0) // anonymous set of anything.
            members.push_back(StructureMember(&Compiler::read_set, name));
        else { // structured set, arg[0].cmd is ::type.

            type = (*m->args.begin())->cmd.substr(2, m->cmd.length() - 1);
            if (isSet(type))
                members.push_back(StructureMember(&Compiler::read_set, name, type, StructureMember::I_SET));
            else if (type == Class::Type)
                members.push_back(StructureMember(&Compiler::read_set, name, type, StructureMember::I_DCLASS));
            else
                members.push_back(StructureMember(&Compiler::read_set, name, type, StructureMember::I_EXPRESSION));
        }
        break;
    case RepliStruct::Atom:
        if (m->cmd == "nil")
            break;
        p = m->cmd.find(':');
        name = m->cmd.substr(0, p);
        type = m->cmd.substr(p + 1, m->cmd.length());
        if (type == "")
            members.push_back(StructureMember(&Compiler::read_any, name));
        else if (type == "nb")
            members.push_back(StructureMember(&Compiler::read_number, name));
        else if (type == "us")
            members.push_back(StructureMember(&Compiler::read_timestamp, name));
        else if (type == "bl")
            members.push_back(StructureMember(&Compiler::read_boolean, name));
        else if (type == "st")
            members.push_back(StructureMember(&Compiler::read_string, name));
        else if (type == "did")
            members.push_back(StructureMember(&Compiler::read_device, name));
        else if (type == "fid")
            members.push_back(StructureMember(&Compiler::read_function, name));
        else if (type == "nid")
            members.push_back(StructureMember(&Compiler::read_node, name));
        else if (type == Class::Expression)
            members.push_back(StructureMember(&Compiler::read_expression, name));
        else if (type == "~") {

            RepliStruct *_m = tpl_args.back();
            tpl_args.pop_back();
            switch (_m->type) {
            case RepliStruct::Structure: { // the tpl arg is an instantiated tpl set class.
                std::string instantiated_class_name;
                instantiateClass(template_classes.find(_m->cmd)->second, _m->args, instantiated_class_name);
                members.push_back(StructureMember(&Compiler::read_set, _m->label.substr(0, _m->label.length() - 1), instantiated_class_name, StructureMember::I_CLASS));
                break;
            } default:
                getMember(members, _m, tpl_args, true);
                break;
            }
        } else if (type == Class::Type)
            members.push_back(StructureMember(&Compiler::read_class, name));
        else // type is a class name.
            members.push_back(StructureMember(&Compiler::read_expression, name, type));
        break;
    case RepliStruct::Structure: { // template instantiation; (*m)->cmd is the template class, (*m)->args are the actual parameters.
        RepliStruct *template_class = template_classes.find(m->cmd)->second;
        if (instantiate) {

            std::string instantiated_class_name;
            instantiateClass(template_class, m->args, instantiated_class_name);
            members.push_back(StructureMember(&Compiler::read_set, m->label.substr(0, m->label.length() - 1), instantiated_class_name, StructureMember::I_CLASS));
        } else {

            for (std::vector<RepliStruct *>::reverse_iterator i = m->args.rbegin(); i != m->args.rend(); ++i) // append the passed args to the ones held by m.
                tpl_args.push_back(*i);
            getMembers(template_class, members, tpl_args, false);
        }
        break;
    } case RepliStruct::Development:
        getMembers(m, members, tpl_args, instantiate);
        break;
    default:
        break;
    }
}

void Preprocessor::getMembers(RepliStruct *s, std::vector<StructureMember> &members, std::list<RepliStruct *> &tpl_args, bool instantiate) {

    for (std::vector<RepliStruct *>::iterator j(s->args.begin()); j != s->args.end(); ++j)
        getMember(members, *j, tpl_args, instantiate);
}

ReturnType Preprocessor::getReturnType(RepliStruct *s) {

    if (s->tail == ":nb")
        return NUMBER;
    else if (s->tail == ":us")
        return TIMESTAMP;
    else if (s->tail == ":bl")
        return BOOLEAN;
    else if (s->tail == ":st")
        return STRING;
    else if (s->tail == ":nid")
        return NODE_ID;
    else if (s->tail == ":did")
        return DEVICE_ID;
    else if (s->tail == ":fid")
        return FUNCTION_ID;
    else if (s->tail == ":[]")
        return SET;
    return ANY;
}

void Preprocessor::initialize(Metadata *metadata)
{
    this->metadata = metadata;

    class_opcode = 0;
    uint16_t function_opcode = 0;
    uint16_t operator_opcode = 0;

    std::vector<StructureMember> r_xpr;
    metadata->classes[std::string(Class::Expression)] = Class(Atom::Object(class_opcode, 0), Class::Expression, r_xpr); // to read unspecified expressions in classes and sets.
    ++class_opcode;
    std::vector<StructureMember> r_type;
    metadata->classes[std::string(Class::Type)] = Class(Atom::Object(class_opcode, 0), Class::Type, r_type); // to read object types in expressions and sets.
    ++class_opcode;

    for (std::vector<RepliStruct *>::iterator i(root->args.begin()); i != root->args.end(); ++i) {
        if ((*i)->type != RepliStruct::Directive)
            continue;

        RepliStruct *s = *(*i)->args.begin();
        std::vector<StructureMember> members;
        if ((*i)->cmd == "!class") {

            std::string sset = "[]";
            std::string class_name = s->cmd;
            size_t p = class_name.find(sset);
            ClassType class_type = (p == std::string::npos ? T_CLASS : T_SET);
            if (class_type == T_SET) // remove the trailing [] since the RepliStructs for instantiated classes do so.
                class_name = class_name.substr(0, class_name.length() - sset.length());

            if (isTemplateClass(s)) {

                template_classes[class_name] = s;
                continue;
            }

            std::list<RepliStruct *> tpl_args;
            getMembers(s, members, tpl_args, false);

            Atom atom;
            if (class_name == "grp")
                atom = Atom::Group(class_opcode, members.size());
            else if (class_name == "ipgm")
                atom = Atom::InstantiatedProgram(class_opcode, members.size());
            else if (class_name == "icpp_pgm")
                atom = Atom::InstantiatedCPPProgram(class_opcode, members.size());
            else if (class_name == "cst")
                atom = Atom::CompositeState(class_opcode, members.size());
            else if (class_name == "mdl")
                atom = Atom::Model(class_opcode, members.size());
            else if (class_name.find("mk.") != string::npos)
                atom = Atom::Marker(class_opcode, members.size());
            else
                atom = Atom::Object(class_opcode, members.size());

            if (class_type == T_CLASS) { // find out if the class is a sys class, i.e. is an instantiation of _obj, _grp or _fact.

                std::string base_class = s->args.front()->cmd;
                if (base_class == "_obj" || base_class == "_grp" || base_class == "_fact")
                    class_type = T_SYS_CLASS;
            }

            metadata->class_names[class_opcode] = class_name;
            switch (class_type) {
            case T_SYS_CLASS:
                metadata->classes_by_opcodes[class_opcode] = metadata->classes[class_name] = metadata->sys_classes[class_name] = Class(atom, class_name, members);
                break;
            case T_CLASS:
                metadata->classes_by_opcodes[class_opcode] = metadata->classes[class_name] = Class(atom, class_name, members);
                break;
            case T_SET:
                metadata->classes_by_opcodes[class_opcode] = metadata->classes[class_name] = Class(Atom::SSet(class_opcode, members.size()), class_name, members);
                break;
            default:
                break;
            }
            ++class_opcode;
        } else if ((*i)->cmd == "!op") {

            std::list<RepliStruct *> tpl_args;
            getMembers(s, members, tpl_args, false);
            ReturnType return_type = getReturnType(s);

            std::string operator_name = s->cmd;
            metadata->operator_names.push_back(operator_name);
            metadata->classes[operator_name] = Class(Atom::Operator(operator_opcode, s->args.size()), operator_name, members, return_type);
            ++operator_opcode;
        } else if ((*i)->cmd == "!dfn") { // don't bother to read the members, it's always a set.
            std::vector<StructureMember> r_set;
            r_set.push_back(StructureMember(&Compiler::read_set, ""));

            std::string function_name = s->cmd;
            metadata->function_names.push_back(function_name);
            metadata->classes[function_name] = Class(Atom::DeviceFunction(function_opcode), function_name, r_set);
            ++function_opcode;
        }
    }
}
}
