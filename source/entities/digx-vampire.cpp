#include "entities/digx-vampire.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"
#include "assets/texture-cache.hpp"

#include <cmath>

namespace digx
{
    vampire::vampire(uint32_t network_id)
        : zwodee::entity(network_id, texture_cache::get().vampire_sleeping_tex.get(), 50)
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
        auto& tc = texture_cache::get();

        if (!player || m_is_neutralized)
        {
            set_texture(tc.vampire_sleeping_tex.get());
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
            set_texture(tc.vampire_triggered_tex.get());
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
            if (auto tex = texture_cache::get().vampire_sleeping_tex.get())
            {
                float fw = static_cast<float>(tex->get_width());
                float fh = static_cast<float>(tex->get_height());
                m_snorZ.tex = tex;
                m_snorZ.src_x = 0;
                m_snorZ.src_y = 0;
                m_snorZ.src_w = static_cast<int>(fw);
                m_snorZ.src_h = static_cast<int>(fh);
                m_snorZ.w = fw;
                m_snorZ.h = fh;
                m_snorZ.is_ui = false;
            }
            set_texture(tc.vampire_sleeping_tex.get());
        }

        if (m_is_active && player->get_breath_active_time() > 0.0f)
        {
            m_is_neutralized = true;
            m_is_active = false;
            set_texture(texture_cache::get().vampire_sleeping_tex.get());
            return;
        }

        if (m_is_active && dist < 16.0f)
        {
            if (player->get_garlic_count() > 0)
            {
                player->use_garlic();
                m_is_neutralized = true;
                m_is_active = false;
                set_texture(texture_cache::get().vampire_sleeping_tex.get());
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
