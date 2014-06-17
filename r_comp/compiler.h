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

#include <fstream>
#include <sstream>

#include "out_stream.h"
#include "segments.h"


using namespace r_code;

namespace r_comp {

class RepliStruct;

class dll_export Compiler {
private:
    std::string error;
    std::string m_errorFile;
    int m_errorLine;

    bool trace;

    Class current_class; // the sys-class currently parsed.
    ImageObject *current_object; // the sys-object currently parsed.
    uint64 current_object_index; // ordinal of the current sys-object in the code segment.
    int64 current_view_index; // ordinal of a view in the sys-object's view set.

    r_comp::Image *_image;
    r_comp::Metadata *_metadata;

    class State {
    public:
        State(): pattern_lvl(0),
            no_arity_check(false) {}
        State(Compiler *c): pattern_lvl(c->state.pattern_lvl),
            no_arity_check(c->state.no_arity_check) {}
        uint16 pattern_lvl; // add one when parsing skel in (ptn skel guards), sub one when done.
        bool no_arity_check; // set to true when a tail wildcard is encountered while parsing an expression, set back to false when done parsing the expression.
    };

    State state;
    State save_state(); // called before trying to read an expression.
    void restore_state(State s); // called after failing to read an expression.

    void set_error(const std::string &s, RepliStruct *node);
    void set_arity_error(RepliStruct *node, uint16 expected, uint16 got);

    /// labels and variables declared inside objects (cleared before parsing each sys-object): translate to value pointers.
    UNORDERED_MAP<std::string, Reference> local_references;
   
    // labels declared outside sys-objects. translate to reference pointers.
    UNORDERED_MAP<std::string, Reference> global_references;
    bool addLocalReference(const std::string reference_name, const uint16 index, const Class &p); // detect cast.
    bool getGlobalReferenceIndex(const std::string reference_name, const ReturnType t, ImageObject *object, uint16 &index, Class *&_class); // index points to the reference set.
// return false when not found.

    /// In high-level pattern
    bool in_hlp;
    std::vector<std::string> hlp_references;
    uint8 add_hlp_reference(std::string reference_name);
    uint8 get_hlp_reference(std::string reference_name);

// Utility.
    bool read_nil(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_nil_set(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_nil_nb(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_nil_us(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_forever_nb(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_nil_nid(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_nil_did(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_nil_fid(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_nil_bl(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_nil_st(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_variable(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write, const Class p);
    bool read_reference(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write, const ReturnType t);
    bool read_wildcard(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);
    bool read_tail_wildcard(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write);

    bool err; // set to true when parsing fails in the functions below.

// All functions below return false (a) upon eof or, (b) when the class structure is not matched; in both cases, characters are pushed back.

// Lexical units.
    bool local_reference(RepliStruct *node, uint16 &index, const ReturnType t); // must conform to t; indicates if the ref is to ba valuated in the value array (in_pattern set to true).
    bool global_reference(RepliStruct *node, uint16 &index, const ReturnType t); // no conformance: return type==ANY.
    bool hlp_reference(RepliStruct *node, uint16 &index);
    bool this_indirection(RepliStruct *node, std::vector<int16> &v, const ReturnType t); // ex: this.res.
    bool local_indirection(RepliStruct *node, std::vector<int16> &v, const ReturnType t, uint32_t &cast_opcode); // ex: p.res where p is a label/variable declared within the object; cast_opcode=0x0FFF if no cast.
    bool global_indirection(RepliStruct *node, std::vector<int16> &v, const ReturnType t); // ex: p.res where p is a label/variable declared outside the object.
    bool object(RepliStruct *node, Class &p); // looks first in sys_objects, then in objects.
    bool object(RepliStruct *node, const Class &p); // must conform to p.
    bool sys_object(RepliStruct *node, Class &p); // looks only in sys_objects.
    bool sys_object(RepliStruct *node, const Class &p); // must conform to p.
    bool marker(RepliStruct *node, Class &p);
    bool op(RepliStruct *node, Class &p, const ReturnType t); // operator; must conform to t. return true if type matches t or ANY.
    bool op(RepliStruct *node, const Class &p); // must conform to p.
    bool function(RepliStruct *node, Class &p); // device function.
    bool expression_head(RepliStruct *node, Class &p, const ReturnType t); // starts from the first element; arity does not count the head; must conform to t.
    bool expression_head(RepliStruct *node, const Class &p); // starts from the first element; arity does not count the head; must conform to p.
    bool expression_tail(RepliStruct *node, const Class &p, uint16 write_index, uint16 &extent_index, bool write); // starts from the second element; must conform to p.

// Structural units; check for heading labels.
    bool expression(RepliStruct *node, const ReturnType t, uint16 write_index, uint16 &extent_index, bool write); // must conform to t.
    bool expression(RepliStruct *node, const Class &p, uint16 write_index, uint16 &extent_index, bool write); // must conform to p.
    bool set(RepliStruct *node, uint16 write_index, uint16 &extent_index, bool write); // no conformance, i.e. set of anything. [ ] is illegal; use |[] instead, or [nil].
    bool set(RepliStruct *node, const Class &p, uint16 write_index, uint16 &extent_index, bool write); // must conform to p. for class defs like member-name:[member-list] or !class (name[] member-list).

    bool read(RepliStruct *node, const StructureMember &m, bool enforce, uint16 write_index, uint16 &extent_index, bool write);

    bool read_sys_object(RepliStruct *node, RepliStruct *view); // compiles one object; return false when there is an error.
public:
    Compiler();

    bool compile(RepliStruct *rootNode, // stream must be open.
                 r_comp::Image *_image,
                 r_comp::Metadata *_metadata,
                 std::string &error,
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
    bool read_any(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write); // calls all of the functions below.
    bool read_number(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write);
    bool read_timestamp(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write); // p always NULL
    bool read_boolean(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write);
    bool read_string(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write);
    bool read_node(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write);
    bool read_device(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write);
    bool read_function(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write);
    bool read_expression(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write);
    bool read_set(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write);
    bool read_class(RepliStruct *node, bool enforce, const Class *p, uint16 write_index, uint16 &extent_index, bool write);

// Convenience to retrieve axiom names by index.
    std::string getObjectName(const uint16 index) const;
};
}


#endif
