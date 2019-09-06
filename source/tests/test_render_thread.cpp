#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

#include "level/chunk.h"

#include "render_thread.h"
#include "clock.hpp"

#include "debug/logger.h"

using namespace erwin;

std::vector<RenderKey> keys = { 1, 5000, 498, 632, 0, 455, 456, 798, 222, 101};
std::vector<RenderCommand> commands = { {0}, {1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}, {9}};
void fill_commands(std::reference_wrapper<RenderThread> rthread)
{
    std::vector<RenderCommand> cmds(commands);
    rthread.get().enqueue(keys, std::move(cmds));
}

int main()
{
    RenderThread rthread;

    rthread.spawn();

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
        if(t_tot>2.f)
            running = false;

        // Feed the render thread with draw commands
        fill_commands(rthread);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        fill_commands(rthread);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        auto frame_d = frame_clock.restart();
        auto sleep_duration = frame_duration_ns_ - frame_d;
        std::this_thread::sleep_for(sleep_duration);

        // Kick off sorted draw-call submissions
        rthread.flush();
    }

    rthread.kill();


    Cell* data = nullptr;
    TerrainChunk chunk(8,16,data);

    for(int ii=0; ii<8; ++ii)
        for(int jj=0; jj<16; ++jj)
            chunk.set_cell(ii,jj,make_cell(1,CELL_FLOOR,CELL_FLAG_NONE));

    chunk.set_cell(0,0,make_cell(1,CellWall(CELL_FLOOR|CELL_WALL_NE),CELL_FLAG_NONE));
    chunk.set_cell(1,5,make_cell(1,CellWall(CELL_FLOOR|CELL_WALL_SW|CELL_WALL_NE),CELL_FLAG_NONE));

    chunk.dbg_display();

    chunk.write_stream(std::cout);

    EventBus::Kill();
    return 0;
}
