//	init.cpp
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

#include "init.h"

#include <r_code/atom.h>                // for Atom
#include <r_code/object.h>              // for Code
#include <r_comp/class.h>               // for Class
#include <r_comp/compiler.h>            // for Compiler
#include <r_comp/decompiler.h>          // for Decompiler
#include <r_comp/preprocessor.h>        // for Preprocessor
#include <r_comp/replistruct.h>         // for RepliStruct
#include <r_comp/segments.h>            // for Metadata, Image
#include <r_exec/callbacks.h>           // for Callbacks
#include <r_exec/cpp_programs.h>        // for CPPPrograms
#include <r_exec/init.h>                // for TDecompiler, etc
#include <r_exec/opcodes.h>             // for Opcodes, Opcodes::Add, etc
#include <r_exec/operator.h>            // for Operator, add, dis, div, e10, etc
#include <r_exec/view.h>                // for View, View::ViewOpcode
#include <stdlib.h>                     // for exit
#include <string.h>                     // for memset
#include <cstdint>                      // for uint16_t, uint64_t, uint8_t
#include <iostream>                     // for operator<<, basic_ostream, etc
#include <unordered_map>                // for unordered_map, etc
#include <utility>                      // for pair, move
#include <sstream>                      // for ostringstream

#include "CoreLibrary/debug.h"          // for debug, DebugStream
#include "CoreLibrary/sharedlibrary.h"  // for SharedLibrary

namespace r_exec {
class Controller;
}  // namespace r_exec

