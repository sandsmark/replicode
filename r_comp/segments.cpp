//	segments.cpp
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

#include "segments.h"

#include <r_code/atom.h>            // for Atom, Atom::::COMPOSITE_STATE, etc
#include <r_code/image.h>           // for GetStringSize, ReadString, etc
#include <r_code/list.h>            // for list<>::const_iterator, list, etc
#include <r_code/object.h>          // for SysObject, Code, View, SysView, etc
#include <r_code/replicode_defs.h>  // for CST_HIDDEN_REFS, MDL_HIDDEN_REFS
#include <r_comp/segments.h>        // for Metadata, Image, CodeSegment, etc
#include <string.h>                 // for memcpy
#include <cstdint>                  // for uint32_t, uintptr_t, uint64_t, etc
#include <unordered_set>            // for unordered_set<>::const_iterator, etc
#include <utility>                  // for pair
#include <vector>                   // for vector

#include "CoreLibrary/base.h"       // for P
#include "CoreLibrary/debug.h"      // for DebugStream, debug


namespace r_comp
{

Reference::Reference()
{
}

Reference::Reference(const uintptr_t i, const Class &c, const Class &cc): index(i), _class(c), cast_class(cc)
{
}

////////////////////////////////////////////////////////////////

Metadata::Metadata()
{
}

std::string Metadata::getObjectName(const uint16_t index) const
{
    std::unordered_map<std::string, Reference>::const_iterator r;

    for (r = global_references.begin(); r != global_references.end(); ++r) {
        if (r->second.index == index) {
            return r->first;
        }
    }

    std::string s;
    return s;
}

Class *Metadata::get_class(std::string &class_name)
{
    std::unordered_map<std::string, Class>::iterator it = classes.find(class_name);

    if (it != classes.end()) {
        return &it->second;
    }

    return nullptr;
}

Class *Metadata::get_class(size_t opcode)
{
    return &classes_by_opcodes[opcode];
}

void Metadata::write(uint32_t *data)
{
    data[0] = classes_by_opcodes.size();
    size_t i;
    size_t offset = 1;

    for (i = 0; i < classes_by_opcodes.size(); ++i) {
        classes_by_opcodes[i].write(data + offset);
        offset += classes_by_opcodes[i].get_size();
    }

    data[offset++] = classes.size();
    std::unordered_map<std::string, Class>::iterator it = classes.begin();

    for (; it != classes.end(); ++it) {
        r_code::WriteString(data + offset, it->first);
        offset += r_code::GetStringSize(it->first);
        data[offset] = it->second.atom.asOpcode();
        offset++;
    }

    data[offset++] = sys_classes.size();
    it = sys_classes.begin();

    for (; it != sys_classes.end(); ++it) {
        r_code::WriteString(data + offset, it->first);
        offset += r_code::GetStringSize(it->first);
        data[offset] = it->second.atom.asOpcode();
        offset++;
    }

    data[offset++] = class_names.size();

    for (i = 0; i < class_names.size(); ++i) {
        r_code::WriteString(data + offset, class_names[i]);
        offset += r_code::GetStringSize(class_names[i]);
    }

    data[offset++] = operator_names.size();

    for (i = 0; i < operator_names.size(); ++i) {
        r_code::WriteString(data + offset, operator_names[i]);
        offset += r_code::GetStringSize(operator_names[i]);
    }

    data[offset++] = function_names.size();

    for (i = 0; i < function_names.size(); ++i) {
        r_code::WriteString(data + offset, function_names[i]);
        offset += r_code::GetStringSize(function_names[i]);
    }
}

void Metadata::read(uint32_t *data, size_t size)
{
    size_t class_count = data[0];
    size_t i;
    size_t offset = 1;

    for (i = 0; i < class_count; ++i) {
        Class c;
        c.read(data + offset);
        classes_by_opcodes.push_back(c);
        offset += c.get_size();
    }

    size_t classes_count = data[offset++];

    for (i = 0; i < classes_count; ++i) {
        std::string name = r_code::ReadString(data + offset);
        offset += r_code::GetStringSize(name);
        classes[name] = classes_by_opcodes[data[offset++]];
    }

    size_t sys_classes_count = data[offset++];

    for (i = 0; i < sys_classes_count; ++i) {
        std::string name = r_code::ReadString(data + offset);
        offset += r_code::GetStringSize(name);
        sys_classes[name] = classes_by_opcodes[data[offset++]];
    }

    size_t class_names_count = data[offset++];

    for (i = 0; i < class_names_count; ++i) {
        std::string name = r_code::ReadString(data + offset);
        class_names.push_back(name);
        offset += r_code::GetStringSize(name);
    }

    size_t operator_names_count = data[offset++];

    for (i = 0; i < operator_names_count; ++i) {
        std::string name = r_code::ReadString(data + offset);
        operator_names.push_back(name);
        offset += r_code::GetStringSize(name);
    }

    size_t function_names_count = data[offset++];

    for (i = 0; i < function_names_count; ++i) {
        std::string name = r_code::ReadString(data + offset);
        function_names.push_back(name);
        offset += r_code::GetStringSize(name);
    }
}

size_t Metadata::get_size()
{
    return get_class_array_size() +
           get_classes_size() +
           get_sys_classes_size() +
           get_class_names_size() +
           get_operator_names_size() +
           get_function_names_size();
}

// RAM layout:
// - class array
// - number of elements
// - list of classes:
// - atom
// - string (str_opcode)
// - return type
// - usage
// - things to read:
// - number of elements
// - list of structure members:
// - ID of a Compiler::*_Read function
// - return type
// - string (_class)
// - iteration
// - classes:
// - number of elements
// - list of pairs:
// - string
// - index of a class in the class array
// - sys_classes:
// - number of elements
// - list of pairs:
// - string
// - index of a class in the class array
// - class names:
// - number of elements
// - list of strings
// - operator names:
// - number of elements
// - list of strings
// - function names:
// - number of elements
// - list of strings
//
// String layout:
// - size in size_t
// - list of words: contain the charaacters; the last one is \0; some of the least significant bytes of the last word my be empty

uintptr_t Metadata::get_class_array_size()
{
    size_t size = 1; // size of the array

    for (const Class &c : classes_by_opcodes) {
        size += c.get_size();
    }

    return size;
}

uintptr_t Metadata::get_classes_size()
{
    size_t size = 1; // size of the hash table

    for (const auto &elem : classes) {
        size += r_code::GetStringSize(elem.first) + 1;    // +1: index to the class in the class array
    }

    return size;
}

uintptr_t Metadata::get_sys_classes_size()
{
    size_t size = 1; // size of the hash table

    for (const auto &elem : sys_classes) {
        size += r_code::GetStringSize(elem.first) + 1;    // +1: index to the class in the class array
    }

    return size;
}

size_t Metadata::get_class_names_size()
{
    size_t size = 1; // size of the vector

    for (const std::string & name : class_names) {
        size += r_code::GetStringSize(name);
    }

    return size;
}

size_t Metadata::get_operator_names_size()
{
    size_t size = 1; // size of the vector

    for (const std::string &name : operator_names) {
        size += r_code::GetStringSize(name);
    }

    return size;
}

size_t Metadata::get_function_names_size()
{
    size_t size = 1; // size of the vector

    for (const std::string &name : function_names) {
        size += r_code::GetStringSize(name);
    }

    return size;
}

////////////////////////////////////////////////////////////////

void ObjectMap::shift(uint32_t offset)
{
    for (uint32_t &object : objects) {
        object += offset;
    }
}

void ObjectMap::write(uint32_t *data)
{
    for (size_t i = 0; i < objects.size(); ++i) {
        data[i] = objects[i];
    }
}

void ObjectMap::read(uint32_t *data, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        objects.push_back(data[i]);
    }
}

