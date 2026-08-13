#include "entities/digx-mummy.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"
#include "assets/texture-cache.hpp"

#include <cmath>

namespace digx
{
    mummy::mummy(uint32_t network_id)
        : enemy_base(network_id, texture_cache::get().mummy_front_tex.get(), 0.4f / 3.0f, 150)
    {
    }

    void mummy::tick()
    {
        auto& tc = texture_cache::get();

        if (!m_is_spawned || is_dead() || is_stunned())
        {
            set_texture(tc.mummy_front_tex.get());
            enemy_base::tick();
            return;
        }

        enemy_base::tick();

        // Update active texture according to movement direction
        if (m_dir_y < 0.0f)
        {
            set_texture(tc.mummy_back_tex.get());
            set_flip_horizontal(false);
        }
        else if (m_dir_y > 0.0f)
        {
            set_texture(tc.mummy_front_tex.get());
            set_flip_horizontal(false);
        }
        else if (m_dir_x < 0.0f)
        {
            set_texture(tc.mummy_side_tex.get());
            set_flip_horizontal(true);
        }
        else if (m_dir_x > 0.0f)
        {
            set_texture(tc.mummy_side_tex.get());
            set_flip_horizontal(false);
        }
        else
        {
            set_texture(tc.mummy_front_tex.get());
            set_flip_horizontal(false);
        }
    }

    void mummy::update_behavior(player* player)
    {
        if (!player || !m_is_spawned || is_dead())
        {
            return;
        }

        if (is_stunned())
        {
            return;
        }

        if (update_fart_stun(player))
        {
            return;
        }

        float dx = player->get_x() - m_x;
        float dy = player->get_y() - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < 16.0f)
        {
            player->take_damage(25);
        }

        update_enemy_movement(player);
    }
}
