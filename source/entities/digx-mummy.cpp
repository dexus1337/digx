#include "entities/digx-mummy.hpp"
#include "entities/digx-player.hpp"
#include <cmath>

namespace digx
{
    mummy::mummy(uint32_t network_id, const zwodee::texture* tex)
        : zwodee::entity(network_id, tex, 150)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void mummy::tick()
    {
        if (!m_is_spawned || is_dead())
        {
            m_vx = 0.0f;
            m_vy = 0.0f;
            return;
        }

        zwodee::entity::tick();
    }

    void mummy::update_behavior(player* player)
    {
        if (!player || !m_is_spawned || is_dead())
        {
            return;
        }

        // Move towards the player slowly
        float dx = player->get_x() - m_x;
        float dy = player->get_y() - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 0.0f)
        {
            m_vx = (dx / dist) * m_speed;
            m_vy = (dy / dist) * m_speed;
        }

        if (dist < 16.0f)
        {
            player->take_damage(25);
        }
    }

    bool mummy::is_spawned() const
    {
        return m_is_spawned;
    }

    void mummy::trigger_spawn()
    {
        m_is_spawned = true;
    }
}
