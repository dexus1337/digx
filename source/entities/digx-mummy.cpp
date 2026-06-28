#include "entities/digx-mummy.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"

#include <cmath>

namespace digx
{
    mummy::mummy(uint32_t network_id, const zwodee::texture* front, const zwodee::texture* back, const zwodee::texture* side)
        : enemy_base(network_id, front, 0.4f / 3.0f, 150),
          m_front_tex(front),
          m_back_tex(back),
          m_side_tex(side)
    {
    }

    void mummy::tick()
    {
        if (!m_is_spawned || is_dead())
        {
            m_vx = 0.0f;
            m_vy = 0.0f;
            m_is_moving = false;
            set_texture(m_front_tex);
            return;
        }

        enemy_base::tick();

        // Update active texture according to movement direction
        if (m_dir_y < 0.0f)
        {
            set_texture(m_back_tex);
            set_flip_horizontal(false);
        }
        else if (m_dir_y > 0.0f)
        {
            set_texture(m_front_tex);
            set_flip_horizontal(false);
        }
        else if (m_dir_x < 0.0f)
        {
            set_texture(m_side_tex);
            set_flip_horizontal(true);
        }
        else if (m_dir_x > 0.0f)
        {
            set_texture(m_side_tex);
            set_flip_horizontal(false);
        }
        else
        {
            set_texture(m_front_tex);
            set_flip_horizontal(false);
        }
    }

    void mummy::update_behavior(player* player)
    {
        if (!player || !m_is_spawned || is_dead())
        {
            return;
        }

        // Check if player takes damage on collision
        float dx = player->get_x() - m_x;
        float dy = player->get_y() - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 16.0f)
        {
            player->take_damage(25);
        }

        // Shared grid movement logic
        update_enemy_movement(player);
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
