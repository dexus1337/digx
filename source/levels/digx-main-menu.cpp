#include "levels/digx-main-menu.hpp"
#include "levels/digx-level.hpp"
#include <iostream>

namespace digx
{
    main_menu::main_menu(zwodee::engine& engine)
        : m_engine(engine)
    {
        // Load the TTF font from our assets folder at 72px for high resolution
        m_font = std::make_unique<zwodee::font>(m_engine.get_renderer(), "assets/fonts/Roboto-Medium.ttf", 72.0f);

        // Sync sound state and initialize button layouts
        m_sound_enabled = !m_engine.get_audio_manager().is_muted();
        update_button_layouts();
    }

    void main_menu::on_enter()
    {
        m_selected_index = 0;
        m_in_settings = false;
        
        m_sound_enabled = !m_engine.get_audio_manager().is_muted();
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

        const auto& active_buttons = m_in_settings ? m_settings_buttons : m_main_buttons;

        // Hover detection
        bool hovered_any = false;
        for (size_t i = 0; i < active_buttons.size(); ++i)
        {
            if (active_buttons[i].is_hovered(mx, my))
            {
                m_selected_index = static_cast<int>(i);
                hovered_any = true;
                break;
            }
        }

        // Keyboard navigation
        if (is_key_pressed(zwodee::input_state::move_up))
        {
            m_selected_index = (m_selected_index - 1 + static_cast<int>(active_buttons.size())) % static_cast<int>(active_buttons.size());
        }
        else if (is_key_pressed(zwodee::input_state::move_down))
        {
            m_selected_index = (m_selected_index + 1) % static_cast<int>(active_buttons.size());
        }

        // Selection Action (Enter/Space or left click on hovered button)
        bool trigger_action = is_key_pressed(zwodee::input_state::action_1) || (left_clicked && hovered_any);

        if (trigger_action)
        {
            if (!m_in_settings)
            {
                if (m_selected_index == 0) // Start
                {
                    std::cout << "[Menu] Starting Game..." << std::endl;
                    
                    // Create a clean demo level instance and register it
                    auto level = std::make_unique<digx::level>(35, 35);
                    level->load_demo_level(m_engine);
                    m_engine.get_level_manager().register_level("demo", std::move(level));
                    m_engine.get_level_manager().transition_to("demo");
                }
                else if (m_selected_index == 1) // Settings
                {
                    m_in_settings = true;
                    m_selected_index = 0;
                }
                else if (m_selected_index == 2) // Exit
                {
                    std::cout << "[Menu] Exiting Game..." << std::endl;
                    m_engine.stop();
                }
            }
            else
            {
                if (m_selected_index == 0) // Sound toggle
                {
                    m_sound_enabled = !m_sound_enabled;
                    m_engine.get_audio_manager().set_muted(!m_sound_enabled);
                }
                else if (m_selected_index == 1) // FPS Cap toggle
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
                }
                else if (m_selected_index == 2) // Back
                {
                    m_in_settings = false;
                    m_selected_index = 1; // Highlight settings option
                }
                update_button_layouts();
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
        float screen_h = static_cast<float>(display_h);

        zwodee::render_snapshot snapshot;

        // Render Title Text "DIG X" centered
        std::string title = "DIG X";
        float title_scale = 1.0f; // 72px base size
        
        float title_w = 0.0f;
        if (m_font)
        {
            for (char c : title)
            {
                title_w += m_font->get_glyph(c).xadvance * title_scale;
            }
            float tx = (screen_w - title_w) * 0.5f;
            std::vector<zwodee::render_node> title_nodes = m_font->get_text_nodes(title, tx, 80.0f, title_scale, 255, 215, 0, 255); // Premium Gold title
            for (auto& node : title_nodes)
            {
                node.is_ui = true;
            }
            snapshot.insert(snapshot.end(), title_nodes.begin(), title_nodes.end());

            // Render Sub-Title
            std::string subtitle = m_in_settings ? "Settings" : "Main Menu";
            float sub_scale = 0.45f;
            float sub_w = 0.0f;
            for (char c : subtitle)
            {
                sub_w += m_font->get_glyph(c).xadvance * sub_scale;
            }
            float sx = (screen_w - sub_w) * 0.5f;
            std::vector<zwodee::render_node> sub_nodes = m_font->get_text_nodes(subtitle, sx, 170.0f, sub_scale, 180, 180, 220, 255);
            for (auto& node : sub_nodes)
            {
                node.is_ui = true;
            }
            snapshot.insert(snapshot.end(), sub_nodes.begin(), sub_nodes.end());

            // Render Buttons
            const auto& active_buttons = m_in_settings ? m_settings_buttons : m_main_buttons;
            for (size_t i = 0; i < active_buttons.size(); ++i)
            {
                active_buttons[i].add_to_snapshot(snapshot, *m_font, m_selected_index == static_cast<int>(i));
            }

            // Draw help text
            std::string help = "Use Arrow Keys/W-S & Enter or Mouse to select";
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
        m_main_buttons.push_back(button("Start Game", btn_x, 260.0f, btn_w, btn_h));
        m_main_buttons.push_back(button("Settings", btn_x, 330.0f, btn_w, btn_h));
        m_main_buttons.push_back(button("Exit", btn_x, 400.0f, btn_w, btn_h));

        m_settings_buttons.clear();
        m_settings_buttons.push_back(button(m_sound_enabled ? "Sound: ON" : "Sound: OFF", btn_x, 260.0f, btn_w, btn_h));

        std::string fps_label = "FPS Cap: Unknown";
        switch (m_engine.get_fps_limit())
        {
            case zwodee::engine::fps_limit::vsync:    fps_label = "FPS Cap: VSync"; break;
            case zwodee::engine::fps_limit::fps_60:   fps_label = "FPS Cap: 60 FPS"; break;
            case zwodee::engine::fps_limit::fps_144:  fps_label = "FPS Cap: 144 FPS"; break;
            case zwodee::engine::fps_limit::fps_240:  fps_label = "FPS Cap: 240 FPS"; break;
            case zwodee::engine::fps_limit::fps_360:  fps_label = "FPS Cap: 360 FPS"; break;
            case zwodee::engine::fps_limit::fps_480:  fps_label = "FPS Cap: 480 FPS"; break;
            case zwodee::engine::fps_limit::unlocked: fps_label = "FPS Cap: Unlocked"; break;
        }
        m_settings_buttons.push_back(button(fps_label, btn_x, 330.0f, btn_w, btn_h));
        m_settings_buttons.push_back(button("Back", btn_x, 400.0f, btn_w, btn_h));
    }
}
