#include <iostream>
#include <chrono>

#include "MeshViewerApp.h"

using namespace GLOO;

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << ":" << std::endl
                << "MODEL where MODEL is "
                    "relative to assets/assignment2"
                << std::endl
                << "VERTEX_COUNT int"
                << std::endl;
        return -1;
    }

    std::unique_ptr<MeshViewerApp> app = make_unique<MeshViewerApp>(
        "Assignment Final", glm::ivec2(1440, 900), std::string(argv[1]), (size_t)std::atoi(argv[2]));

    app->SetupScene();

    using Clock = std::chrono::high_resolution_clock;
    using TimePoint =
        std::chrono::time_point<Clock, std::chrono::duration<double>>;
    
    TimePoint last_tick_time = Clock::now();
    TimePoint start_tick_time = last_tick_time;

    while (!app->IsFinished()) {
        TimePoint current_tick_time = Clock::now();
        double delta_time = (current_tick_time - last_tick_time).count();
        double total_elapsed_time = (current_tick_time - start_tick_time).count();
        app->Tick(delta_time, total_elapsed_time);
        last_tick_time = current_tick_time;
    }
    return 0;
}
