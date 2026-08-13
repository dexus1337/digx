#pragma once

/**
 * @file    digx-vampire.hpp
 * @author  dexus1337
 * @brief   Defines the vampire enemy class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "zwodee.hpp"

namespace digx
{
    class player;

    class vampire : public zwodee::entity
    {
    public:
        vampire(uint32_t network_id);

        void tick() override;

        void update_behavior(player* player);

        void render(zwodee::renderer& target_renderer, double alpha) override;
        zwodee::render_node get_render_node() const override;

        inline bool                   is_active()                   const { return m_is_active; }
        inline bool                   is_neutralized()              const { return m_neutralized_ticks > 0; }

    private:
        bool m_is_active = false;
        int m_neutralized_ticks = 0;
        zwodee::render_node m_snorZ{};
    };
}
