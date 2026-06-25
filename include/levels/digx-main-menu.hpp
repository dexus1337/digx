#pragma once

#include "zwodee.hpp"
#include "levels/digx-button.hpp"
#include <vector>
#include <memory>

namespace digx
{
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
        [[nodiscard]] zwodee::render_snapshot get_render_snapshot(int display_w, int display_h) const override;

    private:
        zwodee::engine& m_engine;
        std::unique_ptr<zwodee::font> m_font;
        std::shared_ptr<zwodee::texture> m_logo_tex;

        int m_selected_index = 0;
        bool m_in_settings = false;

        // Settings option toggles
        bool m_sound_enabled = true;

        std::vector<button> m_main_buttons;
        std::vector<button> m_settings_buttons;

        zwodee::input_state m_last_input{};
        zwodee::input_state m_current_input{};

        // Check if key is pressed (was up, now down)
        [[nodiscard]] bool is_key_pressed(zwodee::input_state::button_mask btn) const;
        void update_button_layouts();
    };
}
