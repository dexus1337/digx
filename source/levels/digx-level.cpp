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
#include <fstream>
#include <string_view>

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
        m_first_input = true;
        m_ignored_buttons = 0;
    }

    void level::on_exit()
    {
        zwodee::tile_level::on_exit();
    }

    void level::set_player_input(const zwodee::input_state& input)
    {
        if (m_first_input)
        {
            m_ignored_buttons = input.buttons;
            m_first_input = false;
        }

        m_ignored_buttons &= input.buttons;

        zwodee::input_state filtered_input = input;
        filtered_input.buttons &= ~m_ignored_buttons;

        m_last_input = m_current_input;
        m_current_input = filtered_input;

        // Toggle pause when escape key (action_2) is pressed
        if (m_current_input.is_down(zwodee::input_state::action_2) && !m_last_input.is_down(zwodee::input_state::action_2))
        {
            m_is_paused = !m_is_paused;
            m_in_settings = false;
            m_pause_selected_index = 0;
        }

        if (!m_is_paused)
        {
            zwodee::tile_level::set_player_input(filtered_input);
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

            static float prev_mx_over = -1.0f;
            static float prev_my_over = -1.0f;
            bool mouse_moved = (mx != prev_mx_over || my != prev_my_over);
            prev_mx_over = mx;
            prev_my_over = my;

            bool hovered_any = false;
            for (size_t i = 0; i < m_game_over_buttons.size(); ++i)
            {
                if (m_game_over_buttons[i].is_hovered(mx, my))
                {
                    if (mouse_moved) m_game_over_selected_index = static_cast<int>(i);
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

        if (m_game_won)
        {
            float screen_w = static_cast<float>(m_engine->get_window().get_width());
            float btn_w = 300.0f;
            float btn_h = 50.0f;
            float btn_x = (screen_w - btn_w) * 0.5f;

            m_game_won_buttons.clear();
            m_game_won_buttons.push_back(button("Restart Game", btn_x, 260.0f, btn_w, btn_h));
            m_game_won_buttons.push_back(button("Main Menu", btn_x, 330.0f, btn_w, btn_h));

            // Mouse controls
            float mx = 0.0f, my = 0.0f;
            uint32_t mouse_buttons = SDL_GetMouseState(&mx, &my);
            float scale = m_engine->get_window().get_scale_factor();
            mx /= scale;
            my /= scale;
            bool is_left_down = (mouse_buttons & SDL_BUTTON_LMASK) != 0;

            static bool was_left_down_won = false;
            bool left_clicked = is_left_down && !was_left_down_won;
            was_left_down_won = is_left_down;

            static float prev_mx_won = -1.0f;
            static float prev_my_won = -1.0f;
            bool mouse_moved = (mx != prev_mx_won || my != prev_my_won);
            prev_mx_won = mx;
            prev_my_won = my;

            bool hovered_any = false;
            for (size_t i = 0; i < m_game_won_buttons.size(); ++i)
            {
                if (m_game_won_buttons[i].is_hovered(mx, my))
                {
                    if (mouse_moved) m_game_won_selected_index = static_cast<int>(i);
                    hovered_any = true;
                    break;
                }
            }

            // Keyboard navigation
            if (m_current_input.is_down(zwodee::input_state::move_up) && !m_last_input.is_down(zwodee::input_state::move_up))
            {
                m_game_won_selected_index = (m_game_won_selected_index - 1 + static_cast<int>(m_game_won_buttons.size())) % static_cast<int>(m_game_won_buttons.size());
            }
            else if (m_current_input.is_down(zwodee::input_state::move_down) && !m_last_input.is_down(zwodee::input_state::move_down))
            {
                m_game_won_selected_index = (m_game_won_selected_index + 1) % static_cast<int>(m_game_won_buttons.size());
            }

            // Trigger selected menu item
            bool trigger_action = (m_current_input.is_down(zwodee::input_state::action_1) && !m_last_input.is_down(zwodee::input_state::action_1)) || (left_clicked && hovered_any);

            if (trigger_action)
            {
                if (m_game_won_selected_index == 0) // Restart
                {
                    m_game_won = false;
                    auto new_level = std::make_unique<digx::level>(get_width(), get_height(), 1);
                    new_level->init(*m_engine, "level1");
                    
                    static int restart_counter = 0;
                    std::string restart_id = "restart_level_" + std::to_string(++restart_counter);
                    m_engine->get_level_manager().register_level(restart_id, std::move(new_level));
                    m_engine->get_level_manager().transition_to(restart_id);
                }
                else if (m_game_won_selected_index == 1) // Main Menu
                {
                    m_game_won = false;
                    m_engine->get_level_manager().transition_to("main_menu");
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

            static float prev_mx_pause = -1.0f;
            static float prev_my_pause = -1.0f;
            bool mouse_moved = (mx != prev_mx_pause || my != prev_my_pause);
            prev_mx_pause = mx;
            prev_my_pause = my;

            bool hovered_any = false;
            for (size_t i = 0; i < m_pause_buttons.size(); ++i)
            {
                if (m_pause_buttons[i].is_hovered(mx, my))
                {
                    if (mouse_moved) m_pause_selected_index = static_cast<int>(i);
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

                            
                            auto m = std::make_unique<mummy>(m_next_dynamic_mummy_id++);
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
                m_player->set_texture(texture_cache::get().player_dead_tex.get() ? texture_cache::get().player_dead_tex.get() : texture_cache::get().fallback_tex.get());
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
            if (idx < tiles.size() && (!tiles[idx] || (tiles[idx]->get_texture() != texture_cache::get().digged_tex.get() && !tiles[idx]->is_collidable())))
            {
                set_tile(pgx, pgy, 1, 0, texture_cache::get().digged_tex.get());
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
                advance_to_next_level();
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
            bool is_rock = (tex == texture_cache::get().static_stone_textures[0].get() ||
                            tex == texture_cache::get().static_stone_textures[1].get() ||
                            tex == texture_cache::get().static_stone_textures[2].get() ||
                            tex == texture_cache::get().static_stone_textures[3].get());

            if (is_rock && texture_cache::get().dirt_tex.get())
            {
                target_renderer.draw_sprite(*texture_cache::get().dirt_tex.get(), 0, 0, texture_cache::get().dirt_tex->get_width(), texture_cache::get().dirt_tex->get_height(), tile->get_x(), tile->get_y(), tile->get_width(), tile->get_height());
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
    }

    void level::init(zwodee::engine& engine, const std::string& level_name)
    {
        m_level_name = level_name;
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

        auto& tc = texture_cache::get();
        tc.load_all(r);

        // Remove full stretched background texture
        set_background_texture(nullptr);
        
        load_from_zwl("assets/levels/" + level_name + ".zwl");
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
        init(*m_engine, m_level_name);
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
            return tiles[idx]->get_texture() == texture_cache::get().digged_tex.get();
        }
        return false;
    }

    void level::dig_tile(int gx, int gy)
    {
        if (gx >= 0 && gx < static_cast<int>(get_width()) &&
            gy >= 0 && gy < static_cast<int>(get_height()))
        {
            set_tile(gx, gy, 1, 0, texture_cache::get().digged_tex.get());
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
            bool is_rock = (node.tex && (node.tex == texture_cache::get().static_stone_textures[0].get() ||
                                         node.tex == texture_cache::get().static_stone_textures[1].get() ||
                                         node.tex == texture_cache::get().static_stone_textures[2].get() ||
                                         node.tex == texture_cache::get().static_stone_textures[3].get()));
            if (is_rock && texture_cache::get().dirt_tex.get())
            {
                zwodee::render_node dirt_node;
                dirt_node.x = node.x;
                dirt_node.y = node.y;
                dirt_node.w = node.w;
                dirt_node.h = node.h;
                dirt_node.tex = texture_cache::get().dirt_tex.get();
                dirt_node.src_x = 0;
                dirt_node.src_y = 0;
                dirt_node.src_w = texture_cache::get().dirt_tex->get_width();
                dirt_node.src_h = texture_cache::get().dirt_tex->get_height();
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
                dead_node.tex = texture_cache::get().player_dead_tex.get() ? texture_cache::get().player_dead_tex.get() : texture_cache::get().fallback_tex.get();
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

                if (texture_cache::get().dirt_breaking_texs[stage])
                {
                    zwodee::render_node break_node;
                    break_node.x = m_player->get_target_x();
                    break_node.y = m_player->get_target_y();
                    break_node.w = 32.0f;
                    break_node.h = 32.0f;
                    break_node.tex = texture_cache::get().dirt_breaking_texs[stage].get();
                    break_node.src_x = 0;
                    break_node.src_y = 0;
                    break_node.src_w = texture_cache::get().dirt_breaking_texs[stage]->get_width();
                    break_node.src_h = texture_cache::get().dirt_breaking_texs[stage]->get_height();
                    break_node.flip_horizontal = false;
                    break_node.flip_vertical = false;
                    break_node.color_mod = 255;
                    snapshot.push_back(break_node);
                }
            }
 
            // Apply level darkness and vertical depth gradient to dirt tiles
            for (auto& node : snapshot)
            {
                if (node.tex && (node.tex == texture_cache::get().dirt_tex.get() ||
                                 node.tex == texture_cache::get().digged_tex.get() ||
                                 node.tex == texture_cache::get().dirt_breaking_texs[0].get() ||
                                 node.tex == texture_cache::get().dirt_breaking_texs[1].get() ||
                                 node.tex == texture_cache::get().dirt_breaking_texs[2].get()))
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
            if (texture_cache::get().bg_tex.get())
            {
                // Header (flipped horizontally)
                zwodee::render_node header_node;
                header_node.x = 0.0f;
                header_node.y = 0.0f;
                header_node.w = static_cast<float>(get_width() * 32);
                header_node.h = 32.0f;
                header_node.tex = texture_cache::get().bg_tex.get();
                header_node.src_x = 0;
                header_node.src_y = 0;
                header_node.src_w = texture_cache::get().bg_tex->get_width();
                header_node.src_h = texture_cache::get().bg_tex->get_height();
                header_node.flip_horizontal = true;
                header_node.color_mod = 255;
                snapshot.push_back(header_node);

                // Footer (flipped vertically)
                zwodee::render_node footer_node;
                footer_node.x = 0.0f;
                footer_node.y = static_cast<float>((get_height() - 1) * 32);
                footer_node.w = static_cast<float>(get_width() * 32);
                footer_node.h = 32.0f;
                footer_node.tex = texture_cache::get().bg_tex.get();
                footer_node.src_x = 0;
                footer_node.src_y = 0;
                footer_node.src_w = texture_cache::get().bg_tex->get_width();
                footer_node.src_h = texture_cache::get().bg_tex->get_height();
                footer_node.flip_horizontal = false;
                footer_node.flip_vertical = true;
                footer_node.color_mod = 255;
                snapshot.push_back(footer_node);
            }

            // Fart visual effect node
            if (m_fart_effect_ticks > 0 && texture_cache::get().fart_tex.get())
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
                fart_node.tex = texture_cache::get().fart_tex.get();
                fart_node.src_x = 0;
                fart_node.src_y = 0;
                fart_node.src_w = texture_cache::get().fart_tex->get_width();
                fart_node.src_h = texture_cache::get().fart_tex->get_height();
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
                if (node.tex == texture_cache::get().player_dead_tex.get())
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
                    if (node.tex == texture_cache::get().bg_tex.get()) return 0;
                    if (node.tex == texture_cache::get().digged_tex.get() || 
                        node.tex == texture_cache::get().static_stone_textures[0].get() || node.tex == texture_cache::get().static_stone_textures[1].get() ||
                        node.tex == texture_cache::get().static_stone_textures[2].get() || node.tex == texture_cache::get().static_stone_textures[3].get() ||
                        node.tex == texture_cache::get().dirt_tex.get())
                    {
                        return 1;
                    }
                    if (node.tex == texture_cache::get().door_closed_tex.get() || node.tex == texture_cache::get().door_open_tex.get() ||
                        node.tex == texture_cache::get().dirt_breaking_texs[0].get() ||
                        node.tex == texture_cache::get().dirt_breaking_texs[1].get() ||
                        node.tex == texture_cache::get().dirt_breaking_texs[2].get() ||
                        (texture_cache::get().fart_tex.get() && node.tex == texture_cache::get().fart_tex.get()) ||
                        (texture_cache::get().vampire_sleeping_tex.get() && node.tex == texture_cache::get().vampire_sleeping_tex.get()) ||
                        (texture_cache::get().vampire_triggered_tex.get() && node.tex == texture_cache::get().vampire_triggered_tex.get()))
                    {
                        return 2;
                    }
                    if (node.tex == texture_cache::get().player_dead_tex.get())
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
            left_items.push_back({texture_cache::get().onion_tex.get(), tx, icon_sz, ": " + std::to_string(m_player->get_onion_count()) + "    "});
            tx += icon_sz + 4.0f;
            float o_w = 0.0f;
            for (char c : left_items.back().text) o_w += m_font->get_glyph(c).xadvance * font_scale;
            tx += o_w;

            // Garlic
            left_items.push_back({texture_cache::get().garlic_tex.get(), tx, icon_sz, ": " + std::to_string(m_player->get_garlic_count()) + "    "});
            tx += icon_sz + 4.0f;
            float g_w = 0.0f;
            for (char c : left_items.back().text) g_w += m_font->get_glyph(c).xadvance * font_scale;
            tx += g_w;

            // Coin
            left_items.push_back({texture_cache::get().coin_tex.get(), tx, icon_sz, ": " + std::to_string(m_player->get_gold_count())});
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

        if (m_game_won)
        {
            float screen_w = static_cast<float>(display_w);
            float screen_h = static_cast<float>(display_h);

            // 1. Semi-transparent gold overlay with blur effect
            zwodee::render_node overlay_node{};
            overlay_node.x = 0.0f;
            overlay_node.y = 0.0f;
            overlay_node.w = screen_w;
            overlay_node.h = screen_h;
            overlay_node.tex = nullptr;
            overlay_node.is_ui = true;
            overlay_node.is_blur = true;
            overlay_node.r = 64; overlay_node.g = 48; overlay_node.b = 0; overlay_node.a = 180; // Dark gold overlay
            snapshot.push_back(overlay_node);

            if (m_font)
            {
                std::string win_text = "YOU WIN!";
                float text_scale = 1.0f;
                float text_w = 0.0f;
                for (char c : win_text)
                {
                    text_w += m_font->get_glyph(c).xadvance * text_scale;
                }
                float tx = (screen_w - text_w) * 0.5f;
                std::vector<zwodee::render_node> text_nodes = m_font->get_text_nodes(win_text, tx, 120.0f, text_scale, 255, 215, 0, 255);
                for (auto& node : text_nodes) node.is_ui = true;
                snapshot.insert(snapshot.end(), text_nodes.begin(), text_nodes.end());

                std::string score_text = "Final Score: " + std::to_string(m_player ? m_player->get_score() : 0);
                float score_scale = 0.6f;
                float score_w = 0.0f;
                for (char c : score_text)
                {
                    score_w += m_font->get_glyph(c).xadvance * score_scale;
                }
                float sx = (screen_w - score_w) * 0.5f;
                std::vector<zwodee::render_node> score_nodes = m_font->get_text_nodes(score_text, sx, 200.0f, score_scale, 255, 255, 255, 255);
                for (auto& node : score_nodes) node.is_ui = true;
                snapshot.insert(snapshot.end(), score_nodes.begin(), score_nodes.end());

                // 3. Render Buttons
                for (size_t i = 0; i < m_game_won_buttons.size(); ++i)
                {
                    m_game_won_buttons[i].add_to_snapshot(snapshot, *m_font, m_game_won_selected_index == static_cast<int>(i));
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

    void level::load_from_zwl(const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in)
        {
            return;
        }

        zwodee::level_header header;
        if (!in.read(reinterpret_cast<char*>(&header), sizeof(header)))
        {
            return;
        }

        if (std::string_view(header.magic, 4) != std::string_view("ZWL\0", 4))
        {
            return;
        }

        resize(header.width, header.height);
        
        // Read tiles
        for (uint32_t i = 0; i < header.tile_count; ++i)
        {
            zwodee::binary_tile bt;
            if (!in.read(reinterpret_cast<char*>(&bt), sizeof(bt))) break;

            uint32_t x = i % header.width;
            uint32_t y = i / header.width;

            const zwodee::texture* tex = nullptr;
            
            if (bt.tile_id == 0)
            {
                bt.tile_id = 1;
                bt.flags = 0;
                tex = texture_cache::get().digged_tex.get();
            }
            else if (bt.tile_id == 1) tex = texture_cache::get().dirt_tex.get();
            else if (bt.tile_id == 2) tex = texture_cache::get().static_stone_textures[std::rand() % 4].get();

            if (!tex && bt.tile_id != 0)
                tex = texture_cache::get().fallback_tex.get();

            set_tile(x, y, bt.tile_id, bt.flags, tex);
            
            size_t idx = static_cast<size_t>(y) * get_width() + static_cast<size_t>(x);
            if (idx < get_static_objects().size() && get_static_objects()[idx])
            {
                get_static_objects()[idx]->set_collidable(bt.flags & 1);
                
                if (bt.tile_id == 1)
                {
                    get_static_objects()[idx]->set_collidable(false);
                    get_static_objects()[idx]->set_flip_horizontal(std::rand() % 2 == 0);
                }
                else if (bt.tile_id == 2)
                {
                    get_static_objects()[idx]->set_collidable(true);
                }
            }
        }

        uint32_t next_stone_id = 800;
        m_target_gold = 0;

        // Read entities
        for (uint32_t i = 0; i < header.entity_count; ++i)
        {
            zwodee::binary_entity be;
            if (!in.read(reinterpret_cast<char*>(&be), sizeof(be))) break;

            int gx = static_cast<int>(std::round(be.x / 32.0f));
            int gy = static_cast<int>(std::round(be.y / 32.0f));
            
            if (be.type_id == 1) // Player
            {
                auto goblin = std::make_unique<player>(1, m_engine ? &m_engine->get_audio_manager() : nullptr);
                goblin->set_grid_bounds(get_width(), get_height());
                goblin->set_level(this);
                goblin->set_grid_position(gx, gy);
                m_player = goblin.get();
                
                if (m_has_persisted_state)
                {
                    m_player->apply_persistent_state(
                        m_persisted_state.score,
                        m_persisted_state.diamonds,
                        m_persisted_state.garlic,
                        m_persisted_state.onion,
                        m_persisted_state.has_pickaxe
                    );
                }

                add_entity(std::move(goblin));
            }
            else if (be.type_id == 10) // Mummy
            {
                auto m = std::make_unique<mummy>(m_next_dynamic_mummy_id++);
                m->set_grid_position(gx, gy);
                m->trigger_spawn(); 
                add_entity(std::move(m));
            }
            else if (be.type_id == 11) // Soldier
            {
                auto s = std::make_unique<soldier>(m_next_dynamic_mummy_id++);
                s->set_grid_position(gx, gy);
                add_entity(std::move(s));
            }
            else if (be.type_id == 12) // Vampire
            {
                auto v = std::make_unique<vampire>(m_next_dynamic_mummy_id++);
                v->set_grid_position(gx, gy);
                add_entity(std::move(v));
            }
            else if (be.type_id == 13) // Dragon
            {
                auto d = std::make_unique<dragon>(m_next_dynamic_mummy_id++);
                d->set_grid_position(gx, gy);
                add_entity(std::move(d));
            }
            else if (be.type_id == 20) // Stone
            {
                stone::stone_color col = stone::color_mid;
                if (be.health == 100) col = stone::color_high;
                else if (be.health == 50) col = stone::color_mid;
                else col = stone::color_low;
                
                auto s = std::make_unique<stone>(next_stone_id++, col);
                s->set_grid_position(gx, gy);
                add_entity(std::move(s));
            }
            else if (be.type_id == 21) // Diamond
            {
                auto d = std::make_unique<diamond>(next_stone_id++);
                d->set_grid_position(gx, gy);
                add_entity(std::move(d));
            }
            else if (be.type_id == 22) // Gold Coin
            {
                auto c = std::make_unique<gold_coin>(next_stone_id++);
                c->set_grid_position(gx, gy);
                add_entity(std::move(c));
                m_target_gold++;
            }
            else if (be.type_id == 23) // Lamp
            {
                auto l = std::make_unique<lamp>(next_stone_id++);
                l->set_grid_position(gx, gy);
                add_entity(std::move(l));
            }
            else if (be.type_id == 24) // Garlic
            {
                auto g = std::make_unique<garlic_bulb>(next_stone_id++);
                g->set_grid_position(gx, gy);
                add_entity(std::move(g));
            }
            else if (be.type_id == 25) // Onion
            {
                auto o = std::make_unique<onion_bulb>(next_stone_id++);
                o->set_grid_position(gx, gy);
                add_entity(std::move(o));
            }
            else if (be.type_id == 26) // Pickaxe
            {
                auto p = std::make_unique<pickaxe>(next_stone_id++);
                p->set_grid_position(gx, gy);
                add_entity(std::move(p));
            }
            else if (be.type_id == 27) // Exit door
            {
                auto door = std::make_unique<exit_door>(15);
                door->set_grid_position(gx, gy);
                m_exit_x = static_cast<float>(gx * 32);
                m_exit_y = static_cast<float>(gy * 32);
                add_entity(std::move(door));
            }
        }

        if (header.target_score != -1)
        {
            m_target_gold = header.target_score;
        }
    }

    void level::set_persistent_state(const player_persistent_state& state)
    {
        m_persisted_state = state;
        m_has_persisted_state = true;
        
        if (m_player)
        {
            m_player->apply_persistent_state
            (
                m_persisted_state.score,
                m_persisted_state.diamonds,
                m_persisted_state.garlic,
                m_persisted_state.onion,
                m_persisted_state.has_pickaxe
            );
        }
    }

    void level::advance_to_next_level()
    {
        int next_level = m_level_number + 1;
        std::string next_file = "assets/levels/level" + std::to_string(next_level) + ".zwl";
        
        std::ifstream f(next_file);
        if (f.good())
        {
            f.close();
            // Load next level
            auto new_level = std::make_unique<digx::level>(get_width(), get_height(), next_level);
            new_level->init(*m_engine, "level" + std::to_string(next_level));
            
            player_persistent_state state;
            if (m_player)
            {
                state.score = m_player->get_score();
                state.diamonds = m_player->get_diamond_count();
                state.garlic = m_player->get_garlic_count();
                state.onion = m_player->get_onion_count();
                state.has_pickaxe = m_player->has_pickaxe();
            }
            new_level->set_persistent_state(state);
            
            std::string level_id = "play_level_" + std::to_string(next_level);
            m_engine->get_level_manager().register_level(level_id, std::move(new_level));
            m_engine->get_level_manager().transition_to(level_id);
        }
        else
        {
            // WINNER!
            m_game_won = true;
            m_game_won_selected_index = 0;
            if (auto* audio = m_player ? m_player->get_audio_manager() : nullptr)
            {
                audio->play_sound("coin_collected"); // Fallback win sound
            }
        }
    }
}
