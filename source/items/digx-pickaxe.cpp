#include "items/digx-pickaxe.hpp"
#include "assets/texture-cache.hpp"

namespace digx
{
    pickaxe::pickaxe(uint32_t network_id)
        : zwodee::entity(network_id, texture_cache::get().pickaxe_tex.get(), 1)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void pickaxe::tick()
    {
    }
}
