#pragma once

#include "zwodee.hpp"

namespace digx
{
    class player;

    class enemy_base : public zwodee::entity
    {
    public:
        enemy_base(uint32_t network_id, const zwodee::texture* tex, float speed, int max_hp = 100);
        ~enemy_base() override = default;

        void tick() override;

        // Shared AI movement tick
        void update_enemy_movement(player* player);

    protected:
        float m_speed = 0.4f;
        
        // Grid target coordinates
        float m_target_x = 0.0f;
        float m_target_y = 0.0f;
        bool m_is_moving = false;

        // Current target player position lock
        float m_lock_x = 0.0f;
        float m_lock_y = 0.0f;
        bool m_has_lock = false;

        float m_dir_x = 0.0f;
        float m_dir_y = 0.0f;

        bool m_initialized_grid = false;

        bool is_direction_clear(class level* lvl, float dir_x, float dir_y) const;
        bool check_tile_clear(class level* lvl, int tx, int ty) const;
    };
}
