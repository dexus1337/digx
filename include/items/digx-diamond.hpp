#pragma once

/**
 * @file    digx-diamond.hpp
 * @author  dexus1337
 * @brief   Defines the diamond class representing score builder gems.
 * @version 1.0
 * @date    22.06.2026
 */

#include "entities/entity.hpp"

namespace digx
{
    class diamond : public zwodee::entity
    {
    public:
        diamond(uint32_t network_id, const zwodee::texture* tex);

        void tick() override;

        void set_revealed(bool reveal);
        [[nodiscard]] bool is_revealed() const;

        void render(zwodee::renderer& target_renderer, double alpha) override;

    private:
        bool m_is_revealed = false;
    };
}
