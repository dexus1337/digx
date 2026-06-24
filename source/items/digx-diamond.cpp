#include "items/digx-diamond.hpp"
#include "levels/digx-level.hpp"
#include <cmath>
#include <cstdlib>

namespace digx
{
    diamond::diamond(uint32_t network_id, const zwodee::texture* tex, const zwodee::texture* blink_tex)
        : zwodee::entity(network_id, tex, 1), m_blink_tex(blink_tex)
    {
        m_width = 32.0f;
        m_height = 32.0f;
        m_blink_timer = (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX)) * 5.0f;
    }

    void diamond::tick()
    {
        if (m_blink_timer > 0.0f)
        {
            m_blink_timer -= 1.0f / 128.0f;
        }
        else
        {
            m_blink_timer = 5.0f;
        }
    }

    void diamond::set_revealed(bool reveal)
    {
        m_is_revealed = m_permanently_revealed || reveal;
    }

    bool diamond::is_revealed() const
    {
        return m_is_revealed || m_permanently_revealed;
    }

    void diamond::set_permanently_revealed(bool perm)
    {
        m_permanently_revealed = perm;
        if (perm)
        {
            m_is_revealed = true;
        }
    }

    bool diamond::is_permanently_revealed() const
    {
        return m_permanently_revealed;
    }

    void diamond::set_level(zwodee::tile_level* lvl)
    {
        m_level = lvl;
    }

    void diamond::render(zwodee::renderer& target_renderer, double alpha)
    {
        bool show_normally = m_is_revealed || m_permanently_revealed;
        
        if (!show_normally && m_level)
        {
            int gx = static_cast<int>(std::round(m_x / 32.0f));
            int gy = static_cast<int>(std::round(m_y / 32.0f));
            if (auto* digx_lvl = dynamic_cast<digx::level*>(m_level))
            {
                if (digx_lvl->is_tile_digged(gx, gy))
                {
                    show_normally = true;
                }
            }
        }

        if (show_normally)
        {
            zwodee::entity::render(target_renderer, alpha);
        }
        else if (m_blink_tex && m_blink_timer <= 0.5f)
        {
            float render_x = m_x + (m_vx * static_cast<float>(alpha));
            float render_y = m_y + (m_vy * static_cast<float>(alpha));
            target_renderer.draw_sprite(*m_blink_tex, 0, 0, m_blink_tex->get_width(), m_blink_tex->get_height(), render_x, render_y, m_width, m_height);
        }
    }

    zwodee::render_node diamond::get_render_node() const
    {
        bool show_normally = m_is_revealed || m_permanently_revealed;
        
        if (!show_normally && m_level)
        {
            int gx = static_cast<int>(std::round(m_x / 32.0f));
            int gy = static_cast<int>(std::round(m_y / 32.0f));
            if (auto* digx_lvl = dynamic_cast<digx::level*>(m_level))
            {
                if (digx_lvl->is_tile_digged(gx, gy))
                {
                    show_normally = true;
                }
            }
        }

        if (show_normally)
        {
            return zwodee::entity::get_render_node();
        }
        else if (m_blink_tex && m_blink_timer <= 0.5f)
        {
            return zwodee::render_node{ m_x, m_y, m_width, m_height, m_blink_tex, 0, 0, m_blink_tex->get_width(), m_blink_tex->get_height() };
        }
        else
        {
            return zwodee::render_node{ m_x, m_y, 0.0f, 0.0f, m_blink_tex, 0, 0, 0, 0 };
        }
    }
}
