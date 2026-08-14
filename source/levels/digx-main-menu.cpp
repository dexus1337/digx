#include "levels/digx-main-menu.hpp"
#include "levels/digx-level.hpp"
#include "config-manager.hpp"
#include "assets/texture-cache.hpp"
#include <iostream>
#include <fstream>
#include <string_view>
#include <SDL3/SDL.h>

namespace digx
{
    main_menu::main_menu(zwodee::engine& engine)
        : m_engine(engine),
          m_sound_switch("Sound Effects", true, 0.0f, 0.0f, 300.0f, 40.0f),
          m_volume_slider("Audio Volume", engine.get_audio_manager().get_volume(), 0.0f, 1.0f, 0.0f, 0.0f, 300.0f, 40.0f)
    {
        // Load the TTF font from our assets folder at 72px for high resolution
        m_font = texture_cache::get().default_font;
        if (!m_font)
        {
            m_font = std::make_shared<zwodee::font>(m_engine.get_renderer(), "assets/fonts/Roboto-Medium.ttf", 72.0f);
            texture_cache::get().default_font = m_font;
        }
        m_logo_tex = m_engine.get_renderer().load_dds_texture("assets/textures/mainmenu-text.dds");

        // Sync sound state and initialize button layouts
        m_sound_enabled = !m_engine.get_audio_manager().is_muted();
        update_button_layouts();
    }

    void main_menu::on_enter()
    {
        m_selected_index = 0;
        m_in_settings = false;
        
        m_has_savegame = false;
        std::ifstream save_file("savegame.dat", std::ios::binary);
        if (save_file.good())
        {
            savegame_data data;
            if (save_file.read(reinterpret_cast<char*>(&data), sizeof(data)))
            {
                if (std::string_view(data.magic, 4) == std::string_view("DIGS", 4) && data.version == 1)
                {
                    m_save_data = data;
                    m_has_savegame = true;
                }
            }
        }

        m_sound_enabled = !m_engine.get_audio_manager().is_muted();
        m_volume_slider.set_value(m_engine.get_audio_manager().get_volume());
        update_button_layouts();
    }

    void main_menu::on_exit()
    {
    }

    void main_menu::set_player_input(const zwodee::input_state& input)
    {
        m_last_input = m_current_input;
        m_current_input = input;
    }

    bool main_menu::is_key_pressed(zwodee::input_state::button_mask btn) const
    {
        return m_current_input.is_down(btn) && !m_last_input.is_down(btn);
    }

