//	init.h
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

#ifndef init_h
#define init_h

#include <r_code/list.h>       // for list
#include <stdint.h>            // for uint64_t
#include <string>              // for string
#include <thread>              // for thread
#include <vector>              // for vector

#include "CoreLibrary/base.h"  // for P, _Object
#include "CoreLibrary/dll.h"   // for dll_export

namespace r_code {
class Code;
}  // namespace r_code
namespace r_comp {
class Image;
class Metadata;
}  // namespace r_comp

namespace r_exec
{

// Time base; either Time::Get or network-aware synced time.
extern dll_export uint64_t(*Now)();

// Loaded once for all.
// Results from the compilation of user.classes.replicode.
// The latter contains all class definitions and all shared objects (e.g. ontology); does not contain any dynamic (res!=forever) objects.
//extern dll_export r_comp::Metadata Metadata;
//extern dll_export r_comp::Image Seed;
//dll_export r_comp::Metadata *getMetadata();
//dll_export r_comp::Image *getSeed();

// Both functions add the compiled object to Seed.code_image.
// Source files: use ANSI encoding (not Unicode).
bool Compile(const char* filename, std::string& error, r_comp::Image *image, r_comp::Metadata *metadata, bool compile_metadata = false);

// Threaded decompiler, for decompiling on the fly asynchronously.
// Named objects are referenced but not decompiled.
class dll_export TDecompiler:
    public core::_Object
{
private:
    static const uint64_t ObjectsInitialSize = 16;
    void decompile();


    uint64_t ostream_id; // 0 is std::cout.
    std::string header;
    r_code::list<core::P<r_code::Code> > objects;
    std::thread *_thread;
    volatile uint64_t spawned;
    r_comp::Metadata *metadata;

public:
    TDecompiler(uint64_t ostream_id, std::string header, r_comp::Metadata *metadata);
    ~TDecompiler();

    void add_object(r_code::Code *object);
    void add_objects(const r_code::list<core::P<r_code::Code> > &objects);
    void add_objects(const std::vector<core::P<r_code::Code> > &objects);
    void runDecompiler();
};

// Spawns an instance of output_window.exe (see output_window project) and opens a pipe between the main process and output_window.
// Temporary solution:
// (a) not portable,
// (b) shall be defined in CoreLibrary instead of here,
// (c) the stream pool management (PipeOStream::Open(), PipeOStream::Close() and PipeOStream::Get()) shall be decoupled from this implementation (it's an IDE feature),
// (d) PipeOStream shall be based on std::ostringstream instead of std::ostream with a refined std::stringbuf (override sync() to write in the pipe).
#if defined(WIN32) || defined(WIN64)
class dll_export PipeOStream:
    public std::ostream
{
private:
    static std::vector<PipeOStream *> Streams;
    static PipeOStream NullStream;

    int m_pipeRead[2];
    int m_pipeWrite[2];
    //HANDLE pipe_read;
    //HANDLE pipe_write;

    void init(); // create one child process and a pipe.
    PipeOStream();
public:
    static void Open(uint8_t count); // open count streams.
    static void Close(); // close all streams.
    static PipeOStream &Get(uint8_t id); // return NullStream if id is out of range.

    ~PipeOStream();

    PipeOStream &operator <<(std::string &s);
    PipeOStream &operator <<(const char *s);
};
#endif

// Initialize Now, compile user.classes.replicode, builds the Seed and loads the user-defined operators.
// Return false in case of a problem (e.g. file not found, operator not found, etc.).
bool dll_export Init(const char *user_operator_library_path,
                     uint64_t(*time_base)(),
                     const char *seed_path,
                     r_comp::Image *seed,
                     r_comp::Metadata *metadata);
}


#endif
