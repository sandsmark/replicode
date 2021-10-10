//	compiler.h
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

#ifndef compiler_h
#define compiler_h




#include <r_comp/class.h>             // for Class
#include <r_comp/segments.h>          // for Reference
#include <r_comp/structure_member.h>  // for ReturnType
#include <r_comp/replistruct.h>
#include <stdint.h>                   // for uint16_t, int16_t, uint32_t, etc
#include <string>                     // for string
#include <unordered_map>              // for unordered_map
#include <vector>                     // for vector

#include <replicode_common.h>          // for REPLICODE_EXPORT

namespace r_code {
class ImageObject;
}  // namespace r_code
namespace r_comp {
class Image;
class Metadata;
class Reference;
}  // namespace r_comp

using namespace r_code;

namespace r_comp
{

class RepliStruct;

class REPLICODE_EXPORT Compiler
{

    std::string error;
    std::string m_errorFile;
    int m_errorLine;

    bool trace;

    Class current_class; // the sys-class currently parsed.
    ImageObject *current_object = nullptr; // the sys-object currently parsed.
    uint64_t current_object_index; // ordinal of the current sys-object in the code segment.

    r_comp::Image *_image;
    r_comp::Metadata *_metadata;

    struct State {
        State(): pattern_lvl(0),
            no_arity_check(false) {}
        State(Compiler *c): pattern_lvl(c->state.pattern_lvl),
            no_arity_check(c->state.no_arity_check) {}
        uint16_t pattern_lvl; // add one when parsing skel in (ptn skel guards), sub one when done.
        bool no_arity_check; // set to true when a tail wildcard is encountered while parsing an expression, set back to false when done parsing the expression.
    };

    State state;
    State save_state(); // called before trying to read an expression.
    void restore_state(State s); // called after failing to read an expression.

    void set_error(const std::string &s, RepliStruct::Ptr node);
    void set_arity_error(RepliStruct::Ptr node, uint16_t expected, uint16_t got);

    /// labels and variables declared inside objects (cleared before parsing each sys-object): translate to value pointers.
    std::unordered_map<std::string, Reference> local_references;

    // labels declared outside sys-objects. translate to reference pointers.
    bool addLocalReference(const std::string reference_name, const uint16_t index, const Class &p); // detect cast.
    bool getGlobalReferenceIndex(const std::string reference_name, const ReturnType t, ImageObject *object, uint16_t &index, Class *&_class); // index points to the reference set.
    // return false when not found.

    /// In high-level pattern
    bool in_hlp;
    std::vector<std::string> hlp_references;
    uint32_t add_hlp_reference(std::string reference_name);
    uint8_t get_hlp_reference(std::string reference_name);

