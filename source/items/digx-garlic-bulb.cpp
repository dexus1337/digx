#include "items/digx-garlic-bulb.hpp"

namespace digx
{
    garlic_bulb::garlic_bulb(uint32_t network_id, const zwodee::texture* tex)
        : zwodee::entity(network_id, tex, 1)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void garlic_bulb::tick()
    {
    }
}
