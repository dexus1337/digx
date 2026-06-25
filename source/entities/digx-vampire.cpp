#include "entities/digx-vampire.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"

#include <cmath>

namespace digx
{
    vampire::vampire(uint32_t network_id, const zwodee::texture* sleeping_tex, const zwodee::texture* triggered_tex)
        : zwodee::entity(network_id, sleeping_tex, 50),
          m_sleeping_tex(sleeping_tex),
          m_triggered_tex(triggered_tex)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void vampire::tick()
    {
        zwodee::entity::tick();
    }

    void vampire::update_behavior(player* player)
    {
        m_vx = 0.0f;
        m_vy = 0.0f;

        if (!player || m_is_neutralized)
        {
            set_texture(m_sleeping_tex);
            return;
        }

        float dx = player->get_x() - m_x;
        float dy = player->get_y() - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);

        bool was_active = m_is_active;

        if (std::abs(dx) <= 32.1f && std::abs(dy) <= 32.1f) // 1 tile radius (including diagonals)
        {
            m_is_active = true;
        }
        else
        {
            m_is_active = false;
        }

        if (m_is_active)
        {
            set_texture(m_triggered_tex);
            if (!was_active)
            {
                if (auto* audio = player->get_audio_manager())
                {
                    audio->play_sound("vampire_triggered");
                }
            }
        }
        else
        {
            set_texture(m_sleeping_tex);
        }

        if (m_is_active && player->get_breath_active_time() > 0.0f)
        {
            m_is_neutralized = true;
            m_is_active = false;
            set_texture(m_sleeping_tex);
            return;
        }

        if (m_is_active && dist < 16.0f)
        {
            if (player->get_garlic_count() > 0)
            {
                player->use_garlic();
                m_is_neutralized = true;
                m_is_active = false;
                set_texture(m_sleeping_tex);
            }
            else
            {
                player->take_damage(999);
            }
        }
    }

    bool vampire::is_active() const
    {
        return m_is_active;
    }

    bool vampire::is_neutralized() const
    {
        return m_is_neutralized;
    }
}
