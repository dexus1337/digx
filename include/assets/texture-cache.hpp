#pragma once

#include "zwodee.hpp"
#include <memory>
#include <vector>
#include <array>
#include <string>

namespace digx
{
    class texture_cache
    {
    public:
        static texture_cache& get()
        {
            static texture_cache instance;
            return instance;
        }

        void load_all(zwodee::renderer& r);

        bool loaded = false;

        std::shared_ptr<zwodee::texture> player_shovel_tex;
        std::shared_ptr<zwodee::texture> player_shovel_running_tex;
        std::shared_ptr<zwodee::texture> player_shovel_running_up_tex;
        std::shared_ptr<zwodee::texture> player_shovel_running_down_tex;
        std::shared_ptr<zwodee::texture> player_pickaxe_tex;
        std::shared_ptr<zwodee::texture> player_pickaxe_running_tex;
        std::shared_ptr<zwodee::texture> player_pickaxe_running_up_tex;
        std::shared_ptr<zwodee::texture> player_pickaxe_running_down_tex;

        std::array<std::shared_ptr<zwodee::texture>, 2> player_shovel_running_texs;
        std::array<std::shared_ptr<zwodee::texture>, 2> player_shovel_running_up_texs;
        std::array<std::shared_ptr<zwodee::texture>, 2> player_shovel_running_down_texs;

        std::array<std::shared_ptr<zwodee::texture>, 2> player_pickaxe_running_texs;
        std::array<std::shared_ptr<zwodee::texture>, 2> player_pickaxe_running_up_texs;
        std::array<std::shared_ptr<zwodee::texture>, 2> player_pickaxe_running_down_texs;

        std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_shovel_tex;
        std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_shovel_up_tex;
        std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_shovel_down_tex;

        std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_pickaxe_tex;
        std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_pickaxe_up_tex;
        std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_pickaxe_down_tex;

        std::shared_ptr<zwodee::texture> stone_high_tex;
        std::shared_ptr<zwodee::texture> stone_low_tex;
        std::shared_ptr<zwodee::texture> stone_mid_tex;
        std::shared_ptr<zwodee::texture> pickaxe_tex;
        std::shared_ptr<zwodee::texture> coin_tex;
        std::shared_ptr<zwodee::texture> door_closed_tex;
        std::shared_ptr<zwodee::texture> door_open_tex;
        std::shared_ptr<zwodee::texture> garlic_tex;
        std::shared_ptr<zwodee::texture> onion_tex;
        std::shared_ptr<zwodee::texture> lamp_tex;
        std::shared_ptr<zwodee::texture> blink_tex;
        std::shared_ptr<zwodee::texture> digged_tex;
        
        std::array<std::shared_ptr<zwodee::texture>, 4> static_stone_textures;
        std::vector<std::shared_ptr<zwodee::texture>> diamond_textures;

        std::shared_ptr<zwodee::texture> bg_tex;
        std::shared_ptr<zwodee::texture> fallback_tex;

        std::shared_ptr<zwodee::texture> vampire_sleeping_tex;
        std::shared_ptr<zwodee::texture> vampire_triggered_tex;
        std::shared_ptr<zwodee::texture> player_dead_tex;
        std::shared_ptr<zwodee::texture> fart_tex;
        std::shared_ptr<zwodee::texture> soldier_tex;
        std::shared_ptr<zwodee::texture> soldier_front_tex;
        std::shared_ptr<zwodee::texture> soldier_back_tex;
        std::shared_ptr<zwodee::texture> soldier_side_tex;
        std::shared_ptr<zwodee::texture> mummy_tex;
        std::shared_ptr<zwodee::texture> mummy_front_tex;
        std::shared_ptr<zwodee::texture> mummy_back_tex;
        std::shared_ptr<zwodee::texture> mummy_side_tex;
        std::shared_ptr<zwodee::texture> dragon_red_tex;
        std::shared_ptr<zwodee::texture> dragon_green_tex;
        std::shared_ptr<zwodee::texture> dirt_tex;
        std::array<std::shared_ptr<zwodee::texture>, 3> dirt_breaking_texs;
        
    private:
        texture_cache() = default;
        ~texture_cache() = default;

        // Prevent copying
        texture_cache(const texture_cache&) = delete;
        texture_cache& operator=(const texture_cache&) = delete;
    };
}
