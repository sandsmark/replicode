//	test.cpp
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

#include <r_code/atom.h>            // for Atom
#include <r_code/image.h>           // for Image
#include <r_code/image_impl.h>      // for ImageImpl
#include <r_code/object.h>          // for Code, View, View::::SYNC_ONCE
#include <r_code/replicode_defs.h>  // for VIEW_ARITY, VIEW_HOST, VIEW_IJT, etc
#include <r_code/utils.h>           // for Utils
#include <r_code/vector.h>          // for vector
#include <r_comp/decompiler.h>      // for Decompiler
#include <r_comp/segments.h>        // for Image, ObjectNames, Metadata
#include <r_exec/factory.h>         // for Fact
#include <r_exec/init.h>            // for Now, Compile, Init
#include <r_exec/mem.h>             // for _Mem, Mem, MemStatic, etc
#include <r_exec/object.h>          // for LObject
#include <r_exec/opcodes.h>         // for Opcodes, Opcodes::MkVal
#include <r_exec/view.h>            // for View, View::ViewOpcode
#include <stdint.h>                 // for uint64_t, uint16_t, uintptr_t
#include <stdlib.h>                 // for srand
#include <chrono>                   // for microseconds, duration_cast, etc
#include <fstream>                  // for ofstream, ostream, ifstream, etc
#include <iostream>                 // for cout, operator|, ios_base, etc
#include <ratio>                    // for ratio
#include <string>                   // for string, operator==, basic_string, etc
#include <thread>                   // for sleep_for
#include <type_traits>              // for enable_if<>::type
#include <unordered_map>            // for unordered_map, etc
#include <utility>                  // for pair
#include <sstream>                  // for ostringstream


#include <replicode_common.h>      // for debug, DebugStream
#include "settings.h"               // for Settings

static bool fileExists(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

//#define DECOMPILE_ONE_BY_ONE

using namespace r_comp;

r_exec::View *build_view(uint64_t time, Code* rstdin)   // this is application dependent WRT view->sync.
{
    r_exec::View *view = new r_exec::View();
    const uint64_t arity = VIEW_ARITY; // reminder: opcode not included in the arity.
    uint16_t write_index = 0;
    uint16_t extent_index = arity + 1;
    view->code(VIEW_OPCODE) = Atom::SSet(r_exec::View::ViewOpcode, arity);
    view->code(VIEW_SYNC) = Atom::Float(View::SYNC_ONCE); // sync on front.
    view->code(VIEW_IJT) = Atom::IPointer(extent_index); // iptr to injection time.
    view->code(VIEW_SLN) = Atom::Float(1.0); // sln.
    view->code(VIEW_RES) = Atom::Float(1); // res is set to 1 upr of the destination group.
    view->code(VIEW_HOST) = Atom::RPointer(0); // stdin/stdout is the only reference.
    view->code(VIEW_ORG) = Atom::Nil(); // org.
    Utils::SetTimestamp(&view->code(extent_index), time);
    view->references[0] = rstdin;
    return view;
}

Code *make_object(r_exec::_Mem *mem, Code* rstdin, double i)
{
    Code *object = new r_exec::LObject(mem);
    object->code(0) = Atom::Marker(r_exec::Opcodes::MkVal, MK_VAL_ARITY); // Caveat: arity does not include the opcode.
    object->code(MK_VAL_OBJ) = Atom::RPointer(0);
    object->code(MK_VAL_ATTR) = Atom::RPointer(1);
    object->code(MK_VAL_VALUE) = Atom::Float(i);
    object->code(MK_VAL_PSLN_THR) = Atom::Float(1); // psln_thr.
    object->set_reference(0, rstdin);
    object->set_reference(1, rstdin);
    return object;
}

void test_injection(r_exec::_Mem *mem, double n)
{
    Code* rstdin = mem->get_stdin();
    // mem->timings_report.clear();
    // uint64_t tt1 = 0;
    // uint64_t tt2 = 0;
    // uint64_t tt3 = 0;
    // uint64_t tt4 = 0;
    // std::vector<uint64> v1, v2, v3, v4;
    // v1.reserve(n);
    // v2.reserve(n);
    // v3.reserve(n);
    // v4.reserve(n);
    uint64_t t0 = r_exec::Now();

    for (double i = 0; i < n; ++i) {
        // tt1 = r_exec::Now();
        Code* object = make_object(mem, rstdin, i);
        uint64_t now = r_exec::Now();
        // v1.push_back(now - tt1);
        // Build a fact.
        // tt2 = now;
        Code *fact = new r_exec::Fact(object, now, now, 1, 1);
        // uint64_t tt = r_exec::Now();
        // v2.push_back(tt - tt2);
        // Build a default view for the fact.
        // tt3 = tt;
        r_exec::View *view = build_view(now, rstdin);
        // tt = r_exec::Now();
        // v3.push_back(tt - tt3);
        // Inject the view.
        // tt4 = tt;
        view->set_object(fact);
        mem->inject(view);
        // v4.push_back(r_exec::Now() - tt4);
    }

    uint64_t t1 = r_exec::Now();
    uint64_t t2 = t1 - t0;
    debug("main") << "for-loop total time: " << t2;
    /* uint64_t acc=0;
    for(uint64_t i=0;i<n;++i){
    acc+=v1[i];
    std::cout<<v1[i]<<'\t';
    }
    std::cout<<"\nfor-loop accumulated time make_object: "<<acc<< std::endl;
    acc=0;
    for(uint64_t i=0;i<n;++i){
    acc+=v2[i];
    // std::cout<<v2[i]<<'/'<<mem->timings_report[i]<<'\t';
    std::cout<<v2[i]<<'\t';
    }
    std::cout<<"\nfor-loop accumulated time new Fact : "<<acc<< std::endl;
    acc=0;
    for(uint64_t i=0;i<n;++i){
    acc+=v3[i];
    std::cout<<v3[i]<<'\t';
    }
    std::cout<<"\nfor-loop accumulated time build_view : "<<acc<< std::endl;
    acc=0;
    for(uint64_t i=0;i<n;++i){
    acc+=v4[i];
    std::cout<<v4[i]<<'\t';
    }
    std::cout<<"\nfor-loop accumulated time mem->inject: "<<acc<< std::endl;
    */
}

void test_many_injections(r_exec::_Mem *mem, uint64_t sampling_period_ms, uint64_t nRuns, double nObjects)
{
    for (; nRuns; --nRuns) {
        uint64_t start = r_exec::Now();
        debug("test many injections") << "number of runs:" << nRuns;
        test_injection(mem, nObjects);
        uint64_t taken_ms = (r_exec::Now() - start) / 1000;

        if (taken_ms > sampling_period_ms) {
            debug("test many injections") << "Good grief! I exceeded the sampling period!";
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(sampling_period_ms - taken_ms));
        }
    }
}

