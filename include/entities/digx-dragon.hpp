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

    class dragon : public zwodee::entity
    {
    public:
        dragon(uint32_t network_id, const zwodee::texture* tex);

        void tick() override;

        void update_behavior(player* player);

        void take_damage(int amount) override; // Overridden to be invincible

    private:
        float m_speed = 2.0f;
        bool m_moving_right = true;
        float m_min_x = 0.0f;
        float m_max_x = 500.0f;
    };
}
