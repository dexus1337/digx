#include "entities/digx-stone.hpp"

namespace digx
{
    stone::stone(uint32_t network_id, const zwodee::texture* tex, stone_color col)
        : zwodee::entity(network_id, tex, 100), m_color(col)
    {
        configure_animator(1, 1, false);
    }

    void stone::tick()
    {
        m_vy = m_is_falling ? m_fall_speed : 0.0f;
        zwodee::entity::tick();
        m_vx = 0.0f;
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

        return zwodee::render_node{ m_x, m_y, m_width, m_height, m_texture, 0, 0, frame_width, frame_height };
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

    void stone::set_falling(bool falling)
    {
        m_is_falling = falling;
    }
}
