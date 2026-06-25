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
        vampire(uint32_t network_id, const zwodee::texture* sleeping_tex, const zwodee::texture* triggered_tex);

        void tick() override;

        void update_behavior(player* player);

        [[nodiscard]] bool is_active() const;
        [[nodiscard]] bool is_neutralized() const;

    private:
        const zwodee::texture* m_sleeping_tex = nullptr;
        const zwodee::texture* m_triggered_tex = nullptr;
        bool m_is_active = false;
        bool m_is_neutralized = false;
    };
}
