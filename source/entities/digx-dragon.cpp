#include "entities/digx-dragon.hpp"
#include "entities/digx-player.hpp"
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

        zwodee::entity::tick();

        if (m_x >= m_max_x)
        {
            m_x = m_max_x;
            m_moving_right = false;
        }
        else if (m_x <= m_min_x)
        {
            m_x = m_min_x;
            m_moving_right = true;
        }
    }

    void dragon::update_behavior(player* player)
    {
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
