//	preprocessor.h
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

#ifndef preprocessor_h
#define preprocessor_h

#include <r_comp/structure_member.h>  // for ReturnType
#include <r_comp/replistruct.h>
#include <stddef.h>                   // for NULL
#include <stdint.h>                   // for uint16_t
#include <list>                       // for list
#include <string>                     // for string
#include <unordered_map>              // for unordered_map
#include <vector>                     // for vector

#include <replicode_common.h>          // for REPLICODE_EXPORT

namespace r_comp {
class Metadata;
class RepliStruct;
}  // namespace r_comp

namespace r_comp
{
class REPLICODE_EXPORT Preprocessor
{
public:
    RepliStruct::Ptr root;

    Preprocessor();
    ~Preprocessor();
    RepliStruct::Ptr process(const char *file, // if an ifstream, stream must be open.
                         std::string &error, // set when function fails, e.g. returns false.
                         Metadata *metadata = NULL); // process will fill class_image, or use the exiting one if NULL.

private:
    enum ClassType {
        T_CLASS = 0,
        T_SYS_CLASS = 1,
        T_SET = 2
    };

    void instantiateClass(RepliStruct::Ptr tpl_class, std::vector<RepliStruct::Ptr> &tpl_args, std::string &instantiated_class_name);
    bool isSet(std::string className);
    bool isTemplateClass(RepliStruct::Ptr replistruct);
    void getMember(std::vector<StructureMember> &members, RepliStruct::Ptr m, std::list<RepliStruct::Ptr> &tpl_args, bool instantiate);
    void getMembers(RepliStruct::Ptr s, std::vector<StructureMember> &members, std::list<RepliStruct::Ptr> &tpl_args, bool instantiate);
    ReturnType getReturnType(RepliStruct::Ptr s);
    void initialize(Metadata *metadata); // init definition_segment

    Metadata *m_metadata;
    uint16_t m_classOpcode; // shared with sys_classes
    std::unordered_map<std::string, RepliStruct::Ptr> m_templateClasses;
};
}


#endif
