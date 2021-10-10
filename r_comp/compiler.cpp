//	compiler.cpp
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

#include "compiler.h"

#include <r_code/atom.h>         // for Atom, Atom::::DEVICE, Atom::::NODE, etc
#include <r_code/object.h>       // for ImageObject, SysObject, SysView
#include <r_code/vector.h>       // for vector
#include <r_comp/compiler.h>     // for Compiler, Compiler::State
#include <r_comp/replistruct.h>  // for RepliStruct, RepliStruct::Type::Set, etc
#include <r_comp/segments.h>     // for Metadata, Reference, Image, etc
#include <stddef.h>              // for size_t
#include <stdlib.h>              // for strtoll, strtod, strtoull
#include <cstdint>               // for uint16_t, int16_t, uint32_t, etc
#include <fstream>               // for char_traits
#include <stdexcept>             // for invalid_argument
#include <utility>               // for pair
#include <sstream>               // for istringstream


namespace r_comp
{

static bool Output = true;

Compiler::Compiler(Image *_image, Metadata *_metadata)
{
    this->_image = _image;
    this->_metadata = _metadata;
}


Compiler::State Compiler::save_state()
{
    State s(this);
    return s;
}

void Compiler::restore_state(State s)
{
    state = s;
}

std::string Compiler::getError()
{
    return m_errorFile + ":" + std::to_string(m_errorLine) + ": " + error;
}

void Compiler::set_error(const std::string &s, RepliStruct::Ptr node)
{
    if (!err && Output) {
        m_errorFile = node->fileName;
        m_errorLine = node->line;
        err = true;
        error = s;
    }
}

void Compiler::set_arity_error(RepliStruct::Ptr node, uint16_t expected, uint16_t got)
{
    set_error("got " + std::to_string(got) +
              " elements, expected " + std::to_string(expected) +
              " for " + node->cmd, node);
}

////////////////////////////////////////////////////////////////////////////////////////////////

/*
  root
  |-directive1 (ignore)
  |-directive2 (ignore)
  ....
  |-node1
  |-view for node1
  |-node2
  |-view for node2
  ...
  */

bool Compiler::compile(RepliStruct::Ptr root, bool trace)
{
    this->err = false;
    this->trace = trace;
    current_object_index = _image->object_map.objects.size();

    for (std::vector<RepliStruct::Ptr >::iterator iter = root->args.begin(); iter != root->args.end(); iter++) {
        RepliStruct::Ptr node = *iter;

        if (node->type == RepliStruct::Directive) {
            continue;
        }

        iter++;

        if (iter == root->args.end()) {
            set_error("missing view", node);
            return false;
        }

        RepliStruct::Ptr view = *iter;

        if (!read_sys_object(node, view)) {
            return false;
        }

        current_object_index++;
    }

    return !err;
}

bool Compiler::read_sys_object(RepliStruct::Ptr node, RepliStruct::Ptr view)
{
    if (node->type != RepliStruct::Structure) {
        set_error("expected expression", node);
        return false;
    }

    local_references.clear();
    hlp_references.clear();
    bool hasLabel = node->label != "";
    in_hlp = false;

    if (node->args.size() == 0) {
        if (hasLabel) {
            set_error("error: label not followed by expression", node);
        } else {
            set_error("error: missing expression opening", node);
        }

        return false;
    }

    if (_metadata->classes.count(node->cmd) == 0) {
        set_error("error: unknown class '" + node->cmd + "'", node);
        return false;
    }

    current_class = _metadata->classes.find(node->cmd)->second;

    if (current_class.str_opcode == "mdl" || current_class.str_opcode == "cst") {
        in_hlp = true;
    }

    current_object = new SysObject();

    if (hasLabel) {
        std::string label = node->label.substr(0, node->label.size() - 1);
        _metadata->global_references[label] = Reference(_image->code_segment.objects.size(), current_class, Class());
    }

    current_object->code[0] = current_class.atom;

    if (current_class.atom.getAtomCount()) {
        uint16_t extent_index = current_class.atom.getAtomCount() + 1;

        if (!expression_tail(node, current_class, 1, extent_index, true)) {
            return false;
        }
    }

    SysObject *sys_object = (SysObject *)current_object; // current_object will point to views, if any.

    // compile view set
    // input format:
    // []
    // [view-data]
    // ...
    // [view-data]
    // or:
    // |[]

    if (view->args.size() > 0) {
        if (view->type != RepliStruct::Set) {
            set_error("expected a view set, got " + view->cmd, view);
            return false;
        }

        if (current_class.str_opcode == "grp") {
            current_class = _metadata->classes.find("grp_view")->second;
        } else if (current_class.str_opcode == "ipgm" ||
                   current_class.str_opcode == "icpp_pgm" ||
                   current_class.str_opcode == "mdl" ||
                   current_class.str_opcode == "cst") {
            current_class = _metadata->classes.find("pgm_view")->second;
        } else {
            current_class = _metadata->classes.find("view")->second;
        }

        current_class.use_as = StructureMember::I_CLASS;
        uint16_t count = 0;

        for (RepliStruct::Ptr argNode : view->args) {
            current_object = new SysView();
            uint16_t extent_index = 0;

            if (!read_set(argNode, true, &current_class, 0, extent_index, true)) {
                set_error(" error: illegal element in set", argNode);
                delete current_object;
                return false;
            }

            count++;
            sys_object->views.push_back((SysView *)current_object);
        }
    } else if (view->type != RepliStruct::Atom && view->cmd != "|[]") {
        set_error("expected a view set", view);
        return false;
    }

    if (trace) {
        sys_object->trace();
    }

    // TODO: what happens if hasLabel == false
    std::string label = node->label.substr(0, node->label.size() - 1);
    _image->add_sys_object(sys_object, label);
    return true;
}

bool Compiler::read(RepliStruct::Ptr node, const StructureMember &m, bool enforce, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (Class *p = m.get_class(_metadata)) {
        p->use_as = m.getIteration();
        return (this->*m.read())(node, enforce, p, write_index, extent_index, write);
    }

    return (this->*m.read())(node, enforce, nullptr, write_index, extent_index, write);
}

bool Compiler::getGlobalReferenceIndex(const std::string reference_name, const ReturnType t, ImageObject *object, uint16_t &index, Class *&_class)
{
    std::unordered_map<std::string, Reference>::iterator it = _metadata->global_references.find(reference_name);

    if (it != _metadata->global_references.end() && (t == ANY || (t != ANY && it->second._class.type == t))) {
        _class = &it->second._class;

        for (uint16_t j = 0; j < object->references.size(); ++j)
            if (object->references[j] == it->second.index) { // the object has already been referenced.
                index = j; // rptr points to object->reference_set[j], which in turn points to it->second.index.
                return true;
            }

        object->references.push_back(it->second.index); // add new reference to the object.
        index = object->references.size() - 1; // rptr points to the last element of object->reference_set, which in turn points to it->second.index.
        return true;
    }

    return false;
}

bool Compiler::addLocalReference(const std::string reference_name, const uint16_t index, const Class &p)
{
    // cast detection.
    size_t pos = reference_name.find('#');

    if (pos != std::string::npos) {
        std::string class_name = reference_name.substr(pos + 1);
        std::string ref_name = reference_name.substr(0, pos);
        std::unordered_map<std::string, Class>::iterator it = _metadata->classes.find(class_name);

        if (it != _metadata->classes.end()) {
            local_references[ref_name] = Reference(index, p, it->second);
        } else {
            return false;
        }
    } else {
        local_references[reference_name] = Reference(index, p, Class());
    }

    return true;
}

uint32_t Compiler::add_hlp_reference(std::string reference_name)
{
    for (uint32_t i = 0; i < hlp_references.size(); ++i)
        if (reference_name == hlp_references[i]) {
            return i;
        }

    hlp_references.push_back(reference_name);
    return hlp_references.size() - 1;
}

uint8_t Compiler::get_hlp_reference(std::string reference_name)
{
    for (uint32_t i = 0; i < hlp_references.size(); ++i) {
        if (reference_name == hlp_references[i]) {
            return i;
        }
    }

    return 0xFF;
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool Compiler::local_reference(RepliStruct::Ptr node, uint16_t &index, const ReturnType t)
{
    std::unordered_map<std::string, Reference>::iterator it = local_references.find(node->cmd);

    if (it != local_references.end() && (t == ANY || (t != ANY && it->second._class.type == t))) {
        index = it->second.index;
        return true;
    }

    return false;
}

bool Compiler::global_reference(RepliStruct::Ptr node, uint16_t &index, const ReturnType t)
{
    if (node->cmd == "") {
        return false;
    }

    Class *unused;
    return getGlobalReferenceIndex(node->cmd, t, current_object, index, unused);
}

bool Compiler::hlp_reference(RepliStruct::Ptr node, uint16_t &index)
{
    if (node->label != "") {
        return false;
    }

    for (uint8_t i = 0; i < hlp_references.size(); ++i) {
        if (node->cmd == hlp_references[i]) {
            index = i;
            return true;
        }
    }

    return false;
}

bool Compiler::this_indirection(RepliStruct::Ptr node, std::vector<int16_t> &indices, const ReturnType returnType)
{
    if (!Utils::StartsWith(node->cmd, "this.")) {
        return false;
    }

    Class *reference_class; // in general, p starts as the current_class; exception: in pgm, this refers to the instantiated program.

    if (current_class.str_opcode == "pgm") {
        reference_class = &_metadata->sys_classes["ipgm"];
    } else {
        reference_class = &current_class;
    }

    std::string path = node->cmd;
    path.erase(0, path.find(".") + 1); // remove "this."
    return indirection(node, reference_class, path, &indices, returnType);
}

bool Compiler::local_indirection(RepliStruct::Ptr node, std::vector<int16_t> &indices, const ReturnType expected_type, uint32_t &cast_opcode)
{
    std::string path = node->cmd;
    size_t pos = path.find(".");

    if (pos == std::string::npos) {
        return false;
    }

    std::string name = path.substr(0, pos); // first name is a reference to a label or a variable

    if (local_references.count(name) == 0) {
        return false;
    }

    Reference &reference = local_references[name];
    indices.push_back(reference.index);
    Class *reference_class;

    if (reference.cast_class.str_opcode == "undefined") { // find out if there was a cast for this reference.
        reference_class = &reference._class;
        cast_opcode = 0x0FFFFFFF;
    } else {
        reference_class = &reference.cast_class;
        cast_opcode = reference_class->atom.asOpcode();
    }

    path.erase(0, pos + 1); // remove the first member
    return indirection(node, reference_class, path, &indices, expected_type);
}

bool Compiler::indirection(RepliStruct::Ptr node, Class *reference_class, std::string path, std::vector<int16_t> *indices, const ReturnType expected_type)
{
    uint16_t index;
    Class *fetched_class;
    ReturnType type;

    while (path.length() > 0) {
        size_t pos = path.find(".");
        std::string name = path.substr(0, pos);

        if (name == "vw") {
            fetched_class = &_metadata->classes.find("pgm_view")->second;
            type = ANY;
            indices->push_back(-1);
        } else if (name == "mks") {
            fetched_class = nullptr;
            type = SET;
            indices->push_back(-2);
        } else if (name == "vws") {
            fetched_class = nullptr;
            type = SET;
            indices->push_back(-3);
        } else if (!reference_class->get_member_index(_metadata, name, index, fetched_class)) {
            set_error(" error: " + name + " is not a member of " + reference_class->str_opcode, node);
            return false;
        } else {
            type = reference_class->get_member_type(index);
            indices->push_back(index);
        }

        // No more dots
        if (pos == std::string::npos) {
            break;
        }

        if (!fetched_class) {
            set_error(" error: " + name + " is not an addressable structure", node);
            return false;
        }

        reference_class = fetched_class;
        path.erase(0, pos + 1); // Remove fetched name + .
    }

    if (type == expected_type || expected_type == ANY) {
        return true;
    } else {
        return false;
    }
}

bool Compiler::global_indirection(RepliStruct::Ptr node, std::vector<int16_t> &indices, const ReturnType expected_type)
{
    std::string path = node->cmd;
    size_t pos = path.find(".");

    if (pos == std::string::npos) { // first m is a reference
        return false;
    }

    std::string name = path.substr(0, pos);
    Class *reference_class;
    uint16_t index;

    if (!getGlobalReferenceIndex(name, ANY, current_object, index, reference_class)) {
        return false;
    }

    indices.push_back(index);
    // TODO: Merge with the other indirections
    ReturnType type;
    Class *fetched_class;
    bool first_member = true;

    while (path.length() > 0) {
        pos = path.find(".");
        name = path.substr(0, pos);

        if (name == "vw") {
            set_error(" error: vw is not accessible on global references", node);
            return false;
        } else if (name == "mks") {
            fetched_class = nullptr;
            type = SET;
            indices.push_back(-2);
        } else if (name == "vws") {
            fetched_class = nullptr;
            type = SET;
            indices.push_back(-3);
        } else if (!reference_class->get_member_index(_metadata, name, index, fetched_class)) {
            set_error(" error: " + name + " is not a member of " + reference_class->str_opcode, node);
            return false;
        } else {
            type = reference_class->get_member_type(index);

            if (first_member && index == 0) { // indicates the first member; store in the RObject, after the leading atom, hence index=1.
                index = 1;
            }

            indices.push_back(index);
        }

        // No more dots
        if (pos == std::string::npos) {
            break;
        }

        if (!fetched_class) {
            set_error(" error: " + name + " is not a structure", node);
            break;
        }

        first_member = false;
        path.erase(0, pos + 1); // remove fetched name + .
        reference_class = fetched_class;
    }

    if (type == expected_type || expected_type == ANY) {
        return true;
    } else {
        return false;
    }
}

bool Compiler::object(RepliStruct::Ptr node, Class &p)
{
    if (sys_object(node, p)) {
        return true;
    }

    std::unordered_map<std::string, Class>::const_iterator it = _metadata->classes.find(node->cmd);

    if (it == _metadata->classes.end()) {
        return false;
    }

    p = it->second;
    return true;
}

bool Compiler::is_object(RepliStruct::Ptr node, const Class &p)
{
    return (node->cmd == p.str_opcode);
}

bool Compiler::sys_object(RepliStruct::Ptr node, Class &p)
{
    if (node->type != RepliStruct::Set) {
        return false;
    }

    std::unordered_map<std::string, Class>::const_iterator it = _metadata->sys_classes.find(node->cmd);

    if (it == _metadata->sys_classes.end()) {
        return false;
    }

    p = it->second;
    return true;
}

bool Compiler::marker(RepliStruct::Ptr node, Class &p)
{
    if (!Utils::StartsWith(node->cmd, "mk.")) {
        return false;
    }

    std::unordered_map<std::string, Class>::const_iterator it = _metadata->sys_classes.find(node->cmd);

    if (it == _metadata->sys_classes.end()) {
        return false;
    }

    p = it->second;
    return true;
}

bool Compiler::op(RepliStruct::Ptr node, Class &p, const ReturnType t)
{
    std::unordered_map<std::string, Class>::const_iterator it = _metadata->classes.find(node->cmd);

    if (it == _metadata->classes.end() || (t != ANY && it->second.type != ANY && it->second.type != t)) {
        return false;
    }

    p = it->second;
    return true;
}

bool Compiler::is_op(RepliStruct::Ptr node, const Class &p)
{
    return (node->cmd == p.str_opcode);
}

bool Compiler::function(RepliStruct::Ptr node, Class &p)
{
    std::unordered_map<std::string, Class>::const_iterator it = _metadata->classes.find(node->cmd);

    if (it == _metadata->classes.end()) {
        return false;
    }

    p = it->second;
    return true;
}

bool Compiler::expression_head(RepliStruct::Ptr node, Class &p, const ReturnType t)
{
    if (t == ANY) {
        if (!object(node, p)) {
            if (!marker(node, p)) {
                if (!op(node, p, ANY)) {
                    return false;
                }
            }
        }
    } else if (!op(node, p, t)) {
        return false;
    }

    return true;
}

bool Compiler::is_expression_head(RepliStruct::Ptr node, const Class &p)
{
    if (is_object(node, p) || is_op(node, p)) {
        return true;
    } else {
        return false;
    }
}

bool Compiler::expression_tail(RepliStruct::Ptr node, const Class &p, uint16_t write_index, uint16_t &extent_index, bool write)  // arity>0.
{
    bool entered_pattern;
    uint16_t pattern_end_index;

    if (in_hlp) {
        entered_pattern = true; //p.is_fact(_metadata);
        pattern_end_index = p.atom.getAtomCount() - 1;
    } else {
        entered_pattern = p.is_pattern(_metadata);
        pattern_end_index = 0;
    }

    if (write && state.pattern_lvl) { // fill up with wildcards that will be overwritten up to ::.
        for (uint16_t j = write_index; j < write_index + p.atom.getAtomCount(); ++j) {
            current_object->code[j] = Atom::Wildcard();
        }
    }

    if (node->args.size() != p.atom.getAtomCount() &&
        node->args[node->args.size() - 1]->cmd != "::") {
        set_arity_error(node, p.atom.getAtomCount(), node->args.size());
        return false;
    }

    for (size_t i = 0; i < node->args.size(); i++) {
        if (entered_pattern && i == 0) { // pattern begin.
            ++state.pattern_lvl;
        }

        if (!read(node->args[i], p.things_to_read.at(i), true, write_index + i, extent_index, write)) {
            set_error(" error: parsing element in expression", node);
            return false;
        }

        if (entered_pattern && i == pattern_end_index) { // pattern end.
            --state.pattern_lvl;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool Compiler::expression(RepliStruct::Ptr node, const ReturnType t, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->type != RepliStruct::Structure) {
        return false;
    }

    bool lbl = false;
    std::string label;

    if (node->label != "") {
        label = node->label.substr(0, node->label.size() - 1);
        lbl = true;
    }

    Class p;

    if (!expression_head(node, p, t)) {
        return false;
    }

    if (lbl && !in_hlp) {
        if (!addLocalReference(label, write_index, p)) {
            set_error("cast to " + label.substr(label.find("#") + 1) + ": unknown class", node);
            return false;
        }
    }

    uint8_t tail_write_index = 0;

    if (write) {
        if (lbl && in_hlp) {
            uint8_t variable_index = get_hlp_reference(label);

            if (variable_index == 0xFF) {
                set_error(" error: undeclared variable", node);
                return false;
            }

            current_object->code[write_index] = Atom::AssignmentPointer(variable_index, extent_index);
        } else {
            current_object->code[write_index] = Atom::IPointer(extent_index);
        }

        current_object->code[extent_index++] = p.atom;
        tail_write_index = extent_index;
        extent_index += p.atom.getAtomCount();
    }

    if (!expression_tail(node, p, tail_write_index, extent_index, write)) {
        return false;
    }

    return true;
}

bool Compiler::expression(RepliStruct::Ptr node, const Class &p, uint16_t write_index, uint16_t &extent_index, bool write)
{
    bool lbl = false;
    std::string label;

    if (node->label != "") {
        label = node->label.substr(0, node->label.size() - 1);
        lbl = true;
    }

    if (!is_expression_head(node, p)) {
        return false;
    }

    if (lbl) {
        if (!addLocalReference(label, write_index, p)) {
            set_error("cast to " + label.substr(label.find("#") + 1) + ": unknown class", node);
            return false;
        }
    }

    uint16_t tail_write_index = 0;

    if (write) {
        if (lbl && in_hlp) {
            uint32_t variable_index = get_hlp_reference(label);

            if (variable_index == 0xFF) {
                set_error(" error: undeclared variable", node);
                return false;
            }

            current_object->code[write_index] = Atom::AssignmentPointer(variable_index, extent_index);
        } else {
            current_object->code[write_index] = Atom::IPointer(extent_index);
        }

        current_object->code[extent_index++] = p.atom;
        tail_write_index = extent_index;
        extent_index += p.atom.getAtomCount();
    }

    if (!expression_tail(node, p, tail_write_index, extent_index, write)) {
        return false;
    }

    return true;
}

bool Compiler::set(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->type != RepliStruct::Set) {
        return false;
    }

    bool lbl = false;
    std::string label;

    if (node->label != "") {
        label = node->label.substr(0, node->label.size() - 1);
        lbl = true;
    }

    if (lbl) {
        if (!addLocalReference(label, write_index, Class(SET))) {
            set_error("cast to " + label.substr(label.find("#") + 1) + ": unknown class", node);
            return false;
        }
    }

    uint16_t content_write_index = 0;

    if (write) {
        current_object->code[write_index] = Atom::IPointer(extent_index);
        uint8_t element_count = node->args.size();//set_element_count(indented);
        current_object->code[extent_index++] = Atom::Set(element_count);
        content_write_index = extent_index;
        extent_index += element_count;
    }

    for (size_t i = 0; i < node->args.size(); i++) {
        if (!read_any(node->args[i], false, nullptr, content_write_index + i, extent_index, write)) {
            set_error(" error: illegal element in set", node->args[i]);
            return false;
        }
    }

    return true;
}

bool Compiler::set(RepliStruct::Ptr node, const Class &p, uint16_t write_index, uint16_t &extent_index, bool write)
{
    bool lbl = false;
    std::string label;

    if (node->label != "") {
        label = node->label.substr(0, node->label.size() - 1);
        lbl = true;
    }

    if (lbl) {
        if (!addLocalReference(label, write_index, p)) {
            set_error("cast to " + label.substr(label.find("#") + 1) + ": unknown class", node);
            return false;
        }
    }

    uint16_t content_write_index = 0;

    if (write) {
        current_object->code[write_index] = Atom::IPointer(extent_index);
        uint8_t element_count;

        if (p.atom.getDescriptor() == Atom::S_SET && p.use_as != StructureMember::I_SET) {
            element_count = p.atom.getAtomCount();
            current_object->code[extent_index++] = p.atom;
        } else {
            element_count = node->args.size();//set_element_count(node);
            current_object->code[extent_index++] = Atom::Set(element_count);
        }

        content_write_index = extent_index;
        extent_index += element_count;
    }

    uint32_t arity = UINT32_MAX;

    if (p.use_as == StructureMember::I_CLASS) { // undefined arity for unstructured sets.
        arity = p.atom.getAtomCount();

        if (write) // fill up with wildcards that will be overwritten up to ::.
            for (uint16_t j = content_write_index; j < content_write_index + arity; ++j) {
                current_object->code[j] = Atom::Wildcard();
            }
    }

    if (node->args.size() != arity && arity != UINT32_MAX) {
        if (state.no_arity_check) {
            state.no_arity_check = false;
        } else {
            set_arity_error(node, arity, node->args.size());
            return false;
        }
    }

    for (size_t i = 0; i < node->args.size(); i++) {
        bool r;

        switch (p.use_as) {
        case StructureMember::I_EXPRESSION:
            r = read_expression(node->args[i], true, &p, content_write_index + i, extent_index, write);
            break;

        case StructureMember::I_SET: {
            Class _p = p;
            _p.use_as = StructureMember::I_CLASS;
            r = read_set(node->args[i], true, &_p, content_write_index + i, extent_index, write);
            break;
        }

        case StructureMember::I_CLASS:
            r = read(node->args[i], p.things_to_read[i], true, content_write_index + i, extent_index, write);
            break;

        case StructureMember::I_DCLASS:
            r = read_class(node->args[i], true, nullptr, content_write_index + i, extent_index, write);
            break;
        }

        if (!r) {
            set_error(" error: illegal element in set", node);
            return false;
        }
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool Compiler::read_any(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write)   // enforce always false, p always NULL.
{
    if (read_number(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (err) {
        return false;
    }

    if (read_timestamp(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (err) {
        return false;
    }

    if (read_string(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (err) {
        return false;
    }

    if (read_boolean(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (err) {
        return false;
    }

    if (read_function(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (err) {
        return false;
    }

    if (read_node(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (err) {
        return false;
    }

    if (read_device(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (err) {
        return false;
    }

    if (read_class(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (err) {
        return false;
    }

    if (read_expression(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (err) {
        return false;
    }

    if (read_set(node, false, nullptr, write_index, extent_index, write)) {
        return true;
    }

    if (write) {
        set_error("error: expecting more elements in " + node->cmd + ", has " + std::to_string(node->args.size()) + " arguments", node);
    }

    return false;
}

bool Compiler::read_number(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write)   // p always NULL
{
    if (read_nil_nb(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_forever_nb(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_variable(node, write_index, extent_index, write, NUMBER)) {
        return true;
    }

    if (read_reference(node, write_index, extent_index, write, NUMBER)) {
        return true;
    }

    if (read_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_tail_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (node->cmd != "" && !Utils::StartsWith(node->cmd, "0x") && !Utils::EndsWith(node->cmd, "us")) { // Make sure it isn't a hex num or a timestamp
        std::istringstream stream(node->cmd);
        stream.imbue(std::locale("C"));
        double num;
        stream >> num;
        if (!stream.fail()) {
            if (write) {
                current_object->code[write_index] = Atom::Float(num);
            }
            return true;
        }
    }

    State s = save_state();

    if (expression(node, NUMBER, write_index, extent_index, write)) {
        return true;
    }

    restore_state(s);

    if (enforce) {
        set_error(" error: expected a number or an expr evaluating to a number, got: " + node->cmd, node);
        return false;
    }

    return false;
}

bool Compiler::read_boolean(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write)   // p always NULL
{
    if (read_nil_bl(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_variable(node, write_index, extent_index, write, BOOLEAN)) {
        return true;
    }

    if (read_reference(node, write_index, extent_index, write, BOOLEAN)) {
        return true;
    }

    if (read_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_tail_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (node->cmd == "true" || node->cmd == "false") {
        if (write) {
            current_object->code[write_index] = Atom::Boolean((node->cmd == "true"));
        }

        return true;
    }

    State s = save_state();

    if (expression(node, BOOLEAN, write_index, extent_index, write)) {
        return true;
    }

    restore_state(s);

    if (enforce) {
        set_error(" error: expected a Boolean or an expr evaluating to a Boolean, got: " + node->cmd, node);
        return false;
    }

    return false;
}

bool Compiler::read_timestamp(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (read_nil_us(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_variable(node, write_index, extent_index, write, TIMESTAMP)) {
        return true;
    }

    if (read_reference(node, write_index, extent_index, write, TIMESTAMP)) {
        return true;
    }

    if (read_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_tail_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (Utils::EndsWith(node->cmd, "us")) {
        std::string number = node->cmd.substr(0, node->cmd.find("us"));

        try {
            char *p;
            uint64_t ts = std::strtoull(number.c_str(), &p, 10);

            if (*p == 0) {
                if (write) {
                    current_object->code[write_index] = Atom::IPointer(extent_index);
                    current_object->code[extent_index++] = Atom::Timestamp();
                    current_object->code[extent_index++] = ts >> 32;
                    current_object->code[extent_index++] = (ts & UINT32_MAX);
                }

                return true;
            }
        } catch (const std::invalid_argument &ia) { }
    } else {
    }

    State s = save_state();

    if (expression(node, TIMESTAMP, write_index, extent_index, write)) {
        return true;
    }

    restore_state(s);

    if (enforce) {
        set_error(" error: expected a timestamp or an expr evaluating to a timestamp", node);
        return false;
    }

    return false;
}

bool Compiler::read_string(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write) // p always NULL
{
    if (read_nil_st(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_variable(node, write_index, extent_index, write, STRING)) {
        return true;
    }

    if (read_reference(node, write_index, extent_index, write, STRING)) {
        return true;
    }

    if (read_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_tail_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (node->cmd.size() > 2 && node->cmd[0] == '"' && node->cmd[node->cmd.length() - 1] == '"') {
        if (write) {
            std::string str = node->cmd.substr(1, node->cmd.size() - 2);
            uint16_t l = (uint16_t)str.length(); // TODO: check string length
            current_object->code[write_index] = Atom::IPointer(extent_index);
            current_object->code[extent_index++] = Atom::String(l);
            uint64_t _st = 0;
            int8_t shift = 0;

            for (uint16_t i = 0; i < l; ++i) {
                _st |= str[i] << shift;
                shift += 8;

                if (shift == 32) {
                    current_object->code[extent_index++] = _st;
                    _st = 0;
                    shift = 0;
                }
            }

            if (l % 4) {
                current_object->code[extent_index++] = _st;
            }
        }

        return true;
    }

    State s = save_state();

    if (expression(node, STRING, write_index, extent_index, write)) {
        return true;
    }

    restore_state(s);

    if (enforce) {
        set_error(" error: expected a string", node);
        return false;
    }

    return false;
}

bool Compiler::read_node(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write) // p always NULL
{
    if (read_nil_nid(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_variable(node, write_index, extent_index, write, NODE_ID)) {
        return true;
    }

    if (read_reference(node, write_index, extent_index, write, NODE_ID)) {
        return true;
    }

    if (read_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_tail_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (Utils::StartsWith(node->cmd, "0x")) {
        try {
            char *p;
            uint64_t h = std::strtoll(node->cmd.c_str(), &p, 16);

            if (*p == 0) {
                if (Atom(h).getDescriptor() == Atom::NODE) {
                    if (write) {
                        current_object->code[write_index] = Atom(h);
                    }

                    return true;
                }
            }
        } catch (const std::invalid_argument &ia) { }
    }

    State s = save_state();

    if (expression(node, NODE_ID, write_index, extent_index, write)) {
        return true;
    }

    restore_state(s);

    if (enforce) {
        set_error(" error: expected a node id", node);
        return false;
    }

    return false;
}

bool Compiler::read_device(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write) // p always NULL.
{
    if (read_nil_did(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_variable(node, write_index, extent_index, write, DEVICE_ID)) {
        return true;
    }

    if (read_reference(node, write_index, extent_index, write, DEVICE_ID)) {
        return true;
    }

    if (read_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_tail_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (Utils::StartsWith(node->cmd, "0x")) {
        try {
            char *p;
            uintptr_t h = std::strtoll(node->cmd.c_str(), &p, 16);

            if (*p == 0) {
                if (Atom(h).getDescriptor() == Atom::DEVICE) {
                    if (write) {
                        current_object->code[write_index] = Atom(h);
                    }

                    return true;
                }
            }
        } catch (const std::invalid_argument &ia) { }
    }

    State s = save_state();

    if (expression(node, DEVICE_ID, write_index, extent_index, write)) {
        return true;
    }

    restore_state(s);

    if (enforce) {
        set_error(" error: expected a device id", node);
        return false;
    }

    return false;
}

bool Compiler::read_function(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write) // p always NULL
{
    if (read_nil_fid(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_variable(node, write_index, extent_index, write, FUNCTION_ID)) {
        return true;
    }

    if (read_reference(node, write_index, extent_index, write, FUNCTION_ID)) {
        return true;
    }

    if (read_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_tail_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    Class _p;

    if (function(node, _p)) { // TODO: _p shall be used to parse the args in the embedding expression
        if (write) {
            current_object->code[write_index] = _p.atom;
        }

        return true;
    }

    State s = save_state();

    if (expression(node, FUNCTION_ID, write_index, extent_index, write)) {
        return true;
    }

    restore_state(s);

    if (enforce) {
        set_error(" error: expected a device function", node);
        return false;
    }

    return false;
}

bool Compiler::read_expression(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (read_nil(node, write_index, extent_index, write)) {
        return true;
    }

    if (p && p->str_opcode != Class::Expression) {
        if (read_variable(node, write_index, extent_index, write, *p)) {
            return true;
        }

        if (read_reference(node, write_index, extent_index, write, p->type)) {
            return true;
        }
    } else {
        if (read_variable(node, write_index, extent_index, write, Class())) {
            return true;
        }

        if (read_reference(node, write_index, extent_index, write, ANY)) {
            return true;
        }
    }

    if (read_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (read_tail_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (p && p->str_opcode != Class::Expression) {
        if (expression(node, *p, write_index, extent_index, write)) {
            return true;
        }
    } else if (expression(node, ANY, write_index, extent_index, write)) {
        return true;
    }

    if (enforce) {
        std::string s = " error: expected an expression";

        if (p) {
            s += " of type: ";
            s += p->str_opcode;
            s += " got: " + node->cmd;
        }

        set_error(s, node);
        return false;
    }

    return false;
}

bool Compiler::read_set(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (read_nil_set(node, write_index, extent_index, write)) {
        return true;
    } else if (read_variable(node, write_index, extent_index, write, Class(SET))) {
        return true;
    } else if (read_reference(node, write_index, extent_index, write, SET)) {
        return true;
    } else if (read_wildcard(node, write_index, extent_index, write)) {
        return true;
    } else if (read_tail_wildcard(node, write_index, extent_index, write)) {
        return true;
    }

    if (p) {
        if (set(node, *p, write_index, extent_index, write)) {
            return true;
        }
    } else if (set(node, write_index, extent_index, write)) {
        return true;
    }

    if (enforce) {
        set_error(" error: expected a set", node);
        return false;
    }

    return false;
}

bool Compiler::read_class(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write)   // p always NULL.
{
    if (node->label != "") {
        std::string label = node->label.substr(0, node->label.size() - 1);
        Class _p;

        if (!object(node, _p)) {
            if (!marker(node, _p)) {
                return false;
            }
        }

        local_references[label] = Reference(write_index, _p, Class());

        if (write) {
            current_object->code[write_index] = _p.atom;
        }

        return true;
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool Compiler::read_nil(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "nil") {
        if (write) {
            current_object->code[write_index] = Atom::Nil();
        }

        return true;
    }

    return false;
}

bool Compiler::read_nil_set(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "|[]") {
        if (write) {
            current_object->code[write_index] = Atom::IPointer(extent_index);
            current_object->code[extent_index++] = Atom::Set(0);
        }

        return true;
    }

    return false;
}

bool Compiler::read_nil_nb(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "|nb") {
        if (write) {
            current_object->code[write_index] = Atom::UndefinedFloat();
        }

        return true;
    }

    return false;
}

bool Compiler::read_nil_us(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "|ms") {
        if (write) {
            current_object->code[write_index] = Atom::IPointer(extent_index);
            current_object->code[extent_index++] = Atom::UndefinedTimestamp();
            current_object->code[extent_index++] = UINT32_MAX;
            current_object->code[extent_index++] = UINT32_MAX;
        }

        return true;
    }

    return false;
}

bool Compiler::read_forever_nb(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "forever") {
        if (write) {
            current_object->code[write_index] = Atom::PlusInfinity();
        }

        return true;
    }

    return false;
}

bool Compiler::read_nil_nid(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "|nid") {
        if (write) {
            current_object->code[write_index] = Atom::UndefinedNode();
        }

        return true;
    }

    return false;
}

bool Compiler::read_nil_did(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "|did") {
        if (write) {
            current_object->code[write_index] = Atom::UndefinedDevice();
        }

        return true;
    }

    return false;
}

bool Compiler::read_nil_fid(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "|fid") {
        if (write) {
            current_object->code[write_index] = Atom::UndefinedDeviceFunction();
        }

        return true;
    }

    return false;
}

bool Compiler::read_nil_bl(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "|bl") {
        if (write) {
            current_object->code[write_index] = Atom::UndefinedBoolean();
        }

        return true;
    }

    return false;
}

bool Compiler::read_nil_st(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "|st") {
        if (write) {
            current_object->code[write_index] = Atom::UndefinedString();
        }

        return true;
    }

    return false;
}

bool Compiler::read_variable(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write, const Class p)
{
    if (!Utils::EndsWith(node->cmd, ":") || Utils::StartsWith(node->cmd, ":") || node->type != RepliStruct::Atom) {
        return false;
    }

    if (!state.pattern_lvl) {
        set_error(" error: no variables allowed outside a pattern skeleton", node);
        return false;
    }

    std::string v = node->cmd.substr(0, node->cmd.size() - 1); // strip the :

    if (in_hlp) {
        uint32_t variable_index = add_hlp_reference(v);

        if (write) {
            current_object->code[write_index] = Atom::VLPointer(variable_index);
        }
    } else {
        addLocalReference(v, write_index, p);

        if (!addLocalReference(v, write_index, p)) {
            set_error("cast to " + node->label.substr(node->label.find("#") + 1) + ": unknown class", node);
            return false;
        }

        if (write) {
            current_object->code[write_index] = Atom::Wildcard(p.atom.asOpcode()); // useless in skeleton expressions (already filled up in expression_head); usefull when the skeleton itself is a variable
        }
    }

    return true;
}

bool Compiler::read_reference(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write, const ReturnType t)
{
    uint16_t index;

    if ((t == ANY || (t != ANY && current_class.type == t)) && node->cmd == "this") {
        if (write) {
            current_object->code[write_index] = Atom::This();
        }

        return true;
    }

    if (local_reference(node, index, t)) {
        if (write) {
            current_object->code[write_index] = Atom::VLPointer(index);    // local references are always pointing to the value array
        }

        return true;
    }

    if (global_reference(node, index, t)) { // index is the index held by a reference pointer
        if (write) {
            current_object->code[write_index] = Atom::RPointer(index);
        }

        return true;
    }

    std::vector<int16_t> v;

    if (this_indirection(node, v, t)) {
        if (write) {
            current_object->code[write_index] = Atom::IPointer(extent_index);
            current_object->code[extent_index++] = Atom::CPointer(v.size() + 1);
            current_object->code[extent_index++] = Atom::This();

            for (int16_t type : v) {
                switch (type) {
                case -1:
                    current_object->code[extent_index++] = Atom::View();
                    break;

                case -2:
                    current_object->code[extent_index++] = Atom::Mks();
                    break;

                case -3:
                    current_object->code[extent_index++] = Atom::Vws();
                    break;

                default:
                    current_object->code[extent_index++] = Atom::IPointer(type);
                    break;
                }
            }
        }

        return true;
    }

    uint32_t cast_opcode;

    if (local_indirection(node, v, t, cast_opcode)) {
        if (write) {
            current_object->code[write_index] = Atom::IPointer(extent_index);
            current_object->code[extent_index++] = Atom::CPointer(v.size());
            current_object->code[extent_index++] = Atom::VLPointer(v[0], cast_opcode);

            for (uint16_t i = 1; i < v.size(); ++i) {
                switch (v[i]) {
                case -1:
                    current_object->code[extent_index++] = Atom::View();
                    break;

                case -2:
                    current_object->code[extent_index++] = Atom::Mks();
                    break;

                case -3:
                    current_object->code[extent_index++] = Atom::Vws();
                    break;

                default:
                    current_object->code[extent_index++] = Atom::IPointer(v[i]);
                    break;
                }
            }
        }

        return true;
    }

    if (global_indirection(node, v, t)) { // v[0] is the index held by a reference pointer
        if (write) {
            current_object->code[write_index] = Atom::IPointer(extent_index);
            current_object->code[extent_index++] = Atom::CPointer(v.size());
            current_object->code[extent_index++] = Atom::RPointer(v[0]);

            for (uint16_t i = 1; i < v.size(); ++i) {
                switch (v[i]) {
                case -2:
                    current_object->code[extent_index++] = Atom::Mks();
                    break;

                case -3:
                    current_object->code[extent_index++] = Atom::Vws();
                    break;

                default:
                    current_object->code[extent_index++] = Atom::IPointer(v[i]);
                    break;
                }
            }
        }

        return true;
    }

    if (hlp_reference(node, index)) {
        if (write) {
            current_object->code[write_index] = Atom::VLPointer(index);
        }

        return true;
    }

    return false;
}

bool Compiler::read_wildcard(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == ":") {
        if (state.pattern_lvl) {
            if (write) {
                current_object->code[write_index] = Atom::Wildcard();
            }

            return true;
        } else {
            set_error(" error: no wildcards allowed outside a pattern skeleton", node);
            return false;
        }
    }

    return false;
}

bool Compiler::read_tail_wildcard(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write)
{
    if (node->cmd == "::") {
        if (state.pattern_lvl) {
            if (write) {
                current_object->code[write_index] = Atom::TailWildcard();
            }

            return true;
        } else {
            set_error(" error: no wildcards allowed outside a pattern skeleton", node);
            return false;
        }
    }

    return false;
}

}
