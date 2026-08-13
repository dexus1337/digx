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

        if (m_neutralized_ticks > 0)
        {
            m_neutralized_ticks--;
            m_is_active = false;
            set_texture(tc.vampire_sleeping_tex.get());
            return;
        }

        if (!player)
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
            set_texture(tc.vampire_sleeping_tex.get());
        }

        if (m_is_active && dist < 16.0f)
        {
            if (player->get_garlic_count() > 0)
            {
                player->use_garlic();
                m_neutralized_ticks = 640; // Stunned for 5 seconds
                m_is_active = false;
                set_texture(texture_cache::get().vampire_sleeping_tex.get());
                if (auto* audio = player->get_audio_manager())
                {
                    audio->play_sound("garlic_chew");
                }
            }
            else
            {
                player->take_damage(999);
            }
        }
    }

    void vampire::render(zwodee::renderer& target_renderer, double alpha)
    {
        if (!m_texture) return;
        float render_x = m_x + (m_vx * static_cast<float>(alpha));
        float render_y = m_y + (m_vy * static_cast<float>(alpha));

        if (m_neutralized_ticks > 0)
        {
            render_x += std::sin(static_cast<float>(m_neutralized_ticks) * 0.375f) * 2.0f;
        }

        int frame_width = m_texture->get_width();
        int frame_height = m_texture->get_height();
        target_renderer.draw_sprite(*m_texture, 0, 0, frame_width, frame_height, render_x, render_y, m_width, m_height);
    }

    zwodee::render_node vampire::get_render_node() const
    {
        if (!m_texture)
        {
            return zwodee::render_node{ m_x, m_y, m_width, m_height, nullptr, 0, 0, 0, 0 };
        }
        int frame_width = m_texture->get_width();
        int frame_height = m_texture->get_height();

        float rx = m_x;
        if (m_neutralized_ticks > 0)
        {
            rx += std::sin(static_cast<float>(m_neutralized_ticks) * 0.375f) * 2.0f;
        }

        return zwodee::render_node{ rx, m_y, m_width, m_height, m_texture, 0, 0, frame_width, frame_height };
    }


}
