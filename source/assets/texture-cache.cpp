#include "assets/texture-cache.hpp"
#include <SDL3/SDL.h>

namespace digx
{
    void texture_cache::load_all(zwodee::renderer& r)
    {
        if (loaded) return;

        // Player
        player_dead_tex                 = r.load_dds_texture("assets/textures/goblin-dead.dds");
        fart_tex                        = r.load_dds_texture("assets/textures/fart.dds");
        player_shovel_tex               = r.load_dds_texture("assets/textures/goblin-idle-shovel.dds");
        player_shovel_running_tex       = r.load_dds_texture("assets/textures/goblin-running-shovel-1.dds");
        player_shovel_running_up_tex    = r.load_dds_texture("assets/textures/goblin-running-up-shovel-1.dds");
        player_shovel_running_down_tex  = r.load_dds_texture("assets/textures/goblin-running-down-shovel-1.dds");
        player_shovel_running_texs[0]   = r.load_dds_texture("assets/textures/goblin-running-shovel-1.dds");
        player_shovel_running_texs[1]   = r.load_dds_texture("assets/textures/goblin-running-shovel-2.dds");
        player_shovel_running_up_texs[0] = r.load_dds_texture("assets/textures/goblin-running-up-shovel-1.dds");
        player_shovel_running_up_texs[1] = r.load_dds_texture("assets/textures/goblin-running-up-shovel-2.dds");
        player_shovel_running_down_texs[0] = r.load_dds_texture("assets/textures/goblin-running-down-shovel-1.dds");
        player_shovel_running_down_texs[1] = r.load_dds_texture("assets/textures/goblin-running-down-shovel-2.dds");
        player_digging_shovel_tex[0]    = r.load_dds_texture("assets/textures/goblin-digging-shovel-1.dds");
        player_digging_shovel_tex[1]    = r.load_dds_texture("assets/textures/goblin-digging-shovel-2.dds");
        player_digging_shovel_up_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-up-shovel-1.dds");
        player_digging_shovel_up_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-up-shovel-2.dds");
        player_digging_shovel_down_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-down-shovel-1.dds");
        player_digging_shovel_down_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-down-shovel-2.dds");
        player_pickaxe_tex              = r.load_dds_texture("assets/textures/goblin-idle-pickaxe.dds");
        player_pickaxe_running_tex      = r.load_dds_texture("assets/textures/goblin-running-pickaxe-1.dds");
        player_pickaxe_running_up_tex   = r.load_dds_texture("assets/textures/goblin-running-up-pickaxe-1.dds");
        player_pickaxe_running_down_tex = r.load_dds_texture("assets/textures/goblin-running-down-pickaxe-1.dds");
        player_pickaxe_running_texs[0]  = r.load_dds_texture("assets/textures/goblin-running-pickaxe-1.dds");
        player_pickaxe_running_texs[1]  = r.load_dds_texture("assets/textures/goblin-running-pickaxe-2.dds");
        player_pickaxe_running_up_texs[0] = r.load_dds_texture("assets/textures/goblin-running-up-pickaxe-1.dds");
        player_pickaxe_running_up_texs[1] = r.load_dds_texture("assets/textures/goblin-running-up-pickaxe-2.dds");
        player_pickaxe_running_down_texs[0] = r.load_dds_texture("assets/textures/goblin-running-down-pickaxe-1.dds");
        player_pickaxe_running_down_texs[1] = r.load_dds_texture("assets/textures/goblin-running-down-pickaxe-2.dds");
        player_digging_pickaxe_tex[0]   = r.load_dds_texture("assets/textures/goblin-digging-pickaxe-1.dds");
        player_digging_pickaxe_tex[1]   = r.load_dds_texture("assets/textures/goblin-digging-pickaxe-2.dds");
        player_digging_pickaxe_up_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-up-pickaxe-1.dds");
        player_digging_pickaxe_up_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-up-pickaxe-2.dds");
        player_digging_pickaxe_down_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-down-pickaxe-1.dds");
        player_digging_pickaxe_down_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-down-pickaxe-2.dds");

        for (int i = 0; i < 2; ++i)
        {
            if (!player_digging_pickaxe_tex[i]) player_digging_pickaxe_tex[i] = player_digging_shovel_tex[i];
            if (!player_digging_pickaxe_up_tex[i]) player_digging_pickaxe_up_tex[i] = player_digging_shovel_up_tex[i];
            if (!player_digging_pickaxe_down_tex[i]) player_digging_pickaxe_down_tex[i] = player_digging_shovel_down_tex[i];
        }

        // Enemies
        vampire_sleeping_tex            = r.load_dds_texture("assets/textures/vampire-sleeping.dds");
        vampire_triggered_tex           = r.load_dds_texture("assets/textures/vampire-triggered.dds");
        soldier_tex                     = r.load_dds_texture("assets/textures/soldier-front.dds");
        soldier_front_tex               = r.load_dds_texture("assets/textures/soldier-front.dds");
        soldier_back_tex                = r.load_dds_texture("assets/textures/soldier-back.dds");
        soldier_side_tex                = r.load_dds_texture("assets/textures/soldier-side.dds");
        mummy_tex                       = r.load_dds_texture("assets/textures/mummy.dds");
        mummy_front_tex                 = r.load_dds_texture("assets/textures/mummy-front.dds");
        mummy_back_tex                  = r.load_dds_texture("assets/textures/mummy-back.dds");
        mummy_side_tex                  = r.load_dds_texture("assets/textures/mummy-side.dds");
        dragon_red_tex                  = r.load_dds_texture("assets/textures/dragon-red.dds");
        dragon_green_tex                = r.load_dds_texture("assets/textures/dragon-green.dds");
        goblin_head_tex                 = r.load_dds_texture("assets/textures/goblin-head.dds");

        // Environment
        dirt_tex                        = r.load_dds_texture("assets/textures/dirt.dds");
        dirt_breaking_texs[0]           = r.load_dds_texture("assets/textures/dirt-breaking-1.dds");
        dirt_breaking_texs[1]           = r.load_dds_texture("assets/textures/dirt-breaking-2.dds");
        dirt_breaking_texs[2]           = r.load_dds_texture("assets/textures/dirt-breaking-3.dds");
        digged_tex                      = r.load_dds_texture("assets/textures/digged.dds");
        stone_high_tex                  = r.load_dds_texture("assets/textures/stone-high.dds");
        stone_mid_tex                   = r.load_dds_texture("assets/textures/stone-mid.dds");
        stone_low_tex                   = r.load_dds_texture("assets/textures/stone-low.dds");
        static_stone_textures[0]        = r.load_dds_texture("assets/textures/stone-1.dds");
        static_stone_textures[1]        = r.load_dds_texture("assets/textures/stone-2.dds");
        static_stone_textures[2]        = r.load_dds_texture("assets/textures/stone-3.dds");
        static_stone_textures[3]        = r.load_dds_texture("assets/textures/stone-4.dds");
        door_closed_tex                 = r.load_dds_texture("assets/textures/door-closed.dds");
        door_open_tex                   = r.load_dds_texture("assets/textures/door-open.dds");

        // Items
        pickaxe_tex                     = r.load_dds_texture("assets/textures/pickaxe.dds");
        coin_tex                        = r.load_dds_texture("assets/textures/coin.dds");
        const std::vector<std::string> diamond_colors = { "green", "orange", "purple", "blue" };
        for (const auto& color : diamond_colors)
        {
            if (auto tex = r.load_dds_texture("assets/textures/diamond-" + color + ".dds"))
            {
                diamond_textures.push_back(std::move(tex));
            }
        }
        garlic_tex                      = r.load_dds_texture("assets/textures/garlic.dds");
        onion_tex                       = r.load_dds_texture("assets/textures/onion.dds");
        lamp_tex                        = r.load_dds_texture("assets/textures/lamp.dds");

        // Effects
        blink_tex                       = r.load_dds_texture("assets/textures/blink.dds");
        explosion_tex                   = r.load_dds_texture("assets/textures/explosion.dds");

        // UI & System
        bg_tex                          = r.load_dds_texture("assets/textures/header.dds");
        logo_tex                        = r.load_dds_texture("assets/textures/mainmenu-text.dds");
        
        auto create_solid = [&r](int w, int h, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) {
            SDL_Texture* sdl_tex = SDL_CreateTexture(r.get_raw_renderer(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
            if (!sdl_tex) return std::unique_ptr<zwodee::texture>(nullptr);
            uint32_t pixel = (red) | (green << 8) | (blue << 16) | (alpha << 24);
            std::vector<uint32_t> pixels(w * h, pixel);
            SDL_UpdateTexture(sdl_tex, nullptr, pixels.data(), w * 4);
            SDL_SetTextureBlendMode(sdl_tex, SDL_BLENDMODE_BLEND);
            return std::make_unique<zwodee::texture>(sdl_tex, w, h);
        };
        fallback_tex = create_solid(32, 32, 255, 0, 0, 255);

        loaded = true;
    }
}
