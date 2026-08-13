#pragma once

#include "entities/digx-enemy-base.hpp"

namespace digx
{
    class player;

    class soldier : public enemy_base
    {
    public:
        soldier(uint32_t network_id);

        void tick() override;

        void update_behavior(player* player);

        inline float                  get_stun_time_remaining()     const { return static_cast<float>(m_stun_ticks) / 128.0f; }

    private:
        player* m_player = nullptr;
    };
}
