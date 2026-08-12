#pragma once

#include "zwodee.hpp"
#include <vector>
#include <memory>

#include "levels/digx-level.hpp"

namespace digx
{
    struct savegame_data
    {
        char magic[4] = {'D','I','G','S'};
        int version = 1;
        int current_level;
        digx::level::player_persistent_state player_state;
    };

    class main_menu : public zwodee::level
    {
    public:
        explicit main_menu(zwodee::engine& engine);
        ~main_menu() override = default;

        void on_enter() override;
        void on_exit() override;
        void set_player_input(const zwodee::input_state& input) override;
        void tick() override;
        void render(zwodee::renderer& target_renderer, double alpha) override;
        zwodee::render_snapshot get_render_snapshot(int display_w, int display_h) const override;

    private:
        zwodee::engine& m_engine;
        std::unique_ptr<zwodee::font> m_font;
        std::shared_ptr<zwodee::texture> m_logo_tex;

        int m_selected_index = 0;
        bool m_in_settings = false;

        // Settings option toggles
        bool m_sound_enabled = true;

        bool m_has_savegame = false;
        savegame_data m_save_data{};

        std::vector<zwodee::button> m_main_buttons;
        std::vector<zwodee::button> m_settings_buttons;
        zwodee::toggle_switch m_sound_switch;
        zwodee::slider m_volume_slider;

        zwodee::input_state m_last_input{};
        zwodee::input_state m_current_input{};

        // Check if key is pressed (was up, now down)
        bool is_key_pressed(zwodee::input_state::button_mask btn) const;
        void update_button_layouts();
    };
}
