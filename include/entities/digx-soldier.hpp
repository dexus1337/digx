#pragma once

/**
 * @file    digx-soldier.hpp
 * @author  dexus1337
 * @brief   Defines the soldier enemy class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "zwodee.hpp"

namespace digx
{
    class player;

    class soldier : public zwodee::entity
    {
    public:
        soldier(uint32_t network_id, const zwodee::texture* tex);

        void tick() override;

        void update_behavior(player* player);

        [[nodiscard]] bool is_stunned() const;
        [[nodiscard]] float get_stun_time_remaining() const;

    private:
        float m_patrol_speed = 0.4f;
        int m_stun_ticks = 0; // Number of ticks remaining for stun
        player* m_player = nullptr;
        int m_dir_change_ticks = 0;
    };
}
