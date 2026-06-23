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
#include "items/digx-exit-door.hpp"

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

        // Dig the tile the player is currently standing on
        float px = m_player->get_x();
        float py = m_player->get_y();
        int pgx = static_cast<int>(std::round(px / 32.0f));
        int pgy = static_cast<int>(std::round(py / 32.0f));
        if (pgx >= 0 && pgx < static_cast<int>(get_width()) &&
            pgy >= 0 && pgy < static_cast<int>(get_height()))
        {
            const auto& tiles = get_static_objects();
            size_t idx = static_cast<size_t>(pgy) * get_width() + static_cast<size_t>(pgx);
            if (idx < tiles.size() && (!tiles[idx] || (tiles[idx]->get_texture() != m_digged_tex.get() && !tiles[idx]->is_collidable())))
            {
                set_tile(pgx, pgy, 1, 0, m_digged_tex.get());
                // Make sure the digged tile is not collidable
                if (get_static_objects()[idx])
                {
                    get_static_objects()[idx]->set_collidable(false);
                }
            }
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
        px = m_player->get_x();
        py = m_player->get_y();

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
                    if (auto* audio = m_player->get_audio_manager())
                    {
                        audio->play_sound("coin_collected");
                    }
                }
                else if (auto* d = dynamic_cast<diamond*>(ent.get()))
                {
                    m_player->collect_diamond();
                    d->take_damage(999);
                    if (auto* audio = m_player->get_audio_manager())
                    {
                        audio->play_sound("diamond_collected");
                    }
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
                    if (auto* audio = m_player->get_audio_manager())
                    {
                        audio->play_sound("garlic_chew");
                    }
                }
                else if (auto* ob = dynamic_cast<onion_bulb*>(ent.get()))
                {
                    m_player->collect_onion();
                    ob->take_damage(999);
                    if (auto* audio = m_player->get_audio_manager())
                    {
                        audio->play_sound("onion_chew");
                    }
                }
                else if (auto* pa = dynamic_cast<pickaxe*>(ent.get()))
                {
                    m_player->obtain_pickaxe();
                    pa->take_damage(999);
                    if (auto* audio = m_player->get_audio_manager())
                    {
                        audio->play_sound("equip");
                    }
                }
                else if (auto* door = dynamic_cast<exit_door*>(ent.get()))
                {
                    if (door->is_open())
                    {
                        // Exit reached!
                        // e.g. complete level or win logic here
                    }
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

        // 3. Update movable stones (wiggle and fall)
        for (const auto& ent : get_entities())
        {
            if (auto* st = dynamic_cast<stone*>(ent.get()))
            {
                if (st->is_dead())
                {
                    continue;
                }

                // If not currently moving, wiggling, or falling, check if it should start wiggling
                if (!st->is_moving() && !st->is_falling() && st->get_wiggle_ticks() == 0)
                {
                    int sgx = static_cast<int>(std::round(st->get_x() / 32.0f));
                    int sgy = static_cast<int>(std::round(st->get_y() / 32.0f));

                    int below_gx = sgx;
                    int below_gy = sgy + 1;

                    bool tile_below_digged = is_tile_digged(below_gx, below_gy);
                    bool blocked_below = false;

                    for (const auto& other : get_entities())
                    {
                        if (other.get() != st && !other->is_dead())
                        {
                            if (dynamic_cast<stone*>(other.get()) || dynamic_cast<player*>(other.get()))
                            {
                                int ogx = static_cast<int>(std::round(other->get_x() / 32.0f));
                                int ogy = static_cast<int>(std::round(other->get_y() / 32.0f));
                                if (ogx == below_gx && ogy == below_gy)
                                {
                                    blocked_below = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (tile_below_digged && !blocked_below)
                    {
                        if (st->was_pushed())
                        {
                            st->set_falling(true);
                            st->clear_pushed();
                        }
                        else
                        {
                            st->start_wiggle();
                        }
                    }
                    else
                    {
                        st->clear_pushed();
                    }
                }

                // If it is falling and not currently moving, check if it should continue falling or stop
                if (st->is_falling() && !st->is_moving())
                {
                    int sgx = static_cast<int>(std::round(st->get_x() / 32.0f));
                    int sgy = static_cast<int>(std::round(st->get_y() / 32.0f));

                    int below_gx = sgx;
                    int below_gy = sgy + 1;

                    bool tile_below_digged = is_tile_digged(below_gx, below_gy);
                    bool blocked_below = false;
                    stone* other_stone = nullptr;

                    for (const auto& other : get_entities())
                    {
                        if (other.get() != st && !other->is_dead())
                        {
                            if (auto* ost = dynamic_cast<stone*>(other.get()))
                            {
                                int ogx = static_cast<int>(std::round(ost->get_x() / 32.0f));
                                int ogy = static_cast<int>(std::round(ost->get_y() / 32.0f));
                                if (ogx == below_gx && ogy == below_gy)
                                {
                                    blocked_below = true;
                                    other_stone = ost;
                                    break;
                                }
                            }
                            else if (dynamic_cast<player*>(other.get()))
                            {
                                int ogx = static_cast<int>(std::round(other->get_x() / 32.0f));
                                int ogy = static_cast<int>(std::round(other->get_y() / 32.0f));
                                if (ogx == below_gx && ogy == below_gy)
                                {
                                    blocked_below = true;
                                    break;
                                }
                            }
                        }
                    }

                    if (tile_below_digged && !blocked_below)
                    {
                        st->start_move(0.0f, 1.0f);
                    }
                    else
                    {
                        st->stop_falling();
                        if (m_player)
                        {
                            if (auto* audio = m_player->get_audio_manager())
                            {
                                audio->play_sound("stone_impact");
                            }
                        }

                        if (other_stone)
                        {
                            st->take_damage(999);
                            explode_stone(other_stone);
                        }
                    }
                }
            }
        }

        // Open door check
        if (!m_exit_open && m_player->get_gold_count() >= m_target_gold)
        {
            m_exit_open = true;
            for (const auto& ent : get_entities())
            {
                if (auto* door = dynamic_cast<exit_door*>(ent.get()))
                {
                    door->open();
                }
            }
            if (auto* audio = m_player->get_audio_manager())
            {
                audio->play_sound("level_done");
            }
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

        struct texture_cache
        {
            std::shared_ptr<zwodee::texture> player_shovel_tex;
            std::shared_ptr<zwodee::texture> player_shovel_running_tex;
            std::shared_ptr<zwodee::texture> player_shovel_running_up_tex;
            std::shared_ptr<zwodee::texture> player_shovel_running_down_tex;
            std::shared_ptr<zwodee::texture> player_pickaxe_tex;
            std::shared_ptr<zwodee::texture> player_pickaxe_running_tex;
            std::shared_ptr<zwodee::texture> player_pickaxe_running_up_tex;
            std::shared_ptr<zwodee::texture> player_pickaxe_running_down_tex;
            std::shared_ptr<zwodee::texture> stone_black_tex;
            std::shared_ptr<zwodee::texture> stone_grey_tex;
            std::shared_ptr<zwodee::texture> stone_brown_tex;
            std::shared_ptr<zwodee::texture> pickaxe_tex;
            std::shared_ptr<zwodee::texture> coin_tex;
            std::shared_ptr<zwodee::texture> door_closed_tex;
            std::shared_ptr<zwodee::texture> door_open_tex;
            std::vector<std::shared_ptr<zwodee::texture>> diamond_textures;
            std::shared_ptr<zwodee::texture> garlic_tex;
            std::shared_ptr<zwodee::texture> onion_tex;
            std::shared_ptr<zwodee::texture> digged_tex;
            std::array<std::shared_ptr<zwodee::texture>, 4> static_stone_textures;
            std::shared_ptr<zwodee::texture> bg_tex;
            std::shared_ptr<zwodee::texture> fallback_tex;
            
            std::shared_ptr<zwodee::texture> vampire_tex;
            std::shared_ptr<zwodee::texture> soldier_tex;
            std::shared_ptr<zwodee::texture> mummy_tex;
            std::shared_ptr<zwodee::texture> dragon_tex;

            bool loaded = false;

            void load_all(zwodee::renderer& r)
            {
                if (loaded) return;

                player_shovel_tex             = r.load_dds_texture("assets/textures/goblin-idle-shovel.dds");
                player_shovel_running_tex     = r.load_dds_texture("assets/textures/goblin-running-shovel.dds");
                player_shovel_running_up_tex  = r.load_dds_texture("assets/textures/goblin-running-up-shovel.dds");
                player_shovel_running_down_tex = r.load_dds_texture("assets/textures/goblin-running-down-shovel.dds");
                player_pickaxe_tex            = r.load_dds_texture("assets/textures/goblin-idle-pickaxe.dds");
                player_pickaxe_running_tex    = r.load_dds_texture("assets/textures/goblin-running-pickaxe.dds");
                player_pickaxe_running_up_tex  = r.load_dds_texture("assets/textures/goblin-running-up-pickaxe.dds");
                player_pickaxe_running_down_tex = r.load_dds_texture("assets/textures/goblin-running-down-pickaxe.dds");
                stone_black_tex               = r.load_dds_texture("assets/textures/stone-black.dds");
                stone_grey_tex                = r.load_dds_texture("assets/textures/stone-grey.dds");
                stone_brown_tex               = r.load_dds_texture("assets/textures/stone-brown.dds");
                pickaxe_tex                   = r.load_dds_texture("assets/textures/pickaxe.dds");
                coin_tex                      = r.load_dds_texture("assets/textures/coin.dds");
                door_closed_tex               = r.load_dds_texture("assets/textures/door-closed.dds");
                door_open_tex                 = r.load_dds_texture("assets/textures/door-open.dds");
                garlic_tex                    = r.load_dds_texture("assets/textures/garlic.dds");
                onion_tex                     = r.load_dds_texture("assets/textures/onion.dds");
                digged_tex                    = r.load_dds_texture("assets/textures/digged.dds");
                
                static_stone_textures[0]      = r.load_dds_texture("assets/textures/stone-1.dds");
                static_stone_textures[1]      = r.load_dds_texture("assets/textures/stone-2.dds");
                static_stone_textures[2]      = r.load_dds_texture("assets/textures/stone-3.dds");
                static_stone_textures[3]      = r.load_dds_texture("assets/textures/stone-4.dds");

                const std::vector<std::string> diamond_colors = { "green", "orange", "purple", "blue" };
                for (const auto& color : diamond_colors)
                {
                    if (auto tex = r.load_dds_texture("assets/textures/diamond-" + color + ".dds"))
                    {
                        diamond_textures.push_back(std::move(tex));
                    }
                }
                
                bg_tex                        = r.load_dds_texture("assets/textures/background.dds");
                fallback_tex                  = create_solid_color_texture(r, 32, 32, 255, 0, 0, 255);

                vampire_tex                   = r.load_dds_texture("assets/textures/vampire.dds");
                soldier_tex                   = r.load_dds_texture("assets/textures/soldier.dds");
                mummy_tex                     = r.load_dds_texture("assets/textures/mummy.dds");
                dragon_tex                    = r.load_dds_texture("assets/textures/dragon.dds");

                loaded = true;
            }
        };

        texture_cache g_textures;
    }

    void level::load_demo_level(zwodee::engine& engine)
    {
        auto& r = engine.get_renderer();
        auto& audio = engine.get_audio_manager();

        // Load game sounds
        for (int i = 1; i <= 8; ++i)
        {
            audio.load_sound("running_" + std::to_string(i), "assets/sounds/running/running-" + std::to_string(i) + ".wav");
        }
        audio.load_sound("coin_collected", "assets/sounds/coin-collected.wav");
        audio.load_sound("diamond_collected", "assets/sounds/diamond-collected.wav");
        audio.load_sound("equip", "assets/sounds/equip.wav");
        audio.load_sound("garlic_chew", "assets/sounds/garlic-chew.wav");
        audio.load_sound("onion_chew", "assets/sounds/onion-chew.wav");
        audio.load_sound("level_done", "assets/sounds/level-done.wav");
        audio.load_sound("stone_move", "assets/sounds/stone-move.wav");
        audio.load_sound("explosion", "assets/sounds/explosion.wav");
        audio.load_sound("stone_impact", "assets/sounds/stone-impact.wav");

        // Load digging sounds
        for (int i = 1; i <= 4; ++i)
        {
            audio.load_sound("shovel_dig_" + std::to_string(i), "assets/sounds/digging/shovel/shovel-" + std::to_string(i) + ".wav");
            audio.load_sound("pickaxe_dig_" + std::to_string(i), "assets/sounds/digging/pickaxe/pickaxe-" + std::to_string(i) + ".wav");
        }

        // Load textures inside level class (preloaded once in g_textures)
        g_textures.load_all(r);

        m_player_shovel_tex             = g_textures.player_shovel_tex;
        m_player_shovel_running_tex     = g_textures.player_shovel_running_tex;
        m_player_shovel_running_up_tex  = g_textures.player_shovel_running_up_tex;
        m_player_shovel_running_down_tex = g_textures.player_shovel_running_down_tex;
        m_player_pickaxe_tex            = g_textures.player_pickaxe_tex;
        m_player_pickaxe_running_tex    = g_textures.player_pickaxe_running_tex;
        m_player_pickaxe_running_up_tex  = g_textures.player_pickaxe_running_up_tex;
        m_player_pickaxe_running_down_tex = g_textures.player_pickaxe_running_down_tex;
        m_stone_black_tex               = g_textures.stone_black_tex;
        m_stone_grey_tex                = g_textures.stone_grey_tex;
        m_stone_brown_tex               = g_textures.stone_brown_tex;
        m_pickaxe_tex                   = g_textures.pickaxe_tex;
        m_coin_tex                      = g_textures.coin_tex;
        m_door_closed_tex               = g_textures.door_closed_tex;
        m_door_open_tex                 = g_textures.door_open_tex;
        m_garlic_tex                    = g_textures.garlic_tex;
        m_onion_tex                     = g_textures.onion_tex;
        m_digged_tex                    = g_textures.digged_tex;
        
        m_static_stone_textures[0]      = g_textures.static_stone_textures[0];
        m_static_stone_textures[1]      = g_textures.static_stone_textures[1];
        m_static_stone_textures[2]      = g_textures.static_stone_textures[2];
        m_static_stone_textures[3]      = g_textures.static_stone_textures[3];

        m_diamond_textures              = g_textures.diamond_textures;
        
        m_bg_tex                        = g_textures.bg_tex;
        m_fallback_tex                  = g_textures.fallback_tex;

        m_vampire_tex                   = g_textures.vampire_tex;
        m_soldier_tex                   = g_textures.soldier_tex;
        m_mummy_tex                     = g_textures.mummy_tex;
        m_dragon_tex                    = g_textures.dragon_tex;

        const zwodee::texture* shovel_idle = m_player_shovel_tex ? m_player_shovel_tex.get() : m_fallback_tex.get();
        const zwodee::texture* shovel_run = m_player_shovel_running_tex ? m_player_shovel_running_tex.get() : m_fallback_tex.get();
        const zwodee::texture* shovel_run_up = m_player_shovel_running_up_tex ? m_player_shovel_running_up_tex.get() : m_fallback_tex.get();
        const zwodee::texture* shovel_run_down = m_player_shovel_running_down_tex ? m_player_shovel_running_down_tex.get() : m_fallback_tex.get();
        const zwodee::texture* pickaxe_idle = m_player_pickaxe_tex ? m_player_pickaxe_tex.get() : m_fallback_tex.get();
        const zwodee::texture* pickaxe_run = m_player_pickaxe_running_tex ? m_player_pickaxe_running_tex.get() : m_fallback_tex.get();
        const zwodee::texture* pickaxe_run_up = m_player_pickaxe_running_up_tex ? m_player_pickaxe_running_up_tex.get() : m_fallback_tex.get();
        const zwodee::texture* pickaxe_run_down = m_player_pickaxe_running_down_tex ? m_player_pickaxe_running_down_tex.get() : m_fallback_tex.get();
        const zwodee::texture* stone_black_tex = m_stone_black_tex ? m_stone_black_tex.get() : m_fallback_tex.get();
        const zwodee::texture* stone_grey_tex = m_stone_grey_tex ? m_stone_grey_tex.get() : m_fallback_tex.get();
        const zwodee::texture* stone_brown_tex = m_stone_brown_tex ? m_stone_brown_tex.get() : m_fallback_tex.get();
        const zwodee::texture* pickaxe_tex = m_pickaxe_tex ? m_pickaxe_tex.get() : m_fallback_tex.get();
        const zwodee::texture* coint_text = m_coin_tex ? m_coin_tex.get() : m_fallback_tex.get();
        const zwodee::texture* door_closed = m_door_closed_tex ? m_door_closed_tex.get() : m_fallback_tex.get();
        const zwodee::texture* door_open = m_door_open_tex ? m_door_open_tex.get() : m_fallback_tex.get();
        const zwodee::texture* garlic_tex = m_garlic_tex ? m_garlic_tex.get() : m_fallback_tex.get();
        const zwodee::texture* onion_tex = m_onion_tex ? m_onion_tex.get() : m_fallback_tex.get();
        const zwodee::texture* fallback_tex_ptr = m_fallback_tex.get();

        if (m_bg_tex)
        {
            set_background_texture(m_bg_tex.get());
        }

        // Initialize outer borders and inner walls of static stones
        std::vector<std::pair<int, int>> static_stone_positions;
        
        // Left and Right outer borders
        for (int y = 2; y <= 32; ++y)
        {
            static_stone_positions.push_back({0, y});
            static_stone_positions.push_back({34, y});
        }
        // Top and Bottom outer borders (defining the 2-tile margin offset at top/bottom)
        for (int x = 1; x < 34; ++x)
        {
            static_stone_positions.push_back({x, 2});
            static_stone_positions.push_back({x, 32});
        }

        // Inner walls with gaps to allow player passage/exploration
        // Wall at y = 10
        for (int x = 1; x <= 22; ++x)
        {
            if (x != 10 && x != 11)
            {
                static_stone_positions.push_back({x, 10});
            }
        }
        // Wall at x = 22
        for (int y = 3; y <= 10; ++y)
        {
            if (y != 6)
            {
                static_stone_positions.push_back({22, y});
            }
        }
        // Wall at y = 17
        for (int x = 10; x <= 33; ++x)
        {
            if (x != 20 && x != 21)
            {
                static_stone_positions.push_back({x, 17});
            }
        }
        // Wall at x = 10
        for (int y = 17; y <= 25; ++y)
        {
            if (y != 21)
            {
                static_stone_positions.push_back({10, y});
            }
        }
        // Wall at y = 26
        for (int x = 1; x <= 24; ++x)
        {
            if (x != 12 && x != 13)
            {
                static_stone_positions.push_back({x, 26});
            }
        }

        // Set the static stone tiles
        for (const auto& pos : static_stone_positions)
        {
            int rand_tex_idx = std::rand() % 4;
            const zwodee::texture* stone_tex = m_static_stone_textures[rand_tex_idx] ? m_static_stone_textures[rand_tex_idx].get() : fallback_tex_ptr;
            set_tile(pos.first, pos.second, 2, 1, stone_tex);
            size_t idx = static_cast<size_t>(pos.second) * get_width() + static_cast<size_t>(pos.first);
            if (idx < get_static_objects().size() && get_static_objects()[idx])
            {
                get_static_objects()[idx]->set_collidable(true);
            }
        }

        auto dig_tile_at = [this, fallback_tex_ptr](int gx, int gy) {
            set_tile(gx, gy, 1, 0, m_digged_tex ? m_digged_tex.get() : fallback_tex_ptr);
            size_t idx = static_cast<size_t>(gy) * get_width() + static_cast<size_t>(gx);
            if (idx < get_static_objects().size() && get_static_objects()[idx])
            {
                get_static_objects()[idx]->set_collidable(false);
            }
        };

        // Dig a starting chamber (3x3 around the player spawn)
        for (int y = 3; y <= 5; ++y)
        {
            for (int x = 1; x <= 3; ++x)
            {
                dig_tile_at(x, y);
            }
        }

        // Add player goblin
        auto goblin = std::make_unique<player>(1, shovel_idle, shovel_run, shovel_run_up, shovel_run_down, pickaxe_idle, pickaxe_run, pickaxe_run_up, pickaxe_run_down, &audio);
        goblin->set_grid_bounds(get_width(), get_height());
        goblin->set_level(this);
        goblin->set_grid_position(2, 3);
        m_player = goblin.get();
        add_entity(std::move(goblin));

        // Dig spawns for movable stones (blocking paths)
        dig_tile_at(4, 3);
        auto stone1 = std::make_unique<stone>(2, stone_black_tex, stone::color_black);
        stone1->set_grid_position(4, 3);
        add_entity(std::move(stone1));

        dig_tile_at(23, 6);
        auto stone2 = std::make_unique<stone>(13, stone_grey_tex, stone::color_grey);
        stone2->set_grid_position(23, 6);
        add_entity(std::move(stone2));

        dig_tile_at(18, 12);
        auto stone3 = std::make_unique<stone>(14, stone_brown_tex, stone::color_brown);
        stone3->set_grid_position(18, 12);
        add_entity(std::move(stone3));

        // Additional stones for testing explosions and mechanics
        // Placed in patrolling corridors and enemy areas (away from starting spawn)
        
        // Vampire corridor area
        dig_tile_at(16, 5);
        auto stone_extra1 = std::make_unique<stone>(800, stone_grey_tex, stone::color_grey);
        stone_extra1->set_grid_position(16, 5);
        add_entity(std::move(stone_extra1));

        dig_tile_at(14, 5);
        auto stone_extra2 = std::make_unique<stone>(801, stone_black_tex, stone::color_black);
        stone_extra2->set_grid_position(14, 5);
        add_entity(std::move(stone_extra2));

        // Near the y=10 ledge/wall
        dig_tile_at(9, 9);
        auto stone_extra3 = std::make_unique<stone>(802, stone_brown_tex, stone::color_brown);
        stone_extra3->set_grid_position(9, 9);
        add_entity(std::move(stone_extra3));

        dig_tile_at(12, 9);
        auto stone_extra4 = std::make_unique<stone>(803, stone_black_tex, stone::color_black);
        stone_extra4->set_grid_position(12, 9);
        add_entity(std::move(stone_extra4));

        // Mummy corridor area
        dig_tile_at(3, 18);
        auto stone_extra5 = std::make_unique<stone>(804, stone_grey_tex, stone::color_grey);
        stone_extra5->set_grid_position(3, 18);
        add_entity(std::move(stone_extra5));

        dig_tile_at(7, 18);
        auto stone_extra6 = std::make_unique<stone>(805, stone_brown_tex, stone::color_brown);
        stone_extra6->set_grid_position(7, 18);
        add_entity(std::move(stone_extra6));

        // Dragon patrol path
        dig_tile_at(11, 21);
        auto stone_extra7 = std::make_unique<stone>(806, stone_grey_tex, stone::color_grey);
        stone_extra7->set_grid_position(11, 21);
        add_entity(std::move(stone_extra7));

        dig_tile_at(30, 21);
        auto stone_extra8 = std::make_unique<stone>(807, stone_black_tex, stone::color_black);
        stone_extra8->set_grid_position(30, 21);
        add_entity(std::move(stone_extra8));

        // Place items
        // Pickaxes
        auto p1 = std::make_unique<pickaxe>(8, pickaxe_tex);
        p1->set_grid_position(3, 3);
        add_entity(std::move(p1));

        auto p2 = std::make_unique<pickaxe>(108, pickaxe_tex);
        p2->set_grid_position(32, 3);
        add_entity(std::move(p2));

        auto p3 = std::make_unique<pickaxe>(109, pickaxe_tex);
        p3->set_grid_position(2, 28);
        add_entity(std::move(p3));

        // Coins (12 in total, target is 8)
        std::vector<std::pair<int, int>> coin_positions = {
            {5, 5}, {6, 5},
            {25, 4}, {26, 4}, {27, 4},
            {15, 12}, {16, 12}, {17, 12},
            {30, 20}, {31, 20},
            {5, 30}, {6, 30}
        };
        uint32_t coin_net_id = 300;
        for (const auto& pos : coin_positions)
        {
            auto coin = std::make_unique<gold_coin>(coin_net_id++, coint_text);
            coin->set_grid_position(pos.first, pos.second);
            add_entity(std::move(coin));
        }
        m_target_gold = 8;

        // Diamonds (8 in total)
        std::vector<std::pair<int, int>> diamond_positions = {
            {10, 3}, {11, 3},
            {26, 7}, {27, 7},
            {5, 12},
            {20, 22}, {21, 22},
            {15, 31}
        };
        uint32_t diamond_net_id = 400;
        for (const auto& pos : diamond_positions)
        {
            auto d = std::make_unique<diamond>(diamond_net_id++, get_random_diamond_texture());
            d->set_grid_position(pos.first, pos.second);
            add_entity(std::move(d));
        }

        // Garlic bulbs
        std::vector<std::pair<int, int>> garlic_positions = {
            {7, 3}, {28, 6}, {12, 11}, {25, 30}
        };
        uint32_t garlic_net_id = 500;
        for (const auto& pos : garlic_positions)
        {
            auto g = std::make_unique<garlic_bulb>(garlic_net_id++, garlic_tex);
            g->set_grid_position(pos.first, pos.second);
            add_entity(std::move(g));
        }

        // Onion bulbs
        std::vector<std::pair<int, int>> onion_positions = {
            {8, 3}, {29, 6}, {13, 11}, {26, 30}
        };
        uint32_t onion_net_id = 600;
        for (const auto& pos : onion_positions)
        {
            auto o = std::make_unique<onion_bulb>(onion_net_id++, onion_tex);
            o->set_grid_position(pos.first, pos.second);
            add_entity(std::move(o));
        }

        // Lamps (3 in total)
        std::vector<std::pair<int, int>> lamp_positions = {
            {12, 3}, {14, 12}, {28, 28}
        };
        uint32_t lamp_net_id = 700;
        for (const auto& pos : lamp_positions)
        {
            auto l = std::make_unique<lamp>(lamp_net_id++, fallback_tex_ptr);
            l->set_grid_position(pos.first, pos.second);
            add_entity(std::move(l));
        }

        // Spawn Enemies and dig their corridors
        // Vampire at (15, 6)
        for (int y = 5; y <= 7; ++y)
            for (int x = 14; x <= 16; ++x)
                dig_tile_at(x, y);
        const zwodee::texture* vampire_tex_ptr = m_vampire_tex ? m_vampire_tex.get() : fallback_tex_ptr;
        auto v1 = std::make_unique<vampire>(9, vampire_tex_ptr);
        v1->set_grid_position(15, 6);
        add_entity(std::move(v1));

        // Soldier patrolling top-right
        for (int y = 6; y <= 9; ++y)
            for (int x = 26; x <= 32; ++x)
                dig_tile_at(x, y);
        const zwodee::texture* soldier_tex_ptr = m_soldier_tex ? m_soldier_tex.get() : fallback_tex_ptr;
        auto s1 = std::make_unique<soldier>(10, soldier_tex_ptr);
        s1->set_grid_position(28, 8);
        add_entity(std::move(s1));

        // Mummy at bottom-left
        for (int y = 18; y <= 22; ++y)
            for (int x = 3; x <= 7; ++x)
                dig_tile_at(x, y);
        const zwodee::texture* mummy_tex_ptr = m_mummy_tex ? m_mummy_tex.get() : fallback_tex_ptr;
        auto m1 = std::make_unique<mummy>(11, mummy_tex_ptr);
        m1->set_grid_position(5, 20);
        add_entity(std::move(m1));

        // Dragon patrol path (horizontal corridor)
        for (int y = 20; y <= 21; ++y)
            for (int x = 11; x <= 30; ++x)
                dig_tile_at(x, y);
        const zwodee::texture* dragon_tex_ptr = m_dragon_tex ? m_dragon_tex.get() : fallback_tex_ptr;
        auto dr1 = std::make_unique<dragon>(12, dragon_tex_ptr);
        dr1->set_grid_position(12, 20);
        add_entity(std::move(dr1));

        // Add exit door at bottom right
        auto door = std::make_unique<exit_door>(15, door_closed, door_open);
        door->set_grid_position(32, 31);
        add_entity(std::move(door));
    }

    player* level::get_player() const
    {
        return m_player;
    }

    const zwodee::texture* level::get_random_diamond_texture() const
    {
        if (m_diamond_textures.empty())
        {
            return m_fallback_tex.get();
        }

        const auto idx = static_cast<std::size_t>(std::rand()) % m_diamond_textures.size();
        return m_diamond_textures[idx].get();
    }

    bool level::is_tile_digged(int gx, int gy) const
    {
        if (gx < 0 || gx >= static_cast<int>(get_width()) ||
            gy < 0 || gy >= static_cast<int>(get_height()))
        {
            return false;
        }

        const auto& tiles = get_static_objects();
        size_t idx = static_cast<size_t>(gy) * get_width() + static_cast<size_t>(gx);
        if (idx < tiles.size() && tiles[idx])
        {
            return tiles[idx]->get_texture() == m_digged_tex.get();
        }
        return false;
    }

    void level::dig_tile(int gx, int gy)
    {
        if (gx >= 0 && gx < static_cast<int>(get_width()) &&
            gy >= 0 && gy < static_cast<int>(get_height()))
        {
            set_tile(gx, gy, 1, 0, m_digged_tex.get());
            size_t idx = static_cast<size_t>(gy) * get_width() + static_cast<size_t>(gx);
            if (idx < get_static_objects().size() && get_static_objects()[idx])
            {
                get_static_objects()[idx]->set_collidable(false);
            }
        }
    }

    void level::explode_stone(stone* st)
    {
        if (!st || st->is_dead()) return;

        if (m_player)
        {
            if (auto* audio = m_player->get_audio_manager())
            {
                audio->play_sound("explosion");
            }
        }

        int sgx = static_cast<int>(std::round(st->get_x() / 32.0f));
        int sgy = static_cast<int>(std::round(st->get_y() / 32.0f));

        int radius = st->get_explosion_radius(); // 0 for brown, 1 for grey, 2 for black

        int min_x = sgx;
        int max_x = sgx;
        int min_y = sgy;
        int max_y = sgy;

        if (radius == 1) // Grey 2x2
        {
            min_x = sgx - 1; max_x = sgx;
            min_y = sgy - 1; max_y = sgy;
        }
        else if (radius == 2) // Black 3x3
        {
            min_x = sgx - 1; max_x = sgx + 1;
            min_y = sgy - 1; max_y = sgy + 1;
        }

        st->take_damage(999); // Destroy the stone itself

        // Destroy static stones and un-digged areas in the area
        for (int y = min_y; y <= max_y; ++y)
        {
            for (int x = min_x; x <= max_x; ++x)
            {
                if (x >= 0 && x < static_cast<int>(get_width()) &&
                    y >= 0 && y < static_cast<int>(get_height()))
                {
                    // Dig the tile (turns static stones and un-digged areas into digged tiles)
                    dig_tile(x, y);
                }
            }
        }

        // Kill entities in the area (excluding items)
        for (const auto& ent : get_entities())
        {
            if (ent.get() == st || ent->is_dead())
            {
                continue;
            }

            int egx = static_cast<int>(std::round(ent->get_x() / 32.0f));
            int egy = static_cast<int>(std::round(ent->get_y() / 32.0f));

            if (egx >= min_x && egx <= max_x && egy >= min_y && egy <= max_y)
            {
                // Verify if it is NOT an item
                bool is_item = dynamic_cast<gold_coin*>(ent.get()) ||
                               dynamic_cast<diamond*>(ent.get()) ||
                               dynamic_cast<lamp*>(ent.get()) ||
                               dynamic_cast<garlic_bulb*>(ent.get()) ||
                               dynamic_cast<onion_bulb*>(ent.get()) ||
                               dynamic_cast<pickaxe*>(ent.get()) ||
                               dynamic_cast<exit_door*>(ent.get());

                if (!is_item)
                {
                    ent->take_damage(999);
                }
            }
        }
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
            int max_page_x = static_cast<int>(std::max(0.0f, std::floor((get_width() * 32.0f - 1.0f) / page_width)));
            if (page_x < 0) page_x = 0;
            if (page_x > max_page_x) page_x = max_page_x;
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

            // Layer sorting to render the door above digged tiles but beneath the player
            std::stable_sort(snapshot.begin(), snapshot.end(), [this](const zwodee::render_node& a, const zwodee::render_node& b) {
                auto get_layer = [this](const zwodee::texture* tex) {
                    if (!tex) return 3;
                    if (tex == m_bg_tex.get()) return 0;
                    if (tex == m_digged_tex.get() || 
                        tex == m_static_stone_textures[0].get() || tex == m_static_stone_textures[1].get() ||
                        tex == m_static_stone_textures[2].get() || tex == m_static_stone_textures[3].get())
                    {
                        return 1;
                    }
                    if (tex == m_door_closed_tex.get() || tex == m_door_open_tex.get()) return 2;
                    return 3;
                };
                return get_layer(a.tex) < get_layer(b.tex);
            });
        }

        return snapshot;
    }
}
