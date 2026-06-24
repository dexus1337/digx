#pragma once

/**
 * @file    digx-diamond.hpp
 * @author  dexus1337
 * @brief   Defines the diamond class representing score builder gems.
 * @version 1.0
 * @date    22.06.2026
 */

#include "entities/entity.hpp"

namespace zwodee
{
    class tile_level;
}

namespace digx
{
    class diamond : public zwodee::entity
    {
    public:
        diamond(uint32_t network_id, const zwodee::texture* tex, const zwodee::texture* blink_tex);

        void tick() override;

        void set_revealed(bool reveal);
        [[nodiscard]] bool is_revealed() const;

        void set_permanently_revealed(bool perm);
        [[nodiscard]] bool is_permanently_revealed() const;

        void set_level(zwodee::tile_level* lvl);

        void render(zwodee::renderer& target_renderer, double alpha) override;
        zwodee::render_node get_render_node() const override;

    private:
        bool m_is_revealed = false;
        bool m_permanently_revealed = false;
        const zwodee::texture* m_blink_tex = nullptr;
        zwodee::tile_level* m_level = nullptr;
        float m_blink_timer = 0.0f;
    };
}
