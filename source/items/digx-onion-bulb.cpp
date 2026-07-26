#include "items/digx-onion-bulb.hpp"
#include "assets/texture-cache.hpp"

namespace digx
{
    onion_bulb::onion_bulb(uint32_t network_id)
        : zwodee::entity(network_id, texture_cache::get().onion_tex.get(), 1)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void onion_bulb::tick()
    {
    }
}
