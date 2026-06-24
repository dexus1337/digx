#pragma once

/**
 * @file    digx-lamp.hpp
 * @author  dexus1337
 * @brief   Defines the lamp class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "entities/entity.hpp"

namespace digx
{
    class lamp : public zwodee::entity
    {
    public:
        lamp(uint32_t network_id, const zwodee::texture* tex);

        void tick() override;

        void set_target_diamond(class diamond* target);
        [[nodiscard]] class diamond* get_target_diamond() const;

    private:
        class diamond* m_target_diamond = nullptr;
    };
}