    void main_menu::tick()
    {
        // Update layouts dynamically to fit current screen size
        update_button_layouts();

        float mx = 0.0f, my = 0.0f;
        uint32_t mouse_buttons = SDL_GetMouseState(&mx, &my);
        float scale = m_engine.get_window().get_scale_factor();
        mx /= scale;
        my /= scale;
        bool is_left_down = (mouse_buttons & SDL_BUTTON_LMASK) != 0;

        static bool was_left_down = false;
        bool left_clicked = is_left_down && !was_left_down;
        was_left_down = is_left_down;

        static float prev_mx = -1.0f;
        static float prev_my = -1.0f;
        bool mouse_moved = (mx != prev_mx || my != prev_my);
        prev_mx = mx;
        prev_my = my;

        if (!m_in_settings)
        {
            bool hovered_any = false;
            for (size_t i = 0; i < m_main_buttons.size(); ++i)
            {
                if (m_main_buttons[i].is_hovered(mx, my))
                {
                    if (mouse_moved)
                    {
                        m_selected_index = static_cast<int>(i);
                    }
                    hovered_any = true;
                    break;
                }
            }

            // Keyboard navigation
            if (is_key_pressed(zwodee::input_state::move_up))
            {
                m_selected_index = (m_selected_index - 1 + static_cast<int>(m_main_buttons.size())) % static_cast<int>(m_main_buttons.size());
            }
            else if (is_key_pressed(zwodee::input_state::move_down))
            {
                m_selected_index = (m_selected_index + 1) % static_cast<int>(m_main_buttons.size());
            }

            // Selection Action
            bool trigger_action = is_key_pressed(zwodee::input_state::action_1) || (left_clicked && hovered_any);

            if (trigger_action)
            {
                int btn_idx = m_selected_index;
                
                if (m_has_savegame)
                {
                    if (btn_idx == 0) // Resume
                    {
                        std::cout << "[Menu] Resuming Game..." << std::endl;
                        auto level = std::make_unique<digx::level>(35, 35, m_save_data.current_level);
                        std::string level_name = "level" + std::to_string(m_save_data.current_level);
                        level->init(m_engine, level_name);
                        level->set_persistent_state(m_save_data.player_state);
                        
                        std::string level_id = "play_level_" + std::to_string(m_save_data.current_level);
                        m_engine.get_level_manager().register_level(level_id, std::move(level));
                        m_engine.get_level_manager().transition_to(level_id);
                        return;
                    }
                    btn_idx--;
                }

                if (btn_idx == 0) // Start New Game
                {
                    std::cout << "[Menu] Starting New Game..." << std::endl;
                    
                    auto level = std::make_unique<digx::level>(35, 35, 1);
                    level->init(m_engine, "level1");
                    
                    m_engine.get_level_manager().register_level("play_level_1", std::move(level));
                    m_engine.get_level_manager().transition_to("play_level_1");
                }
                else if (btn_idx == 1) // Settings
                {
                    m_in_settings = true;
                    m_selected_index = 0;
                    m_volume_slider.reset_drag();
                }
                else if (btn_idx == 2) // Exit
                {
                    std::cout << "[Menu] Exiting Game..." << std::endl;
                    m_engine.stop();
                }
            }
        }
        else
        {
            // Settings menu layout: Index 0 = Sound Switch, Index 1 = Volume Slider, Index 2 = FPS Cap, Index 3 = Back
            int total_settings_items = 4;

            if (m_sound_enabled)
            {
                if (m_volume_slider.handle_mouse(mx, my, is_left_down, left_clicked))
                {
                    m_engine.get_audio_manager().set_volume(m_volume_slider.get_value());
                    config_manager::save_config(m_engine);
                }
            }

            bool hovered_any = false;
            if (m_sound_switch.is_hovered(mx, my))
            {
                if (mouse_moved) m_selected_index = 0;
                hovered_any = true;
            }
            else if (m_volume_slider.is_hovered(mx, my))
            {
                if (mouse_moved) m_selected_index = 1;
                hovered_any = true;
            }
            else if (m_settings_buttons[0].is_hovered(mx, my))
            {
                if (mouse_moved) m_selected_index = 2;
                hovered_any = true;
            }
            else if (m_settings_buttons[1].is_hovered(mx, my))
            {
                if (mouse_moved) m_selected_index = 3;
                hovered_any = true;
            }

            // Keyboard navigation
            if (is_key_pressed(zwodee::input_state::move_up))
            {
                m_selected_index = (m_selected_index - 1 + total_settings_items) % total_settings_items;
            }
            else if (is_key_pressed(zwodee::input_state::move_down))
            {
                m_selected_index = (m_selected_index + 1) % total_settings_items;
            }

            // Slider Left / Right adjustment
            if (m_selected_index == 1 && m_sound_enabled)
            {
                if (is_key_pressed(zwodee::input_state::move_left))
                {
                    m_volume_slider.adjust_value(-0.05f);
                    m_engine.get_audio_manager().set_volume(m_volume_slider.get_value());
                    config_manager::save_config(m_engine);
                }
                else if (is_key_pressed(zwodee::input_state::move_right))
                {
                    m_volume_slider.adjust_value(+0.05f);
                    m_engine.get_audio_manager().set_volume(m_volume_slider.get_value());
                    config_manager::save_config(m_engine);
                }
            }

            // Selection action
            bool trigger_action = is_key_pressed(zwodee::input_state::action_1) || (left_clicked && hovered_any);

            if (trigger_action)
            {
                if (m_selected_index == 0) // Sound toggle switch
                {
                    m_sound_enabled = !m_sound_enabled;
                    m_sound_switch.set_on(m_sound_enabled);
                    m_engine.get_audio_manager().set_muted(!m_sound_enabled);
                    update_button_layouts();
                    config_manager::save_config(m_engine);
                }
                else if (m_selected_index == 2) // FPS Cap toggle
                {
                    zwodee::engine::fps_limit next_limit = zwodee::engine::fps_limit::vsync;
                    switch (m_engine.get_fps_limit())
                    {
                        case zwodee::engine::fps_limit::vsync:    next_limit = zwodee::engine::fps_limit::fps_60; break;
                        case zwodee::engine::fps_limit::fps_60:   next_limit = zwodee::engine::fps_limit::fps_144; break;
                        case zwodee::engine::fps_limit::fps_144:  next_limit = zwodee::engine::fps_limit::fps_240; break;
                        case zwodee::engine::fps_limit::fps_240:  next_limit = zwodee::engine::fps_limit::fps_360; break;
                        case zwodee::engine::fps_limit::fps_360:  next_limit = zwodee::engine::fps_limit::fps_480; break;
                        case zwodee::engine::fps_limit::fps_480:  next_limit = zwodee::engine::fps_limit::unlocked; break;
                        case zwodee::engine::fps_limit::unlocked: next_limit = zwodee::engine::fps_limit::vsync; break;
                    }
                    m_engine.set_fps_limit(next_limit);
                    update_button_layouts();
                    config_manager::save_config(m_engine);
                }
                else if (m_selected_index == 3) // Back
                {
                    m_in_settings = false;
                    m_selected_index = 1; // Highlight settings option
                    update_button_layouts();
                }
            }
        }
    }

