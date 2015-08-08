#include <r_exec/init.h>
#include <r_comp/preprocessor.h>
#include <r_comp/compiler.h>
#include <CoreLibrary/debug.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

int main()
{
    std::vector<std::string> testsources = {
//        "drives.replicode",
        "hello.world.replicode"
/*        "pong.2.replicode",
        "pong.replicode",
        "test.1.replicode",
        "test.2.replicode",
        "test.3.replicode",
        "test.4.replicode",
        "test.domain.replicode",
        "user.classes.replicode"*/
    };
    const std::string usr_operator_path("../../build/usr_operators/libusr_operators.so");
    const std::string usr_classes_path("user.classes.replicode");
    for (std::string testsource : testsources) {
        std::ifstream tracefile((testsource + ".trace").c_str(), std::ios::binary);
        if (!tracefile.good()) {
            debug(testsource) << "Unable to open .trace file";
            continue;
        }

        std::string correct_trace( (std::istreambuf_iterator<char>(tracefile)),
                                   (std::istreambuf_iterator<char>()));
        if (correct_trace.length() == 0) {
            debug(testsource) << ".trace file is empty";
            continue;
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
            return 1;
        }
        debug(testsource) << "initialized";

        r_comp::Preprocessor preprocessor;
        r_comp::Compiler compiler;
        std::string error;

        r_comp::RepliStruct *root = preprocessor.process(testsource.c_str(), error);
        if (error.length() != 0) {
            debug(testsource) << "Error from preprocessor:" << error;
            continue;
        }

        if (!root) {
            debug(testsource) << "Preprocessor returned null-root, without error!";
            continue;
        }

        // Redirect cout (the trace)
        streambuf *old_cout = std::cout.rdbuf();
        std::ostringstream result_stream;
        std::cout.rdbuf(result_stream.rdbuf());

        bool compile_success = compiler.compile(root, &image, &metadata, error, true);

        // Restore cout
        std::cout.rdbuf(old_cout);

        if (!compile_success) {
            debug(testsource) << "Compile failed:" << compiler.getError();
            debug(testsource) << "Standard out from compiler:" << result_stream.str();
            continue;
        }

        if (result_stream.str() != correct_trace) {
            debug(testsource) << "Trace does not match expected trace";
        }
    }
    return 0;
}
