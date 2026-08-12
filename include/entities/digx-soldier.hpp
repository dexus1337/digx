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

        void render(zwodee::renderer& target_renderer, double alpha) override;
        zwodee::render_node get_render_node() const override;

        bool is_stunned() const;
        float get_stun_time_remaining() const;

    private:
        int m_stun_ticks = 0; // Number of ticks remaining for stun
        int m_stun_anim_ticks = 0; // Continuous animation ticks for shake oscillation
        bool m_fart_affected = false; // Prevents re-triggering stun during the same fart cloud
        player* m_player = nullptr;
    };
}
