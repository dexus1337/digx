#include "items/digx-diamond.hpp"

namespace digx
{
    diamond::diamond(uint32_t network_id, const zwodee::texture* tex)
        : zwodee::entity(network_id, tex, 1)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void diamond::tick()
    {
    }

    void diamond::set_revealed(bool reveal)
    {
        m_is_revealed = reveal;
    }

    bool diamond::is_revealed() const
    {
        return m_is_revealed;
    }

    void diamond::render(zwodee::renderer& target_renderer, double alpha)
    {
        if (!m_is_revealed)
        {
            return;
        }
        zwodee::entity::render(target_renderer, alpha);
    }
}
