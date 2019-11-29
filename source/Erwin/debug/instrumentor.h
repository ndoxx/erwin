#pragma once

// Adapted from https://gist.github.com/TheCherno/31f135eea6ee729ab5f26a6908eb3a5e
// Basic instrumentation profiler by Cherno

#include <string>
#include <chrono>

namespace erwin
{

struct ProfileResult
{
    std::string name;
    long long start;
    long long end;
    uint32_t thread_id;
};

struct InstrumentationSession
{
    std::string name;
    bool enabled;
};

class Instrumentor
{
public:
    static void begin_session(const std::string& name, const std::string& filepath);
    static void set_session_enabled(bool value);
    static void end_session();
    static void write_profile(const ProfileResult& result);
    static void write_header();
    static void write_footer();
};

class InstrumentationTimer
{
public:
    InstrumentationTimer(const char* name);
    ~InstrumentationTimer();

    void stop();

private:
    const char* name_;
    std::chrono::time_point<std::chrono::high_resolution_clock> start_timepoint_;
    bool stopped_;
};

} // namespace erwin