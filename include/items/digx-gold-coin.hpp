#pragma once

/**
 * @file    digx-gold-coin.hpp
 * @author  dexus1337
 * @brief   Defines the gold_coin class representing collectable gold items.
 * @version 1.0
 * @date    22.06.2026
 */

#include "entities/entity.hpp"

namespace digx
{
    class gold_coin : public zwodee::entity
    {
    public:
        gold_coin(uint32_t network_id, const zwodee::texture* tex);

        void tick() override;
    };
}
