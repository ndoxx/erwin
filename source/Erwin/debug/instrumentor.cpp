#include "debug/instrumentor.h"

#include <algorithm>
#include <fstream>
#include <thread>

struct InstrumentorStorage
{
    InstrumentationSession* current_session = nullptr;
    int profile_count = 0;
    std::ofstream out_stream;
};
static InstrumentorStorage storage;

void Instrumentor::begin_session(const std::string& name, const std::string& filepath)
{
    storage.out_stream.open(filepath);
    write_header();
    storage.current_session = new InstrumentationSession{name, true};
}

void Instrumentor::set_session_enabled(bool value) { storage.current_session->enabled = value; }

void Instrumentor::end_session()
{
    write_footer();
    storage.out_stream.close();
    delete storage.current_session;
    storage.current_session = nullptr;
    storage.profile_count = 0;
}

void Instrumentor::write_profile(const ProfileResult& result)
{
    if(storage.profile_count++ > 0)
        storage.out_stream << ",";

    std::string name = result.name;
    std::replace(name.begin(), name.end(), '"', '\'');

    storage.out_stream << "{"
                       << "\"cat\":\"function\","
                       << "\"dur\":" << (result.end - result.start) << ',' << "\"name\":\"" << name << "\","
                       << "\"ph\":\"X\","
                       << "\"pid\":0,"
                       << "\"tid\":" << result.thread_id << ","
                       << "\"ts\":" << result.start << "}";

    storage.out_stream.flush();
}

void Instrumentor::write_header()
{
    storage.out_stream << "{\"otherData\": {},\"traceEvents\":[";
    storage.out_stream.flush();
}

void Instrumentor::write_footer()
{
    storage.out_stream << "]}";
    storage.out_stream.flush();
}

InstrumentationTimer::InstrumentationTimer(const char* name)
    : name_(name), start_timepoint_(std::chrono::high_resolution_clock::now()), stopped_(false)
{}

InstrumentationTimer::~InstrumentationTimer()
{
    if(!stopped_)
        stop();
}

void InstrumentationTimer::stop()
{
    if(storage.current_session == nullptr)
        return;
    if(!storage.current_session->enabled)
        return;

    auto end_timepoint = std::chrono::high_resolution_clock::now();

    long long start =
        std::chrono::time_point_cast<std::chrono::microseconds>(start_timepoint_).time_since_epoch().count();
    long long end = std::chrono::time_point_cast<std::chrono::microseconds>(end_timepoint).time_since_epoch().count();

    size_t thread_id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    Instrumentor::write_profile({name_, start, end, thread_id});

    stopped_ = true;
}
