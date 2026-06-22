#pragma once

/**
 * @file    digx-level.hpp
 * @author  dexus1337
 * @brief   Defines the level class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "zwodee.hpp"

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

        void load_demo_level(zwodee::renderer& r);

        [[nodiscard]] player* get_player() const;

    private:
        player* m_player = nullptr;
        
        // Textures owned by the level
        std::unique_ptr<zwodee::texture> m_player_shovel_tex;
        std::unique_ptr<zwodee::texture> m_player_pickaxe_tex;
        std::unique_ptr<zwodee::texture> m_stone_black_tex;
        std::unique_ptr<zwodee::texture> m_stone_grey_tex;
        std::unique_ptr<zwodee::texture> m_stone_brown_tex;
        std::unique_ptr<zwodee::texture> m_pickaxe_tex;
        std::unique_ptr<zwodee::texture> m_bg_tex;
        std::unique_ptr<zwodee::texture> m_fallback_tex;

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
