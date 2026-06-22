#pragma once

/**
 * @file    digx-onion-bulb.hpp
 * @author  dexus1337
 * @brief   Defines the onion_bulb class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "entities/entity.hpp"

namespace digx
{
    class onion_bulb : public zwodee::entity
    {
    public:
        onion_bulb(uint32_t network_id, const zwodee::texture* tex);

        void tick() override;
    };
}
