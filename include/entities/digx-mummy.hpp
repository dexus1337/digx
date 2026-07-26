#pragma once

/**
 * @file    digx-mummy.hpp
 * @author  dexus1337
 * @brief   Defines the mummy enemy class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "entities/digx-enemy-base.hpp"

namespace digx
{
    class player;

    class mummy : public enemy_base
    {
    public:
        mummy(uint32_t network_id);

        void tick() override;

        void update_behavior(player* player);

        bool is_spawned() const;
        void trigger_spawn();

    private:
        bool m_is_spawned = false;
    };
}