size_t ObjectMap::get_size() const
{
    return objects.size();
}

////////////////////////////////////////////////////////////////

CodeSegment::~CodeSegment()
{
    for (SysObject *object : objects) {
        delete object;
    }
}

void CodeSegment::write(uint32_t *data)
{
    uint32_t offset = 0;

    for (SysObject *object : objects) {
        object->write(data + offset);
        offset += object->get_size();
    }
}

void CodeSegment::read(uint32_t *data, size_t object_count)
{
    size_t offset = 0;

    for (size_t i = 0; i < object_count; ++i) {
        SysObject *o = new SysObject();
        o->read(data + offset);
        objects.push_back(o);
        offset += o->get_size();
    }
}

size_t CodeSegment::get_size()
{
    size_t size = 0;

    for (SysObject *object : objects) {
        size += object->get_size();
    }

    return size;
}

////////////////////////////////////////////////////////////////


ObjectNames::~ObjectNames()
{
}

void ObjectNames::write(uint32_t *data)
{
    data[0] = symbols.size();
    size_t index = 1;

    for(auto symbol : symbols) {
        data[index] = symbol.first;
        size_t symbol_length = symbol.second.length() + 1; // add a trailing null character (for reading).
        size_t _symbol_length = symbol_length / 4;
        size_t __symbol_length = symbol_length % 4;

        if (__symbol_length) {
            ++_symbol_length;
        }

        data[index + 1] = _symbol_length;
        memcpy(data + index + 2, symbol.second.c_str(), symbol_length);
        index += _symbol_length + 2;
    }
}

