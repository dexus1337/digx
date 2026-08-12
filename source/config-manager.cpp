#include "config-manager.hpp"
#include <fstream>
#include <iostream>

namespace digx
{
    void config_manager::load_config(zwodee::engine& engine)
    {
        config_data data;
        std::ifstream file("config.dat", std::ios::binary);
        if (file.good())
        {
            file.read(reinterpret_cast<char*>(&data), sizeof(config_data));
            if (file.gcount() == sizeof(config_data) &&
                data.magic[0] == 'C' && data.magic[1] == 'F' && data.magic[2] == 'G' && data.magic[3] == '1')
            {
                engine.get_audio_manager().set_muted(!data.sound_enabled);
                engine.get_audio_manager().set_volume(data.volume);
                engine.set_fps_limit(static_cast<zwodee::engine::fps_limit>(data.fps_limit));
                std::cout << "[Config] Loaded config: Sound=" << (data.sound_enabled ? "ON" : "OFF")
                          << ", Volume=" << data.volume
                          << ", FPS Cap=" << data.fps_limit << std::endl;
                return;
            }
        }

        // Save default config if none existed or corrupt
        std::cout << "[Config] Creating default config..." << std::endl;
        save_config(engine);
    }

    void config_manager::save_config(zwodee::engine& engine)
    {
        config_data data;
        data.sound_enabled = !engine.get_audio_manager().is_muted();
        data.volume = engine.get_audio_manager().get_volume();
        data.fps_limit = static_cast<uint32_t>(engine.get_fps_limit());

        std::ofstream file("config.dat", std::ios::binary);
        if (file.good())
        {
            file.write(reinterpret_cast<const char*>(&data), sizeof(config_data));
            std::cout << "[Config] Saved config: Sound=" << (data.sound_enabled ? "ON" : "OFF")
                      << ", Volume=" << data.volume
                      << ", FPS Cap=" << data.fps_limit << std::endl;
        }
    }
}
