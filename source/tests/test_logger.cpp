#include "core/clock.hpp"
#include "core/window.h"
#include "event/event.h"
#include "debug/logger.h"
#include "debug/logger_sink.h"
#include "debug/logger_thread.h"

using namespace erwin;

class vec3stub
{
public:
    vec3stub(float x, float y, float z): x(x),y(y),z(z) {}

    friend std::ostream& operator<< (std::ostream& stream, const vec3stub& value);

private:
    float x,y,z;
};

std::ostream& operator<< (std::ostream& stream, const vec3stub& value)
{
    stream << "(" << value.x << "," << value.y << "," << value.z << ")";
    return stream;
}

struct DamageEvent: public WEvent
{
    EVENT_NAME(DamageEvent)

    DamageEvent(char src, char tgt, int val): source(src), target(tgt), value(val) {}
    virtual void print(std::ostream& stream) const override
    {
        stream << source << " hit target " << target << " (" << value << ")";
    }
    
    char source;
    char target;
    int value;
};

struct ItemFoundEvent: public WEvent
{
    EVENT_NAME(ItemFoundEvent)

    ItemFoundEvent(char ent, const std::string& name, int num): ent(ent), name(name), num(num) {}
    virtual void print(std::ostream& stream) const override
    {
        stream << ent << " found item \"" << name << "\" x" << num;
    }
    
    char ent;
    std::string name;
    int num;
};

class Publisher
{
public:
    void emit(char src, char tgt, int value)
    {
        EVENTBUS.publish(DamageEvent(src, tgt, value));
    }
    void emit(char ent, const std::string& item_name, int quantity)
    {
        EVENTBUS.publish(ItemFoundEvent(ent, item_name, quantity));
    }
};

class Subscriber
{
public:
    Subscriber()
    {
        EVENTBUS.subscribe(this, &Subscriber::on_damage_event);
        EVENTBUS.subscribe(this, &Subscriber::on_item_found_event);
    }

    bool on_damage_event(const DamageEvent& event)
    {
        DLOGN("event") << "Subscriber: event received: " << event.source << "->" << event.target << ": " << event.value << std::endl;
        return false;
    }
    bool on_item_found_event(const ItemFoundEvent& event)
    {
        DLOGN("event") << "Subscriber: event received: " << event.ent << ": " << event.name << "(" << event.num << ")" << std::endl;
        return false;
    }
};


void thread_exec()
{
    for(int ii=0; ii<3; ++ii)
        DLOG("core",0) << "hi from thread: " << WCC('i') << std::this_thread::get_id() << std::endl;
}

struct KillEvent: public WEvent
{
    EVENT_NAME(KillEvent)

    KillEvent(char target): target(target) {}

    virtual void print(std::ostream& stream) const override
    {
        stream << "KILL: " << target;
    }

    char target;
};

struct KillerThread
{
    KillerThread(std::vector<char>&& victim_list): victim_list(std::move(victim_list)) {}

    void operator()()
    {
        DLOG("core",0) << WCC('b') << "KillerThread" << WCC(0) << " sending KillEvents." << std::endl;
        for(char victim: victim_list)
            EVENTBUS.publish(KillEvent(victim));
        DLOG("core",0) << WCC('b') << "KillerThread" << WCC(0) << " done." << std::endl;
    }

    std::vector<char> victim_list;
};

struct VictimThread
{
    VictimThread(char name):
    name(name),
    thread_state(false)
    {
        EVENTBUS.subscribe(this, &VictimThread::on_kill_event);
    }

    void operator()()
    {
        DLOG("core",0) << WCC('g') << "VictimThread " << name << WCC(0) << " waiting for KillEvent." << std::endl;
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [this]()
        {
            return thread_state.load(std::memory_order_acquire);
        });
        DLOG("core",0) << WCC('g') << "VictimThread " << name << WCC(0) << " done." << std::endl;
    }

    bool on_kill_event(const KillEvent& event)
    {
        if(event.target == name)
        {
            DLOG("core",0) << WCC('g') << "VictimThread " << name << WCC(0) << " received KillEvent." << std::endl;
            thread_state.store(true, std::memory_order_release);
            cv.notify_one();
        }
        return false;
    }

    char name;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> thread_state;
};

WScope<Window> Window::create(const WindowProps& props)
{
    return nullptr;
}