void decompile(Decompiler &decompiler, r_comp::Image *image, uint64_t time_offset, bool ignore_named_objects)
{
#ifdef DECOMPILE_ONE_BY_ONE
    uint64_t object_count = decompiler.decompile_references(image);
    std::cout << object_count << " objects in the image\n";

    while (1) {
        std::cout << "> which object (-1 to exit)?\n";
        int64_t index;
        std::cin >> index;

        if (index == -1) {
            break;
        }

        if (index >= object_count) {
            std::cout << "> there is only " << object_count << " objects\n";
            continue;
        }

        std::ostringstream decompiled_code;
        decompiler.decompile_object(index, &decompiled_code, time_offset);
        std::cout << "\n\n> DECOMPILATION\n\n" << decompiled_code.str() << std::endl;
    }

#else
    std::ostringstream decompiled_code;
    uint64_t object_count = decompiler.decompile(image, &decompiled_code, time_offset, ignore_named_objects);
    //uint64_t object_count=image->code_segment.objects.size();
    debug("main") << "decompilation:\n" << decompiled_code.str();
    //debug("main") << "image taken at:" << Time::ToString_year(image->timestamp);
    debug("main") << object_count << "objects";
#endif
}

void write_to_file(r_comp::Image *image, std::string &image_path, Decompiler *decompiler, uint64_t time_offset)
{
    std::ofstream output(image_path.c_str(), std::ios::binary | std::ios::out);
    r_code::Image<r_code::ImageImpl> *i = image->serialize<r_code::Image<r_code::ImageImpl> >();
    r_code::Image<r_code::ImageImpl>::Write(i, output);
    output.close();
    delete i;

    if (!decompiler) {
        return;
    }

    std::ifstream input(image_path.c_str(), std::ios::binary | std::ios::in);

    if (!input.good()) {
        return;
    }

    r_code::Image<r_code::ImageImpl> *img = (r_code::Image<r_code::ImageImpl> *)r_code::Image<r_code::ImageImpl>::Read(input);
    input.close();
    r_code::vector<Code *> objects;
    r_comp::Image *_i = new r_comp::Image();
    _i->load(img);
    decompile(*decompiler, _i, time_offset, false);
    delete _i;
    delete img;
}

