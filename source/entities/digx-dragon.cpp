#include "entities/digx-dragon.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"
#include "assets/texture-cache.hpp"

#include <cmath>
#include <cstdlib>

namespace digx
{
    dragon::dragon(uint32_t network_id)
        : zwodee::entity(network_id, (std::rand() % 2 == 0) ? texture_cache::get().dragon_red_tex.get() : texture_cache::get().dragon_green_tex.get(), 999999)
    {
        // 3x2 grid size (96x64 pixels)
        m_width = 96.0f;
        m_height = 64.0f;
    }

    void dragon::tick()
    {
        set_flip_horizontal(!m_moving_right);

        if (m_level)
        {
            int min_gy = static_cast<int>(std::floor(m_y / 32.0f));
            int max_gy = static_cast<int>(std::floor((m_y + m_height - 0.01f) / 32.0f));

            if (m_moving_right)
            {
                float next_x = m_x + m_speed;
                int check_gx = static_cast<int>(std::floor((next_x + m_width - 0.01f) / 32.0f));

                bool path_clear = true;
                if (check_gx >= static_cast<int>(m_level->get_width()))
                {
                    path_clear = false;
                }
                else
                {
                    for (int gy = min_gy; gy <= max_gy; ++gy)
                    {
                        if (gy < 0 || gy >= static_cast<int>(m_level->get_height()) || !m_level->is_tile_digged(check_gx, gy))
                        {
                            path_clear = false;
                            break;
                        }
                    }
                }

                if (path_clear)
                {
                    m_vx = m_speed;
                }
                else
                {
                    m_moving_right = false;
                    m_vx = -m_speed;
                }
            }
            else
            {
                float next_x = m_x - m_speed;
                int check_gx = static_cast<int>(std::floor(next_x / 32.0f));

                bool path_clear = true;
                if (check_gx < 0)
                {
                    path_clear = false;
                }
                else
                {
                    for (int gy = min_gy; gy <= max_gy; ++gy)
                    {
                        if (gy < 0 || gy >= static_cast<int>(m_level->get_height()) || !m_level->is_tile_digged(check_gx, gy))
                        {
                            path_clear = false;
                            break;
                        }
                    }
                }

                if (path_clear)
                {
                    m_vx = -m_speed;
                }
                else
                {
                    m_moving_right = true;
                    m_vx = m_speed;
                }
            }
        }
        else
        {
            m_vx = m_moving_right ? m_speed : -m_speed;
        }

        m_vy = 0.0f;
        zwodee::entity::tick();
    }

    void dragon::update_behavior(player* player)
    {
        m_player = player;
        if (player && !m_level)
        {
            m_level = dynamic_cast<digx::level*>(player->get_level());
        }

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
