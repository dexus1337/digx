#include "entities/digx-dragon.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"

#include <cmath>

namespace digx
{
    dragon::dragon(uint32_t network_id, const zwodee::texture* tex)
        : zwodee::entity(network_id, tex, 999999)
    {
        // 3x2 grid size (96x64 pixels)
        m_width = 96.0f;
        m_height = 64.0f;
    }

    void dragon::tick()
    {
        m_vx = m_moving_right ? m_speed : -m_speed;
        m_vy = 0.0f;
        set_flip_horizontal(!m_moving_right);

        if (m_player)
        {
            if (auto* lvl = dynamic_cast<digx::level*>(m_player->get_level()))
            {
                float next_x = m_x + m_vx;
                int min_gx = static_cast<int>(std::floor(next_x / 32.0f));
                int max_gx = static_cast<int>(std::floor((next_x + m_width - 0.01f) / 32.0f));
                int min_gy = static_cast<int>(std::floor(m_y / 32.0f));
                int max_gy = static_cast<int>(std::floor((m_y + m_height - 0.01f) / 32.0f));

                bool path_clear = true;
                for (int gy = min_gy; gy <= max_gy; ++gy)
                {
                    for (int gx = min_gx; gx <= max_gx; ++gx)
                    {
                        if (!lvl->is_tile_digged(gx, gy))
                        {
                            path_clear = false;
                            break;
                        }
                    }
                    if (!path_clear) break;
                }

                if (!path_clear)
                {
                    m_moving_right = !m_moving_right;
                    m_vx = m_moving_right ? m_speed : -m_speed;
                }
            }
        }

        zwodee::entity::tick();
    }

    void dragon::update_behavior(player* player)
    {
        m_player = player;
        if (!player)
        {
            return;
        }

        // AABB box check since Dragon is 96x64
        bool overlap_x = (player->get_x() < m_x + m_width) && (player->get_x() + player->get_width() > m_x);
        bool overlap_y = (player->get_y() < m_y + m_height) && (player->get_y() + player->get_height() > m_y);

        if (overlap_x && overlap_y)
        {
            // Direct insta-kill
            player->take_damage(999);
        }
    }

    void dragon::take_damage(int amount)
    {
        (void)amount;
        // Dragons are invincible!
    }
}
