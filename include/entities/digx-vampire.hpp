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

        bool is_active() const;
        bool is_neutralized() const;

    private:
        bool m_is_active = false;
        bool m_is_neutralized = false;
        zwodee::render_node m_snorZ{};
    };
}
