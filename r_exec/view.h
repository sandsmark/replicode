//	view.h
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

#ifndef view_h
#define view_h

#include <r_code/atom.h>            // for Atom
#include <r_code/object.h>          // for View::SyncMode, View
#include <r_code/replicode_defs.h>  // for VIEW_CTRL_0, VIEW_CTRL_1
#include <stdint.h>                 // for uint64_t, int64_t, uint16_t, etc
#include <mutex>                    // for mutex

#include "CoreLibrary/base.h"       // for P
#include "CoreLibrary/dll.h"        // for REPLICODE_EXPORT

namespace r_exec {
class Controller;
}  // namespace r_exec


namespace r_exec
{

class Group;

// OID is hidden at _code[VIEW_OID].
// Shared resources:
// none: all mod/set operations are pushed on the group and executed at update time.
class REPLICODE_EXPORT View:
    public r_code::View
{
private:
    static uint64_t LastOID;
    static uint64_t GetOID();

    // Ctrl values.
    uint64_t sln_changes;
    float acc_sln;
    uint64_t act_changes;
    float acc_act;
    uint64_t vis_changes;
    float acc_vis;
    uint64_t res_changes;
    float acc_res;
    void reset_ctrl_values();

    // Monitoring
    double initial_sln;
    double initial_act;

    void init(SyncMode sync,
              uint64_t ijt,
              double sln,
              int64_t res,
              r_code::Code *host,
              r_code::Code *origin,
              r_code::Code *object);
protected:
    void reset_init_sln();
    void reset_init_act();
public:
    static uint16_t ViewOpcode;

    P<Controller> controller; // built upon injection of the view (if the object is an ipgm/icpp_pgm/cst/mdl).

    static double MorphValue(double value, double source_thr, double destination_thr);
    static double MorphChange(double change, double source_thr, double destination_thr);

    uint64_t periods_at_low_sln;
    uint64_t periods_at_high_sln;
    uint64_t periods_at_low_act;
    uint64_t periods_at_high_act;

    View();
    View(r_code::SysView *source, r_code::Code *object);
    View(View *view, Group *group); // copy the view and assigns it to the group (used for cov); morph ctrl values.
    View(const View *view, bool new_OID = false); // simple copy.
    View(SyncMode sync,
         uint64_t ijt,
         double sln,
         int64_t res,
         r_code::Code *host,
         r_code::Code *origin,
         r_code::Code *object); // regular view; res set to -1 means forever.
    View(SyncMode sync,
         uint64_t ijt,
         double sln,
         int64_t res,
         r_code::Code *host,
         r_code::Code *origin,
         r_code::Code *object,
         double act); // pgm/mdl view; res set to -1 means forever.
    ~View();

    void reset();
    void set_object(r_code::Code *object);

    uint64_t get_oid() const;

    virtual bool isNotification() const;

    Group *get_host();

    SyncMode get_sync();
    float get_res();
    float get_sln();
    float get_act();
    bool get_cov();
    float get_vis();
    uint32_t &ctrl0()
    {
        return _code[VIEW_CTRL_0].atom; // use only for non-group views.
    }
    uint32_t &ctrl1()
    {
        return _code[VIEW_CTRL_1].atom; // idem.
    }

    void mod_res(double value);
    void set_res(double value);
    void mod_sln(double value);
    void set_sln(double value);
    void mod_act(double value);
    void set_act(double value);
    void mod_vis(double value);
    void set_vis(double value);

    double update_res();
    double update_sln(double low, double high);
    double update_act(double low, double high);
    double update_vis();

    float update_sln_delta();
    float update_act_delta();

    void force_res(double value); // unmediated.

    // Target res, sln, act, vis.
    void mod(uint16_t member_index, double value);
    void set(uint16_t member_index, double value);

    void delete_from_object();
    void delete_from_group();
private:
    std::mutex m_groupMutex;
};

class REPLICODE_EXPORT NotificationView:
    public View
{
public:
    NotificationView(r_code::Code *origin, r_code::Code *destination, r_code::Code *marker); // res=1, sln=1.

    bool isNotification() const;
};
}

#endif
