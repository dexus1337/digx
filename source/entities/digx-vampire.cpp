#include "entities/digx-vampire.hpp"
#include "entities/digx-player.hpp"
#include <cmath>

namespace digx
{
    vampire::vampire(uint32_t network_id, const zwodee::texture* tex)
        : zwodee::entity(network_id, tex, 50)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void vampire::tick()
    {
    }

    void vampire::update_behavior(player* player)
    {
        if (!player || m_is_neutralized)
        {
            return;
        }

        float dx = player->get_x() - m_x;
        float dy = player->get_y() - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= 64.0f)
        {
            m_is_active = true;
        }

        if (m_is_active && player->get_breath_active_time() > 0.0f)
        {
            m_is_neutralized = true;
            m_is_active = false;
            return;
        }

        if (m_is_active && dist < 16.0f)
        {
            player->take_damage(999);
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
