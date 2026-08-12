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
        lamp(uint32_t network_id);

        void tick() override;

        void set_target_diamond(class diamond* target);
        class diamond* get_target_diamond() const;

        void set_reveals_all_diamonds(bool reveals_all) { m_reveals_all = reveals_all; }
        bool reveals_all_diamonds() const { return m_reveals_all; }

    private:
        class diamond* m_target_diamond = nullptr;
        bool m_reveals_all = false;
    };
}