    void main_menu::render(zwodee::renderer& target_renderer, double alpha)
    {
        (void)target_renderer;
        (void)alpha;
    }

    zwodee::render_snapshot main_menu::get_render_snapshot(int display_w, int display_h) const
    {
        float screen_w = static_cast<float>(display_w);
        zwodee::render_snapshot snapshot;

        // Render Title Logo centered at the top
        if (m_logo_tex)
        {
            float logo_w = static_cast<float>(m_logo_tex->get_width());
            float logo_h = static_cast<float>(m_logo_tex->get_height());
            float target_logo_w = screen_w * 0.5f;
            float target_logo_h = logo_h * (target_logo_w / logo_w);
            float lx = (screen_w - target_logo_w) * 0.5f;
            float ly = 0.0f; // No gap on top
            
            zwodee::render_node logo_node;
            logo_node.tex = m_logo_tex.get();
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
            std::string title = "DIG X";
            float title_scale = 1.0f; // 72px base size
            float title_w = 0.0f;
            for (char c : title)
            {
                title_w += m_font->get_glyph(c).xadvance * title_scale;
            }
            float tx = (screen_w - title_w) * 0.5f;
            std::vector<zwodee::render_node> title_nodes = m_font->get_text_nodes(title, tx, 80.0f, title_scale, 255, 215, 0, 255);
            for (auto& node : title_nodes)
            {
                node.is_ui = true;
            }
            snapshot.insert(snapshot.end(), title_nodes.begin(), title_nodes.end());
        }

        if (m_font)
        {


            // Render Buttons / UI
            if (!m_in_settings)
            {
                for (size_t i = 0; i < m_main_buttons.size(); ++i)
                {
                    m_main_buttons[i].add_to_snapshot(snapshot, *m_font, m_selected_index == static_cast<int>(i));
                }
            }
            else
            {
                // Sound Switch (Index 0)
                m_sound_switch.add_to_snapshot(snapshot, *m_font, m_selected_index == 0);

                // Volume Slider (Index 1)
                m_volume_slider.add_to_snapshot(snapshot, *m_font, m_selected_index == 1);

                // FPS Cap Label & Button (Index 2)
                std::string fps_title = "FPS Cap";
                float label_scale = 0.35f;
                float fps_w = 0.0f;
                for (char c : fps_title) fps_w += m_font->get_glyph(c).xadvance * label_scale;
                float fx = (screen_w - fps_w) * 0.5f;
                float fy = m_settings_buttons[0].get_y() - 6.0f;

                std::vector<zwodee::render_node> fps_nodes = m_font->get_text_nodes(fps_title, fx, fy, label_scale, 200, 200, 200, 255);
                for (auto& node : fps_nodes) node.is_ui = true;
                snapshot.insert(snapshot.end(), fps_nodes.begin(), fps_nodes.end());

                m_settings_buttons[0].add_to_snapshot(snapshot, *m_font, m_selected_index == 2);

                // Back Button (Index 3)
                m_settings_buttons[1].add_to_snapshot(snapshot, *m_font, m_selected_index == 3);
            }

            // Draw help text
            std::string help = m_in_settings ? "Use Arrow Keys (Left/Right to adjust volume) & Enter or Mouse" : "Use Arrow Keys/W-S & Enter or Mouse to select";
            float help_scale = 0.28f;
            float help_w = 0.0f;
            for (char c : help)
            {
                help_w += m_font->get_glyph(c).xadvance * help_scale;
            }
            float hx = (screen_w - help_w) * 0.5f;
            std::vector<zwodee::render_node> help_nodes = m_font->get_text_nodes(help, hx, 560.0f, help_scale, 120, 120, 140, 255);
            for (auto& node : help_nodes)
            {
                node.is_ui = true;
            }
            snapshot.insert(snapshot.end(), help_nodes.begin(), help_nodes.end());
        }

        return snapshot;
    }