int main(int argc, char **argv)
{
    Settings settings;

    // First try to load local config, otherwise user config, otherwise just
    // use default values
    if (fileExists("settings.ini")) {
        settings.load("settings.ini");
    } else {
        std::cout << "loading from home" << std::endl;
        settings.load("~/.config/replicode/replicode.conf");
    }

    r_comp::Image seed;
    r_comp::Metadata metadata;
    debug("main") << "Initializing with user operator library and user class code...";
    using namespace std::chrono;

    if (!r_exec::Init(settings.usr_operator_path.c_str(),
                      []() -> uint64_t {
                         return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
                      },
                      settings.usr_class_path.c_str(),
                      &seed,
                      &metadata)) {
        return 2;
    }
    srand(r_exec::Now());
    debug("main") << "compiling source...";
    std::string error;

    if (!r_exec::Compile(settings.source_file_name.c_str(), error, &seed, &metadata, false)) {
        std::cerr << " <- " << error << std::endl;
        return 3;
    }

    debug("main") << "source compiled!";
#if defined(WIN32) || defined(WIN64)
    r_exec::PipeOStream::Open(settings.debug_windows);
#endif
    Decompiler decompiler;
    decompiler.init(&metadata);
    r_exec::_Mem *mem;

    if (settings.get_objects) {
        mem = new r_exec::Mem<r_exec::LObject, r_exec::MemStatic>();
    } else {
        mem = new r_exec::Mem<r_exec::LObject, r_exec::MemVolatile>();
    }

    r_code::vector<r_code::Code *> ram_objects;
    seed.get_objects(mem, ram_objects);
    mem->metadata = &metadata;
    mem->init(settings.base_period,
              settings.reduction_core_count,
              settings.time_core_count,
              settings.mdl_inertia_sr_thr,
              settings.mdl_inertia_cnt_thr,
              settings.tpx_dsr_thr,
              settings.min_sim_time_horizon,
              settings.max_sim_time_horizon,
              settings.sim_time_horizon,
              settings.tpx_time_horizon,
              settings.perf_sampling_period,
              settings.float_tolerance,
              settings.time_tolerance,
              settings.primary_thz,
              settings.secondary_thz,
              settings.debug,
              settings.ntf_mk_resilience,
              settings.goal_pred_success_resilience,
              settings.probe_level,
              settings.trace_levels);
    uint64_t stdin_oid;
    std::string stdin_symbol("stdin");
    uint64_t stdout_oid;
    std::string stdout_symbol("stdout");
    uint64_t self_oid;
    std::string self_symbol("self");
    std::unordered_map<uintptr_t, std::string>::const_iterator n;

    for (const auto symbol : seed.object_names.symbols) {
        if (symbol.second == stdin_symbol) {
            stdin_oid = symbol.first;
        } else if (symbol.second == stdout_symbol) {
            stdout_oid = symbol.first;
        } else if (symbol.second == self_symbol) {
            self_oid = symbol.first;
        }
    }

    if (!mem->load(ram_objects.as_std(), stdin_oid, stdout_oid, self_oid)) {
        return 4;
    }

    uint64_t starting_time = mem->start();
    debug("main") << "running for" << settings.run_time << "ms";
    std::this_thread::sleep_for(std::chrono::milliseconds(settings.run_time));
    /*Thread::Sleep(settings.run_time/2);
      test_many_injections(mem,
      argc > 2 ? atoi(argv[2]) : 100, // sampling period in ms
      argc > 3 ? atoi(argv[3]) : 600, // number of batches
      argc > 4 ? atoi(argv[4]) : 66); // number of objects per batch
      Thread::Sleep(settings.run_time/2);*/
    debug("main") << "shutting rMem down...";
    mem->stop();

    if (settings.get_objects) {
        //TimeProbe probe;
        //probe.set();
        r_comp::Image *image = mem->get_objects();
        //probe.check();
        image->object_names.symbols = image->object_names.symbols;

        if (settings.write_objects) {
            write_to_file(image, settings.objects_path, settings.test_objects ? &decompiler : nullptr, starting_time);
        }

        if (settings.decompile_objects && (!settings.write_objects || !settings.test_objects)) {
            if (settings.decompile_to_file) { // argv[2] is a file to redirect the decompiled code to.
                std::ofstream outfile;
                outfile.open(settings.decompilation_file_path.c_str(), std::ios_base::trunc);
                std::streambuf *coutbuf = std::cout.rdbuf(outfile.rdbuf());
                decompile(decompiler, image, starting_time, settings.ignore_named_objects);
                std::cout.rdbuf(coutbuf);
                outfile.close();
            } else {
                decompile(decompiler, image, starting_time, settings.ignore_named_objects);
            }
        }

        delete image;
        //std::cout<<"get_image(): "<<probe.us()<<"us"<<std::endl;
    }

    if (settings.get_models) {
        //TimeProbe probe;
        //probe.set();
        r_comp::Image *image = mem->get_models();
        //probe.check();
        image->object_names.symbols = image->object_names.symbols;

        if (settings.write_models) {
            write_to_file(image, settings.models_path, settings.test_models ? &decompiler : nullptr, starting_time);
        }

        if (settings.decompile_models && (!settings.write_models || !settings.test_models)) {
            if (argc > 2) { // argv[2] is a file to redirect the decompiled code to.
                std::ofstream outfile;
                outfile.open(argv[2], std::ios_base::trunc);
                std::streambuf *coutbuf = std::cout.rdbuf(outfile.rdbuf());
                decompile(decompiler, image, starting_time, settings.ignore_named_models);
                std::cout.rdbuf(coutbuf);
                outfile.close();
            } else {
                decompile(decompiler, image, starting_time, settings.ignore_named_models);
            }
        }

        delete image;
        //std::cout<<"get_models(): "<<probe.us()<<"us"<<std::endl;
    }

    delete mem;
#if defined(WIN32) || defined(WIN64)
    r_exec::PipeOStream::Close();
#endif
    return 0;
}
