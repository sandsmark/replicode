//	settings.h
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

#ifndef settings_h
#define settings_h

#include <CoreLibrary/inifile.h>
#include <CoreLibrary/debug.h>
#include <thread>

class Settings {
public:
// Load.
    std::string usr_operator_path;
    std::string usr_class_path;
    std::string source_file_name;

// Init.
    uint64_t base_period;
    uint64_t reduction_core_count;
    uint64_t time_core_count;

// System.
    double mdl_inertia_sr_thr;
    uint64_t mdl_inertia_cnt_thr;
    double tpx_dsr_thr;
    uint64_t min_sim_time_horizon;
    uint64_t max_sim_time_horizon;
    double sim_time_horizon;
    uint64_t tpx_time_horizon;
    uint64_t perf_sampling_period;
    double float_tolerance;
    uint64_t time_tolerance;
    uint64_t primary_thz;
    uint64_t secondary_thz;

// Debug.
    bool debug;
    uint64_t ntf_mk_resilience;
    uint64_t goal_pred_success_resilience;
    uint64_t debug_windows;
    uint64_t trace_levels;
    bool get_objects;
    bool decompile_objects;
    bool decompile_to_file;
    std::string decompilation_file_path;
    bool ignore_named_objects;
    bool write_objects;
    std::string objects_path;
    bool test_objects;

//Run.
    uint64_t run_time;
    uint64_t probe_level;
    bool get_models;
    bool decompile_models;
    bool ignore_named_models;
    bool write_models;
    std::string models_path;
    bool test_models;

    bool load(const char *file_name) {
        IniFile settingsFile;
        if (!settingsFile.readFile(file_name)) {
            ::debug("settings") << "Unable to parse settings, using defaults";
        }
        ::debug("settings") << "Loading settings file" << file_name;

        usr_operator_path = settingsFile.getString("Load", "User Operator Module Path", "../build/usr_operators/libusr_operators.so");
        usr_class_path = settingsFile.getString("Load", "User Class File Path", "./V1.2/user.classes.replicode");
        source_file_name = settingsFile.getString("Load", "Source File Path",  "./V1.2/test.2.replicode");

        unsigned int cores = std::thread::hardware_concurrency();
        ::debug("settings") << "number of cores available:" << cores;

        base_period = settingsFile.getInt("Init", "Base Period", 50000);
        reduction_core_count = settingsFile.getInt("Init", "Reduction Core Count", (cores * 3) / 4 + 1);
        time_core_count = settingsFile.getInt("Init", "Time Core Count", (cores - ((cores*3)/4)) + 1);

        mdl_inertia_sr_thr = settingsFile.getDouble("System", "Model Inertia Success Rate Threshold", 0.9);
        mdl_inertia_cnt_thr = settingsFile.getInt("System", "Model Inertia Count Threshold", 6);
        tpx_dsr_thr = settingsFile.getDouble("System", "Targeted Pattern Extractor Delta Success Rate Threshold", 0.1);
        min_sim_time_horizon = settingsFile.getInt("System", "Minimum Simulation Time Horizon", 0);
        max_sim_time_horizon = settingsFile.getInt("System", "Maximum Simulation Time Horizon", 0);
        sim_time_horizon = settingsFile.getDouble("System", "Simulation Time Horizon", 0.3);
        tpx_time_horizon = settingsFile.getInt("System", "Targeted Pattern Extractor Time Horizon", 500000);
        perf_sampling_period = settingsFile.getInt("System", "Perf Sampling Period", 250000);
        float_tolerance = settingsFile.getDouble("System", "Float Tolerance", 0.00001);
        time_tolerance = settingsFile.getInt("System", "Timer Tolerance", 10000);
        primary_thz = settingsFile.getInt("System", "Primary Time Horizon", 3600000);
        secondary_thz = settingsFile.getInt("System", "Secondary Time Horizon", 7200000);

        this->debug = settingsFile.getBool("Debug", "Debug", true);
        debug_windows = settingsFile.getInt("Debug", "Debug Windows", 1);
        trace_levels = std::stoi(settingsFile.getString("Debug", "Trace Levels", "CC"), nullptr, 16);

        ntf_mk_resilience = settingsFile.getInt("Resilience",  "Notification Marker Resilience", 1);
        goal_pred_success_resilience = settingsFile.getInt("Resilience", "Goal Prediction Success Resilience", 1000);

        get_objects = settingsFile.getBool("Objects", "Get Objects", false);
        decompile_objects = settingsFile.getBool("Objects", "Decompile Objects", false);
        decompile_to_file = settingsFile.getBool("Objects", "Decompile To File", false);
        decompilation_file_path = settingsFile.getString("Objects", "Decompilation Files Paths", "../Test/decompiled.replicode");
        ignore_named_objects = settingsFile.getBool("Objects", "Ignore Named Objects", false);
        write_objects = settingsFile.getBool("Objects", "write_objects", false);
        objects_path = settingsFile.getString("Objects", "Objects Path", "../decompiled-objects.replicode");
        test_objects = settingsFile.getBool("Objects", "Test Objects", false);

        run_time = settingsFile.getInt("Run", "Run Time", 10080);
        probe_level = settingsFile.getInt("Run", "Probe Level", 2);

        get_models = settingsFile.getBool("Models", "Get Models", false);
        decompile_models = settingsFile.getBool("Models", "Decompile Models", false);
        ignore_named_models = settingsFile.getBool("Models", "Ignore Named Models", false);
        write_models = settingsFile.getBool("Models", "Write Models", false);
        models_path = settingsFile.getString("Models", "Models Path", "../decompiled-models.replicode");
        test_models = settingsFile.getBool("Models", "Test Models", false);

        return true;
    }
};


#endif
