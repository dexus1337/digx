#include "entities/digx-soldier.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"
#include "assets/texture-cache.hpp"

#include <cmath>

namespace digx
{
    soldier::soldier(uint32_t network_id)
        : enemy_base(network_id, texture_cache::get().soldier_front_tex.get(), 0.4f, 100)
    {
    }

    void soldier::tick()
    {
        auto& tc = texture_cache::get();

        if (m_stun_ticks > 0)
        {
            m_stun_ticks--;
            m_stun_anim_ticks++;
            m_vx = 0.0f;
            m_vy = 0.0f;
            m_is_moving = false;
            set_texture(tc.soldier_front_tex.get());
            zwodee::entity::tick();
            return;
        }

        enemy_base::tick();

        // Update active texture according to movement direction
        if (m_dir_y < 0.0f)
        {
            set_texture(tc.soldier_back_tex.get());
            set_flip_horizontal(false);
        }
        else if (m_dir_y > 0.0f)
        {
            set_texture(tc.soldier_front_tex.get());
            set_flip_horizontal(false);
        }
        else if (m_dir_x < 0.0f)
        {
            set_texture(tc.soldier_side_tex.get());
            set_flip_horizontal(true);
        }
        else if (m_dir_x > 0.0f)
        {
            set_texture(tc.soldier_side_tex.get());
            set_flip_horizontal(false);
        }
        else
        {
            set_texture(tc.soldier_front_tex.get());
            set_flip_horizontal(false);
        }
    }

    void soldier::update_behavior(player* player)
    {
        m_player = player;
        if (!player || is_dead())
        {
            return;
        }

        float dx = player->get_x() - m_x;
        float dy = player->get_y() - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);

        bool fart_active = false;
        float fart_x = 0.0f;
        float fart_y = 0.0f;
        if (auto* lvl = dynamic_cast<digx::level*>(player->get_level()))
        {
            fart_active = lvl->is_fart_active();
            fart_x = lvl->get_fart_x();
            fart_y = lvl->get_fart_y();
        }

        // Reset m_fart_affected when the fart cloud finishes
        if (!fart_active)
        {
            m_fart_affected = false;
        }

        // Check if fart cloud is active and soldier is in 1-tile proximity to the CLOUD
        if (!m_fart_affected && fart_active)
        {
            float fdx = fart_x - m_x;
            float fdy = fart_y - m_y;
            if (std::abs(fdx) <= 32.1f && std::abs(fdy) <= 32.1f)
            {
                // Stunned for 2.5 seconds (320 ticks at 128Hz)
                m_stun_ticks = 320;
                m_fart_affected = true;
                m_vx = 0.0f;
                m_vy = 0.0f;
                m_is_moving = false;
                return;
            }
        }

        // If not stunned, player takes damage on collision
        if (m_stun_ticks == 0 && dist < 16.0f)
        {
            player->take_damage(20);
        }

        if (m_stun_ticks > 0)
        {
            m_vx = 0.0f;
            m_vy = 0.0f;
            m_is_moving = false;
            return;
        }

        // Shared grid movement logic
        update_enemy_movement(player);
    }

    void soldier::render(zwodee::renderer& target_renderer, double alpha)
    {
        if (!m_texture) return;
        float render_x = m_x + (m_vx * static_cast<float>(alpha));
        float render_y = m_y + (m_vy * static_cast<float>(alpha));

        if (m_stun_ticks > 0)
        {
            render_x += std::sin(static_cast<float>(m_stun_anim_ticks) * 0.375f) * 2.0f;
        }

        int frame_width = m_texture->get_width();
        int frame_height = m_texture->get_height();
        target_renderer.draw_sprite(*m_texture, 0, 0, frame_width, frame_height, render_x, render_y, m_width, m_height, m_flip_horizontal);
    }

    zwodee::render_node soldier::get_render_node() const
    {
        zwodee::render_node node = enemy_base::get_render_node();
        if (m_stun_ticks > 0)
        {
            node.x += std::sin(static_cast<float>(m_stun_anim_ticks) * 0.375f) * 2.0f;
        }
        return node;
    }

    bool soldier::is_stunned() const
    {
        return m_stun_ticks > 0;
    }

    float soldier::get_stun_time_remaining() const
    {
        return static_cast<float>(m_stun_ticks) / 128.0f;
    }
}
