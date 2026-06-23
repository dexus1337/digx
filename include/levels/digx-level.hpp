#pragma once

/**
 * @file    digx-level.hpp
 * @author  dexus1337
 * @brief   Defines the level class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "zwodee.hpp"
#include <array>

namespace digx
{
    class player;

    class level : public zwodee::tile_level
    {
    public:
        level(uint32_t width, uint32_t height);
        ~level() override = default;

        void on_enter() override;
        void on_exit() override;
        void tick() override;
        void render(zwodee::renderer& target_renderer, double alpha) override;
        zwodee::render_snapshot get_render_snapshot(int display_w, int display_h) const override;

        void load_demo_level(zwodee::engine& engine);

        [[nodiscard]] player* get_player() const;

        /// Returns a randomly chosen diamond texture from m_diamond_textures,
        /// falling back to m_fallback_tex if the pool is empty.
        [[nodiscard]] const zwodee::texture* get_random_diamond_texture() const;
        [[nodiscard]] bool is_tile_digged(int gx, int gy) const;
        void dig_tile(int gx, int gy);
        void explode_stone(class stone* st);

    private:
        player* m_player = nullptr;
        
        // Textures preloaded and shared across levels
        std::shared_ptr<zwodee::texture> m_player_shovel_tex;
        std::shared_ptr<zwodee::texture> m_player_shovel_running_tex;
        std::shared_ptr<zwodee::texture> m_player_shovel_running_up_tex;
        std::shared_ptr<zwodee::texture> m_player_shovel_running_down_tex;
        std::shared_ptr<zwodee::texture> m_player_pickaxe_tex;
        std::shared_ptr<zwodee::texture> m_player_pickaxe_running_tex;
        std::shared_ptr<zwodee::texture> m_player_pickaxe_running_up_tex;
        std::shared_ptr<zwodee::texture> m_player_pickaxe_running_down_tex;
        std::shared_ptr<zwodee::texture> m_stone_black_tex;
        std::shared_ptr<zwodee::texture> m_stone_grey_tex;
        std::shared_ptr<zwodee::texture> m_stone_brown_tex;
        std::shared_ptr<zwodee::texture> m_pickaxe_tex;
        std::shared_ptr<zwodee::texture> m_coin_tex;
        std::shared_ptr<zwodee::texture> m_door_closed_tex;
        std::shared_ptr<zwodee::texture> m_door_open_tex;
        std::vector<std::shared_ptr<zwodee::texture>> m_diamond_textures;
        std::shared_ptr<zwodee::texture> m_garlic_tex;
        std::shared_ptr<zwodee::texture> m_onion_tex;
        std::shared_ptr<zwodee::texture> m_digged_tex;
        std::array<std::shared_ptr<zwodee::texture>, 4> m_static_stone_textures;
        std::shared_ptr<zwodee::texture> m_bg_tex;
        std::shared_ptr<zwodee::texture> m_fallback_tex;
        
        // Preloaded enemy textures
        std::shared_ptr<zwodee::texture> m_vampire_tex;
        std::shared_ptr<zwodee::texture> m_soldier_tex;
        std::shared_ptr<zwodee::texture> m_mummy_tex;
        std::shared_ptr<zwodee::texture> m_dragon_tex;

        // Target number of gold coins to collect to open the exit
        int m_target_gold = 0;
        bool m_exit_open = false;
        
        // Exit coordinates
        float m_exit_x = 0.0f;
        float m_exit_y = 0.0f;
        
        // Track visual state of lamp (reveal diamonds)
        float m_lamp_timer = 0.0f;
    };
}
