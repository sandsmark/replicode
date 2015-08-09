#include <r_exec/init.h>
#include <r_comp/preprocessor.h>
#include <r_comp/compiler.h>
#include <CoreLibrary/debug.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        debug("compiler test") << "missing source file argument";
        return -1;
    }

    const std::string usr_operator_path("../../build/usr_operators/libusr_operators.so");
    const std::string usr_classes_path("user.classes.replicode");

    std::string testsource = argv[1];
    std::ifstream tracefile((testsource + ".trace").c_str(), std::ios::binary);
    if (!tracefile.good()) {
        debug(testsource) << "Unable to open .trace file";
        return 1;
    }

    std::string correct_trace( (std::istreambuf_iterator<char>(tracefile)),
                               (std::istreambuf_iterator<char>()));
    if (correct_trace.length() == 0) {
        debug(testsource) << ".trace file is empty";
        return 2;
    }

    r_comp::Image image;
    r_comp::Metadata metadata;

    if (!r_exec::Init(usr_operator_path.c_str(),
                      []() -> uint64_t {
                          return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                      },
                      usr_classes_path.c_str(),
                      &image,
                      &metadata)) {
        debug("compiler test") << "Unable to initialize metadata and image";
        return 3;
    }
    debug(testsource) << "initialized";

    r_comp::Preprocessor preprocessor;
    r_comp::Compiler compiler;
    std::string error;

    r_comp::RepliStruct *root = preprocessor.process(testsource.c_str(), error, &metadata);
    if (error.length() != 0) {
        debug(testsource) << "Error from preprocessor:" << error;
        return 4;
    }

    if (!root) {
        debug(testsource) << "Preprocessor returned null-root, without error!";
        return 5;
    }

    if (!compiler.compile(root, &image, &metadata, error, false)) {
        debug(testsource) << "Compile failed:" << compiler.getError();
        return 6;
    }

    // Redirect cout (the trace)
    streambuf *old_cout = std::cout.rdbuf();
    std::ostringstream result_stream;
    std::cout.rdbuf(result_stream.rdbuf());
    r_code::vector<SysObject*> &objects = image.code_segment.objects;
    for (size_t i=0;i<objects.size(); i++) {
        objects[i]->trace();
    }

    // Restore cout
    std::cout.rdbuf(old_cout);

    if (result_stream.str() != correct_trace) {
        debug(testsource) << "Trace does not match expected trace" << result_stream.str().length() << correct_trace.length();
        std::ofstream outfile((testsource + ".wrong").c_str(), std::ios::trunc);
        outfile << result_stream.str();
        return 7;
    }

    return 0;
}
