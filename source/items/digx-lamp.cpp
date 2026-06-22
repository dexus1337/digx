#include "items/digx-lamp.hpp"

namespace digx
{
    lamp::lamp(uint32_t network_id, const zwodee::texture* tex)
        : zwodee::entity(network_id, tex, 1)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void lamp::tick()
    {
    }
}
