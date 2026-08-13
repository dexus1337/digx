#include "entities/digx-stone.hpp"
#include "assets/texture-cache.hpp"
#include <cmath>

namespace digx
{
    namespace
    {
        const zwodee::texture* get_stone_tex(stone::stone_color col)
        {
            if (col == stone::color_high) return texture_cache::get().stone_high_tex.get();
            if (col == stone::color_mid) return texture_cache::get().stone_mid_tex.get();
            return texture_cache::get().stone_low_tex.get();
        }
    }

    stone::stone(uint32_t network_id, stone_color col)
        : zwodee::entity(network_id, get_stone_tex(col), 100), m_color(col)
    {
        configure_animator(1, 1, false);
    }

    void stone::tick()
    {
        if (m_wiggle_ticks > 0)
        {
            m_wiggle_ticks--;
            m_vx = 0.0f;
            m_vy = 0.0f;
            
            if (m_wiggle_ticks <= 0)
            {
                m_is_falling = true;
                m_fall_ticks = 0;
            }
            return;
        }

        if (m_is_moving)
        {
            float speed = 1.0f;
            if (m_is_falling)
            {
                m_fall_ticks++;
                speed = 0.25f + (static_cast<float>(m_fall_ticks) * 0.025f);
                if (speed > 2.0f)
                {
                    speed = 2.0f;
                }
            }
            m_vx = m_dir_x * speed;
            m_vy = m_dir_y * speed;
            m_x += m_vx;
            m_y += m_vy;

            // Check if reached destination
            bool reached_x = (m_dir_x == 0.0f) || (m_dir_x > 0.0f && m_x >= m_target_x) || (m_dir_x < 0.0f && m_x <= m_target_x);
            bool reached_y = (m_dir_y == 0.0f) || (m_dir_y > 0.0f && m_y >= m_target_y) || (m_dir_y < 0.0f && m_y <= m_target_y);

            if (reached_x && reached_y)
            {
                m_x = m_target_x;
                m_y = m_target_y;
                m_vx = 0.0f;
                m_vy = 0.0f;
                m_is_moving = false;
            }
        }
    }

    void stone::render(zwodee::renderer& target_renderer, double alpha)
    {
        if (!m_texture)
        {
            return;
        }

        // Interpolated position
        float render_x = m_x + (m_vx * static_cast<float>(alpha));
        float render_y = m_y + (m_vy * static_cast<float>(alpha));

        if (m_wiggle_ticks > 0)
        {
            render_x += std::sin(static_cast<float>(m_wiggle_ticks) * 0.375f) * 2.0f;
        }

        int frame_width = m_texture->get_width();
        int frame_height = m_texture->get_height();

        target_renderer.draw_sprite(*m_texture, 0, 0, frame_width, frame_height, render_x, render_y, m_width, m_height);
    }

    zwodee::render_node stone::get_render_node() const
    {
        if (!m_texture)
        {
            return zwodee::render_node{ m_x, m_y, m_width, m_height, nullptr, 0, 0, 0, 0 };
        }
        int frame_width = m_texture->get_width();
        int frame_height = m_texture->get_height();

        float rx = m_x;
        if (m_wiggle_ticks > 0)
        {
            rx += std::sin(static_cast<float>(m_wiggle_ticks) * 0.375f) * 2.0f;
        }

        return zwodee::render_node{ rx, m_y, m_width, m_height, m_texture, 0, 0, frame_width, frame_height };
    }



    int stone::get_explosion_radius() const
    {
        if (m_color == color_high)
        {
            return 2;
        }
        else if (m_color == color_mid)
        {
            return 1;
        }
        return 0;
    }



    void stone::start_move(float dx, float dy)
    {
        m_dir_x = dx;
        m_dir_y = dy;
        m_target_x = m_x + dx * 32.0f;
        m_target_y = m_y + dy * 32.0f;
        m_is_moving = true;
        if (dx != 0.0f)
        {
            m_was_pushed = true;
        }
    }


}
