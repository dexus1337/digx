#include "items/digx-pickaxe.hpp"

namespace digx
{
    pickaxe::pickaxe(uint32_t network_id, const zwodee::texture* tex)
        : zwodee::entity(network_id, tex, 1)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void pickaxe::tick()
    {
    }
}
