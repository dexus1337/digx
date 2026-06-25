#include "digx-game.hpp"
#include "levels/digx-main-menu.hpp"

#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    std::cout << "Starting Digx Game..." << std::endl;

    // Create engine instance
    zwodee::engine engine("DigX", 1280, 720, true);

    // Create and register start menu
    auto menu = std::make_unique<digx::main_menu>(engine);
    engine.get_level_manager().register_level("main_menu", std::move(menu));

    // Register demo level (it will also be reloaded/recreated dynamically when Start Game is selected)
    auto level = std::make_unique<digx::level>(35, 35);
    level->load_demo_level(engine);
    engine.get_level_manager().register_level("demo", std::move(level));

    engine.get_level_manager().transition_to("main_menu");

    std::cout << "Entering main game loop..." << std::endl;

    // Run the main game loop
    engine.run();

    std::cout << "Exiting Digx Game cleanly." << std::endl;
    return 0;
}