    void main_menu::update_button_layouts()
    {
        float screen_w = static_cast<float>(m_engine.get_window().get_width());
        float btn_w = 300.0f;
        float btn_h = 50.0f;
        float btn_x = (screen_w - btn_w) * 0.5f;

        m_main_buttons.clear();
        
        float start_y = 260.0f;
        
        if (m_has_savegame)
        {
            m_main_buttons.push_back(zwodee::button("Resume", btn_x, start_y, btn_w, btn_h));
            start_y += 70.0f;
        }

        m_main_buttons.push_back(zwodee::button("Start New Game", btn_x, start_y, btn_w, btn_h));
        start_y += 70.0f;
        
        m_main_buttons.push_back(zwodee::button("Settings", btn_x, start_y, btn_w, btn_h));
        start_y += 70.0f;
        
        m_main_buttons.push_back(zwodee::button("Exit", btn_x, start_y, btn_w, btn_h));

        m_sound_enabled = !m_engine.get_audio_manager().is_muted();

        m_sound_switch.set_position(btn_x, 240.0f);
        m_sound_switch.set_size(btn_w, 40.0f);
        m_sound_switch.set_on(m_sound_enabled);

        m_volume_slider.set_position(btn_x, 315.0f);
        m_volume_slider.set_size(btn_w, 40.0f);
        m_volume_slider.set_value(m_engine.get_audio_manager().get_volume());
        m_volume_slider.set_enabled(m_sound_enabled);

        m_settings_buttons.clear();

        std::string fps_val = "VSync";
        switch (m_engine.get_fps_limit())
        {
            case zwodee::engine::fps_limit::vsync:    fps_val = "VSync"; break;
            case zwodee::engine::fps_limit::fps_60:   fps_val = "60 FPS"; break;
            case zwodee::engine::fps_limit::fps_144:  fps_val = "144 FPS"; break;
            case zwodee::engine::fps_limit::fps_240:  fps_val = "240 FPS"; break;
            case zwodee::engine::fps_limit::fps_360:  fps_val = "360 FPS"; break;
            case zwodee::engine::fps_limit::fps_480:  fps_val = "480 FPS"; break;
            case zwodee::engine::fps_limit::unlocked: fps_val = "Unlocked"; break;
        }
        m_settings_buttons.push_back(zwodee::button(fps_val, btn_x, 390.0f, btn_w, 40.0f));
        m_settings_buttons.push_back(zwodee::button("Back", btn_x, 465.0f, btn_w, 40.0f));
    }
}
