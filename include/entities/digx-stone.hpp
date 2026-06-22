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
        
        void set_falling(bool falling);

    private:
        stone_color m_color = color_black;
        bool m_is_falling = false;
        float m_fall_speed = 4.0f;
    };
}
