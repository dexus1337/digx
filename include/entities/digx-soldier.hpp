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

        bool is_stunned() const;
        float get_stun_time_remaining() const;

    private:
        int m_stun_ticks = 0; // Number of ticks remaining for stun
        player* m_player = nullptr;
    };
}
