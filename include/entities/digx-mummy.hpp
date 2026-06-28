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
        mummy(uint32_t network_id, const zwodee::texture* front, const zwodee::texture* back, const zwodee::texture* side);

        void tick() override;

        void update_behavior(player* player);

        [[nodiscard]] bool is_spawned() const;
        void trigger_spawn();

    private:
        bool m_is_spawned = false;
        const zwodee::texture* m_front_tex = nullptr;
        const zwodee::texture* m_back_tex = nullptr;
        const zwodee::texture* m_side_tex = nullptr;
    };
}
