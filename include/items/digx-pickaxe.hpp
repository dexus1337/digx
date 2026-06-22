#pragma once

/**
 * @file    digx-pickaxe.hpp
 * @author  dexus1337
 * @brief   Defines the pickaxe class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "entities/entity.hpp"

namespace digx
{
    class pickaxe : public zwodee::entity
    {
    public:
        pickaxe(uint32_t network_id, const zwodee::texture* tex);

        void tick() override;
    };
}
