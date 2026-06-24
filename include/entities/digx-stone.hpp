#pragma once

/**
 * @file    digx-stone.hpp
 * @author  dexus1337
 * @brief   Defines the stone class representing falling and exploding boulders.
 * @version 1.0
 * @date    22.06.2026
 */

#include "zwodee.hpp"

namespace digx
{
    class stone : public zwodee::entity
    {
    public:
        enum stone_color : uint8_t
        {
            color_black,
            color_grey,
            color_brown
        };

        stone(uint32_t network_id, const zwodee::texture* tex, stone_color col);

        void tick() override;

        void render(zwodee::renderer& target_renderer, double alpha) override;

        zwodee::render_node get_render_node() const override;

        void push(float speed_x);

        [[nodiscard]] stone_color get_color() const;
        [[nodiscard]] int get_explosion_radius() const;
        [[nodiscard]] bool is_falling() const;
        [[nodiscard]] bool is_moving() const;
        [[nodiscard]] int get_wiggle_ticks() const;
        [[nodiscard]] bool was_pushed() const;
        
        void set_falling(bool falling);
        void start_wiggle();
        void start_move(float dx, float dy);
        void stop_falling();
        void clear_pushed();

    private:
        stone_color m_color = color_black;
        bool m_is_falling = false;
        float m_fall_speed = 2.0f;

        bool m_is_moving = false;
        bool m_was_pushed = false;
        int m_wiggle_ticks = 0;
        float m_target_x = 0.0f;
        float m_target_y = 0.0f;
        float m_dir_x = 0.0f;
        float m_dir_y = 0.0f;
    };
}
