#include "entities/digx-soldier.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"

#include <cmath>

namespace digx
{
    soldier::soldier(uint32_t network_id, const zwodee::texture* tex)
        : zwodee::entity(network_id, tex, 100)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void soldier::tick()
    {
        if (m_stun_ticks > 0)
        {
            m_stun_ticks--;
            m_vx = 0.0f;
            m_vy = 0.0f;
            zwodee::entity::tick();
            return;
        }

        zwodee::entity::tick();
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

        // Check if player farted (garlic) and is in proximity (e.g. 96 pixels)
        if (player->get_fart_active_time() > 0.0f && dist <= 96.0f)
        {
            // Stunned!
            m_stun_ticks = static_cast<int>(player->get_fart_active_time() * 128.0f);
            m_vx = 0.0f;
            m_vy = 0.0f;
            return;
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
            return;
        }

        auto* lvl = dynamic_cast<digx::level*>(player->get_level());
        if (!lvl)
        {
            return;
        }

        // Helper to check if a direction is clear of undigged tiles
        auto is_direction_clear = [this, lvl](float vx, float vy) -> bool {
            if (std::abs(vx) < 0.01f && std::abs(vy) < 0.01f)
            {
                return false;
            }
            float next_x = m_x + vx;
            float next_y = m_y + vy;
            int min_gx = static_cast<int>(std::floor(next_x / 32.0f));
            int max_gx = static_cast<int>(std::floor((next_x + m_width - 0.01f) / 32.0f));
            int min_gy = static_cast<int>(std::floor(next_y / 32.0f));
            int max_gy = static_cast<int>(std::floor((next_y + m_height - 0.01f) / 32.0f));

            for (int gy = min_gy; gy <= max_gy; ++gy)
            {
                for (int gx = min_gx; gx <= max_gx; ++gx)
                {
                    if (!lvl->is_tile_digged(gx, gy))
                    {
                        return false;
                    }
                }
            }
            return true;
        };

        // If direction is blocked or timer expired, select a new random direction biased by player height
        m_dir_change_ticks--;
        if (m_dir_change_ticks <= 0 || !is_direction_clear(m_vx, m_vy))
        {
            m_dir_change_ticks = 60 + std::rand() % 60; // 0.5s to 1s before next potential change

            float w_right = is_direction_clear(m_patrol_speed, 0.0f) ? 1.0f : 0.0f;
            float w_left  = is_direction_clear(-m_patrol_speed, 0.0f) ? 1.0f : 0.0f;
            float w_up    = is_direction_clear(0.0f, -m_patrol_speed) ? 1.0f : 0.0f;
            float w_down  = is_direction_clear(0.0f, m_patrol_speed) ? 1.0f : 0.0f;

            float player_y = player->get_y();
            if (player_y < m_y - 16.0f && w_up > 0.0f)
            {
                w_up += 2.0f;
            }
            if (player_y > m_y + 16.0f && w_down > 0.0f)
            {
                w_down += 2.0f;
            }

            float total_w = w_right + w_left + w_up + w_down;
            if (total_w > 0.0f)
            {
                float r = static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX) * total_w;
                if (r < w_right)
                {
                    m_vx = m_patrol_speed; m_vy = 0.0f;
                }
                else if (r < w_right + w_left)
                {
                    m_vx = -m_patrol_speed; m_vy = 0.0f;
                }
                else if (r < w_right + w_left + w_up)
                {
                    m_vx = 0.0f; m_vy = -m_patrol_speed;
                }
                else
                {
                    m_vx = 0.0f; m_vy = m_patrol_speed;
                }
            }
            else
            {
                m_vx = 0.0f;
                m_vy = 0.0f;
            }
        }
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
