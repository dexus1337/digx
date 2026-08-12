#pragma once

#include "zwodee.hpp"

namespace digx
{
    struct config_data
    {
        char magic[4] = {'C', 'F', 'G', '1'};
        int version = 1;
        bool sound_enabled = true;
        float volume = 1.0f;
        uint32_t fps_limit = 0; // index matching zwodee::engine::fps_limit
    };

    class config_manager
    {
    public:
        static void load_config(zwodee::engine& engine);
        static void save_config(zwodee::engine& engine);
    };
}
