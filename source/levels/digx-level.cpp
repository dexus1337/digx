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
    level::level(uint32_t width, uint32_t height, int level_number)
        : zwodee::tile_level(width, height), m_level_number(level_number)
    {
        m_target_darkness = std::max(0.2f, 1.0f - (level_number - 1) * 0.15f);
        m_current_darkness = 1.0f; // Smoothly fades to target darkness on entry
    }

    void level::on_enter()
    {
        zwodee::tile_level::on_enter();
    }

    void level::on_exit()
    {
        zwodee::tile_level::on_exit();
    }

    void level::set_player_input(const zwodee::input_state& input)
    {
        m_last_input = m_current_input;
        m_current_input = input;

        // Toggle pause when escape key (action_2) is pressed
        if (m_current_input.is_down(zwodee::input_state::action_2) && !m_last_input.is_down(zwodee::input_state::action_2))
        {
            m_is_paused = !m_is_paused;
            m_in_settings = false;
            m_pause_selected_index = 0;
        }

        if (!m_is_paused)
        {
            zwodee::tile_level::set_player_input(input);
        }
    }

    void level::tick()
    {
        if (!m_player) return;

        if (m_fart_effect_ticks > 0)
        {
            m_fart_effect_ticks--;
        }

        if (m_game_over)
        {
            float screen_w = static_cast<float>(m_engine->get_window().get_width());
            float btn_w = 300.0f;
            float btn_h = 50.0f;
            float btn_x = (screen_w - btn_w) * 0.5f;

            m_game_over_buttons.clear();
            m_game_over_buttons.push_back(button("Restart", btn_x, 260.0f, btn_w, btn_h));
            m_game_over_buttons.push_back(button("Main Menu", btn_x, 330.0f, btn_w, btn_h));
            m_game_over_buttons.push_back(button("Exit", btn_x, 400.0f, btn_w, btn_h));

            // Mouse controls
            float mx = 0.0f, my = 0.0f;
            uint32_t mouse_buttons = SDL_GetMouseState(&mx, &my);
            float scale = m_engine->get_window().get_scale_factor();
            mx /= scale;
            my /= scale;
            bool is_left_down = (mouse_buttons & SDL_BUTTON_LMASK) != 0;

            static bool was_left_down = false;
            bool left_clicked = is_left_down && !was_left_down;
            was_left_down = is_left_down;

            bool hovered_any = false;
            for (size_t i = 0; i < m_game_over_buttons.size(); ++i)
            {
                if (m_game_over_buttons[i].is_hovered(mx, my))
                {
                    m_game_over_selected_index = static_cast<int>(i);
                    hovered_any = true;
                    break;
                }
            }

            // Keyboard navigation
            if (m_current_input.is_down(zwodee::input_state::move_up) && !m_last_input.is_down(zwodee::input_state::move_up))
            {
                m_game_over_selected_index = (m_game_over_selected_index - 1 + static_cast<int>(m_game_over_buttons.size())) % static_cast<int>(m_game_over_buttons.size());
            }
            else if (m_current_input.is_down(zwodee::input_state::move_down) && !m_last_input.is_down(zwodee::input_state::move_down))
            {
                m_game_over_selected_index = (m_game_over_selected_index + 1) % static_cast<int>(m_game_over_buttons.size());
            }

            // Trigger selected menu item
            bool trigger_action = (m_current_input.is_down(zwodee::input_state::action_1) && !m_last_input.is_down(zwodee::input_state::action_1)) || (left_clicked && hovered_any);

            if (trigger_action)
            {
                if (m_game_over_selected_index == 0) // Restart
                {
                    m_game_over = false;
                    m_death_sequence_ticks = -1;
                    restart();
                }
                else if (m_game_over_selected_index == 1) // Main Menu
                {
                    m_game_over = false;
                    m_death_sequence_ticks = -1;
                    m_engine->get_level_manager().transition_to("main_menu");
                }
                else if (m_game_over_selected_index == 2) // Exit
                {
                    m_engine->stop();
                }
            }
            m_last_input = m_current_input;
            return;
        }

        if (m_is_paused)
        {
            // Update layouts dynamically to fit current screen size
            float screen_w = static_cast<float>(m_engine->get_window().get_width());
            float btn_w = 300.0f;
            float btn_h = 50.0f;
            float btn_x = (screen_w - btn_w) * 0.5f;

            m_pause_buttons.clear();
            if (!m_in_settings)
            {
                m_pause_buttons.push_back(button("Resume", btn_x, 260.0f, btn_w, btn_h));
                m_pause_buttons.push_back(button("Settings", btn_x, 330.0f, btn_w, btn_h));
                m_pause_buttons.push_back(button("Back to Main Menu", btn_x, 400.0f, btn_w, btn_h));
                m_pause_buttons.push_back(button("Exit", btn_x, 470.0f, btn_w, btn_h));
            }
            else
            {
                bool sound_enabled = !m_engine->get_audio_manager().is_muted();
                m_pause_buttons.push_back(button(sound_enabled ? "Sound: ON" : "Sound: OFF", btn_x, 260.0f, btn_w, btn_h));

                std::string fps_label = "FPS Cap: Unknown";
                switch (m_engine->get_fps_limit())
                {
                    case zwodee::engine::fps_limit::vsync:    fps_label = "FPS Cap: VSync"; break;
                    case zwodee::engine::fps_limit::fps_60:   fps_label = "FPS Cap: 60 FPS"; break;
                    case zwodee::engine::fps_limit::fps_144:  fps_label = "FPS Cap: 144 FPS"; break;
                    case zwodee::engine::fps_limit::fps_240:  fps_label = "FPS Cap: 240 FPS"; break;
                    case zwodee::engine::fps_limit::fps_360:  fps_label = "FPS Cap: 360 FPS"; break;
                    case zwodee::engine::fps_limit::fps_480:  fps_label = "FPS Cap: 480 FPS"; break;
                    case zwodee::engine::fps_limit::unlocked: fps_label = "FPS Cap: Unlocked"; break;
                }
                m_pause_buttons.push_back(button(fps_label, btn_x, 330.0f, btn_w, btn_h));
                m_pause_buttons.push_back(button("Back", btn_x, 400.0f, btn_w, btn_h));
            }

            // Mouse controls
            float mx = 0.0f, my = 0.0f;
            uint32_t mouse_buttons = SDL_GetMouseState(&mx, &my);
            float scale = m_engine->get_window().get_scale_factor();
            mx /= scale;
            my /= scale;
            bool is_left_down = (mouse_buttons & SDL_BUTTON_LMASK) != 0;

            static bool was_left_down = false;
            bool left_clicked = is_left_down && !was_left_down;
            was_left_down = is_left_down;

            bool hovered_any = false;
            for (size_t i = 0; i < m_pause_buttons.size(); ++i)
            {
                if (m_pause_buttons[i].is_hovered(mx, my))
                {
                    m_pause_selected_index = static_cast<int>(i);
                    hovered_any = true;
                    break;
                }
            }

            // Keyboard navigation
            if (m_current_input.is_down(zwodee::input_state::move_up) && !m_last_input.is_down(zwodee::input_state::move_up))
            {
                m_pause_selected_index = (m_pause_selected_index - 1 + static_cast<int>(m_pause_buttons.size())) % static_cast<int>(m_pause_buttons.size());
            }
            else if (m_current_input.is_down(zwodee::input_state::move_down) && !m_last_input.is_down(zwodee::input_state::move_down))
            {
                m_pause_selected_index = (m_pause_selected_index + 1) % static_cast<int>(m_pause_buttons.size());
            }

            // Trigger selected menu item
            bool trigger_action = (m_current_input.is_down(zwodee::input_state::action_1) && !m_last_input.is_down(zwodee::input_state::action_1)) || (left_clicked && hovered_any);

            if (trigger_action)
            {
                if (!m_in_settings)
                {
                    if (m_pause_selected_index == 0) // Resume
                    {
                        m_is_paused = false;
                    }
                    else if (m_pause_selected_index == 1) // Settings
                    {
                        m_in_settings = true;
                        m_pause_selected_index = 0;
                    }
                    else if (m_pause_selected_index == 2) // Back to Main Menu
                    {
                        m_is_paused = false;
                        m_engine->get_level_manager().transition_to("main_menu");
                    }
                    else if (m_pause_selected_index == 3) // Exit
                    {
                        m_engine->stop();
                    }
                }
                else
                {
                    if (m_pause_selected_index == 0) // Sound toggle
                    {
                        bool sound_enabled = !m_engine->get_audio_manager().is_muted();
                        m_engine->get_audio_manager().set_muted(sound_enabled);
                    }
                    else if (m_pause_selected_index == 1) // FPS Cap toggle
                    {
                        zwodee::engine::fps_limit next_limit = zwodee::engine::fps_limit::vsync;
                        switch (m_engine->get_fps_limit())
                        {
                            case zwodee::engine::fps_limit::vsync:    next_limit = zwodee::engine::fps_limit::fps_60; break;
                            case zwodee::engine::fps_limit::fps_60:   next_limit = zwodee::engine::fps_limit::fps_144; break;
                            case zwodee::engine::fps_limit::fps_144:  next_limit = zwodee::engine::fps_limit::fps_240; break;
                            case zwodee::engine::fps_limit::fps_240:  next_limit = zwodee::engine::fps_limit::fps_360; break;
                            case zwodee::engine::fps_limit::fps_360:  next_limit = zwodee::engine::fps_limit::fps_480; break;
                            case zwodee::engine::fps_limit::fps_480:  next_limit = zwodee::engine::fps_limit::unlocked; break;
                            case zwodee::engine::fps_limit::unlocked: next_limit = zwodee::engine::fps_limit::vsync; break;
                        }
                        m_engine->set_fps_limit(next_limit);
                    }
                    else if (m_pause_selected_index == 2) // Back
                    {
                        m_in_settings = false;
                        m_pause_selected_index = 1; // Highlight settings option
                    }
                }
            }
            return;
        }

        // Smoothly interpolate level darkness
        if (m_current_darkness != m_target_darkness)
        {
            m_current_darkness += (m_target_darkness - m_current_darkness) * 0.02f;
            if (std::abs(m_current_darkness - m_target_darkness) < 0.001f)
            {
                m_current_darkness = m_target_darkness;
            }
        }

        zwodee::tile_level::tick();

        if (!m_player)
        {
            return;
        }

        // Update mummy spawning triggers
        {
            int p_gx = static_cast<int>(std::round(m_player->get_x() / 32.0f));
            int p_gy = static_cast<int>(std::round(m_player->get_y() / 32.0f));

            for (auto& trigger : m_mummy_triggers)
            {
                if (!trigger.triggered)
                {
                    if (p_gx == trigger.gx && p_gy == trigger.gy)
                    {
                        trigger.triggered = true;
                        trigger.cooldown_ticks = 384; // 3 seconds initial delay (3 * 128 ticks)
                    }
                }

                if (trigger.triggered)
                {
                    if (trigger.cooldown_ticks <= 0)
                    {
                        // Check if player is NOT currently standing on the spawn tile
                        if (p_gx != trigger.gx || p_gy != trigger.gy)
                        {
                            const zwodee::texture* mummy_front = m_mummy_front_tex ? m_mummy_front_tex.get() : m_fallback_tex.get();
                            const zwodee::texture* mummy_back = m_mummy_back_tex ? m_mummy_back_tex.get() : m_fallback_tex.get();
                            const zwodee::texture* mummy_side = m_mummy_side_tex ? m_mummy_side_tex.get() : m_fallback_tex.get();
                            
                            auto m = std::make_unique<mummy>(m_next_dynamic_mummy_id++, mummy_front, mummy_back, mummy_side);
                            m->set_grid_position(trigger.gx, trigger.gy);
                            m->trigger_spawn(); // Set spawned true
                            add_entity(std::move(m));
                            
                            // Reset cooldown to 5 seconds
                            trigger.cooldown_ticks = 640;
                        }
                    }
                    else
                    {
                        trigger.cooldown_ticks--;
                    }
                }
            }
        }

        if (m_player->is_dead())
        {
            if (m_death_sequence_ticks == -1)
            {
                if (auto* audio = m_player->get_audio_manager())
                {
                    audio->play_sound("death");
                }
                m_player->set_texture(m_player_dead_tex ? m_player_dead_tex.get() : m_fallback_tex.get());
                m_death_sequence_ticks = 384; // 3 seconds at 128Hz
            }
            else if (m_death_sequence_ticks > 0)
            {
                m_death_sequence_ticks--;
                if (m_death_sequence_ticks == 0)
                {
                    m_game_over = true;
                    m_game_over_selected_index = 0;
                    if (auto* audio = m_player->get_audio_manager())
                    {
                        audio->play_sound("gameover");
                    }
                }
            }
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
                restart();
                return;
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

                    diamond* revealed_diamond = nullptr;

                    // 1. Check if the lamp has a specific target diamond
                    if (auto* target = dl->get_target_diamond())
                    {
                        if (!target->is_dead() && !target->is_permanently_revealed())
                        {
                            int gx = static_cast<int>(std::round(target->get_x() / 32.0f));
                            int gy = static_cast<int>(std::round(target->get_y() / 32.0f));
                            if (!is_tile_digged(gx, gy))
                            {
                                revealed_diamond = target;
                            }
                        }
                    }

                    // 2. Fallback to visible or nearest diamond
                    if (!revealed_diamond)
                    {
                        float camera_x = 0.0f;
                        float camera_y = 0.0f;
                        float win_w = 1280.0f;
                        float win_h = 720.0f;
                        if (m_engine)
                        {
                            win_w = static_cast<float>(m_engine->get_window().get_width());
                            win_h = static_cast<float>(m_engine->get_window().get_height());
                        }

                        float curr_px = m_player ? m_player->get_x() : 0.0f;
                        float curr_py = m_player ? m_player->get_y() : 0.0f;

                        // Horizontal page flipping camera logic matching get_render_snapshot
                        int page_x = static_cast<int>(std::floor(curr_px / win_w));
                        int max_page_x = static_cast<int>(std::max(0.0f, std::floor((get_width() * 32.0f - 1.0f) / win_w)));
                        if (page_x < 0) page_x = 0;
                        if (page_x > max_page_x) page_x = max_page_x;
                        camera_x = page_x * win_w;

                        // Vertical smooth centering camera logic matching get_render_snapshot
                        float half_height = win_h / 2.0f;
                        camera_y = curr_py - half_height;
                        float max_camera_y = static_cast<float>(get_height() * 32) - win_h;
                        if (max_camera_y < 0.0f) max_camera_y = 0.0f;
                        if (camera_y < 0.0f) camera_y = 0.0f;
                        if (camera_y > max_camera_y) camera_y = max_camera_y;

                        std::vector<diamond*> visible_diamonds;
                        std::vector<diamond*> all_non_digged_diamonds;

                        for (const auto& other : get_entities())
                        {
                            if (auto* dm = dynamic_cast<diamond*>(other.get()))
                            {
                                if (!dm->is_dead() && !dm->is_permanently_revealed())
                                {
                                    int gx = static_cast<int>(std::round(dm->get_x() / 32.0f));
                                    int gy = static_cast<int>(std::round(dm->get_y() / 32.0f));
                                    if (!is_tile_digged(gx, gy))
                                    {
                                        all_non_digged_diamonds.push_back(dm);

                                        float dx = dm->get_x();
                                        float dy = dm->get_y();
                                        if (dx >= camera_x && dx <= camera_x + win_w &&
                                            dy >= camera_y && dy <= camera_y + win_h)
                                        {
                                            visible_diamonds.push_back(dm);
                                        }
                                    }
                                }
                            }
                        }

                        if (!visible_diamonds.empty())
                        {
                            int rand_idx = std::rand() % visible_diamonds.size();
                            revealed_diamond = visible_diamonds[rand_idx];
                        }
                        else if (!all_non_digged_diamonds.empty() && m_player)
                        {
                            // Find the nearest one to the player
                            diamond* nearest = nullptr;
                            float min_dist_sq = -1.0f;
                            for (auto* dm : all_non_digged_diamonds)
                            {
                                float diff_x = dm->get_x() - curr_px;
                                float diff_y = dm->get_y() - curr_py;
                                float dist_sq = diff_x * diff_x + diff_y * diff_y;
                                if (min_dist_sq < 0.0f || dist_sq < min_dist_sq)
                                {
                                    min_dist_sq = dist_sq;
                                    nearest = dm;
                                }
                            }
                            revealed_diamond = nearest;
                        }
                    }

                    if (revealed_diamond)
                    {
                        revealed_diamond->set_permanently_revealed(true);
                    }

                    if (m_player)
                    {
                        if (auto* audio = m_player->get_audio_manager())
                        {
                            audio->play_sound("appear");
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

                // Check collision with other entities while moving
                if (st->is_moving())
                {
                    for (const auto& other : get_entities())
                    {
                        if (other.get() == st || other->is_dead())
                        {
                            continue;
                        }

                        if (st->collides_with(*other))
                        {
                            if (other.get() == m_player)
                            {
                                if (st->is_falling())
                                {
                                    int st_gx = static_cast<int>(std::round(st->get_x() / 32.0f));
                                    int p_gx = static_cast<int>(std::round(m_player->get_x() / 32.0f));
                                    if (st_gx == p_gx)
                                    {
                                        m_player->take_damage(999);
                                    }
                                }
                            }
                            else
                            {
                                bool is_killable = !dynamic_cast<stone*>(other.get()) &&
                                                   !dynamic_cast<gold_coin*>(other.get()) &&
                                                   !dynamic_cast<diamond*>(other.get()) &&
                                                   !dynamic_cast<lamp*>(other.get()) &&
                                                   !dynamic_cast<garlic_bulb*>(other.get()) &&
                                                   !dynamic_cast<onion_bulb*>(other.get()) &&
                                                   !dynamic_cast<pickaxe*>(other.get()) &&
                                                   !dynamic_cast<vampire*>(other.get()) &&
                                                   !dynamic_cast<exit_door*>(other.get());
                                if (is_killable)
                                {
                                    if (st->is_falling())
                                    {
                                        int st_gx = static_cast<int>(std::round(st->get_x() / 32.0f));
                                        int ot_gx = static_cast<int>(std::round(other->get_x() / 32.0f));
                                        if (st_gx == ot_gx)
                                        {
                                            other->take_damage(999);
                                        }
                                    }
                                }
                            }
                        }
                    }
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
                            if (dynamic_cast<stone*>(other.get()))
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
                            int min_radius = std::min(st->get_explosion_radius(), other_stone->get_explosion_radius());
                            st->take_damage(999);
                            explode_stone(other_stone, min_radius);
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
        // Render background if any
        if (get_background_texture())
        {
            float lvl_w = static_cast<float>(get_width() * 32);
            float lvl_h = static_cast<float>(get_height() * 32);
            int tex_w = get_background_texture()->get_width();
            int tex_h = get_background_texture()->get_height();

            target_renderer.draw_sprite(*get_background_texture(), 0, 0, tex_w, tex_h, 0.0f, 0.0f, lvl_w, lvl_h);
        }

        // Render static tiles
        const auto& tiles = get_static_objects();
        for (const auto& tile : tiles)
        {
            if (!tile)
            {
                continue;
            }

            const zwodee::texture* tex = tile->get_texture();
            bool is_rock = (tex == m_static_stone_textures[0].get() ||
                            tex == m_static_stone_textures[1].get() ||
                            tex == m_static_stone_textures[2].get() ||
                            tex == m_static_stone_textures[3].get());

            if (is_rock && m_dirt_tex)
            {
                target_renderer.draw_sprite(*m_dirt_tex, 0, 0, m_dirt_tex->get_width(), m_dirt_tex->get_height(), tile->get_x(), tile->get_y(), tile->get_width(), tile->get_height());
            }

            tile->render(target_renderer, alpha);
        }

        // Render dynamic entities
        for (const auto& ent : get_entities())
        {
            if (ent->is_dead())
            {
                continue;
            }

            ent->render(target_renderer, alpha);
        }
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
            std::array<std::shared_ptr<zwodee::texture>, 2> player_shovel_running_texs;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_shovel_running_up_texs;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_shovel_running_down_texs;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_pickaxe_running_texs;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_pickaxe_running_up_texs;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_pickaxe_running_down_texs;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_shovel_tex;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_shovel_up_tex;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_shovel_down_tex;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_pickaxe_tex;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_pickaxe_up_tex;
            std::array<std::shared_ptr<zwodee::texture>, 2> player_digging_pickaxe_down_tex;
            std::shared_ptr<zwodee::texture> stone_high_tex;
            std::shared_ptr<zwodee::texture> stone_low_tex;
            std::shared_ptr<zwodee::texture> stone_mid_tex;
            std::shared_ptr<zwodee::texture> pickaxe_tex;
            std::shared_ptr<zwodee::texture> coin_tex;
            std::shared_ptr<zwodee::texture> door_closed_tex;
            std::shared_ptr<zwodee::texture> door_open_tex;
            std::vector<std::shared_ptr<zwodee::texture>> diamond_textures;
            std::shared_ptr<zwodee::texture> garlic_tex;
            std::shared_ptr<zwodee::texture> onion_tex;
            std::shared_ptr<zwodee::texture> lamp_tex;
            std::shared_ptr<zwodee::texture> blink_tex;
            std::shared_ptr<zwodee::texture> digged_tex;
            std::array<std::shared_ptr<zwodee::texture>, 4> static_stone_textures;
            std::shared_ptr<zwodee::texture> bg_tex;
            std::shared_ptr<zwodee::texture> fallback_tex;
            
            std::shared_ptr<zwodee::texture> vampire_sleeping_tex;
            std::shared_ptr<zwodee::texture> vampire_triggered_tex;
            std::shared_ptr<zwodee::texture> player_dead_tex;
            std::shared_ptr<zwodee::texture> fart_tex;
            std::shared_ptr<zwodee::texture> soldier_tex;
            std::shared_ptr<zwodee::texture> soldier_front_tex;
            std::shared_ptr<zwodee::texture> soldier_back_tex;
            std::shared_ptr<zwodee::texture> soldier_side_tex;
            std::shared_ptr<zwodee::texture> mummy_tex;
            std::shared_ptr<zwodee::texture> mummy_front_tex;
            std::shared_ptr<zwodee::texture> mummy_back_tex;
            std::shared_ptr<zwodee::texture> mummy_side_tex;
            std::shared_ptr<zwodee::texture> dragon_red_tex;
            std::shared_ptr<zwodee::texture> dragon_green_tex;
            std::shared_ptr<zwodee::texture> dirt_tex;
            std::array<std::shared_ptr<zwodee::texture>, 3> dirt_breaking_texs;
 
            bool loaded = false;
 
            void load_all(zwodee::renderer& r)
            {
                if (loaded) return;
 
                player_shovel_tex             = r.load_dds_texture("assets/textures/goblin-idle-shovel.dds");
                player_shovel_running_tex     = r.load_dds_texture("assets/textures/goblin-running-shovel-1.dds");
                player_shovel_running_up_tex  = r.load_dds_texture("assets/textures/goblin-running-up-shovel-1.dds");
                player_shovel_running_down_tex = r.load_dds_texture("assets/textures/goblin-running-down-shovel-1.dds");
                player_pickaxe_tex            = r.load_dds_texture("assets/textures/goblin-idle-pickaxe.dds");
                player_pickaxe_running_tex    = r.load_dds_texture("assets/textures/goblin-running-pickaxe-1.dds");
                player_pickaxe_running_up_tex  = r.load_dds_texture("assets/textures/goblin-running-up-pickaxe-1.dds");
                player_pickaxe_running_down_tex = r.load_dds_texture("assets/textures/goblin-running-down-pickaxe-1.dds");

                player_shovel_running_texs[0] = r.load_dds_texture("assets/textures/goblin-running-shovel-1.dds");
                player_shovel_running_texs[1] = r.load_dds_texture("assets/textures/goblin-running-shovel-2.dds");
                player_shovel_running_up_texs[0] = r.load_dds_texture("assets/textures/goblin-running-up-shovel-1.dds");
                player_shovel_running_up_texs[1] = r.load_dds_texture("assets/textures/goblin-running-up-shovel-2.dds");
                player_shovel_running_down_texs[0] = r.load_dds_texture("assets/textures/goblin-running-down-shovel-1.dds");
                player_shovel_running_down_texs[1] = r.load_dds_texture("assets/textures/goblin-running-down-shovel-2.dds");

                player_pickaxe_running_texs[0] = r.load_dds_texture("assets/textures/goblin-running-pickaxe-1.dds");
                player_pickaxe_running_texs[1] = r.load_dds_texture("assets/textures/goblin-running-pickaxe-2.dds");
                player_pickaxe_running_up_texs[0] = r.load_dds_texture("assets/textures/goblin-running-up-pickaxe-1.dds");
                player_pickaxe_running_up_texs[1] = r.load_dds_texture("assets/textures/goblin-running-up-pickaxe-2.dds");
                player_pickaxe_running_down_texs[0] = r.load_dds_texture("assets/textures/goblin-running-down-pickaxe-1.dds");
                player_pickaxe_running_down_texs[1] = r.load_dds_texture("assets/textures/goblin-running-down-pickaxe-2.dds");
                 
                player_digging_shovel_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-shovel-1.dds");
                player_digging_shovel_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-shovel-2.dds");
                player_digging_shovel_up_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-up-shovel-1.dds");
                player_digging_shovel_up_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-up-shovel-2.dds");
                player_digging_shovel_down_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-down-shovel-1.dds");
                player_digging_shovel_down_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-down-shovel-2.dds");

                player_digging_pickaxe_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-pickaxe-1.dds");
                player_digging_pickaxe_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-pickaxe-2.dds");
                player_digging_pickaxe_up_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-up-pickaxe-1.dds");
                player_digging_pickaxe_up_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-up-pickaxe-2.dds");
                player_digging_pickaxe_down_tex[0] = r.load_dds_texture("assets/textures/goblin-digging-down-pickaxe-1.dds");
                player_digging_pickaxe_down_tex[1] = r.load_dds_texture("assets/textures/goblin-digging-down-pickaxe-2.dds");

                for (int i = 0; i < 2; ++i)
                {
                    if (!player_digging_pickaxe_tex[i]) player_digging_pickaxe_tex[i] = player_digging_shovel_tex[i];
                    if (!player_digging_pickaxe_up_tex[i]) player_digging_pickaxe_up_tex[i] = player_digging_shovel_up_tex[i];
                    if (!player_digging_pickaxe_down_tex[i]) player_digging_pickaxe_down_tex[i] = player_digging_shovel_down_tex[i];
                }

                stone_high_tex                = r.load_dds_texture("assets/textures/stone-high.dds");
                stone_low_tex                 = r.load_dds_texture("assets/textures/stone-low.dds");
                stone_mid_tex                 = r.load_dds_texture("assets/textures/stone-mid.dds");
                pickaxe_tex                   = r.load_dds_texture("assets/textures/pickaxe.dds");
                coin_tex                      = r.load_dds_texture("assets/textures/coin.dds");
                door_closed_tex               = r.load_dds_texture("assets/textures/door-closed.dds");
                door_open_tex                 = r.load_dds_texture("assets/textures/door-open.dds");
                garlic_tex                    = r.load_dds_texture("assets/textures/garlic.dds");
                onion_tex                     = r.load_dds_texture("assets/textures/onion.dds");
                lamp_tex                      = r.load_dds_texture("assets/textures/lamp.dds");
                blink_tex                     = r.load_dds_texture("assets/textures/blink.dds");
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
                 
                bg_tex                        = r.load_dds_texture("assets/textures/header.dds");
                fallback_tex                  = create_solid_color_texture(r, 32, 32, 255, 0, 0, 255);
 
                vampire_sleeping_tex          = r.load_dds_texture("assets/textures/vampire-sleeping.dds");
                vampire_triggered_tex         = r.load_dds_texture("assets/textures/vampire-triggered.dds");
                player_dead_tex               = r.load_dds_texture("assets/textures/goblin-dead.dds");
                fart_tex                      = r.load_dds_texture("assets/textures/fart.dds");
                soldier_tex                   = r.load_dds_texture("assets/textures/soldier-front.dds");
                soldier_front_tex             = r.load_dds_texture("assets/textures/soldier-front.dds");
                soldier_back_tex              = r.load_dds_texture("assets/textures/soldier-back.dds");
                soldier_side_tex              = r.load_dds_texture("assets/textures/soldier-side.dds");
                mummy_tex                     = r.load_dds_texture("assets/textures/mummy.dds");
                mummy_front_tex               = r.load_dds_texture("assets/textures/mummy-front.dds");
                mummy_back_tex                = r.load_dds_texture("assets/textures/mummy-back.dds");
                mummy_side_tex                = r.load_dds_texture("assets/textures/mummy-side.dds");
                dragon_red_tex                = r.load_dds_texture("assets/textures/dragon-red.dds");
                dragon_green_tex              = r.load_dds_texture("assets/textures/dragon-green.dds");
                dirt_tex                      = r.load_dds_texture("assets/textures/dirt.dds");
                dirt_breaking_texs[0]         = r.load_dds_texture("assets/textures/dirt-breaking-1.dds");
                dirt_breaking_texs[1]         = r.load_dds_texture("assets/textures/dirt-breaking-2.dds");
                dirt_breaking_texs[2]         = r.load_dds_texture("assets/textures/dirt-breaking-3.dds");
 
                loaded = true;
            }
        };

        texture_cache g_textures;
    }

    void level::load_demo_level(zwodee::engine& engine)
    {
        m_engine = &engine;
        m_font = std::make_unique<zwodee::font>(engine.get_renderer(), "assets/fonts/Roboto-Medium.ttf", 72.0f);
        m_pause_buttons.clear();
        
        auto& r = engine.get_renderer();
        auto& audio = engine.get_audio_manager();

        // Load game sounds
        for (int i = 1; i <= 8; ++i)
        {
            audio.load_sound("running_" + std::to_string(i), "assets/sounds/running/running-" + std::to_string(i) + ".wav");
        }
        audio.load_sound("coin_collected", "assets/sounds/coin-collected.wav");
        audio.load_sound("vampire_triggered", "assets/sounds/vampire-triggered.wav");
        audio.load_sound("diamond_collected", "assets/sounds/diamond-collected.wav");
        audio.load_sound("equip", "assets/sounds/equip.wav");
        audio.load_sound("garlic_chew", "assets/sounds/garlic-chew.wav");
        audio.load_sound("onion_chew", "assets/sounds/onion-chew.wav");
        audio.load_sound("level_done", "assets/sounds/level-done.wav");
        audio.load_sound("stone_move", "assets/sounds/stone-move.wav");
        audio.load_sound("explosion", "assets/sounds/explosion.wav");
        audio.load_sound("stone_impact", "assets/sounds/stone-impact.wav");
        audio.load_sound("appear", "assets/sounds/appear.wav");
        audio.load_sound("gameover", "assets/sounds/gameover.wav");
        audio.load_sound("death", "assets/sounds/death.wav");

        // Load fart sounds
        for (int i = 1; i <= 5; ++i)
        {
            audio.load_sound("fart-" + std::to_string(i), "assets/sounds/fart-" + std::to_string(i) + ".wav");
        }

        // Load digging sounds
        for (int i = 1; i <= 4; ++i)
        {
            audio.load_sound("shovel_dig_" + std::to_string(i), "assets/sounds/digging/shovel/shovel-" + std::to_string(i) + ".wav");
            audio.load_sound("pickaxe_dig_" + std::to_string(i), "assets/sounds/digging/pickaxe/pickaxe-" + std::to_string(i) + ".wav");
        }

        // Load textures inside level class (preloaded once in g_textures)
        g_textures.load_all(r);

        m_player_shovel_tex             = g_textures.player_shovel_tex;
        m_player_shovel_running_texs     = g_textures.player_shovel_running_texs;
        m_player_shovel_running_up_texs  = g_textures.player_shovel_running_up_texs;
        m_player_shovel_running_down_texs = g_textures.player_shovel_running_down_texs;
        m_player_pickaxe_running_texs    = g_textures.player_pickaxe_running_texs;
        m_player_pickaxe_running_up_texs  = g_textures.player_pickaxe_running_up_texs;
        m_player_pickaxe_running_down_texs = g_textures.player_pickaxe_running_down_texs;
        m_player_pickaxe_tex            = g_textures.player_pickaxe_tex;
        m_player_digging_shovel_tex       = g_textures.player_digging_shovel_tex;
        m_player_digging_shovel_up_tex    = g_textures.player_digging_shovel_up_tex;
        m_player_digging_shovel_down_tex  = g_textures.player_digging_shovel_down_tex;
        m_player_digging_pickaxe_tex      = g_textures.player_digging_pickaxe_tex;
        m_player_digging_pickaxe_up_tex    = g_textures.player_digging_pickaxe_up_tex;
        m_player_digging_pickaxe_down_tex  = g_textures.player_digging_pickaxe_down_tex;
        m_stone_high_tex                = g_textures.stone_high_tex;
        m_stone_low_tex                 = g_textures.stone_low_tex;
        m_stone_mid_tex                 = g_textures.stone_mid_tex;
        m_pickaxe_tex                   = g_textures.pickaxe_tex;
        m_coin_tex                      = g_textures.coin_tex;
        m_door_closed_tex               = g_textures.door_closed_tex;
        m_door_open_tex                 = g_textures.door_open_tex;
        m_garlic_tex                    = g_textures.garlic_tex;
        m_onion_tex                     = g_textures.onion_tex;
        m_lamp_tex                      = g_textures.lamp_tex;
        m_blink_tex                     = g_textures.blink_tex;
        m_digged_tex                    = g_textures.digged_tex;
        
        m_static_stone_textures[0]      = g_textures.static_stone_textures[0];
        m_static_stone_textures[1]      = g_textures.static_stone_textures[1];
        m_static_stone_textures[2]      = g_textures.static_stone_textures[2];
        m_static_stone_textures[3]      = g_textures.static_stone_textures[3];

        m_diamond_textures              = g_textures.diamond_textures;
        
        m_bg_tex                        = g_textures.bg_tex;
        m_fallback_tex                  = g_textures.fallback_tex;

        m_vampire_sleeping_tex          = g_textures.vampire_sleeping_tex;
        m_vampire_triggered_tex         = g_textures.vampire_triggered_tex;
        m_player_dead_tex               = g_textures.player_dead_tex;
        m_fart_tex                      = g_textures.fart_tex;
        m_soldier_tex                   = g_textures.soldier_tex;
        m_soldier_front_tex             = g_textures.soldier_front_tex;
        m_soldier_back_tex              = g_textures.soldier_back_tex;
        m_soldier_side_tex              = g_textures.soldier_side_tex;
        m_mummy_tex                     = g_textures.mummy_tex;
        m_mummy_front_tex               = g_textures.mummy_front_tex;
        m_mummy_back_tex                = g_textures.mummy_back_tex;
        m_mummy_side_tex                = g_textures.mummy_side_tex;
        m_dragon_red_tex                = g_textures.dragon_red_tex;
        m_dragon_green_tex              = g_textures.dragon_green_tex;
        m_dirt_tex                      = g_textures.dirt_tex;
        m_dirt_breaking_texs            = g_textures.dirt_breaking_texs;

        const zwodee::texture* fallback_tex_ptr = m_fallback_tex.get();
        const zwodee::texture* shovel_idle = m_player_shovel_tex ? m_player_shovel_tex.get() : fallback_tex_ptr;
        const zwodee::texture* shovel_run = m_player_shovel_running_tex ? m_player_shovel_running_tex.get() : fallback_tex_ptr;
        const zwodee::texture* shovel_run_up = m_player_shovel_running_up_tex ? m_player_shovel_running_up_tex.get() : fallback_tex_ptr;
        const zwodee::texture* shovel_run_down = m_player_shovel_running_down_tex ? m_player_shovel_running_down_tex.get() : fallback_tex_ptr;
        const zwodee::texture* pickaxe_idle = m_player_pickaxe_tex ? m_player_pickaxe_tex.get() : fallback_tex_ptr;
        const zwodee::texture* pickaxe_run = m_player_pickaxe_running_tex ? m_player_pickaxe_running_tex.get() : fallback_tex_ptr;
        const zwodee::texture* pickaxe_run_up = m_player_pickaxe_running_up_tex ? m_player_pickaxe_running_up_tex.get() : fallback_tex_ptr;
        const zwodee::texture* pickaxe_run_down = m_player_pickaxe_running_down_tex ? m_player_pickaxe_running_down_tex.get() : fallback_tex_ptr;
        const zwodee::texture* stone_high_tex = m_stone_high_tex ? m_stone_high_tex.get() : fallback_tex_ptr;
        const zwodee::texture* stone_low_tex = m_stone_low_tex ? m_stone_low_tex.get() : fallback_tex_ptr;
        const zwodee::texture* stone_mid_tex = m_stone_mid_tex ? m_stone_mid_tex.get() : fallback_tex_ptr;
        const zwodee::texture* pickaxe_tex = m_pickaxe_tex ? m_pickaxe_tex.get() : fallback_tex_ptr;
        const zwodee::texture* coint_text = m_coin_tex ? m_coin_tex.get() : fallback_tex_ptr;
        const zwodee::texture* door_closed = m_door_closed_tex ? m_door_closed_tex.get() : fallback_tex_ptr;
        const zwodee::texture* door_open = m_door_open_tex ? m_door_open_tex.get() : fallback_tex_ptr;
        const zwodee::texture* garlic_tex = m_garlic_tex ? m_garlic_tex.get() : fallback_tex_ptr;
        const zwodee::texture* onion_tex = m_onion_tex ? m_onion_tex.get() : fallback_tex_ptr;
        const zwodee::texture* lamp_tex = m_lamp_tex ? m_lamp_tex.get() : fallback_tex_ptr;

        // Remove full stretched background texture
        set_background_texture(nullptr);

        // Populate the entire grid with header, footer, and dirt tiles
        const zwodee::texture* dirt_tex_ptr = m_dirt_tex ? m_dirt_tex.get() : fallback_tex_ptr;

        for (uint32_t y = 0; y < get_height(); ++y)
        {
            for (uint32_t x = 0; x < get_width(); ++x)
            {
                if (y == 0 || y == 34)
                {
                    // Empty grid space (rendered as stretched header/footer)
                    set_tile(x, y, 0, 0, nullptr);
                }
                else
                {
                    // Un-digged dirt earth by default (non-collidable so player can dig into them)
                    set_tile(x, y, 2, 1, dirt_tex_ptr);
                    size_t idx = static_cast<size_t>(y) * get_width() + x;
                    if (idx < get_static_objects().size() && get_static_objects()[idx])
                    {
                        get_static_objects()[idx]->set_collidable(false);
                        get_static_objects()[idx]->set_flip_horizontal(std::rand() % 2 == 0);
                    }
                }
            }
        }

        // Initialize static stone barriers (barriers at y = 6, y = 14, and y = 24 to block downward progress)
        std::vector<std::pair<int, int>> static_stone_positions;

        // Barrier 1 at y = 6
        for (int x = 0; x < 35; ++x)
        {
            static_stone_positions.push_back({x, 6});
        }
        // Barrier 2 at y = 14
        for (int x = 0; x < 35; ++x)
        {
            static_stone_positions.push_back({x, 14});
        }
        // Barrier 3 at y = 24
        for (int x = 0; x < 35; ++x)
        {
            static_stone_positions.push_back({x, 24});
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

        // Dig a starting chamber at the very top (3x3 around player spawn at 17, 2)
        for (int y = 2; y <= 4; ++y)
        {
            for (int x = 16; x <= 18; ++x)
            {
                if (x == 17 && y == 4)
                {
                    continue; // Leave as dirt to support puzzle1_stone1 at (17, 3)
                }
                dig_tile_at(x, y);
            }
        }

        // Add player goblin at the top center
        auto goblin = std::make_unique<player>(1, shovel_idle, shovel_run, shovel_run_up, shovel_run_down, pickaxe_idle, pickaxe_run, pickaxe_run_up, pickaxe_run_down, &audio);
        goblin->set_digging_textures(
            m_player_digging_shovel_tex[0] ? m_player_digging_shovel_tex[0].get() : nullptr,
            m_player_digging_shovel_tex[1] ? m_player_digging_shovel_tex[1].get() : nullptr,
            m_player_digging_shovel_up_tex[0] ? m_player_digging_shovel_up_tex[0].get() : nullptr,
            m_player_digging_shovel_up_tex[1] ? m_player_digging_shovel_up_tex[1].get() : nullptr,
            m_player_digging_shovel_down_tex[0] ? m_player_digging_shovel_down_tex[0].get() : nullptr,
            m_player_digging_shovel_down_tex[1] ? m_player_digging_shovel_down_tex[1].get() : nullptr,
            m_player_digging_pickaxe_tex[0] ? m_player_digging_pickaxe_tex[0].get() : nullptr,
            m_player_digging_pickaxe_tex[1] ? m_player_digging_pickaxe_tex[1].get() : nullptr,
            m_player_digging_pickaxe_up_tex[0] ? m_player_digging_pickaxe_up_tex[0].get() : nullptr,
            m_player_digging_pickaxe_up_tex[1] ? m_player_digging_pickaxe_up_tex[1].get() : nullptr,
            m_player_digging_pickaxe_down_tex[0] ? m_player_digging_pickaxe_down_tex[0].get() : nullptr,
            m_player_digging_pickaxe_down_tex[1] ? m_player_digging_pickaxe_down_tex[1].get() : nullptr
        );
        goblin->set_running_textures(
            m_player_shovel_running_texs[0] ? m_player_shovel_running_texs[0].get() : nullptr,
            m_player_shovel_running_texs[1] ? m_player_shovel_running_texs[1].get() : nullptr,
            m_player_shovel_running_up_texs[0] ? m_player_shovel_running_up_texs[0].get() : nullptr,
            m_player_shovel_running_up_texs[1] ? m_player_shovel_running_up_texs[1].get() : nullptr,
            m_player_shovel_running_down_texs[0] ? m_player_shovel_running_down_texs[0].get() : nullptr,
            m_player_shovel_running_down_texs[1] ? m_player_shovel_running_down_texs[1].get() : nullptr,
            m_player_pickaxe_running_texs[0] ? m_player_pickaxe_running_texs[0].get() : nullptr,
            m_player_pickaxe_running_texs[1] ? m_player_pickaxe_running_texs[1].get() : nullptr,
            m_player_pickaxe_running_up_texs[0] ? m_player_pickaxe_running_up_texs[0].get() : nullptr,
            m_player_pickaxe_running_up_texs[1] ? m_player_pickaxe_running_up_texs[1].get() : nullptr,
            m_player_pickaxe_running_down_texs[0] ? m_player_pickaxe_running_down_texs[0].get() : nullptr,
            m_player_pickaxe_running_down_texs[1] ? m_player_pickaxe_running_down_texs[1].get() : nullptr
        );
        goblin->set_grid_bounds(get_width(), get_height());
        goblin->set_level(this);
        goblin->set_grid_position(17, 2);
        m_player = goblin.get();
        add_entity(std::move(goblin));

        // Puzzle 1 (Opening Barrier at y=6):
        // Place two stone-mid blocks with a 1-tile gap (at y=3 and y=5) in the starting corridor.
        // Digging under the top stone lets it fall onto the bottom one to trigger an explosion!
        auto puzzle1_stone1 = std::make_unique<stone>(810, stone_mid_tex, stone::color_mid);
        puzzle1_stone1->set_grid_position(17, 3);
        add_entity(std::move(puzzle1_stone1));

        dig_tile_at(17, 5);
        auto puzzle1_stone2 = std::make_unique<stone>(811, stone_mid_tex, stone::color_mid);
        puzzle1_stone2->set_grid_position(17, 5);
        add_entity(std::move(puzzle1_stone2));

        // Dig a pocket for Soldier 1 (Stage 1 Left)
        for (int y = 7; y <= 9; ++y)
            for (int x = 4; x <= 8; ++x)
                dig_tile_at(x, y);

        // Dig a pocket for Soldier 2 (Stage 1 Right)
        for (int y = 7; y <= 9; ++y)
            for (int x = 26; x <= 30; ++x)
                dig_tile_at(x, y);

        // Puzzle 2 (Opening Barrier at y=14):
        // Place two stone-mid blocks with a 1-tile gap (at y=10 and y=12).
        // Digging them out will drop them on each other and blow up the y=14 barrier!
        dig_tile_at(17, 10);
        auto puzzle2_stone1 = std::make_unique<stone>(812, stone_mid_tex, stone::color_mid);
        puzzle2_stone1->set_grid_position(17, 10);
        add_entity(std::move(puzzle2_stone1));

        dig_tile_at(17, 12);
        auto puzzle2_stone2 = std::make_unique<stone>(813, stone_mid_tex, stone::color_mid);
        puzzle2_stone2->set_grid_position(17, 12);
        add_entity(std::move(puzzle2_stone2));

        // Place sleeping vampire guarding the tunnel entrance in Stage 2
        dig_tile_at(17, 17);
        const zwodee::texture* sleeping_tex_ptr = m_vampire_sleeping_tex ? m_vampire_sleeping_tex.get() : fallback_tex_ptr;
        const zwodee::texture* triggered_tex_ptr = m_vampire_triggered_tex ? m_vampire_triggered_tex.get() : fallback_tex_ptr;
        auto v1 = std::make_unique<vampire>(9, sleeping_tex_ptr, triggered_tex_ptr);
        v1->set_grid_position(17, 17);
        add_entity(std::move(v1));

        // Puzzle 3 (Opening Barrier at y=24):
        // Place two stone-high blocks with a 1-tile gap (at y=20 and y=22).
        // Digging them triggers a massive explosion to clear the dragon cavern barrier!
        dig_tile_at(17, 20);
        auto puzzle3_stone1 = std::make_unique<stone>(814, stone_high_tex, stone::color_high);
        puzzle3_stone1->set_grid_position(17, 20);
        add_entity(std::move(puzzle3_stone1));

        dig_tile_at(17, 22);
        auto puzzle3_stone2 = std::make_unique<stone>(815, stone_high_tex, stone::color_high);
        puzzle3_stone2->set_grid_position(17, 22);
        add_entity(std::move(puzzle3_stone2));

        // Stage 4 (Dragons):
        // Dig horizontal corridors for dragon patrols
        for (int y = 27; y <= 28; ++y)
            for (int x = 5; x <= 31; ++x)
                dig_tile_at(x, y);

        for (int y = 29; y <= 30; ++y)
            for (int x = 5; x <= 31; ++x)
                dig_tile_at(x, y);

        // Dragon patrol entities
        auto get_random_dragon_tex = [this, fallback_tex_ptr]() -> const zwodee::texture* {
            if (std::rand() % 2 == 0)
            {
                return m_dragon_red_tex ? m_dragon_red_tex.get() : fallback_tex_ptr;
            }
            else
            {
                return m_dragon_green_tex ? m_dragon_green_tex.get() : fallback_tex_ptr;
            }
        };

        auto dr1 = std::make_unique<dragon>(12, get_random_dragon_tex());
        dr1->set_grid_position(6, 27);
        add_entity(std::move(dr1));

        auto dr2 = std::make_unique<dragon>(112, get_random_dragon_tex());
        dr2->set_grid_position(28, 29);
        add_entity(std::move(dr2));

        // Dig tunnel from Stage 4 to exit door at bottom
        for (int y = 31; y <= 33; ++y)
        {
            dig_tile_at(17, y);
        }

        // Place items
        // Pickaxes (Plenty of pickaxes near spawn for testing, shifted to prevent overlaps)
        auto p1 = std::make_unique<pickaxe>(8, pickaxe_tex);
        p1->set_grid_position(16, 2);
        add_entity(std::move(p1));

        auto p2 = std::make_unique<pickaxe>(108, pickaxe_tex);
        p2->set_grid_position(15, 3);
        add_entity(std::move(p2));

        auto p3 = std::make_unique<pickaxe>(109, pickaxe_tex);
        p3->set_grid_position(3, 8);
        add_entity(std::move(p3));

        // Coins (12 in total, target is 8)
        std::vector<std::pair<int, int>> coin_positions = {
            {5, 7}, {30, 7}, // Side rooms Stage 1
            {17, 16},        // Behind Vampire
            {11, 13}, {23, 13}, // Vampire side chambers (shifted away from y=14 static stones)
            {12, 22}, {22, 22}, // Mummy chambers
            {17, 25},
            {4, 28}, {30, 28}, // Dragon corridors
            {16, 32}, {18, 32} // Exit room
        };
        uint32_t coin_net_id = 300;
        for (const auto& pos : coin_positions)
        {
            auto coin = std::make_unique<gold_coin>(coin_net_id++, coint_text);
            coin->set_grid_position(pos.first, pos.second);
            add_entity(std::move(coin));
        }
        m_target_gold = 8;

        // Mummy triggers
        m_mummy_triggers.clear();
        m_mummy_triggers.push_back({12, 21, false, 0});
        m_mummy_triggers.push_back({22, 21, false, 0});
        m_mummy_triggers.push_back({17, 25, false, 0}); // Shifted away from y=24 static stones
        m_next_dynamic_mummy_id = 5000;

        // Diamonds (8 in total)
        std::vector<std::pair<int, int>> diamond_positions = {
            {6, 7}, {28, 7},
            {10, 13}, {24, 13}, // Shifted away from y=14 static stones
            {13, 23}, {21, 23},
            {10, 30}, {24, 30}
        };
        uint32_t diamond_net_id = 400;
        const zwodee::texture* blink_tex_ptr = m_blink_tex ? m_blink_tex.get() : fallback_tex_ptr;
        for (const auto& pos : diamond_positions)
        {
            auto d = std::make_unique<diamond>(diamond_net_id++, get_random_diamond_texture(), blink_tex_ptr);
            d->set_level(this);
            d->set_grid_position(pos.first, pos.second);
            add_entity(std::move(d));
        }

        // Garlic bulbs (Placed close to spawn for immediate testing)
        std::vector<std::pair<int, int>> garlic_positions = {
            {16, 3}, {16, 4}, {13, 13}, {21, 13}, {17, 18}
        };
        uint32_t garlic_net_id = 500;
        for (const auto& pos : garlic_positions)
        {
            auto g = std::make_unique<garlic_bulb>(garlic_net_id++, garlic_tex);
            g->set_grid_position(pos.first, pos.second);
            add_entity(std::move(g));
        }

        // Onion bulbs (Placed close to spawn for immediate testing)
        std::vector<std::pair<int, int>> onion_positions = {
            {18, 2}, {18, 3}, {2, 7}, {32, 7}, {17, 8} // Shifted away from y=6 static stones
        };
        uint32_t onion_net_id = 600;
        for (const auto& pos : onion_positions)
        {
            auto o = std::make_unique<onion_bulb>(onion_net_id++, onion_tex);
            o->set_grid_position(pos.first, pos.second);
            add_entity(std::move(o));
        }

        // Lamps (Plenty of lamps near spawn)
        std::vector<std::pair<int, int>> lamp_positions = {
            {18, 4}, {16, 11}, {17, 28} // Shifted away from puzzle2_stone2 at (17, 12)
        };
        uint32_t lamp_net_id = 700;
        for (const auto& pos : lamp_positions)
        {
            auto l = std::make_unique<lamp>(lamp_net_id++, lamp_tex);
            l->set_grid_position(pos.first, pos.second);
            add_entity(std::move(l));
        }

        // Soldiers patrolling Stage 1 side chambers
        const zwodee::texture* soldier_front = m_soldier_front_tex ? m_soldier_front_tex.get() : fallback_tex_ptr;
        const zwodee::texture* soldier_back = m_soldier_back_tex ? m_soldier_back_tex.get() : fallback_tex_ptr;
        const zwodee::texture* soldier_side = m_soldier_side_tex ? m_soldier_side_tex.get() : fallback_tex_ptr;

        auto s1 = std::make_unique<soldier>(10, soldier_front, soldier_back, soldier_side);
        s1->set_grid_position(5, 8);
        add_entity(std::move(s1));

        auto s2 = std::make_unique<soldier>(110, soldier_front, soldier_back, soldier_side);
        s2->set_grid_position(29, 8);
        add_entity(std::move(s2));

        // Add exit door at bottom center
        auto door = std::make_unique<exit_door>(15, door_closed, door_open);
        door->set_grid_position(17, 33);
        m_exit_x = 17.0f * 32.0f;
        m_exit_y = 33.0f * 32.0f;
        add_entity(std::move(door));
    }

    void level::restart()
    {
        if (!m_engine) return;

        m_player = nullptr;
        m_exit_open = false;
        m_exit_x = 0.0f;
        m_exit_y = 0.0f;
        m_lamp_timer = 0.0f;
        m_target_gold = 0;
        m_current_darkness = 1.0f;
        m_game_over = false;
        m_death_sequence_ticks = -1;
        m_game_over_selected_index = 0;
        m_fart_effect_ticks = 0;
        m_fart_x = 0.0f;
        m_fart_y = 0.0f;

        clear_level();
        load_demo_level(*m_engine);
    }

    player* level::get_player() const
    {
        return m_player;
    }

    void level::trigger_fart_effect(float x, float y)
    {
        m_fart_effect_ticks = 128;
        m_fart_x = x;
        m_fart_y = y;
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

    void level::explode_stone(stone* st, int custom_radius)
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

        int radius = (custom_radius != -1) ? custom_radius : st->get_explosion_radius();

        int min_x = sgx;
        int max_x = sgx;
        int min_y = sgy;
        int max_y = sgy;

        if (radius == 1) // Grey 2x2 (centered/downward explosion to clear tiles below)
        {
            min_x = sgx - 1; max_x = sgx + 1;
            min_y = sgy;     max_y = sgy + 1;
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
            if (ent.get() == st || ent->is_dead() || dynamic_cast<vampire*>(ent.get()))
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

        // Kill player if caught in the explosion
        if (m_player && !m_player->is_dead())
        {
            int pgx = static_cast<int>(std::round(m_player->get_x() / 32.0f));
            int pgy = static_cast<int>(std::round(m_player->get_y() / 32.0f));
            if (pgx >= min_x && pgx <= max_x && pgy >= min_y && pgy <= max_y)
            {
                m_player->take_damage(999);
            }
        }
    }

    zwodee::render_snapshot level::get_render_snapshot(int display_w, int display_h) const
    {
        zwodee::render_snapshot base_snapshot = zwodee::tile_level::get_render_snapshot(display_w, display_h);
        zwodee::render_snapshot snapshot;
        snapshot.reserve(base_snapshot.size() * 2);

        for (const auto& node : base_snapshot)
        {
            bool is_rock = (node.tex && (node.tex == m_static_stone_textures[0].get() ||
                                         node.tex == m_static_stone_textures[1].get() ||
                                         node.tex == m_static_stone_textures[2].get() ||
                                         node.tex == m_static_stone_textures[3].get()));
            if (is_rock && m_dirt_tex)
            {
                zwodee::render_node dirt_node;
                dirt_node.x = node.x;
                dirt_node.y = node.y;
                dirt_node.w = node.w;
                dirt_node.h = node.h;
                dirt_node.tex = m_dirt_tex.get();
                dirt_node.src_x = 0;
                dirt_node.src_y = 0;
                dirt_node.src_w = m_dirt_tex->get_width();
                dirt_node.src_h = m_dirt_tex->get_height();
                dirt_node.flip_horizontal = false;
                dirt_node.flip_vertical = true; // Use flip_vertical to mark dirt under rock
                dirt_node.color_mod = 255;
                snapshot.push_back(dirt_node);
            }
            snapshot.push_back(node);
        }

        if (m_player)
        {
            float px = m_player->get_x();
            float py = m_player->get_y();

            if (m_player->is_dead())
            {
                zwodee::render_node dead_node;
                dead_node.x = px;
                dead_node.y = py;
                dead_node.w = 32.0f;
                dead_node.h = 32.0f;
                dead_node.tex = m_player_dead_tex ? m_player_dead_tex.get() : m_fallback_tex.get();
                dead_node.src_x = 0;
                dead_node.src_y = 0;
                dead_node.src_w = dead_node.tex->get_width();
                dead_node.src_h = dead_node.tex->get_height();
                dead_node.flip_horizontal = m_player->is_facing_left();
                dead_node.flip_vertical = false;
                dead_node.color_mod = 255;
                snapshot.push_back(dead_node);
            }

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

            // Add breaking textures if the player is actively digging
            if (m_player->is_digging())
            {
                int max_ticks = m_player->has_pickaxe() ? 48 : 96;
                int ticks_left = m_player->get_digging_ticks_remaining();
                float progress = 1.0f - (static_cast<float>(ticks_left) / static_cast<float>(max_ticks));
                
                size_t stage = 0;
                if (progress < 0.333f) stage = 0;
                else if (progress < 0.666f) stage = 1;
                else stage = 2;

                if (m_dirt_breaking_texs[stage])
                {
                    zwodee::render_node break_node;
                    break_node.x = m_player->get_target_x();
                    break_node.y = m_player->get_target_y();
                    break_node.w = 32.0f;
                    break_node.h = 32.0f;
                    break_node.tex = m_dirt_breaking_texs[stage].get();
                    break_node.src_x = 0;
                    break_node.src_y = 0;
                    break_node.src_w = m_dirt_breaking_texs[stage]->get_width();
                    break_node.src_h = m_dirt_breaking_texs[stage]->get_height();
                    break_node.flip_horizontal = false;
                    break_node.flip_vertical = false;
                    break_node.color_mod = 255;
                    snapshot.push_back(break_node);
                }
            }
 
            // Apply level darkness and vertical depth gradient to dirt tiles
            for (auto& node : snapshot)
            {
                if (node.tex && (node.tex == m_dirt_tex.get() ||
                                 node.tex == m_dirt_breaking_texs[0].get() ||
                                 node.tex == m_dirt_breaking_texs[1].get() ||
                                 node.tex == m_dirt_breaking_texs[2].get()))
                {
                    // Calculate depth factor based on absolute level y coordinate (from y = 64 to y = 1024)
                    float depth_factor = (node.y - 64.0f) / (1024.0f - 64.0f);
                    if (depth_factor < 0.0f) depth_factor = 0.0f;
                    if (depth_factor > 1.0f) depth_factor = 1.0f;
                    
                    // Smooth gradient: 1.0f (fully bright) at top, 0.05f (95% darker) at the bottom
                    float vertical_mult = 1.0f - depth_factor * 0.70f;
                    
                    float total_darkness = m_current_darkness * vertical_mult;
                    node.color_mod = static_cast<uint8_t>(total_darkness * 255.0f);
                }
            }

            // Add stretched header and footer nodes
            if (m_bg_tex)
            {
                // Header (flipped horizontally)
                zwodee::render_node header_node;
                header_node.x = 0.0f;
                header_node.y = 0.0f;
                header_node.w = static_cast<float>(get_width() * 32);
                header_node.h = 32.0f;
                header_node.tex = m_bg_tex.get();
                header_node.src_x = 0;
                header_node.src_y = 0;
                header_node.src_w = m_bg_tex->get_width();
                header_node.src_h = m_bg_tex->get_height();
                header_node.flip_horizontal = true;
                header_node.color_mod = 255;
                snapshot.push_back(header_node);

                // Footer (flipped vertically)
                zwodee::render_node footer_node;
                footer_node.x = 0.0f;
                footer_node.y = static_cast<float>((get_height() - 1) * 32);
                footer_node.w = static_cast<float>(get_width() * 32);
                footer_node.h = 32.0f;
                footer_node.tex = m_bg_tex.get();
                footer_node.src_x = 0;
                footer_node.src_y = 0;
                footer_node.src_w = m_bg_tex->get_width();
                footer_node.src_h = m_bg_tex->get_height();
                footer_node.flip_horizontal = false;
                footer_node.flip_vertical = true;
                footer_node.color_mod = 255;
                snapshot.push_back(footer_node);
            }

            // Fart visual effect node
            if (m_fart_effect_ticks > 0 && m_fart_tex)
            {
                float progress = 1.0f - (static_cast<float>(m_fart_effect_ticks) / 128.0f);
                float alpha = 0.0f;
                if (progress < 0.5f)
                {
                    alpha = progress * 2.0f;
                }
                else
                {
                    alpha = (1.0f - progress) * 2.0f;
                }
                uint8_t color_val = static_cast<uint8_t>(alpha * 255.0f);

                zwodee::render_node fart_node{};
                fart_node.x = m_fart_x;
                fart_node.y = m_fart_y;
                fart_node.w = 32.0f;
                fart_node.h = 32.0f;
                fart_node.tex = m_fart_tex.get();
                fart_node.src_x = 0;
                fart_node.src_y = 0;
                fart_node.src_w = m_fart_tex->get_width();
                fart_node.src_h = m_fart_tex->get_height();
                fart_node.flip_horizontal = false;
                fart_node.flip_vertical = false;
                fart_node.is_ui = false;
                fart_node.color_mod = color_val;
                snapshot.push_back(fart_node);
            }

            // Apply the camera offset to all rendering positions
            for (auto& node : snapshot)
            {
                node.x -= camera_x;
                node.y -= camera_y;
            }

            // Apply death sequence fade to the dead player texture
            for (auto& node : snapshot)
            {
                if (node.tex == m_player_dead_tex.get())
                {
                    float fade = 1.0f;
                    if (m_game_over)
                    {
                        fade = 0.0f;
                    }
                    else if (m_death_sequence_ticks >= 0)
                    {
                        fade = static_cast<float>(m_death_sequence_ticks) / 384.0f;
                    }
                    node.color_mod = static_cast<uint8_t>(fade * 255.0f);
                }
            }

            // Layer sorting to render the door above digged tiles but beneath the player
            std::stable_sort(snapshot.begin(), snapshot.end(), [this](const zwodee::render_node& a, const zwodee::render_node& b) {
                auto get_layer = [this](const zwodee::render_node& node) {
                    if (!node.tex) return 3;
                    if (node.tex == m_bg_tex.get()) return 0;
                    if (node.tex == m_digged_tex.get() || 
                        node.tex == m_static_stone_textures[0].get() || node.tex == m_static_stone_textures[1].get() ||
                        node.tex == m_static_stone_textures[2].get() || node.tex == m_static_stone_textures[3].get() ||
                        node.tex == m_dirt_tex.get())
                    {
                        return 1;
                    }
                    if (node.tex == m_door_closed_tex.get() || node.tex == m_door_open_tex.get() ||
                        node.tex == m_dirt_breaking_texs[0].get() ||
                        node.tex == m_dirt_breaking_texs[1].get() ||
                        node.tex == m_dirt_breaking_texs[2].get() ||
                        (m_fart_tex && node.tex == m_fart_tex.get()) ||
                        (m_vampire_sleeping_tex && node.tex == m_vampire_sleeping_tex.get()) ||
                        (m_vampire_triggered_tex && node.tex == m_vampire_triggered_tex.get()))
                    {
                        return 2;
                    }
                    if (node.tex == m_player_dead_tex.get())
                    {
                        return 4;
                    }
                    return 3;
                };
                return get_layer(a) < get_layer(b);
            });
        }

        // Draw HUD overlay (Top Left and Top Right)
        if (m_font && m_player)
        {
            float screen_w = static_cast<float>(display_w);
            float font_scale = 0.28f;
            float bg_y = 0.0f;
            float bg_h = 32.0f;
            float text_y = 22.0f;
            float icon_y = 6.0f;
            float icon_sz = 20.0f;

            // Structure to hold nodes temporarily so we can render the background underneath them
            struct hud_item {
                const zwodee::texture* tex = nullptr;
                float x = 0.0f;
                float w = 0.0f;
                std::string text;
            };

            // Calculate left elements content
            float tx = 8.0f; // Padding from left edge of screen

            // We can pre-calculate the positions and widths
            std::vector<hud_item> left_items;
            
            // Onion
            left_items.push_back({m_onion_tex.get(), tx, icon_sz, ": " + std::to_string(m_player->get_onion_count()) + "    "});
            tx += icon_sz + 4.0f;
            float o_w = 0.0f;
            for (char c : left_items.back().text) o_w += m_font->get_glyph(c).xadvance * font_scale;
            tx += o_w;

            // Garlic
            left_items.push_back({m_garlic_tex.get(), tx, icon_sz, ": " + std::to_string(m_player->get_garlic_count()) + "    "});
            tx += icon_sz + 4.0f;
            float g_w = 0.0f;
            for (char c : left_items.back().text) g_w += m_font->get_glyph(c).xadvance * font_scale;
            tx += g_w;

            // Coin
            left_items.push_back({m_coin_tex.get(), tx, icon_sz, ": " + std::to_string(m_player->get_gold_count())});
            tx += icon_sz + 4.0f;
            float c_w = 0.0f;
            for (char c : left_items.back().text) c_w += m_font->get_glyph(c).xadvance * font_scale;
            tx += c_w;

            float left_panel_w = tx + 8.0f; // Padding at end of left panel

            // Calculate right elements content
            std::string right_str = "Score: " + std::to_string(m_player->get_score()) + "  |  Level " + std::to_string(m_level_number);
            float right_w = 0.0f;
            for (char c : right_str)
            {
                right_w += m_font->get_glyph(c).xadvance * font_scale;
            }
            float right_panel_w = right_w + 16.0f; // 8px padding on each side
            float right_panel_x = screen_w - right_panel_w;
            float right_text_x = right_panel_x + 8.0f;

            // 1. Draw Left Panel Background Node (flush with left edge, x=0)
            zwodee::render_node bg_left{};
            bg_left.x = 0.0f;
            bg_left.y = bg_y;
            bg_left.w = left_panel_w;
            bg_left.h = bg_h;
            bg_left.tex = nullptr;
            bg_left.is_ui = true;
            bg_left.r = 0; bg_left.g = 0; bg_left.b = 0; bg_left.a = 128; // 50% opacity
            snapshot.push_back(bg_left);

            // 2. Draw Right Panel Background Node (flush with right edge)
            zwodee::render_node bg_right{};
            bg_right.x = right_panel_x;
            bg_right.y = bg_y;
            bg_right.w = right_panel_w;
            bg_right.h = bg_h;
            bg_right.tex = nullptr;
            bg_right.is_ui = true;
            bg_right.r = 0; bg_right.g = 0; bg_right.b = 0; bg_right.a = 128; // 50% opacity
            snapshot.push_back(bg_right);

            // Helpers to draw icon and text
            auto draw_hud_icon = [&](const zwodee::texture* tex, float x) {
                if (!tex) return;
                zwodee::render_node icon_node{};
                icon_node.x = x;
                icon_node.y = icon_y;
                icon_node.w = icon_sz;
                icon_node.h = icon_sz;
                icon_node.tex = tex;
                icon_node.src_x = 0;
                icon_node.src_y = 0;
                icon_node.src_w = tex->get_width();
                icon_node.src_h = tex->get_height();
                icon_node.flip_horizontal = false;
                icon_node.flip_vertical = false;
                icon_node.is_ui = true;
                icon_node.color_mod = 255;
                snapshot.push_back(icon_node);
            };

            auto draw_shadow_text = [&](const std::string& text, float x) {
                // Shadow
                auto shadow_nodes = m_font->get_text_nodes(text, x + 1.0f, text_y + 1.0f, font_scale, 0, 0, 0, 255);
                for (auto& node : shadow_nodes) node.is_ui = true;
                snapshot.insert(snapshot.end(), shadow_nodes.begin(), shadow_nodes.end());

                // Text
                auto text_nodes = m_font->get_text_nodes(text, x, text_y, font_scale, 255, 255, 255, 255);
                for (auto& node : text_nodes) node.is_ui = true;
                snapshot.insert(snapshot.end(), text_nodes.begin(), text_nodes.end());
            };

            // 3. Draw Left Panel Content (drawn directly at their calculated x coordinates)
            for (const auto& item : left_items)
            {
                draw_hud_icon(item.tex, item.x);
                draw_shadow_text(item.text, item.x + icon_sz + 4.0f);
            }

            // 4. Draw Right Panel Content
            draw_shadow_text(right_str, right_text_x);
        }

        // Append Game Over Menu overlay and buttons at the end of snapshot (renders on top, no camera offset)
        if (m_game_over)
        {
            float screen_w = static_cast<float>(display_w);
            float screen_h = static_cast<float>(display_h);

            // 1. Semi-transparent black-red overlay with blur effect
            zwodee::render_node overlay_node{};
            overlay_node.x = 0.0f;
            overlay_node.y = 0.0f;
            overlay_node.w = screen_w;
            overlay_node.h = screen_h;
            overlay_node.tex = nullptr;
            overlay_node.is_ui = true;
            overlay_node.is_blur = true;
            overlay_node.r = 64; overlay_node.g = 0; overlay_node.b = 0; overlay_node.a = 160; // Dark red overlay
            snapshot.push_back(overlay_node);

            // 2. "GAME OVER" Title Text
            if (m_font)
            {
                std::string game_over_text = "GAME OVER";
                float text_scale = 0.8f;
                float text_w = 0.0f;
                for (char c : game_over_text)
                {
                    text_w += m_font->get_glyph(c).xadvance * text_scale;
                }
                float tx = (screen_w - text_w) * 0.5f;
                std::vector<zwodee::render_node> text_nodes = m_font->get_text_nodes(game_over_text, tx, 150.0f, text_scale, 255, 50, 50, 255);
                for (auto& node : text_nodes)
                {
                    node.is_ui = true;
                }
                snapshot.insert(snapshot.end(), text_nodes.begin(), text_nodes.end());

                // 3. Render Buttons
                for (size_t i = 0; i < m_game_over_buttons.size(); ++i)
                {
                    m_game_over_buttons[i].add_to_snapshot(snapshot, *m_font, m_game_over_selected_index == static_cast<int>(i));
                }
            }
        }

        // Append Pause Menu overlay and buttons at the end of snapshot (renders on top, no camera offset)
        if (m_is_paused)
        {
            float screen_w = static_cast<float>(display_w);
            float screen_h = static_cast<float>(display_h);

            // 1. Semi-transparent black overlay with blur effect
            zwodee::render_node overlay_node{};
            overlay_node.x = 0.0f;
            overlay_node.y = 0.0f;
            overlay_node.w = screen_w;
            overlay_node.h = screen_h;
            overlay_node.tex = nullptr;
            overlay_node.is_ui = true;
            overlay_node.is_blur = true;
            overlay_node.r = 0; overlay_node.g = 0; overlay_node.b = 0; overlay_node.a = 128; // 50% dark overlay
            snapshot.push_back(overlay_node);

            // 2. "PAUSED" Title Text
            if (m_font)
            {
                std::string paused_text = "PAUSED";
                float text_scale = 0.8f; // ~57px size
                float text_w = 0.0f;
                for (char c : paused_text)
                {
                    text_w += m_font->get_glyph(c).xadvance * text_scale;
                }
                float tx = (screen_w - text_w) * 0.5f;
                std::vector<zwodee::render_node> text_nodes = m_font->get_text_nodes(paused_text, tx, 150.0f, text_scale, 255, 255, 255, 255);
                for (auto& node : text_nodes)
                {
                    node.is_ui = true;
                }
                snapshot.insert(snapshot.end(), text_nodes.begin(), text_nodes.end());

                // 2b. Optional "Settings" Subtitle
                if (m_in_settings)
                {
                    std::string subtitle = "Settings";
                    float sub_scale = 0.45f;
                    float sub_w = 0.0f;
                    for (char c : subtitle)
                    {
                        sub_w += m_font->get_glyph(c).xadvance * sub_scale;
                    }
                    float sx = (screen_w - sub_w) * 0.5f;
                    std::vector<zwodee::render_node> sub_nodes = m_font->get_text_nodes(subtitle, sx, 210.0f, sub_scale, 180, 180, 220, 255);
                    for (auto& node : sub_nodes)
                    {
                        node.is_ui = true;
                    }
                    snapshot.insert(snapshot.end(), sub_nodes.begin(), sub_nodes.end());
                }

                // 3. Render Buttons
                for (size_t i = 0; i < m_pause_buttons.size(); ++i)
                {
                    m_pause_buttons[i].add_to_snapshot(snapshot, *m_font, m_pause_selected_index == static_cast<int>(i));
                }
            }
        }
 
        return snapshot;
    }
}
