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
#include "levels/digx-main-menu.hpp"
#include "config-manager.hpp"

#include <SDL3/SDL.h>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <string_view>
#include <cstdio>

namespace digx
{
    level::level(uint32_t width, uint32_t height, int level_number)
        : zwodee::tile_level(width, height), m_level_number(level_number),
          m_sound_switch("Sound Effects", true, 0.0f, 0.0f, 300.0f, 40.0f),
          m_volume_slider("Audio Volume", 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 300.0f, 40.0f)
    {
        m_target_darkness = 1.0f;
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
            if (m_is_paused) {
                m_is_paused = false;
                m_resume_ticks = 32;
            } else if (m_resume_ticks < 0) {
                m_is_paused = true;
                m_in_settings = false;
                m_pause_selected_index = 0;
                m_pause_ticks = 0;
                m_resume_ticks = -1;
            }
        }

        if (!m_is_paused && m_level_finish_sequence_ticks < 0 && m_level_entry_fade_ticks <= 0)
        {
            zwodee::tile_level::set_player_input(filtered_input);
        }
        else
        {
            zwodee::tile_level::set_player_input(zwodee::input_state{});
        }
    }

    void level::tick()
    {
        if (!m_player) return;

        if (!m_is_paused && m_resume_ticks >= 0)
        {
            m_resume_ticks--;
        }

        // Process Developer Console Commands
        if (m_engine)
        {
            std::string cmd;
            while (m_engine->pop_console_command(cmd))
            {
                if (cmd.rfind("level ", 0) == 0) // starts_with
                {
                    std::string level_name = cmd.substr(6);
                    std::string path = "assets/levels/" + level_name + ".zwl";
                    
                    std::ifstream f(path);
                    if (f.good())
                    {
                        f.close();
                        int level_num = 1; // Default
                        if (level_name.rfind("level", 0) == 0)
                        {
                            try { level_num = std::stoi(level_name.substr(5)); } catch(...) {}
                        }

                        auto new_level = std::make_unique<digx::level>(get_width(), get_height(), level_num);
                        new_level->init(*m_engine, level_name);
                        m_engine->get_level_manager().register_level(level_name, std::move(new_level));
                        m_engine->get_level_manager().transition_to(level_name);
                        return; // Exit tick to avoid accessing destroyed level
                    }
                }
                else if (cmd.rfind("tp ", 0) == 0)
                {
                    try
                    {
                        size_t space1 = cmd.find(' ');
                        size_t space2 = cmd.find(' ', space1 + 1);
                        if (space2 != std::string::npos)
                        {
                            int gx = std::stoi(cmd.substr(space1 + 1, space2 - space1 - 1));
                            int gy = std::stoi(cmd.substr(space2 + 1));
                            m_player->set_grid_position(gx, gy);
                        }
                    }
                    catch(...) {}
                }
                else if (cmd.rfind("tool ", 0) == 0)
                {
                    std::string target = cmd.substr(5);
                    if (target == "pickaxe" || target == "1")
                    {
                        m_player->set_has_pickaxe(true);
                        std::cout << "[Console] Equipped Pickaxe." << std::endl;
                    }
                    else if (target == "shovel" || target == "0")
                    {
                        m_player->set_has_pickaxe(false);
                        std::cout << "[Console] Equipped Shovel." << std::endl;
                    }
                }
                else if (cmd.rfind("speed ", 0) == 0)
                {
                    try
                    {
                        std::string sub = cmd.substr(6);
                        if (sub.rfind("shovel ", 0) == 0)
                        {
                            int val = std::stoi(sub.substr(7));
                            m_player->set_shovel_dig_ticks(val);
                            std::cout << "[Console] Shovel dig duration set to " << val << " ticks." << std::endl;
                        }
                        else if (sub.rfind("pickaxe ", 0) == 0)
                        {
                            int val = std::stoi(sub.substr(8));
                            m_player->set_pickaxe_dig_ticks(val);
                            std::cout << "[Console] Pickaxe dig duration set to " << val << " ticks." << std::endl;
                        }
                        else if (sub.rfind("move ", 0) == 0)
                        {
                            float val = std::stof(sub.substr(5));
                            m_player->set_tunnel_speed(val);
                            std::cout << "[Console] Player movement speed set to " << val << "." << std::endl;
                        }
                    }
                    catch(...) {}
                }
                else if (cmd.rfind("shovel_speed ", 0) == 0)
                {
                    try
                    {
                        int val = std::stoi(cmd.substr(13));
                        m_player->set_shovel_dig_ticks(val);
                        std::cout << "[Console] Shovel dig duration set to " << val << " ticks." << std::endl;
                    }
                    catch(...) {}
                }
                else if (cmd.rfind("pickaxe_speed ", 0) == 0)
                {
                    try
                    {
                        int val = std::stoi(cmd.substr(14));
                        m_player->set_pickaxe_dig_ticks(val);
                        std::cout << "[Console] Pickaxe dig duration set to " << val << " ticks." << std::endl;
                    }
                    catch(...) {}
                }
                else if (cmd.rfind("movespeed ", 0) == 0)
                {
                    try
                    {
                        float val = std::stof(cmd.substr(10));
                        m_player->set_tunnel_speed(val);
                        std::cout << "[Console] Player movement speed set to " << val << "." << std::endl;
                    }
                    catch(...) {}
                }
                else if (cmd == "motherlode")
                {
                    m_player->collect_garlic(5000);
                    m_player->collect_onion(5000);
                    m_player->set_shovel_dig_ticks(0);
                    m_player->set_pickaxe_dig_ticks(0);
                    std::cout << "[Console] Cheat activated: motherlode! Added 5000 garlic, 5000 onions, and set dig speed to 0." << std::endl;
                }
            }
        }

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
            m_game_over_buttons.push_back(zwodee::button("Restart", btn_x, 260.0f, btn_w, btn_h));
            m_game_over_buttons.push_back(zwodee::button("Main Menu", btn_x, 330.0f, btn_w, btn_h));
            m_game_over_buttons.push_back(zwodee::button("Exit", btn_x, 400.0f, btn_w, btn_h));

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
                    m_player = nullptr; // Ignore dead player's state
                    m_persisted_state = player_persistent_state(); // Full wipe
                    
                    m_level_number = 1;
                    m_level_name = "level1";
                    
                    restart();
                    save_game(1);
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

        if (m_retry_screen)
        {
            float screen_w = static_cast<float>(m_engine->get_window().get_width());
            float btn_w = 300.0f;
            float btn_h = 50.0f;
            float btn_x = (screen_w - btn_w) * 0.5f;

            m_retry_buttons.clear();
            m_retry_buttons.push_back(zwodee::button("Retry", btn_x, 260.0f, btn_w, btn_h));
            m_retry_buttons.push_back(zwodee::button("Main Menu", btn_x, 330.0f, btn_w, btn_h));
            m_retry_buttons.push_back(zwodee::button("Exit", btn_x, 400.0f, btn_w, btn_h));

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
            for (size_t i = 0; i < m_retry_buttons.size(); ++i)
            {
                if (m_retry_buttons[i].is_hovered(mx, my))
                {
                    if (mouse_moved) m_retry_selected_index = static_cast<int>(i);
                    hovered_any = true;
                    break;
                }
            }

            // Keyboard navigation
            if (m_current_input.is_down(zwodee::input_state::move_up) && !m_last_input.is_down(zwodee::input_state::move_up))
            {
                m_retry_selected_index = (m_retry_selected_index - 1 + static_cast<int>(m_retry_buttons.size())) % static_cast<int>(m_retry_buttons.size());
            }
            else if (m_current_input.is_down(zwodee::input_state::move_down) && !m_last_input.is_down(zwodee::input_state::move_down))
            {
                m_retry_selected_index = (m_retry_selected_index + 1) % static_cast<int>(m_retry_buttons.size());
            }

            // Trigger selected menu item
            bool trigger_action = (m_current_input.is_down(zwodee::input_state::action_1) && !m_last_input.is_down(zwodee::input_state::action_1)) || (left_clicked && hovered_any);

            if (trigger_action)
            {
                if (m_retry_selected_index == 0) // Retry
                {
                    m_persisted_state.lives--;
                    save_game(m_level_number);
                    m_retry_screen = false;
                    m_death_sequence_ticks = -1;
                    restart(); // Restart current level
                }
                else if (m_retry_selected_index == 1) // Main Menu
                {
                    m_persisted_state.lives--;
                    save_game(m_level_number);
                    m_retry_screen = false;
                    m_death_sequence_ticks = -1;
                    m_engine->get_level_manager().transition_to("main_menu");
                }
                else if (m_retry_selected_index == 2) // Exit
                {
                    m_persisted_state.lives--;
                    save_game(m_level_number);
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
            m_game_won_buttons.push_back(zwodee::button("Restart Game", btn_x, 260.0f, btn_w, btn_h));
            m_game_won_buttons.push_back(zwodee::button("Main Menu", btn_x, 330.0f, btn_w, btn_h));

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
            m_pause_ticks++;

            // Update layouts dynamically to fit current screen size
            float screen_w = static_cast<float>(m_engine->get_window().get_width());
            float btn_w = 300.0f;
            float btn_h = 50.0f;
            float btn_x = (screen_w - btn_w) * 0.5f;

            m_pause_buttons.clear();
            if (!m_in_settings)
            {
                float curr_y = 260.0f;
                m_pause_buttons.push_back(zwodee::button("Resume", btn_x, curr_y, btn_w, btn_h));
                curr_y += 70.0f;
                
                if (m_persisted_state.lives > 0)
                {
                    m_pause_buttons.push_back(zwodee::button("Restart Level", btn_x, curr_y, btn_w, btn_h));
                    curr_y += 70.0f;
                }

                m_pause_buttons.push_back(zwodee::button("Settings", btn_x, curr_y, btn_w, btn_h));
                curr_y += 70.0f;
                m_pause_buttons.push_back(zwodee::button("Back to Main Menu", btn_x, curr_y, btn_w, btn_h));
                curr_y += 70.0f;
                m_pause_buttons.push_back(zwodee::button("Exit", btn_x, curr_y, btn_w, btn_h));
            }
            else
            {
                bool sound_enabled = !m_engine->get_audio_manager().is_muted();

                m_sound_switch.set_position(btn_x, 240.0f);
                m_sound_switch.set_size(btn_w, 40.0f);
                m_sound_switch.set_on(sound_enabled);

                m_volume_slider.set_position(btn_x, 315.0f);
                m_volume_slider.set_size(btn_w, 40.0f);
                m_volume_slider.set_value(m_engine->get_audio_manager().get_volume());
                m_volume_slider.set_enabled(sound_enabled);

                m_pause_buttons.clear();

                std::string fps_val = "VSync";
                switch (m_engine->get_fps_limit())
                {
                    case zwodee::engine::fps_limit::vsync:    fps_val = "VSync"; break;
                    case zwodee::engine::fps_limit::fps_60:   fps_val = "60 FPS"; break;
                    case zwodee::engine::fps_limit::fps_144:  fps_val = "144 FPS"; break;
                    case zwodee::engine::fps_limit::fps_240:  fps_val = "240 FPS"; break;
                    case zwodee::engine::fps_limit::fps_360:  fps_val = "360 FPS"; break;
                    case zwodee::engine::fps_limit::fps_480:  fps_val = "480 FPS"; break;
                    case zwodee::engine::fps_limit::unlocked: fps_val = "Unlocked"; break;
                }
                m_pause_buttons.push_back(zwodee::button(fps_val, btn_x, 390.0f, btn_w, 40.0f));
                m_pause_buttons.push_back(zwodee::button("Back", btn_x, 465.0f, btn_w, 40.0f));
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

            if (!m_in_settings)
            {
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
                    int btn_idx = m_pause_selected_index;
                    
                    if (btn_idx == 0) // Resume
                    {
                        m_is_paused = false;
                        m_resume_ticks = 32;
                    }
                    else
                    {
                        if (m_persisted_state.lives > 0)
                        {
                            if (btn_idx == 1) // Restart Level
                            {
                                m_persisted_state.lives--;
                                m_is_paused = false;
                                m_resume_ticks = -1;
                                restart();
                                save_game(m_level_number);
                                return;
                            }
                            btn_idx--;
                        }

                        if (btn_idx == 1) // Settings
                        {
                            m_in_settings = true;
                            m_pause_selected_index = 0;
                            m_volume_slider.reset_drag();
                        }
                        else if (btn_idx == 2) // Back to Main Menu
                        {
                            save_game();
                            m_is_paused = false;
                            m_engine->get_level_manager().transition_to("main_menu");
                        }
                        else if (btn_idx == 3) // Exit
                        {
                            save_game();
                            m_engine->stop();
                        }
                    }
                }
            }
            else
            {
                int total_settings_items = 4;
                bool sound_enabled = !m_engine->get_audio_manager().is_muted();

                if (sound_enabled)
                {
                    if (m_volume_slider.handle_mouse(mx, my, is_left_down, left_clicked))
                    {
                        m_engine->get_audio_manager().set_volume(m_volume_slider.get_value());
                        config_manager::save_config(*m_engine);
                    }
                }

                bool hovered_any = false;
                if (m_sound_switch.is_hovered(mx, my))
                {
                    if (mouse_moved) m_pause_selected_index = 0;
                    hovered_any = true;
                }
                else if (m_volume_slider.is_hovered(mx, my))
                {
                    if (mouse_moved) m_pause_selected_index = 1;
                    hovered_any = true;
                }
                else if (m_pause_buttons[0].is_hovered(mx, my))
                {
                    if (mouse_moved) m_pause_selected_index = 2;
                    hovered_any = true;
                }
                else if (m_pause_buttons[1].is_hovered(mx, my))
                {
                    if (mouse_moved) m_pause_selected_index = 3;
                    hovered_any = true;
                }

                if (m_current_input.is_down(zwodee::input_state::move_up) && !m_last_input.is_down(zwodee::input_state::move_up))
                {
                    m_pause_selected_index = (m_pause_selected_index - 1 + total_settings_items) % total_settings_items;
                }
                else if (m_current_input.is_down(zwodee::input_state::move_down) && !m_last_input.is_down(zwodee::input_state::move_down))
                {
                    m_pause_selected_index = (m_pause_selected_index + 1) % total_settings_items;
                }

                if (m_pause_selected_index == 1 && sound_enabled)
                {
                    if (m_current_input.is_down(zwodee::input_state::move_left) && !m_last_input.is_down(zwodee::input_state::move_left))
                    {
                        m_volume_slider.adjust_value(-0.05f);
                        m_engine->get_audio_manager().set_volume(m_volume_slider.get_value());
                        config_manager::save_config(*m_engine);
                    }
                    else if (m_current_input.is_down(zwodee::input_state::move_right) && !m_last_input.is_down(zwodee::input_state::move_right))
                    {
                        m_volume_slider.adjust_value(+0.05f);
                        m_engine->get_audio_manager().set_volume(m_volume_slider.get_value());
                        config_manager::save_config(*m_engine);
                    }
                }

                bool trigger_action = (m_current_input.is_down(zwodee::input_state::action_1) && !m_last_input.is_down(zwodee::input_state::action_1)) || (left_clicked && hovered_any);

                if (trigger_action)
                {
                    if (m_pause_selected_index == 0) // Sound toggle switch
                    {
                        bool is_muted = m_engine->get_audio_manager().is_muted();
                        m_engine->get_audio_manager().set_muted(!is_muted);
                        m_sound_switch.set_on(is_muted);
                        m_volume_slider.set_enabled(is_muted);
                        config_manager::save_config(*m_engine);
                    }
                    else if (m_pause_selected_index == 2) // FPS Cap toggle
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
                        config_manager::save_config(*m_engine);
                    }
                    else if (m_pause_selected_index == 3) // Back
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
                if (trigger.has_spawned)
                {
                    continue;
                }

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
                            
                            trigger.has_spawned = true;
                        }
                    }
                    else
                    {
                        trigger.cooldown_ticks--;
                    }
                }
            }
        }

        // Update periodic enemy spawners (soldier & mummy 5-second spawners)
        {
            int p_gx = static_cast<int>(std::round(m_player->get_x() / 32.0f));
            int p_gy = static_cast<int>(std::round(m_player->get_y() / 32.0f));

            for (auto& spawner : m_spawners)
            {
                if (spawner.cooldown_ticks > 0)
                {
                    spawner.cooldown_ticks--;
                }
                else
                {
                    // Check if player is NOT currently standing on the spawn tile
                    if (p_gx != spawner.gx || p_gy != spawner.gy)
                    {
                        if (spawner.type == entity_type::soldier_spawner)
                        {
                            auto s = std::make_unique<soldier>(m_next_dynamic_mummy_id++);
                            s->set_grid_position(spawner.gx, spawner.gy);
                            add_entity(std::move(s));
                        }
                        else if (spawner.type == entity_type::mummy_spawner)
                        {
                            auto m = std::make_unique<mummy>(m_next_dynamic_mummy_id++);
                            m->set_grid_position(spawner.gx, spawner.gy);
                            m->trigger_spawn();
                            add_entity(std::move(m));
                        }

                        spawner.cooldown_ticks = 384; // 3 seconds (3 * 128 ticks)
                    }
                }
            }
        }

        if (m_level_finish_sequence_ticks > 0)
        {
            m_level_finish_sequence_ticks--;
            if (m_level_finish_sequence_ticks == 0)
            {
                advance_to_next_level();
            }
            return;
        }

        if (m_level_entry_fade_ticks > 0)
        {
            m_level_entry_fade_ticks--;
            return;
        }

        // Update active explosion hazards and kill player if stepping onto an exploding tile
        for (auto& exp : m_active_explosions)
        {
            if (exp.ticks_remaining > 0)
            {
                exp.ticks_remaining--;
                if (m_player && !m_player->is_dead())
                {
                    int pgx = static_cast<int>(std::round(m_player->get_x() / 32.0f));
                    int pgy = static_cast<int>(std::round(m_player->get_y() / 32.0f));
                    if (pgx == exp.gx && pgy == exp.gy)
                    {
                        m_player->take_damage(999);
                    }
                }
            }
        }
        std::erase_if(m_active_explosions, [](const active_explosion& exp) {
            return exp.ticks_remaining <= 0;
        });

        if (m_player->is_dead())
        {
            if (m_death_sequence_ticks == -1)
            {
                if (auto* audio = m_player->get_audio_manager())
                {
                    audio->play_sound("death");
                }
                m_player->set_texture(texture_cache::get().player_dead_tex.get() ? texture_cache::get().player_dead_tex.get() : texture_cache::get().fallback_tex.get());
                m_death_sequence_ticks = 230; // ~60% of 384 ticks
            }
            else if (m_death_sequence_ticks > 0)
            {
                m_death_sequence_ticks--;

                if (m_death_sequence_ticks == 230 - 32)
                {
                    if (auto* audio = m_player->get_audio_manager())
                    {
                        audio->play_sound("gameover");
                    }
                }

                if (m_death_sequence_ticks == 0)
                {
                    if (m_persisted_state.lives > 0)
                    {
                        m_retry_screen = true;
                        m_retry_selected_index = 0;
                    }
                    else
                    {
                        m_game_over = true;
                        m_game_over_selected_index = 0;
                        std::remove("savegame.dat"); // Wipe the save so players can't resume after Game Over!
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

        // Check if player reaches ANY open exit door
        if (m_exit_open && m_level_finish_sequence_ticks == -1)
        {
            for (const auto& ent : get_entities())
            {
                if (auto* door = dynamic_cast<exit_door*>(ent.get()))
                {
                    if (door->is_open())
                    {
                        float dx = px - door->get_x();
                        float dy = py - door->get_y();
                        if (std::sqrt(dx * dx + dy * dy) < 24.0f)
                        {
                            int next_level = m_level_number + 1;
                            std::string next_file = "assets/levels/level" + std::to_string(next_level) + ".zwl";
                            std::ifstream f(next_file);
                            bool has_next_level = f.good();

                            if (has_next_level)
                            {
                                m_level_finish_sequence_ticks = 96; // Fade to black sequence for next level
                            }
                            else
                            {
                                advance_to_next_level(); // Instantly show win screen without fade
                            }
                            return;
                        }
                    }
                }
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

                    if (dl->reveals_all_diamonds())
                    {
                        // Reveal ALL hidden diamonds on the map!
                        for (const auto& other : get_entities())
                        {
                            if (auto* dm = dynamic_cast<diamond*>(other.get()))
                            {
                                if (!dm->is_dead())
                                {
                                    dm->set_permanently_revealed(true);
                                }
                            }
                        }
                    }
                    else
                    {
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
                                    int player_gx = static_cast<int>(std::round(m_player->get_x() / 32.0f));
                                    if (st_gx == player_gx)
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
                                            if (dynamic_cast<soldier*>(other.get()))
                                            {
                                                if (auto* audio = m_player ? m_player->get_audio_manager() : nullptr)
                                                {
                                                    audio->play_sound("death");
                                                }
                                            }
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

    void level::save_game(int target_level)
    {
        int level_to_save = (target_level == -1) ? m_level_number : target_level;

        savegame_data save_data;
        save_data.current_level = level_to_save;
        
        player_persistent_state state;
        if (m_player)
        {
            state.score = m_player->get_score();
            state.diamonds = m_player->get_diamond_count();
            state.garlic = m_player->get_garlic_count();
            state.onion = m_player->get_onion_count();
            state.has_pickaxe = m_player->has_pickaxe();
            state.lives = m_persisted_state.lives;
        }
        else
        {
            state = m_persisted_state;
        }
        save_data.player_state = state;

        std::ofstream save_file("savegame.dat", std::ios::binary);
        if (save_file.good())
        {
            save_file.write(reinterpret_cast<const char*>(&save_data), sizeof(savegame_data));
            save_file.close();
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
        m_font = texture_cache::get().default_font;
        if (!m_font)
        {
            m_font = std::make_shared<zwodee::font>(engine.get_renderer(), "assets/fonts/Roboto-Medium.ttf", 72.0f);
            texture_cache::get().default_font = m_font;
        }
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
        for (int i = 1; i <= 6; ++i)
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
        m_level_finish_sequence_ticks = -1;
        m_level_entry_fade_ticks = 32;
        m_game_over_selected_index = 0;
        m_fart_effect_ticks = 0;
        m_fart_x = 0.0f;
        m_fart_y = 0.0f;
        m_active_explosions.clear();
        m_mummy_triggers.clear();
        m_spawners.clear();

        m_persisted_state.garlic = 0;
        m_persisted_state.onion = 0;
        m_persisted_state.has_pickaxe = false;

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

            reveal_diamond_at(gx, gy);
        }
    }

    void level::reveal_diamond_at(int gx, int gy)
    {
        for (const auto& ent : get_entities())
        {
            if (auto* dm = dynamic_cast<diamond*>(ent.get()))
            {
                if (std::abs(dm->get_x() - gx * 32.0f) < 8.0f &&
                    std::abs(dm->get_y() - gy * 32.0f) < 8.0f)
                {
                    dm->set_permanently_revealed(true);
                }
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

        if (radius == 0) // Low tier stone: only current tile
        {
            min_x = sgx; max_x = sgx;
            min_y = sgy; max_y = sgy;
        }
        else if (radius == 1) // Mid tier stone: current tile and 1 below
        {
            min_x = sgx; max_x = sgx;
            min_y = sgy; max_y = sgy + 1;
        }
        else if (radius >= 2) // High tier stone: 3x3 area
        {
            min_x = sgx - 1; max_x = sgx + 1;
            min_y = sgy;     max_y = sgy + 2;
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
                    m_active_explosions.push_back({x, y, 128});
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
                    if (dynamic_cast<soldier*>(ent.get()))
                    {
                        if (auto* audio = m_player ? m_player->get_audio_manager() : nullptr)
                        {
                            audio->play_sound("death");
                        }
                    }
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
                int max_ticks = m_player->has_pickaxe() ? m_player->get_pickaxe_dig_ticks() : m_player->get_shovel_dig_ticks();
                if (max_ticks <= 0) max_ticks = 1;
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

            // Explosion visual effect nodes
            if (texture_cache::get().explosion_tex.get())
            {
                for (const auto& exp : m_active_explosions)
                {
                    if (exp.ticks_remaining > 0)
                    {
                        float progress = 1.0f - (static_cast<float>(exp.ticks_remaining) / 128.0f);
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

                        zwodee::render_node exp_node{};
                        exp_node.x = exp.gx * 32.0f;
                        exp_node.y = exp.gy * 32.0f;
                        exp_node.w = 32.0f;
                        exp_node.h = 32.0f;
                        exp_node.tex = texture_cache::get().explosion_tex.get();
                        exp_node.src_x = 0;
                        exp_node.src_y = 0;
                        exp_node.src_w = texture_cache::get().explosion_tex->get_width();
                        exp_node.src_h = texture_cache::get().explosion_tex->get_height();
                        exp_node.flip_horizontal = false;
                        exp_node.flip_vertical = false;
                        exp_node.is_ui = false;
                        exp_node.color_mod = color_val;
                        snapshot.push_back(exp_node);
                    }
                }
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
                        fade = static_cast<float>(m_death_sequence_ticks) / 230.0f;
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
                        (texture_cache::get().explosion_tex.get() && node.tex == texture_cache::get().explosion_tex.get()) ||
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
            float tx = 2.0f; // Reduced padding from left edge of screen

            // We can pre-calculate the positions and widths
            std::vector<hud_item> left_items;
            
            // Onion
            left_items.push_back({texture_cache::get().onion_tex.get(), tx, icon_sz, " " + std::to_string(m_player->get_onion_count()) + " |"});
            tx += icon_sz;
            float o_w = 0.0f;
            for (char c : left_items.back().text) o_w += m_font->get_glyph(c).xadvance * font_scale;
            tx += o_w + 2.0f;

            // Garlic
            left_items.push_back({texture_cache::get().garlic_tex.get(), tx, icon_sz, " " + std::to_string(m_player->get_garlic_count()) + " |"});
            tx += icon_sz;
            float g_w = 0.0f;
            for (char c : left_items.back().text) g_w += m_font->get_glyph(c).xadvance * font_scale;
            tx += g_w + 2.0f;

            // Coin
            int coins_left = std::max(0, m_target_gold - m_player->get_gold_count());
            left_items.push_back({texture_cache::get().coin_tex.get(), tx, icon_sz, " " + std::to_string(coins_left) + " |"});
            tx += icon_sz;
            float c_w = 0.0f;
            for (char c : left_items.back().text) c_w += m_font->get_glyph(c).xadvance * font_scale;
            tx += c_w + 2.0f;

            // Lives
            left_items.push_back({texture_cache::get().goblin_head_tex.get(), tx, icon_sz, " " + std::to_string(m_persisted_state.lives)});
            tx += icon_sz;
            float l_w = 0.0f;
            for (char c : left_items.back().text) l_w += m_font->get_glyph(c).xadvance * font_scale;
            tx += l_w;

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

            auto draw_rounded_bg = [&](float px, float py, float pw, float ph, bool round_bl, bool round_br) {
                float r = 12.0f; // Original nice radius
                float step = 0.05f; // Extremely high precision (20 elements per virtual unit) for 4K smoothness!
                
                // Top main block
                zwodee::render_node bg{};
                bg.tex = nullptr;
                bg.is_ui = true;
                bg.r = 0; bg.g = 0; bg.b = 0; bg.a = 128;
                
                bg.x = px;
                bg.y = py;
                bg.w = pw;
                bg.h = ph - r;
                snapshot.push_back(bg);

                // Bottom rounded corners (drawn strip by sub-pixel strip)
                for (float dy_offset = 0.0f; dy_offset < r; dy_offset += step)
                {
                    // Correct orientation: dy starts near 0 at the top of the curve, and grows to r at the bottom
                    float dy = dy_offset + (step * 0.5f); 
                    if (dy >= r) dy = r - 0.001f;
                    
                    float dx = std::sqrt(r * r - dy * dy);
                    
                    float strip_x = px;
                    float strip_w = pw;
                    
                    if (round_bl)
                    {
                        strip_x += (r - dx);
                        strip_w -= (r - dx);
                    }
                    if (round_br)
                    {
                        strip_w -= (r - dx);
                    }
                    
                    bg.x = strip_x;
                    bg.y = py + ph - r + dy_offset;
                    bg.w = strip_w;
                    bg.h = step; // Exact abutment for top-left rule rasterization (no overlap darkening)
                    
                    snapshot.push_back(bg);
                }
            };

            // 1. Draw Left Panel Background Node (flush with left edge, x=0)
            draw_rounded_bg(0.0f, bg_y, left_panel_w, bg_h, false, true);

            // 2. Draw Right Panel Background Node (flush with right edge)
            draw_rounded_bg(right_panel_x, bg_y, right_panel_w, bg_h, true, false);

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
                draw_shadow_text(item.text, item.x + icon_sz);
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

        // Append Retry Menu overlay
        if (m_retry_screen)
        {
            float screen_w = static_cast<float>(display_w);
            float screen_h = static_cast<float>(display_h);

            // 1. Semi-transparent black overlay
            zwodee::render_node overlay_node{};
            overlay_node.x = 0.0f;
            overlay_node.y = 0.0f;
            overlay_node.w = screen_w;
            overlay_node.h = screen_h;
            overlay_node.tex = nullptr;
            overlay_node.is_ui = true;
            overlay_node.is_blur = true;
            overlay_node.r = 0; overlay_node.g = 0; overlay_node.b = 0; overlay_node.a = 160;
            snapshot.push_back(overlay_node);

            if (m_font)
            {
                std::string retry_text = "YOU DIED";
                float text_scale = 0.8f;
                float text_w = 0.0f;
                for (char c : retry_text) text_w += m_font->get_glyph(c).xadvance * text_scale;
                float tx = (screen_w - text_w) * 0.5f;
                std::vector<zwodee::render_node> text_nodes = m_font->get_text_nodes(retry_text, tx, 100.0f, text_scale, 255, 100, 100, 255);
                for (auto& node : text_nodes) node.is_ui = true;
                snapshot.insert(snapshot.end(), text_nodes.begin(), text_nodes.end());

                std::string lives_text = "You have " + std::to_string(m_persisted_state.lives) + " more tries for this Level.";
                float lives_scale = 0.4f;
                float lives_w = 0.0f;
                for (char c : lives_text) lives_w += m_font->get_glyph(c).xadvance * lives_scale;
                float lx = (screen_w - lives_w) * 0.5f;
                std::vector<zwodee::render_node> lives_nodes = m_font->get_text_nodes(lives_text, lx, 180.0f, lives_scale, 200, 200, 200, 255);
                for (auto& node : lives_nodes) node.is_ui = true;
                snapshot.insert(snapshot.end(), lives_nodes.begin(), lives_nodes.end());

                // 3. Render Buttons
                for (size_t i = 0; i < m_retry_buttons.size(); ++i)
                {
                    m_retry_buttons[i].add_to_snapshot(snapshot, *m_font, m_retry_selected_index == static_cast<int>(i));
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
        if (m_is_paused || m_resume_ticks >= 0)
        {
            float screen_w = static_cast<float>(display_w);
            float screen_h = static_cast<float>(display_h);
            
            float anim_progress;
            float ease;
            if (m_is_paused) {
                anim_progress = std::min(1.0f, static_cast<float>(m_pause_ticks) / 32.0f);
                ease = 1.0f - (1.0f - anim_progress) * (1.0f - anim_progress); // Ease-out
            } else {
                anim_progress = std::max(0.0f, static_cast<float>(m_resume_ticks) / 32.0f);
                ease = anim_progress * anim_progress; // Ease-in (going up)
            }

            // 1. Semi-transparent black overlay with blur effect
            zwodee::render_node overlay_node{};
            overlay_node.x = 0.0f;
            overlay_node.y = 0.0f;
            overlay_node.w = screen_w;
            overlay_node.h = screen_h;
            overlay_node.tex = nullptr;
            overlay_node.is_ui = true;
            overlay_node.is_blur = true;
            overlay_node.r = 0; overlay_node.g = 0; overlay_node.b = 0; 
            overlay_node.a = static_cast<uint8_t>(128 * anim_progress); // Fade overlay
            snapshot.push_back(overlay_node);

            // 2. Animated Title Logo
            if (auto logo_tex = texture_cache::get().logo_tex)
            {
                float logo_w = static_cast<float>(logo_tex->get_width());
                float logo_h = static_cast<float>(logo_tex->get_height());
                float target_logo_w = screen_w * 0.5f;
                float target_logo_h = logo_h * (target_logo_w / logo_w);
                
                float lx = (screen_w - target_logo_w) * 0.5f;
                float ly_start = -target_logo_h;
                float ly_end = 0.0f;
                float ly = ly_start + (ly_end - ly_start) * ease;
                
                zwodee::render_node logo_node;
                logo_node.tex = logo_tex.get();
                logo_node.src_x = 0;
                logo_node.src_y = 0;
                logo_node.src_w = static_cast<int>(logo_w);
                logo_node.src_h = static_cast<int>(logo_h);
                logo_node.x = lx;
                logo_node.y = ly;
                logo_node.w = target_logo_w;
                logo_node.h = target_logo_h;
                logo_node.is_ui = true;
                snapshot.push_back(logo_node);
            }
            else if (m_font)
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
            }

            if (m_font)
            {

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

                // 3. Render Buttons / UI
                if (m_is_paused)
                {
                    if (!m_in_settings)
                    {
                        for (size_t i = 0; i < m_pause_buttons.size(); ++i)
                        {
                            m_pause_buttons[i].add_to_snapshot(snapshot, *m_font, m_pause_selected_index == static_cast<int>(i));
                        }
                    }
                    else
                    {
                        // Sound Switch (Index 0)
                    m_sound_switch.add_to_snapshot(snapshot, *m_font, m_pause_selected_index == 0);

                    // Volume Slider (Index 1)
                    m_volume_slider.add_to_snapshot(snapshot, *m_font, m_pause_selected_index == 1);

                    // FPS Cap Label & Button (Index 2)
                    std::string fps_title = "FPS Cap";
                    float label_scale = 0.35f;
                    float fps_w = 0.0f;
                    for (char c : fps_title) fps_w += m_font->get_glyph(c).xadvance * label_scale;
                    float fx = (screen_w - fps_w) * 0.5f;
                    float fy = m_pause_buttons[0].get_y() - 6.0f;

                    std::vector<zwodee::render_node> fps_nodes = m_font->get_text_nodes(fps_title, fx, fy, label_scale, 200, 200, 200, 255);
                    for (auto& node : fps_nodes) node.is_ui = true;
                    snapshot.insert(snapshot.end(), fps_nodes.begin(), fps_nodes.end());

                    m_pause_buttons[0].add_to_snapshot(snapshot, *m_font, m_pause_selected_index == 2);

                    // Back Button (Index 3)
                    m_pause_buttons[1].add_to_snapshot(snapshot, *m_font, m_pause_selected_index == 3);
                }
                } // End if (m_is_paused)
            }
        }

        // Render Developer Console
        if (m_engine && m_engine->is_console_active() && m_font)
        {
            float screen_w = static_cast<float>(display_w);
            float bar_h = 40.0f;

            // Background bar
            zwodee::render_node bg_node;
            bg_node.tex = nullptr;
            bg_node.x = 0; bg_node.y = 0; bg_node.w = screen_w; bg_node.h = bar_h;
            bg_node.is_ui = true;
            bg_node.is_blur = false;
            bg_node.r = 20; bg_node.g = 20; bg_node.b = 20; bg_node.a = 230;
            snapshot.push_back(bg_node);

            // Text
            std::string console_text = "> " + m_engine->get_console_buffer() + "_";
            auto text_nodes = m_font->get_text_nodes(console_text, 10.0f, 28.0f, 0.3f, 0, 255, 0, 255); // Green text
            for (auto& node : text_nodes) node.is_ui = true;
            snapshot.insert(snapshot.end(), text_nodes.begin(), text_nodes.end());
        }

        // Render black fade transition for finishing level / entering level (if not in Game Over or Win screen)
        if (!m_game_won && !m_game_over)
        {
            if (m_level_finish_sequence_ticks >= 0)
            {
                float progress = 1.0f - (static_cast<float>(m_level_finish_sequence_ticks) / 96.0f);
                progress = std::clamp(progress, 0.0f, 1.0f);

                zwodee::render_node black_overlay{};
                black_overlay.x = 0.0f;
                black_overlay.y = 0.0f;
                black_overlay.w = static_cast<float>(display_w);
                black_overlay.h = static_cast<float>(display_h);
                black_overlay.tex = nullptr;
                black_overlay.is_ui = true;
                black_overlay.r = 0;
                black_overlay.g = 0;
                black_overlay.b = 0;
                black_overlay.a = static_cast<uint8_t>(progress * 255.0f);
                snapshot.push_back(black_overlay);
            }
            else if (m_level_entry_fade_ticks > 0)
            {
                float progress = static_cast<float>(m_level_entry_fade_ticks) / 32.0f;
                progress = std::clamp(progress, 0.0f, 1.0f);

                zwodee::render_node black_overlay{};
                black_overlay.x = 0.0f;
                black_overlay.y = 0.0f;
                black_overlay.w = static_cast<float>(display_w);
                black_overlay.h = static_cast<float>(display_h);
                black_overlay.tex = nullptr;
                black_overlay.is_ui = true;
                black_overlay.r = 0;
                black_overlay.g = 0;
                black_overlay.b = 0;
                black_overlay.a = static_cast<uint8_t>(progress * 255.0f);
                snapshot.push_back(black_overlay);
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

        if (header.width == 0 || header.height == 0)
        {
            return;
        }

        resize(header.width, header.height);
        
        // Read properties
        m_map_properties.clear();
        for (uint32_t i = 0; i < header.property_count; ++i)
        {
            zwodee::binary_property bp;
            if (!in.read(reinterpret_cast<char*>(&bp), sizeof(bp))) break;
            
            // Find length up to null terminator within 32 chars
            size_t len = 0;
            while (len < sizeof(bp.name) && bp.name[len] != '\0') len++;
            std::string name(bp.name, len);
            
            m_map_properties[name] = bp.value;
        }


        
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
        m_mummy_triggers.clear();
        m_spawners.clear();

        // Read entities
        for (uint32_t i = 0; i < header.entity_count; ++i)
        {
            zwodee::binary_entity be;
            if (!in.read(reinterpret_cast<char*>(&be), sizeof(be))) break;

            int gx = static_cast<int>(std::round(be.x / 32.0f));
            int gy = static_cast<int>(std::round(be.y / 32.0f));
            
            if (be.type_id == static_cast<uint32_t>(entity_type::player)) // Player
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
            else if (be.type_id == static_cast<uint32_t>(entity_type::mummy)) // Mummy Trigger
            {
                m_mummy_triggers.push_back({gx, gy, false, 0, false});
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::soldier_spawner)) // Soldier Spawner (3s)
            {
                m_spawners.push_back({gx, gy, entity_type::soldier_spawner, 384});
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::mummy_spawner)) // Mummy Spawner (3s)
            {
                m_spawners.push_back({gx, gy, entity_type::mummy_spawner, 384});
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::soldier)) // Soldier
            {
                auto s = std::make_unique<soldier>(m_next_dynamic_mummy_id++);
                s->set_grid_position(gx, gy);
                add_entity(std::move(s));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::vampire)) // Vampire
            {
                auto v = std::make_unique<vampire>(m_next_dynamic_mummy_id++);
                v->set_grid_position(gx, gy);
                add_entity(std::move(v));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::dragon)) // Dragon
            {
                auto d = std::make_unique<dragon>(m_next_dynamic_mummy_id++);
                d->set_level(this);
                d->set_grid_position(gx, gy);
                add_entity(std::move(d));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::stone_mid) ||
                     be.type_id == static_cast<uint32_t>(entity_type::stone_low) ||
                     be.type_id == static_cast<uint32_t>(entity_type::stone_high)) // Stone
            {
                stone::stone_color col = stone::color_mid;
                if (be.type_id == static_cast<uint32_t>(entity_type::stone_high)) col = stone::color_high;
                else if (be.type_id == static_cast<uint32_t>(entity_type::stone_mid)) col = stone::color_mid;
                else col = stone::color_low;
                
                auto s = std::make_unique<stone>(next_stone_id++, col);
                s->set_grid_position(gx, gy);
                add_entity(std::move(s));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::diamond)) // Revealed Diamond (on level start)
            {
                auto d = std::make_unique<diamond>(next_stone_id++);
                d->set_grid_position(gx, gy);
                d->set_permanently_revealed(true);
                add_entity(std::move(d));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::diamond_hidden)) // Hidden Diamond (Star icon in Tiled)
            {
                auto d = std::make_unique<diamond>(next_stone_id++);
                d->set_grid_position(gx, gy);
                d->set_permanently_revealed(false);
                add_entity(std::move(d));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::gold_coin)) // Gold Coin
            {
                auto c = std::make_unique<gold_coin>(next_stone_id++);
                c->set_grid_position(gx, gy);
                add_entity(std::move(c));
                m_target_gold++;
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::lamp)) // Single Diamond Lamp
            {
                auto l = std::make_unique<lamp>(next_stone_id++);
                l->set_grid_position(gx, gy);
                l->set_reveals_all_diamonds(false);
                add_entity(std::move(l));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::lamp_all)) // All Diamonds Lamp
            {
                auto l = std::make_unique<lamp>(next_stone_id++);
                l->set_grid_position(gx, gy);
                l->set_reveals_all_diamonds(true);
                add_entity(std::move(l));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::garlic)) // Garlic
            {
                auto g = std::make_unique<garlic_bulb>(next_stone_id++);
                g->set_grid_position(gx, gy);
                add_entity(std::move(g));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::onion)) // Onion
            {
                auto o = std::make_unique<onion_bulb>(next_stone_id++);
                o->set_grid_position(gx, gy);
                add_entity(std::move(o));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::pickaxe)) // Pickaxe
            {
                auto p = std::make_unique<pickaxe>(next_stone_id++);
                p->set_grid_position(gx, gy);
                add_entity(std::move(p));
            }
            else if (be.type_id == static_cast<uint32_t>(entity_type::exit_door)) // Exit door
            {
                auto door = std::make_unique<exit_door>(15);
                door->set_grid_position(gx, gy);
                m_exit_x = static_cast<float>(gx * 32);
                m_exit_y = static_cast<float>(gy * 32);
                add_entity(std::move(door));
                
                // Auto-dig the tile where the door is placed
                size_t idx = static_cast<size_t>(gy) * get_width() + static_cast<size_t>(gx);
                if (idx < get_static_objects().size())
                {
                    set_tile(gx, gy, 1, 0, texture_cache::get().digged_tex.get());
                    if (get_static_objects()[idx])
                    {
                        get_static_objects()[idx]->set_collidable(false);
                    }
                }
            }
        }

        if (m_map_properties.count("required_coins"))
        {
            m_target_gold = m_map_properties["required_coins"];
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
        // Apply level completion rewards
        if (m_map_properties.count("add_lives"))
        {
            m_persisted_state.lives += m_map_properties["add_lives"];
        }

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
                state.has_pickaxe = false; // Pickaxe does not carry over to the next level
                state.lives = m_persisted_state.lives;
            }
            new_level->set_persistent_state(state);
            
            // Save game
            save_game(next_level);

            std::string level_id = "play_level_" + std::to_string(next_level);
            m_engine->get_level_manager().register_level(level_id, std::move(new_level));
            m_engine->get_level_manager().transition_to(level_id);
        }
        else
        {
            // WINNER!
            m_level_finish_sequence_ticks = -1;
            m_game_won = true;
            m_game_won_selected_index = 0;
            if (auto* audio = m_player ? m_player->get_audio_manager() : nullptr)
            {
                audio->play_sound("coin_collected"); // Fallback win sound
            }
        }
    }
}
