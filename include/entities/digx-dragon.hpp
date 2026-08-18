#pragma once

/**
 * @file    digx-dragon.hpp
 * @author  dexus1337
 * @brief   Defines the dragon enemy class (invincible horizontal hazard).
 * @version 1.0
 * @date    22.06.2026
 */

#include "zwodee.hpp"

namespace digx
{
    class player;
    class level;

    class dragon : public zwodee::entity
    {
    public:
        dragon(uint32_t network_id);

        void tick() override;

        void update_behavior(player* player);
        void set_level(level* lvl) { m_level = lvl; }

        void take_damage(int amount) override; // Overridden to be invincible

    private:
        float m_speed = 0.4f;
        bool m_moving_right = true;
        player* m_player = nullptr;
        level* m_level = nullptr;
    };
}
