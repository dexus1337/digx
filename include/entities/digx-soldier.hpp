#pragma once

#include "entities/digx-enemy-base.hpp"

namespace digx
{
    class player;

    class soldier : public enemy_base
    {
    public:
        soldier(uint32_t network_id, const zwodee::texture* front, const zwodee::texture* back, const zwodee::texture* side);

        void tick() override;

        void update_behavior(player* player);

        [[nodiscard]] bool is_stunned() const;
        [[nodiscard]] float get_stun_time_remaining() const;

    private:
        int m_stun_ticks = 0; // Number of ticks remaining for stun
        player* m_player = nullptr;
        const zwodee::texture* m_front_tex = nullptr;
        const zwodee::texture* m_back_tex = nullptr;
        const zwodee::texture* m_side_tex = nullptr;
    };
}
