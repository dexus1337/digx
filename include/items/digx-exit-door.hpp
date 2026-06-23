#pragma once

/**
 * @file    digx-exit-door.hpp
 * @author  dexus1337
 * @brief   Defines the exit_door class representing the level exit.
 * @version 1.0
 * @date    23.06.2026
 */

#include "entities/entity.hpp"

namespace digx
{
    class exit_door : public zwodee::entity
    {
    public:
        exit_door(uint32_t network_id, const zwodee::texture* closed_tex, const zwodee::texture* open_tex)
            : zwodee::entity(network_id, closed_tex, 100),
              m_closed_tex(closed_tex),
              m_open_tex(open_tex)
        {
            configure_animator(1, 1, true);
        }

        void tick() override
        {
        }

        [[nodiscard]] bool is_open() const
        {
            return m_open;
        }

        void open()
        {
            m_open = true;
            if (m_open_tex)
            {
                set_texture(m_open_tex);
            }
        }

    private:
        const zwodee::texture* m_closed_tex = nullptr;
        const zwodee::texture* m_open_tex = nullptr;
        bool m_open = false;
    };
}
