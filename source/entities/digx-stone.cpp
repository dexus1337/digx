#include "entities/digx-stone.hpp"
#include <cmath>

namespace digx
{
    stone::stone(uint32_t network_id, const zwodee::texture* tex, stone_color col)
        : zwodee::entity(network_id, tex, 100), m_color(col)
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
            }
            return;
        }

        if (m_is_moving)
        {
            float speed = m_is_falling ? m_fall_speed : 1.0f;
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
            render_x += std::sin(static_cast<float>(m_wiggle_ticks) * 0.5f) * 2.0f;
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
            rx += std::sin(static_cast<float>(m_wiggle_ticks) * 0.5f) * 2.0f;
        }

        return zwodee::render_node{ rx, m_y, m_width, m_height, m_texture, 0, 0, frame_width, frame_height };
    }

    void stone::push(float speed_x)
    {
        m_vx = speed_x;
    }

    stone::stone_color stone::get_color() const
    {
        return m_color;
    }

    int stone::get_explosion_radius() const
    {
        if (m_color == color_black)
        {
            return 2;
        }
        else if (m_color == color_grey)
        {
            return 1;
        }
        return 0;
    }

    bool stone::is_falling() const
    {
        return m_is_falling;
    }

    bool stone::is_moving() const
    {
        return m_is_moving;
    }

    int stone::get_wiggle_ticks() const
    {
        return m_wiggle_ticks;
    }

    bool stone::was_pushed() const
    {
        return m_was_pushed;
    }

    void stone::set_falling(bool falling)
    {
        m_is_falling = falling;
    }

    void stone::start_wiggle()
    {
        m_wiggle_ticks = 128;
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

    void stone::stop_falling()
    {
        m_is_falling = false;
    }

    void stone::clear_pushed()
    {
        m_was_pushed = false;
    }
}
