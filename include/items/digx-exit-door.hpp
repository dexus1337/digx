#pragma once

/**
 * @file    digx-exit-door.hpp
 * @author  dexus1337
 * @brief   Defines the exit_door class representing the level exit.
 * @version 1.0
 * @date    23.06.2026
 */

#include "entities/entity.hpp"
#include "assets/texture-cache.hpp"

namespace digx
{
    class exit_door : public zwodee::entity
    {
    public:
        exit_door(uint32_t network_id)
            : zwodee::entity(network_id, texture_cache::get().door_closed_tex.get(), 100)
        {
            configure_animator(1, 1, true);
        }

        void tick() override
        {
        }

        bool is_open() const
        {
            return m_open;
        }

        void open()
        {
            m_open = true;
            if (auto open_tex = texture_cache::get().door_open_tex.get())
            {
                set_texture(open_tex);
            }
        }

    private:
        bool m_open = false;
    };
}