void ObjectNames::read(uint32_t *data)
{
    size_t symbol_count = data[0];
    size_t index = 1;

    for (size_t i = 0; i < symbol_count; ++i) {
        uintptr_t oid = data[index];
        size_t symbol_length = data[index + 1]; // number of words needed to store all the characters.
        std::string symbol((char *)(data + index + 2));
        symbols[oid] = symbol;
        index += symbol_length + 2;
    }
}

size_t ObjectNames::get_size()
{
    size_t size = 1; // size of symbols.

    for (auto n : symbols) {
        size += 2; // oid and symbol's length.
        size_t symbol_length = n.second.length() + 1;
        size_t _symbol_length = symbol_length / 4;
        size_t __symbol_length = symbol_length % 4;

        if (__symbol_length) {
            ++_symbol_length;
        }

        size += _symbol_length; // characters packed in 32 bits words.
    }

    return size;
}

////////////////////////////////////////////////////////////////

Image::Image(): map_offset(0), timestamp(0)
{
}

Image::~Image()
{
}

void Image::add_sys_object(SysObject *object, std::string name)
{
    add_sys_object(object);

    if (!name.empty()) {
        object_names.symbols[object->oid] = name;
    }
}

void Image::add_sys_object(SysObject *object)
{
    code_segment.objects.push_back(object);
    object_map.objects.push_back(map_offset);
    map_offset += object->get_size();
}

void Image::add_objects(r_code::list<P<r_code::Code> > &objects)
{
    r_code::list<P<r_code::Code> >::const_iterator o;

    for (o = objects.begin(); o != objects.end(); ++o) {
        if (!(*o)->is_invalidated()) {
            add_object(*o);
        }
    }

    build_references();
}

void Image::add_objects_full(r_code::list<P<r_code::Code> > &objects)
{
    r_code::list<P<r_code::Code> >::const_iterator o;

    for (o = objects.begin(); o != objects.end(); ++o) {
        add_object_full(*o);
    }

    build_references();
}

size_t Image::get_reference_count(const Code* object) const
{
    switch (object->code(0).getDescriptor()) { // ignore the last reference as it is the unpacked version of the object.
    case Atom::MODEL:
        return object->references_size() - MDL_HIDDEN_REFS;

    case Atom::COMPOSITE_STATE:
        return object->references_size() - CST_HIDDEN_REFS;

    default:
        return object->references_size();
    }
}

void Image::add_object(Code *object)
{
    std::unordered_map<Code *, size_t>::iterator it = ptrs_to_indices.find(object);

    if (it != ptrs_to_indices.end()) { // object already there.
        return;
    }

    size_t object_index;
    ptrs_to_indices[object] = object_index = code_segment.objects.as_std()->size();
    SysObject *sys_object = new SysObject(object);
    add_sys_object(sys_object);
    size_t reference_count = get_reference_count(object);

    for (size_t i = 0; i < reference_count; ++i) { // follow reference pointers and recurse.
        Code *reference = object->get_reference(i);

        if (reference->get_oid() == UNDEFINED_OID ||
            reference->is_invalidated()) { // the referenced object is not in the image and will not be added otherwise.
            add_object(reference);
        }
    }

    uintptr_t _object = (uintptr_t)object;
    sys_object->references[0] = (_object & 0x00000000FFFFFFF);
    sys_object->references[1] = (_object >> 32);
}

