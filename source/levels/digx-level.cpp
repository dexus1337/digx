#include "levels/digx-level.hpp"
#include "entities/digx-player.hpp"
#include "entities/digx-stone.hpp"
#include "entities/digx-vampire.hpp"
#include "entities/digx-soldier.hpp"
#include "entities/digx-mummy.hpp"
#include "entities/digx-dragon.hpp"
#include "items/digx-gold-coin.hpp"
#include "items/digx-diamond.hpp"
#include "items/digx-lamp.hpp"
#include "items/digx-garlic-bulb.hpp"
#include "items/digx-onion-bulb.hpp"
#include "items/digx-pickaxe.hpp"

#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>

namespace digx
{
    level::level(uint32_t width, uint32_t height)
        : zwodee::tile_level(width, height)
    {
    }

    void level::on_enter()
    {
        zwodee::tile_level::on_enter();
    }

    void level::on_exit()
    {
        zwodee::tile_level::on_exit();
    }

    void level::tick()
    {
        zwodee::tile_level::tick();

        if (!m_player)
        {
            return;
        }

        // 1. Update active items / lamp timer
        if (m_lamp_timer > 0.0f)
        {
            m_lamp_timer -= 1.0f / 128.0f;
            if (m_lamp_timer <= 0.0f)
            {
                // Disable diamond visual reveals
                for (const auto& ent : get_entities())
                {
                    if (auto* d = dynamic_cast<diamond*>(ent.get()))
                    {
                        d->set_revealed(false);
                    }
                }
            }
        }

        // 2. Perform entity checks and interactions
        float px = m_player->get_x();
        float py = m_player->get_y();

        // Check if player reaches the exit door when it's open
        if (m_exit_open)
        {
            float dx = px - m_exit_x;
            float dy = py - m_exit_y;
            if (std::sqrt(dx * dx + dy * dy) < 24.0f)
            {
                // Exit reached!
            }
        }

        // Handle interactions
        for (const auto& ent : get_entities())
        {
            if (ent.get() == m_player)
            {
                continue;
            }

            if (ent->is_dead())
            {
                continue;
            }

            bool overlap = m_player->collides_with(*ent);

            if (overlap)
            {
                // Collision!
                if (auto* gc = dynamic_cast<gold_coin*>(ent.get()))
                {
                    m_player->collect_gold();
                    gc->take_damage(999); // "collect" it
                }
                else if (auto* d = dynamic_cast<diamond*>(ent.get()))
                {
                    m_player->collect_diamond();
                    d->take_damage(999);
                }
                else if (auto* dl = dynamic_cast<lamp*>(ent.get()))
                {
                    dl->take_damage(999);
                    m_lamp_timer = 10.0f; // 10 seconds reveal
                    for (const auto& other : get_entities())
                    {
                        if (auto* other_d = dynamic_cast<diamond*>(other.get()))
                        {
                            other_d->set_revealed(true);
                        }
                    }
                }
                else if (auto* gb = dynamic_cast<garlic_bulb*>(ent.get()))
                {
                    m_player->collect_garlic();
                    gb->take_damage(999);
                }
                else if (auto* ob = dynamic_cast<onion_bulb*>(ent.get()))
                {
                    m_player->collect_onion();
                    ob->take_damage(999);
                }
                else if (auto* pa = dynamic_cast<pickaxe*>(ent.get()))
                {
                    m_player->obtain_pickaxe();
                    pa->take_damage(999);
                }
            }

            // Update AI behaviors
            if (auto* v = dynamic_cast<vampire*>(ent.get()))
            {
                v->update_behavior(m_player);
            }
            else if (auto* s = dynamic_cast<soldier*>(ent.get()))
            {
                s->update_behavior(m_player);
            }
            else if (auto* m = dynamic_cast<mummy*>(ent.get()))
            {
                // Trigger mummy if player gets close, then update behavior
                if (!m->is_spawned())
                {
                    float dx = px - m->get_x();
                    float dy = py - m->get_y();
                    if (std::sqrt(dx * dx + dy * dy) < 120.0f)
                    {
                        m->trigger_spawn();
                    }
                }
                m->update_behavior(m_player);
            }
            else if (auto* dr = dynamic_cast<dragon*>(ent.get()))
            {
                dr->update_behavior(m_player);
            }
        }

        // Open door check
        if (!m_exit_open && m_player->get_gold_count() >= m_target_gold)
        {
            m_exit_open = true;
        }
    }

    void level::render(zwodee::renderer& target_renderer, double alpha)
    {
        zwodee::tile_level::render(target_renderer, alpha);
    }

    namespace
    {
        std::unique_ptr<zwodee::texture> create_solid_color_texture(zwodee::renderer& r, int w, int h, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha)
        {
            SDL_Texture* sdl_tex = SDL_CreateTexture(r.get_raw_renderer(), SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, w, h);
            if (!sdl_tex)
            {
                return nullptr;
            }
            
            // Pixel value (RGBA8888 format in little-endian is AABBGGRR)
            uint32_t pixel = (red) | (green << 8) | (blue << 16) | (alpha << 24);
            std::vector<uint32_t> pixels(w * h, pixel);
            
            SDL_UpdateTexture(sdl_tex, nullptr, pixels.data(), w * 4);
            SDL_SetTextureBlendMode(sdl_tex, SDL_BLENDMODE_BLEND);
            
            return std::make_unique<zwodee::texture>(sdl_tex, w, h);
        }
    }

