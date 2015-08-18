//	model_base.h
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

#ifndef model_base_h
#define model_base_h


#include <stddef.h>            // for size_t
#include <stdint.h>            // for uint64_t
#include <mutex>               // for mutex
#include <unordered_set>       // for unordered_set

#include "CoreLibrary/base.h"  // for P

namespace r_code {
class Code;
template <typename T> class list;
}  // namespace r_code
namespace r_exec {
class _Fact;
}  // namespace r_exec

namespace r_exec
{

/// TPX guess models: this list is meant for TPXs to (a) avoid re-guessing known failed models and,
/// (b) avoid producing the same models in case they run concurrently.
/// The black list contains bad models (models that were killed). This list is trimmed down on a time basis (black_thz) by the garbage collector.
/// Each bad model is tagged with the last time it was successfully compared to. GC is performed by comparing this time to the thz.
/// The white list contains models that are still alive and is trimmed down when models time out.
/// Models are packed before insertion in the white list.
class ModelBase
{
    friend class _Mem;
private:
    static ModelBase *Singleton;

    std::mutex m_mdlMutex;

    uint64_t thz;

    class MEntry
    {
    private:
        static bool Match(r_code::Code *lhs, r_code::Code *rhs);
        /// use for lhs/rhs.
        static uint64_t _ComputeHashCode(_Fact *component);
    public:
        static uint64_t ComputeHashCode(r_code::Code *mdl, bool packed);

        MEntry();
        MEntry(r_code::Code *mdl, bool packed);

        core::P<r_code::Code> mdl;
        /// last time the mdl was successfully compared to.
        uint64_t touch_time;
        uint64_t hash_code;

        bool match(const MEntry &e) const;

        class Hash
        {
        public:
            size_t operator()(MEntry e) const
            {
                return e.hash_code;
            }
        };

        class Equal
        {
        public:
            bool operator()(const MEntry lhs, const MEntry rhs) const
            {
                return lhs.match(rhs);
            }
        };
    };

    typedef std::unordered_set<MEntry, typename MEntry::Hash, typename MEntry::Equal> MdlSet;

    /// mdls are already packed when inserted (they come from the white list).
    MdlSet black_list;

    /// mdls are packed just before insertion.
    MdlSet white_list;

    /// called by _Mem::start(); set to secondary_thz.
    void set_thz(uint64_t thz)
    {
        this->thz = thz;
    }

    /// called by _Mem::GC().
    void trim_objects();

    ModelBase();
public:
    static ModelBase *Get();

    /// called by _Mem::load(); models with no views go to the black_list.
    /// @variable mdl is already packed.
    void load(r_code::Code *mdl);
    void get_models(r_code::list<core::P<r_code::Code> > &models); // white_list first, black_list next.

    /// caveat: @variable mdl is unpacked; return (a) NULL if the model is in the black list, (b) a model in the white list if the @variable mdl has been registered there or (c) the @variable mdl itself if not in the model base, in which case the @variable mdl is added to the white list.
    r_code::Code *check_existence(r_code::Code *mdl);

    /// @varible m1 is a requirement on @variable m0; @variable _m0 and @variable _m1 are the return values as defined above; @variable m0 added only if @variable m1 is not black listed.
    /// @variable m0 and @variable m1 unpacked.
    void check_existence(r_code::Code *m0, r_code::Code *m1, r_code::Code *&_m0, r_code::Code *&_m1);

    /// moves the mdl from the white to the black list; happens to bad models.
    /// @variable mdl is packed.
    void register_mdl_failure(r_code::Code *mdl);

    /// deletes the mdl from the white list; happen to models that have been unused for primary_thz.
    /// @variable mdl is packed.
    void register_mdl_timeout(r_code::Code *mdl);
};
}


#endif