int main()
{
    WLOGGER.create_channel("TEST", 3);
    WLOGGER.create_channel("material", 2);
    WLOGGER.create_channel("accessibility", 2);
    WLOGGER.create_channel("render", 3);
    WLOGGER.create_channel("collision", 3);
    WLOGGER.attach_all("ConsoleSink", std::make_unique<dbg::ConsoleSink>());
    WLOGGER.attach("MainFileSink", std::make_unique<dbg::LogFileSink>("wcore.log"), {"core"_h, "material"_h, "render"_h, "collision"_h});
    WLOGGER.attach("EventFileSink", std::make_unique<dbg::LogFileSink>("events.log"), {"event"_h});
    WLOGGER.set_backtrace_on_error(true);
    WLOGGER.track_event<DamageEvent>();
    WLOGGER.track_event<ItemFoundEvent>();
    WLOGGER.track_event<KillEvent>();
    WLOGGER.spawn();


    DLOG("TEST",1) << "-------- [FORMATTING & CUSTOM TYPES] --------" << std::endl;
    DLOG("core",1)      << "plop " << 1 << " " << 1.5f << " " << vec3stub(1,2,3) << std::endl;
    DLOG("material",1)  << "Yo, checkout ma vectah: " << vec3stub(4,5,6) << std::endl;
    DLOG("material",0)  << "should display at max verbosity only" << std::endl;
    DLOG("collision",0) << "plop" << std::endl;
    DLOGR("collision")  << "Raw text" << std::endl;
    WLOGGER.flush();


    DLOG("TEST",1) << "-------- [COLORS] --------" << std::endl;
    DLOG("accessibility",1) << "Configuring " << WCC('i') << "accessibility" << WCC(0) << " parameters." << std::endl;
    DLOG("accessibility",1) << "If you are " << WCC('x') << "colorblind" << WCC(0)
                            << " you can't see " << WCC('g') << "this" << WCC(0) << ":" << std::endl;

    for(int ii=0; ii<10; ++ii)
    {
        for(int jj=0; jj<10; ++jj)
            DLOG("accessibility",1) << WCC(25*ii,25*jj,255-25*jj) << char('A'+ii+jj) << " ";
        DLOG("accessibility",1) << std::endl;
    }
    WLOGGER.flush();


    DLOG("TEST",1) << "-------- [SEVERITY & ERROR REPORT] --------" << std::endl;
    BANG();
    DLOGN("render") << "Notify: Trump threatened Iran with \'obliteration\'." << std::endl;
    DLOGI           << "Item 1" << std::endl;
    DLOGI           << "Item 2" << std::endl;
    DLOGI           << "Item 3" << std::endl;
    DLOGW("core") << "Warning: Iran said \"Double dare you!\"" << std::endl;
    DLOGE("core") << "Error 404: Nuclear war heads could not be found." << std::endl;
    DLOGF("core") << "Fatal error: Index out of bounds in array: war_heads." << std::endl;
    WLOGGER.flush();


    DLOG("TEST",1) << "-------- [EVENT TRACKING] --------" << std::endl;
    {
        // Coupling with the event system
        Publisher pub;
        Subscriber sub;
        for(int ii=0; ii<3; ++ii)
            pub.emit('A', 'B', ii);
        pub.emit('B', "Big Fucking Sword",1);
        pub.emit('B', 'A', 9999);
        WLOGGER.flush();
    }

    DLOG("TEST",1) << "-------- [EVENT TRACKING (MULTITHREAD)] --------" << std::endl;
    {
        VictimThread victim_a('A');
        VictimThread victim_b('B');
        KillerThread killer({'A','B'});
        std::thread victim_a_thread(std::ref(victim_a));
        std::thread victim_b_thread(std::ref(victim_b));
        std::thread killer_thread(std::ref(killer));
        if(victim_a_thread.joinable())
            victim_a_thread.join();
        if(victim_b_thread.joinable())
            victim_b_thread.join();
        if(killer_thread.joinable())
            killer_thread.join();
    }

    DLOG("TEST",1) << "-------- [SYNC ACCESS] --------" << std::endl;
    nanoClock frame_clock;
    nanoClock main_clock;
    frame_clock.restart();
    main_clock.restart();

    float target_fps = 60.f;
    const std::chrono::nanoseconds frame_duration_ns_(uint32_t(1e9*1.0f/target_fps));

    bool running = true;
    while(running)
    {
        auto run_d = main_clock.get_elapsed_time();
        float t_tot = std::chrono::duration_cast<std::chrono::duration<float>>(run_d).count();
        if(t_tot>0.018f)
            running = false;

        std::vector<std::thread> workers;
        for(int ii=0; ii<4; ++ii)
        {
            workers.push_back(std::thread(thread_exec));
        }

        for(std::thread& th: workers)
        {
            if(th.joinable())
                th.join();
        }

        DLOG("core",0) << "-------------------------------" << std::endl;

        auto frame_d = frame_clock.restart();
        auto sleep_duration = frame_duration_ns_ - frame_d;
        std::this_thread::sleep_for(sleep_duration);

        // Kick off log statements submissions
        WLOGGER.flush();
    }

    WLOGGER.kill();
    EventBus::Kill();

    return 0;
}