    void level::load_demo_level(zwodee::renderer& r)
    {
        // Load textures inside level class
        m_player_shovel_tex = r.load_dds_texture("assets/goblin-idle-shovel.dds");
        m_player_pickaxe_tex = r.load_dds_texture("assets/goblin-idle-pickaxe.dds");
        m_stone_black_tex = r.load_dds_texture("assets/stone-black.dds");
        m_stone_grey_tex = r.load_dds_texture("assets/stone-grey.dds");
        m_stone_brown_tex = r.load_dds_texture("assets/stone-brown.dds");
        m_pickaxe_tex = r.load_dds_texture("assets/pickaxe.dds");
        m_bg_tex = r.load_dds_texture("assets/background.dds");
        m_fallback_tex = create_solid_color_texture(r, 32, 32, 255, 0, 0, 255);

        const zwodee::texture* player_tex = m_player_shovel_tex ? m_player_shovel_tex.get() : m_fallback_tex.get();
        const zwodee::texture* pickaxe_player_tex = m_player_pickaxe_tex ? m_player_pickaxe_tex.get() : m_fallback_tex.get();
        const zwodee::texture* stone_black_tex = m_stone_black_tex ? m_stone_black_tex.get() : m_fallback_tex.get();
        const zwodee::texture* stone_grey_tex = m_stone_grey_tex ? m_stone_grey_tex.get() : m_fallback_tex.get();
        const zwodee::texture* stone_brown_tex = m_stone_brown_tex ? m_stone_brown_tex.get() : m_fallback_tex.get();
        const zwodee::texture* pickaxe_tex = m_pickaxe_tex ? m_pickaxe_tex.get() : m_fallback_tex.get();
        const zwodee::texture* fallback_tex_ptr = m_fallback_tex.get();

        if (m_bg_tex)
        {
            set_background_texture(m_bg_tex.get());
        }

        // Add player goblin
        auto goblin = std::make_unique<player>(1, player_tex, pickaxe_player_tex);
        goblin->set_grid_bounds(get_width(), get_height());
        goblin->set_level(this);
        goblin->set_grid_position(2, 2);
        m_player = goblin.get();
        add_entity(std::move(goblin));

        // Add stones with individual textures
        auto stone1 = std::make_unique<stone>(2, stone_black_tex, stone::color_black);
        stone1->set_grid_position(4, 2);
        add_entity(std::move(stone1));

        auto stone2 = std::make_unique<stone>(13, stone_grey_tex, stone::color_grey);
        stone2->set_grid_position(5, 2);
        add_entity(std::move(stone2));

        auto stone3 = std::make_unique<stone>(14, stone_brown_tex, stone::color_brown);
        stone3->set_grid_position(6, 2);
        add_entity(std::move(stone3));

        // Add items
        auto p1 = std::make_unique<pickaxe>(8, pickaxe_tex);
        p1->set_grid_position(3, 2);
        add_entity(std::move(p1));

        auto coin1 = std::make_unique<gold_coin>(3, fallback_tex_ptr);
        coin1->set_grid_position(8, 2);
        add_entity(std::move(coin1));
        m_target_gold = 1;

        auto d1 = std::make_unique<diamond>(4, fallback_tex_ptr);
        d1->set_grid_position(9, 2);
        add_entity(std::move(d1));

        auto lamp1 = std::make_unique<lamp>(5, fallback_tex_ptr);
        lamp1->set_grid_position(10, 2);
        add_entity(std::move(lamp1));

        auto g1 = std::make_unique<garlic_bulb>(6, fallback_tex_ptr);
        g1->set_grid_position(11, 2);
        add_entity(std::move(g1));

        auto o1 = std::make_unique<onion_bulb>(7, fallback_tex_ptr);
        o1->set_grid_position(12, 2);
        add_entity(std::move(o1));

        // Add enemies
        auto v1 = std::make_unique<vampire>(9, fallback_tex_ptr);
        v1->set_grid_position(10, 4);
        add_entity(std::move(v1));

        auto s1 = std::make_unique<soldier>(10, fallback_tex_ptr);
        s1->set_grid_position(12, 4);
        add_entity(std::move(s1));

        auto m1 = std::make_unique<mummy>(11, fallback_tex_ptr);
        m1->set_grid_position(2, 6);
        add_entity(std::move(m1));

        auto dr1 = std::make_unique<dragon>(12, fallback_tex_ptr);
        dr1->set_grid_position(4, 6);
        add_entity(std::move(dr1));

        // Exit coordinate
        m_exit_x = 400.0f;
        m_exit_y = 400.0f;
    }

    player* level::get_player() const
    {
        return m_player;
    }

    zwodee::render_snapshot level::get_render_snapshot(int display_w, int display_h) const
    {
        zwodee::render_snapshot snapshot = zwodee::tile_level::get_render_snapshot(display_w, display_h);

        if (m_player)
        {
            float px = m_player->get_x();
            float py = m_player->get_y();

            // Horizontal page flipping based on active display width
            float page_width = static_cast<float>(display_w);
            int page_x = static_cast<int>(std::floor(px / page_width));
            float camera_x = page_x * page_width;

            // Vertical smooth centering based on active display height
            float half_height = static_cast<float>(display_h) / 2.0f;
            float camera_y = py - half_height;
            float max_camera_y = static_cast<float>(get_height() * 32) - static_cast<float>(display_h);
            if (max_camera_y < 0.0f) max_camera_y = 0.0f;
            if (camera_y < 0.0f) camera_y = 0.0f;
            if (camera_y > max_camera_y) camera_y = max_camera_y;

            // Apply the camera offset to all rendering positions
            for (auto& node : snapshot)
            {
                node.x -= camera_x;
                node.y -= camera_y;
            }
        }

        return snapshot;
    }
}
