#include "entities/digx-soldier.hpp"
#include "entities/digx-player.hpp"
#include "levels/digx-level.hpp"
#include "assets/texture-cache.hpp"

#include <cmath>

namespace digx
{
    soldier::soldier(uint32_t network_id)
        : enemy_base(network_id, texture_cache::get().soldier_front_tex.get(), 0.4f, 100)
    {
    }

    void soldier::tick()
    {
        auto& tc = texture_cache::get();

        if (is_stunned())
        {
            set_texture(tc.soldier_front_tex.get());
            enemy_base::tick();
            return;
        }

        enemy_base::tick();

        // Update active texture according to movement direction
        if (m_dir_y < 0.0f)
        {
            set_texture(tc.soldier_back_tex.get());
            set_flip_horizontal(false);
        }
        else if (m_dir_y > 0.0f)
        {
            set_texture(tc.soldier_front_tex.get());
            set_flip_horizontal(false);
        }
        else if (m_dir_x < 0.0f)
        {
            set_texture(tc.soldier_side_tex.get());
            set_flip_horizontal(true);
        }
        else if (m_dir_x > 0.0f)
        {
            set_texture(tc.soldier_side_tex.get());
            set_flip_horizontal(false);
        }
        else
        {
            set_texture(tc.soldier_front_tex.get());
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

        if (is_stunned())
        {
            return;
        }

        float dx = player->get_x() - m_x;
        float dy = player->get_y() - m_y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (update_fart_stun(player))
        {
            return;
        }

        if (dist < 16.0f)
        {
            player->take_damage(20);
        }

        update_enemy_movement(player);
    }
}
