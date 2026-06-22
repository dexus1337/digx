#pragma once

/**
 * @file    digx-mummy.hpp
 * @author  dexus1337
 * @brief   Defines the mummy enemy class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "zwodee.hpp"

namespace digx
{
    class player;

    class mummy : public zwodee::entity
    {
    public:
        mummy(uint32_t network_id, const zwodee::texture* tex);

        void tick() override;

        void update_behavior(player* player);

        [[nodiscard]] bool is_spawned() const;
        void trigger_spawn();

    private:
        bool m_is_spawned = false;
        float m_speed = 1.0f;
    };
}