void Image::add_object_full(Code *object)
{
    std::unordered_map<Code *, size_t>::iterator it = ptrs_to_indices.find(object);

    if (it != ptrs_to_indices.end()) { // object already there.
        return;
    }

    size_t object_index;
    ptrs_to_indices[object] = object_index = code_segment.objects.as_std()->size();
    SysObject *sys_object = new SysObject(object);
    add_sys_object(sys_object);
    size_t reference_count = get_reference_count(object);

    for (size_t i = 0; i < reference_count; ++i) { // follow the object's reference pointers and recurse.
        Code *reference = object->get_reference(i);

        if (reference->get_oid() == UNDEFINED_OID) { // the referenced object is not in the image and will not be added otherwise.
            add_object_full(reference);
        } else { // add the referenced object if not present in the list.
            std::unordered_map<Code *, size_t>::iterator it = ptrs_to_indices.find(reference);

            if (it == ptrs_to_indices.end()) {
                add_object_full(reference);
            }
        }
    }

    object->acq_views();
    std::unordered_set<View *, View::Hash, View::Equal>::const_iterator v;

    for (v = object->views.begin(); v != object->views.end(); ++v) { // follow the view's reference pointers and recurse.
        for (uint8_t j = 0; j < 2; ++j) { // 2 refs maximum per view; may be NULL.
            Code *reference = (*v)->references[j];

            if (reference) {
                std::unordered_map<r_code::Code *, size_t>::const_iterator index = ptrs_to_indices.find(reference);

                if (index == ptrs_to_indices.end()) {
                    add_object_full(reference);
                }
            }
        }
    }

    object->rel_views();
    uint64_t _object = uint64_t(object);
    sys_object->references[0] = (_object & 0x00000000FFFFFFF);
    sys_object->references[1] = (_object >> 32);
}

void Image::build_references()
{
    Code *object;
    SysObject *sys_object;

    for (size_t i = 0; i < code_segment.objects.as_std()->size(); ++i) {
        sys_object = code_segment.objects[i];
        uintptr_t _object = sys_object->references[0];
        _object |= (uint64_t(sys_object->references[1]) << 32);
        object = (Code *)_object;
        sys_object->references.as_std()->clear();
        build_references(sys_object, object);
    }
}

void Image::build_references(SysObject *sys_object, Code *object)
{
    // Translate pointers into indices: valuate the sys_object's references to object, incl. sys_object's view references.
    size_t i;
    size_t referenced_object_index;
    size_t reference_count = get_reference_count(object);

    for (i = 0; i < reference_count; ++i) {
        std::unordered_map<r_code::Code *, size_t>::const_iterator index = ptrs_to_indices.find(object->get_reference(i));

        if (index != ptrs_to_indices.end()) {
            referenced_object_index = index->second;
            sys_object->references.push_back(referenced_object_index);
        }
    }

    object->acq_views();
    std::unordered_set<View *, View::Hash, View::Equal>::const_iterator v;

    for (i = 0, v = object->views.begin(); v != object->views.end(); ++i, ++v) {
        for (uint8_t j = 0; j < 2; ++j) { // 2 refs maximum per view; may be NULL.
            Code *reference = (*v)->references[j];

            if (reference) {
                std::unordered_map<r_code::Code *, size_t>::const_iterator index = ptrs_to_indices.find(reference);

                if (index != ptrs_to_indices.end()) {
                    referenced_object_index = index->second;
                    sys_object->views[i]->references[j] = referenced_object_index;
                }
            }
        }
    }

    object->rel_views();
}

void Image::get_objects(Mem *mem, r_code::vector<Code *> &ram_objects)
{
    debug("image") << "number of objects: " << code_segment.objects.size();

    for (size_t i = 0; i < code_segment.objects.size(); ++i) {
        ram_objects[i] = mem->build_object(code_segment.objects[i]);
    }

    unpack_objects(ram_objects);
}

void Image::unpack_objects(r_code::vector<Code *> &ram_objects)
{
    // For each object, translate its reference indices into pointers; build its views; for each view translate its reference indices into pointers.
    for (size_t i = 0; i < code_segment.objects.size(); ++i) {
        SysObject *sys_object = code_segment.objects[i];
        Code *ram_object = ram_objects[i];

        for (size_t j = 0; j < sys_object->views.as_std()->size(); ++j) {
            SysView *sys_v = sys_object->views[j];
            View *v = ram_object->build_view(sys_v);

            for (size_t k = 0; k < sys_v->references.as_std()->size(); ++k) {
                v->references[k] = ram_objects[sys_v->references[k]];
            }

            ram_object->views.insert(v);
        }

        for (size_t j = 0; j < sys_object->references.as_std()->size(); ++j) {
            ram_object->set_reference(j, ram_objects[sys_object->references[j]]);
        }
    }
}
}
