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
            color_high,
            color_mid,
            color_low
        };

        stone(uint32_t network_id, stone_color col);

        void tick() override;

        void render(zwodee::renderer& target_renderer, double alpha) override;

        zwodee::render_node get_render_node() const override;

        int get_explosion_radius() const;
        void start_move(float dx, float dy);

        inline stone_color            get_color()                   const { return m_color; }
        inline bool                   is_falling()                  const { return m_is_falling; }
        inline bool                   is_moving()                   const { return m_is_moving; }
        inline int                    get_wiggle_ticks()            const { return m_wiggle_ticks; }
        inline bool                   was_pushed()                  const { return m_was_pushed; }

        inline void                   set_falling(bool falling)           { m_is_falling = falling; }
        inline void                   start_wiggle()                      { m_wiggle_ticks = 128; }
        inline void                   stop_falling()                      { m_is_falling = false; }
        inline void                   clear_pushed()                      { m_was_pushed = false; }
        inline void                   push(float speed_x)                 { m_vx = speed_x; }

    private:
        stone_color m_color = color_high;
        bool m_is_falling = false;
        float m_fall_speed = 2.0f;

        bool m_is_moving = false;
        bool m_was_pushed = false;
        int m_wiggle_ticks = 0;
        int m_fall_ticks = 0;
        float m_target_x = 0.0f;
        float m_target_y = 0.0f;
        float m_dir_x = 0.0f;
        float m_dir_y = 0.0f;
    };
}
