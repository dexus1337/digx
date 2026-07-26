#include "items/digx-garlic-bulb.hpp"
#include "assets/texture-cache.hpp"

namespace digx
{
    garlic_bulb::garlic_bulb(uint32_t network_id)
        : zwodee::entity(network_id, texture_cache::get().garlic_tex.get(), 1)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void garlic_bulb::tick()
    {
    }
}
