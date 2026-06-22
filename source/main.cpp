#include "digx-game.hpp"

#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::cout << "Starting Digx Game..." << std::endl;

    // Create engine instance
    zwodee::engine engine("DigX", 1280, 720, true);

    // Create level (25x19 grid spans the 800x600 screen space)
    auto level = std::make_unique<digx::level>(35, 35);
    level->load_demo_level(engine.get_renderer());

    // Register level to level manager and transition to it
    engine.get_level_manager().register_level("demo", std::move(level));
    engine.get_level_manager().transition_to("demo");

    std::cout << "Entering main game loop..." << std::endl;

    // Run the main game loop
    engine.run();

    std::cout << "Exiting Digx Game cleanly." << std::endl;
    return 0;
}
