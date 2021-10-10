#include <replicode_common.h>    // for debug, DebugStream
#include <r_code/object.h>        // for SysObject
#include <r_code/vector.h>        // for vector
#include <r_comp/compiler.h>      // for Compiler
#include <r_comp/decompiler.h>    // for Decompiler
#include <r_comp/preprocessor.h>  // for Preprocessor
#include <r_comp/segments.h>      // for Image, CodeSegment, Metadata
#include <r_exec/init.h>          // for Init
#include <stddef.h>               // for size_t
#include <stdint.h>               // for uint64_t
#include <chrono>                 // for microseconds, duration_cast, etc
#include <iostream>               // for istreambuf_iterator, ostringstream, etc
#include <sstream>
#include <string>                 // for allocator, string, basic_string, etc
#include <type_traits>            // for enable_if<>::type

namespace r_comp {
class RepliStruct;
}  // namespace r_comp

#define USR_OPERATOR_PATH "../../build/usr_operators/libusr_operators.so"
#define USR_CLASSES_PATH  "user.classes.replicode"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        debug("compiler test") << "missing source file argument";
        return -1;
    }

    std::string testfile = argv[1];
    debug("compiler test") << "Testing compiler with file" << testfile;
    std::string tracefilename = testfile + ".trace";
    std::ifstream tracefile(tracefilename.c_str(), std::ios::binary);

    if (!tracefile.good()) {
        debug("compiler test") << "Unable to open .trace file" << tracefilename;
        return 1;
    }

    std::string correct_trace( (std::istreambuf_iterator<char>(tracefile)),
                               (std::istreambuf_iterator<char>()));

    if (correct_trace.length() == 0) {
        debug("compiler test") << ".trace file is empty";
        return 2;
    }

    r_comp::Image image;
    r_comp::Metadata metadata;

    if (!r_exec::Init(USR_OPERATOR_PATH,
    []() -> uint64_t {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    },
    USR_CLASSES_PATH,
    &image,
    &metadata)) {
        debug("compiler test") << "Unable to initialize metadata and image";
        return 3;
    }
    r_comp::Preprocessor preprocessor;
    r_comp::Compiler compiler(&image, &metadata);
    std::string error;
    r_comp::RepliStruct::Ptr root = preprocessor.process(testfile.c_str(), error, &metadata);

    if (error.length() != 0) {
        debug("compiler test") << "Error from preprocessor:" << error;
        return 4;
    }

    if (!root) {
        debug("compiler test") << "Preprocessor returned null-root, without error!";
        return 5;
    }

    if (!compiler.compile(root, false)) {
        debug("compiler test") << "Compile failed:" << compiler.getError();
        return 6;
    }

    // Redirect cout (the trace)
    std::streambuf *old_cout = std::cout.rdbuf();
    std::ostringstream result_stream;
    std::cout.rdbuf(result_stream.rdbuf());
    r_code::vector<SysObject*> &objects = image.code_segment.objects;

    for (SysObject *object : objects) {
        object->trace();
    }

    // Restore cout
    std::cout.rdbuf(old_cout);

    if (result_stream.str() != correct_trace) {
        debug("compiler test") << "Trace does not match expected trace" << result_stream.str().length() << correct_trace.length();
        std::ofstream outfile((testfile + ".trace.wrong").c_str(), std::ios::trunc);
        outfile << result_stream.str();
        r_comp::Decompiler decompiler;
        decompiler.init(&metadata);
        std::ostringstream decompiled_code;
        uint64_t decompiled_object_count = decompiler.decompile(&image, &decompiled_code, 0, false);
        debug("compiler test") << "decompiled objects count:" << decompiled_object_count << "image object count:" << image.code_segment.objects.size();
        std::ofstream decompilefile((testfile + ".decompiled.replicode").c_str(), std::ios::trunc);
        decompilefile << decompiled_code.rdbuf();
        return 7;
    }

    debug("compiler test") << "Test succeeded!";
    return 0;
}
