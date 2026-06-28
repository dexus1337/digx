#include "entities/digx-soldier.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"

#include <cmath>

namespace digx
{
    soldier::soldier(uint32_t network_id, const zwodee::texture* tex)
        : enemy_base(network_id, tex, 0.4f, 100)
    {
    }

    void soldier::tick()
    {
        if (m_stun_ticks > 0)
        {
            m_stun_ticks--;
            m_vx = 0.0f;
            m_vy = 0.0f;
            m_is_moving = false;
            zwodee::entity::tick();
            return;
        }

        enemy_base::tick();

        if (m_dir_x < 0.0f)
        {
            set_flip_horizontal(true);
        }
        else if (m_dir_x > 0.0f)
        {
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

        // Check if player farted (garlic) and is in proximity (e.g. 96 pixels)
        if (player->get_fart_active_time() > 0.0f && dist <= 96.0f)
        {
            // Stunned!
            m_stun_ticks = static_cast<int>(player->get_fart_active_time() * 128.0f);
            m_vx = 0.0f;
            m_vy = 0.0f;
            m_is_moving = false;
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
            m_is_moving = false;
            return;
        }

        // Shared grid movement logic
        update_enemy_movement(player);
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