    // Utility.
    bool read_nil(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_nil_set(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_nil_nb(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_nil_us(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_forever_nb(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_nil_nid(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_nil_did(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_nil_fid(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_nil_bl(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_nil_st(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_variable(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write, const Class p);
    bool read_reference(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write, const ReturnType t);
    bool read_wildcard(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_tail_wildcard(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write);

    bool err; // set to true when parsing fails in the functions below.

    // All functions below return false (a) upon eof or, (b) when the class structure is not matched; in both cases, characters are pushed back.

    // Lexical units.
    bool local_reference(RepliStruct::Ptr node, uint16_t &index, const ReturnType t); // must conform to t; indicates if the ref is to ba valuated in the value array (in_pattern set to true).
    bool global_reference(RepliStruct::Ptr node, uint16_t &index, const ReturnType t); // no conformance: return type==ANY.
    bool hlp_reference(RepliStruct::Ptr node, uint16_t &index);

    /// Helper function for this_indirection, local_indirection, global_indirection
    /// Parses \a path, e. g. "f.vw.ijt", and puts the result into \a indices.
    bool indirection(RepliStruct::Ptr node, Class *reference_class, std::string path, std::vector<int16_t> *indices, const ReturnType t);
    bool this_indirection(RepliStruct::Ptr node, std::vector<int16_t> &indices, const ReturnType t); // ex: this.res.
    bool local_indirection(RepliStruct::Ptr node, std::vector<int16_t> &indices, const ReturnType expected_type, uint32_t &cast_opcode); // ex: p.res where p is a label/variable declared within the object; cast_opcode=0x0FFF if no cast.
    bool global_indirection(RepliStruct::Ptr node, std::vector<int16_t> &indices, const ReturnType t); // ex: p.res where p is a label/variable declared outside the object.

    bool object(RepliStruct::Ptr node, Class &p); // looks first in sys_objects, then in objects.
    bool is_object(RepliStruct::Ptr node, const Class &p); // must conform to p.
    bool sys_object(RepliStruct::Ptr node, Class &p); // looks only in sys_objects.
    bool marker(RepliStruct::Ptr node, Class &p);
    bool op(RepliStruct::Ptr node, Class &p, const ReturnType t); // operator; must conform to t. return true if type matches t or ANY.
    bool is_op(RepliStruct::Ptr node, const Class &p); // must conform to p.
    bool function(RepliStruct::Ptr node, Class &p); // device function.
    bool expression_head(RepliStruct::Ptr node, Class &p, const ReturnType t); // starts from the first element; arity does not count the head; must conform to t.
    bool is_expression_head(RepliStruct::Ptr node, const Class &p); // starts from the first element; arity does not count the head; must conform to p.
    bool expression_tail(RepliStruct::Ptr node, const Class &p, uint16_t write_index, uint16_t &extent_index, bool write); // starts from the second element; must conform to p.

    // Structural units; check for heading labels.
    bool expression(RepliStruct::Ptr node, const ReturnType t, uint16_t write_index, uint16_t &extent_index, bool write); // must conform to t.
    bool expression(RepliStruct::Ptr node, const Class &p, uint16_t write_index, uint16_t &extent_index, bool write); // must conform to p.
    bool set(RepliStruct::Ptr node, uint16_t write_index, uint16_t &extent_index, bool write); // no conformance, i.e. set of anything. [ ] is illegal; use |[] instead, or [nil].
    bool set(RepliStruct::Ptr node, const Class &p, uint16_t write_index, uint16_t &extent_index, bool write); // must conform to p. for class defs like member-name:[member-list] or !class (name[] member-list).

    bool read(RepliStruct::Ptr node, const StructureMember &m, bool enforce, uint16_t write_index, uint16_t &extent_index, bool write);

    bool read_sys_object(RepliStruct::Ptr node, RepliStruct::Ptr view); // compiles one object; return false when there is an error.
public:
    Compiler(r_comp::Image *_image, r_comp::Metadata *_metadata);

    bool compile(RepliStruct::Ptr rootNode,
                 bool trace); // set when compile() fails, e.g. returns false.

    std::string getError();

    // Read functions for defining structure members.
    // Always try to read nil (typed), a variable, a wildcrad or a tail wildcard first; then try to read the lexical unit; then try to read an expression returning the appropriate type.
    // indented: flag indicating if an indent has been found, meaning that a matching indent will have to be enforced.
    // enforce: set to true when the stream content has to conform with the type xxx in read_xxx.
    // _class: specifies the elements that shall compose a structure (expression or set).
    // write_index: the index where the r-atom shall be written (atomic data), or where an internal pointer to a structure shall be written (structural data).
    // extent_index: the index where to write data belonging to a structure (the internal pointer is written at write_index).
    // write: when false, no writing in code->data is performed (needed by set_element_count()).
    bool read_any(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write); // calls all of the functions below.
    bool read_number(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_timestamp(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write); // p always NULL
    bool read_boolean(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_string(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_node(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_device(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_function(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_expression(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_set(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write);
    bool read_class(RepliStruct::Ptr node, bool enforce, const Class *p, uint16_t write_index, uint16_t &extent_index, bool write);
};
}


#endif
