#include "entities/digx-soldier.hpp"
#include "entities/digx-player.hpp"
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
            zwodee::entity::tick();
            return;
        }

        // Patrol behavior: move left/right
        m_vx = m_moving_right ? m_patrol_speed : -m_patrol_speed;
        
        zwodee::entity::tick();

        // Simple bounce logic
        if (m_x >= m_patrol_max_x)
        {
            m_x = m_patrol_max_x;
            m_moving_right = false;
        }
        else if (m_x <= m_patrol_min_x)
        {
            m_x = m_patrol_min_x;
            m_moving_right = true;
        }
    }

    void soldier::update_behavior(player* player)
    {
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
        }

        // If not stunned, player takes damage on collision
        if (m_stun_ticks == 0 && dist < 16.0f)
        {
            player->take_damage(20);
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
