#include "items/digx-gold-coin.hpp"

namespace digx
{
    gold_coin::gold_coin(uint32_t network_id, const zwodee::texture* tex)
        : zwodee::entity(network_id, tex, 1)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void gold_coin::tick()
    {
    }
}
