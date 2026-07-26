#include "items/digx-gold-coin.hpp"
#include "assets/texture-cache.hpp"

namespace digx
{
    gold_coin::gold_coin(uint32_t network_id)
        : zwodee::entity(network_id, texture_cache::get().coin_tex.get(), 1)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void gold_coin::tick()
    {
    }
}