namespace r_exec
{

dll_export uint64_t(*Now)();

////////////////////////////////////////////////////////////////////////////////////////////////

bool Compile(const char* filename,
             std::string &error,
             r_comp::Image *image,
             r_comp::Metadata *metadata,
             bool compile_metadata)
{
    if (!image) {
        debug("init") << "null image supplied";
        return false;
    }

    if (!metadata) {
        debug("init") << "null metadata supplied";
        return false;
    }

    debug("init") << "compiling file: " << filename;
    r_comp::Preprocessor preprocessor;
    r_comp::RepliStruct *root = preprocessor.process(filename, error, compile_metadata ? metadata : nullptr);

    if (!root) {
        error.insert(0, "Preprocessor: " + preprocessor.root->fileName + ":" + std::to_string(preprocessor.root->line) + " ");
        return false;
    }

    r_comp::Compiler compiler(image, metadata);

    if (!compiler.compile(root, false)) {
        std::cerr << "! Compilation failed: " << compiler.getError() << std::endl;
        error = compiler.getError();
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////

void TDecompiler::decompile()
{
    this->spawned = 1;
    r_comp::Decompiler decompiler;
    decompiler.init(this->metadata);
    r_comp::Image *image = new r_comp::Image();
    image->add_objects_full(this->objects);
    //image->object_names.symbols = getSeed()->object_names.symbols;
    std::ostringstream decompiled_code;
    //decompiler.decompile(image, &decompiled_code, Utils::GetTimeReference(), imported_objects);
#if defined(WIN32) || defined(WIN64)
    PipeOStream::Get(this->ostream_id - 1) << this->header.c_str();
    PipeOStream::Get(this->ostream_id - 1) << decompiled_code.str().c_str();
#else
    debug("tdecompiler") << this->header;
    debug("tdecompiler") << decompiled_code.str();
#endif//win
}

TDecompiler::TDecompiler(uint64_t ostream_id, std::string header, r_comp::Metadata *m)
    : _Object(),
      ostream_id(ostream_id),
      header(std::move(header)),
      _thread(nullptr),
      spawned(0),
      metadata(m)
{
    objects.reserve(ObjectsInitialSize);
}

TDecompiler::~TDecompiler()
{
    if (_thread) {
        _thread->join();
        delete _thread;
    }
}

void TDecompiler::add_object(Code *object)
{
    objects.push_back(object);
}

void TDecompiler::add_objects(const r_code::list<P<Code> > &objects)
{
    r_code::list<P<Code> >::const_iterator o;

    for (o = objects.begin(); o != objects.end(); ++o) {
        this->objects.push_back(*o);
    }
}

void TDecompiler::add_objects(const std::vector<P<Code> > &objects)
{
    std::vector<P<Code> >::const_iterator o;

    for (o = objects.begin(); o != objects.end(); ++o) {
        this->objects.push_back(*o);
    }
}

void TDecompiler::runDecompiler()
{
    if (!metadata) {
        debug("tdecompiler") << "unable to run without metadata";
        return;
    }

    _thread = new std::thread(&r_exec::TDecompiler::decompile, this);
}

////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(WIN32) || defined(WIN64)
#pragma warning "TODO http://www.erack.de/download/pipe-fork.c"
std::vector<PipeOStream *> PipeOStream::Streams;

PipeOStream PipeOStream::NullStream;

void PipeOStream::Open(uint8_t count)
{
    for (uint8_t i = 0; i < count; ++i) {
        PipeOStream *p = new PipeOStream();
        p->init();
        Streams.push_back(p);
    }
}

void PipeOStream::Close()
{
    for (uint8_t i = 0; i < Streams.size(); ++i) {
        delete Streams[i];
    }

    Streams.clear();
}

PipeOStream &PipeOStream::Get(uint8_t id)
{
    if (id < Streams.size()) {
        return *Streams[id];
    }

    return NullStream;
}

PipeOStream::PipeOStream(): std::ostream(NULL)   /*,pipe_read(0),pipe_write(0)*/
{
}

void PipeOStream::init()
{
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(saAttr);
    saAttr.bInheritHandle = true;
    saAttr.lpSecurityDescriptor = NULL;
    CreatePipe(&pipe_read, &pipe_write, &saAttr, 0);
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    char handle[255];
    itoa((int)pipe_read, handle, 10);
    std::string command("output_window.exe ");
    command += std::string(handle);
    CreateProcess(NULL, // no module name (use command line)
                  const_cast<char *>(command.c_str()), // command line
                  NULL, // process handle not inheritable
                  NULL, // thread handle not inheritable
                  true, // handle inheritance
                  CREATE_NEW_CONSOLE, // creation flags
                  NULL, // use parent's environment block
                  NULL, // use parent's starting directory
                  &si, // pointer to STARTUPINFO structure
                  &pi); // pointer to PROCESS_INFORMATION structure
}

PipeOStream::~PipeOStream()
{
    if (pipe_read == 0) {
        return;
    }

    std::string stop("close_output_window");
    *this << stop;
    CloseHandle(pipe_read);
    CloseHandle(pipe_write);
}

PipeOStream &PipeOStream::operator <<(std::string &s)
{
    if (pipe_read == 0) {
        return *this;
    }

    uint64_t to_write = s.length();
    uint64_t written;
    WriteFile(pipe_write, s.c_str(), to_write, &written, NULL);
    return *this;
}

PipeOStream& PipeOStream::operator <<(const char *s)
{
    if (pipe_read == 0) {
        return *this;
    }

    uint64_t to_write = strlen(s);
    uint64_t written;
    WriteFile(pipe_write, s, to_write, &written, NULL);
    return *this;
}
#endif//win

////////////////////////////////////////////////////////////////////////////////////////////////

bool Init(const char *user_operator_library_path,
          uint64_t(*time_base)(),
          r_comp::Metadata *metadata)
{
    Now = time_base;
    std::unordered_map<std::string, r_comp::Class>::iterator it;
    std::unordered_map<std::string, uint16_t> &opcodes = metadata->opcodes;

    for (it = metadata->classes.begin(); it != metadata->classes.end(); ++it) {
        opcodes[it->first] = it->second.atom.asOpcode();
        //std::cout<<it->first<<":"<<it->second.atom.asOpcode()<<std::endl;
    }

    for (it = metadata->sys_classes.begin(); it != metadata->sys_classes.end(); ++it) {
        opcodes[it->first] = it->second.atom.asOpcode();
        //std::cout<<it->first<<":"<<it->second.atom.asOpcode()<<std::endl;
    }

    // load class Opcodes.
    View::ViewOpcode = opcodes["view"];
    Opcodes::View    = opcodes["view"];
    Opcodes::PgmView = opcodes["pgm_view"];
    Opcodes::GrpView = opcodes["grp_view"];
    Opcodes::Ent     = opcodes["ent"];
    Opcodes::Ont     = opcodes["ont"];
    Opcodes::MkVal   = opcodes["mk.val"];
    Opcodes::Grp = opcodes["grp"];
    Opcodes::Ptn = opcodes["ptn"];
    Opcodes::AntiPtn = opcodes["|ptn"];
    Opcodes::IPgm = opcodes["ipgm"];
    Opcodes::ICppPgm = opcodes["icpp_pgm"];
    Opcodes::Pgm = opcodes["pgm"];
    Opcodes::AntiPgm = opcodes["|pgm"];
    Opcodes::ICmd = opcodes["icmd"];
    Opcodes::Cmd = opcodes["cmd"];
    Opcodes::Fact = opcodes["fact"];
    Opcodes::AntiFact = opcodes["|fact"];
    Opcodes::Cst = opcodes["cst"];
    Opcodes::Mdl = opcodes["mdl"];
    Opcodes::ICst = opcodes["icst"];
    Opcodes::IMdl = opcodes["imdl"];
    Opcodes::Pred = opcodes["pred"];
    Opcodes::Goal = opcodes["goal"];
    Opcodes::Success = opcodes["success"];
    Opcodes::MkGrpPair = opcodes["mk.grp_pair"];
    Opcodes::MkRdx = opcodes["mk.rdx"];
    Opcodes::Perf = opcodes["perf"];
    Opcodes::MkNew = opcodes["mk.new"];
    Opcodes::MkLowRes = opcodes["mk.low_res"];
    Opcodes::MkLowSln = opcodes["mk.low_sln"];
    Opcodes::MkHighSln = opcodes["mk.high_sln"];
    Opcodes::MkLowAct = opcodes["mk.low_act"];
    Opcodes::MkHighAct = opcodes["mk.high_act"];
    Opcodes::MkSlnChg = opcodes["mk.sln_chg"];
    Opcodes::MkActChg = opcodes["mk.act_chg"];
    // load executive function Opcodes.
    Opcodes::Inject = opcodes["_inj"];
    Opcodes::Eject = opcodes["_eje"];
    Opcodes::Mod = opcodes["_mod"];
    Opcodes::Set = opcodes["_set"];
    Opcodes::NewClass = opcodes["_new_class"];
    Opcodes::DelClass = opcodes["_del_class"];
    Opcodes::LDC = opcodes["_ldc"];
    Opcodes::Swap = opcodes["_swp"];
    Opcodes::Prb = opcodes["_prb"];
    Opcodes::Stop = opcodes["_stop"];
    Opcodes::Add = opcodes["add"];
    Opcodes::Sub = opcodes["sub"];
    Opcodes::Mul = opcodes["mul"];
    Opcodes::Div = opcodes["div"];
    // load std operators.
    uint16_t operator_opcode = 0;
    Operator::Register(operator_opcode++, now);
    Operator::Register(operator_opcode++, rnd);
    Operator::Register(operator_opcode++, equ);
    Operator::Register(operator_opcode++, neq);
    Operator::Register(operator_opcode++, gtr);
    Operator::Register(operator_opcode++, lsr);
    Operator::Register(operator_opcode++, gte);
    Operator::Register(operator_opcode++, lse);
    Operator::Register(operator_opcode++, add);
    Operator::Register(operator_opcode++, sub);
    Operator::Register(operator_opcode++, mul);
    Operator::Register(operator_opcode++, div);
    Operator::Register(operator_opcode++, dis);
    Operator::Register(operator_opcode++, ln);
    Operator::Register(operator_opcode++, exp);
    Operator::Register(operator_opcode++, log);
    Operator::Register(operator_opcode++, e10);
    Operator::Register(operator_opcode++, syn);
    Operator::Register(operator_opcode++, ins);
    Operator::Register(operator_opcode++, red);
    Operator::Register(operator_opcode++, fvw);

    if (!user_operator_library_path) { // when no rMem is used.
        return true;
    }

    // load usr operators and c++ programs.
    if (!(metadata->user_operator_library.load(user_operator_library_path))) {
        return false;
    }

    // Operators.
    typedef uint16_t(*OpcodeRetriever)(const char *);
    typedef void (*UserInit)(r_comp::Metadata * metadata);
    UserInit _Init = metadata->user_operator_library.getFunction<UserInit>("Init");

    if (!_Init) {
        return false;
    }

    typedef uint16_t(*UserGetOperatorCount)();
    UserGetOperatorCount GetOperatorCount = metadata->user_operator_library.getFunction<UserGetOperatorCount>("GetOperatorCount");

    if (!GetOperatorCount) {
        return false;
    }

    typedef void (*UserGetOperatorName)(char *op_name, int op_index);
    UserGetOperatorName GetOperatorName = metadata->user_operator_library.getFunction<UserGetOperatorName>("GetOperatorName");

    if (!GetOperatorName) {
        return false;
    }

    _Init(metadata);
    typedef bool (*UserOperator)(const Context &, uint16_t &);
    uint16_t operatorCount = GetOperatorCount();

    for (uint16_t i = 0; i < operatorCount; ++i) {
        char op_name[256] = { 0 };
        GetOperatorName(op_name, i);
        std::unordered_map<std::string, uint16_t>::iterator it = opcodes.find(op_name);

        if (it == opcodes.end()) {
            std::cerr << "Operator " << op_name << " is undefined" << std::endl;
            exit(-1);
        }

        UserOperator op = metadata->user_operator_library.getFunction<UserOperator>(op_name);

        if (!op) {
            return false;
        }

        Operator::Register(it->second, op);
    }

    // C++ programs.
    typedef uint16_t(*UserGetProgramCount)();
    UserGetProgramCount GetProgramCount = metadata->user_operator_library.getFunction<UserGetProgramCount>("GetProgramCount");

    if (!GetProgramCount) {
        return false;
    }

    typedef void (*UserGetProgramName)(char *);
    UserGetProgramName GetProgramName = metadata->user_operator_library.getFunction<UserGetProgramName>("GetProgramName");

    if (!GetProgramName) {
        return false;
    }

    typedef Controller *(*UserProgram)(r_code::View *);
    uint16_t programCount = GetProgramCount();

    for (uint16_t i = 0; i < programCount; ++i) {
        char pgm_name[256];
        memset(pgm_name, 0, 256);
        GetProgramName(pgm_name);
        std::string _pgm_name = pgm_name;
        UserProgram pgm = metadata->user_operator_library.getFunction<UserProgram>(pgm_name);

        if (!pgm) {
            return false;
        }

        CPPPrograms::Register(_pgm_name, pgm);
    }

    // Callbacks.
    typedef uint16_t(*UserGetCallbackCount)();
    UserGetCallbackCount GetCallbackCount = metadata->user_operator_library.getFunction<UserGetCallbackCount>("GetCallbackCount");

    if (!GetCallbackCount) {
        return false;
    }

    typedef void (*UserGetCallbackName)(char *);
    UserGetCallbackName GetCallbackName = metadata->user_operator_library.getFunction<UserGetCallbackName>("GetCallbackName");

    if (!GetCallbackName) {
        return false;
    }

    typedef bool (*UserCallback)(uint64_t, bool, const char *, uint8_t, Code **);
    uint16_t callbackCount = GetCallbackCount();

    for (uint16_t i = 0; i < callbackCount; ++i) {
        char callback_name[256];
        memset(callback_name, 0, 256);
        GetCallbackName(callback_name);
        std::string _callback_name = callback_name;
        UserCallback callback = metadata->user_operator_library.getFunction<UserCallback>(callback_name);

        if (!callback) {
            return false;
        }

        Callbacks::Register(_callback_name, callback);
    }

    debug("init") << "user-defined operator library" << user_operator_library_path << "loaded";
    return true;
}

bool Init(const char *user_operator_library_path,
          uint64_t(*time_base)(),
          const char *seed_path,
          r_comp::Image *image,
          r_comp::Metadata *metadata)
{
    std::string error;

    if (!Compile(seed_path, error, image, metadata, true)) {
        std::cerr << error << std::endl;
        return false;
    }

    return Init(user_operator_library_path, time_base, metadata);
}

}
