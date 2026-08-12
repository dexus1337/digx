#pragma once

/**
 * @file    digx-level.hpp
 * @author  dexus1337
 * @brief   Defines the level class.
 * @version 1.0
 * @date    22.06.2026
 */

#include "zwodee.hpp"
#include <array>

namespace digx
{
    enum class entity_type : uint32_t
    {
        player = 2,
        mummy = 3,
        soldier = 4,
        vampire = 5,
        dragon = 6,
        stone_mid = 7,
        stone_low = 8,
        stone_high = 9,
        diamond = 10,
        gold_coin = 11,
        lamp = 12,
        garlic = 13,
        onion = 14,
        pickaxe = 15,
        exit_door = 16,
        diamond_hidden = 17,
        lamp_all = 18
    };

    class player;

    class level : public zwodee::tile_level
    {
    public:
        level(uint32_t width, uint32_t height, int level_number = 1);
        ~level() override = default;

        void on_enter() override;
        void on_exit() override;
        void set_player_input(const zwodee::input_state& input) override;
        void tick() override;
        void render(zwodee::renderer& target_renderer, double alpha) override;
        zwodee::render_snapshot get_render_snapshot(int display_w, int display_h) const override;

        void init(zwodee::engine& engine, const std::string& level_name);
        void load_from_zwl(const std::string& path);
        void restart();

        player* get_player() const;


        bool is_tile_digged(int gx, int gy) const;
        void dig_tile(int gx, int gy);
        void explode_stone(class stone* st, int custom_radius = -1);
        void trigger_fart_effect(float x, float y);
        bool is_fart_active() const { return m_fart_effect_ticks > 0; }
        float get_fart_x() const { return m_fart_x; }
        float get_fart_y() const { return m_fart_y; }

    private:
        player* m_player = nullptr;


        // Target number of gold coins to collect to open the exit
        int m_target_gold = 0;
        bool m_exit_open = false;
        
        // Exit coordinates
        float m_exit_x = 0.0f;
        float m_exit_y = 0.0f;
        
        // Track visual state of lamp (reveal diamonds)
        float m_lamp_timer = 0.0f;

        int m_level_number = 1;
        std::string m_level_name;
        float m_current_darkness = 1.0f;
        float m_target_darkness = 1.0f;
        zwodee::engine* m_engine = nullptr;

        struct spawn_trigger_tile
        {
            int gx = 0;
            int gy = 0;
            bool triggered = false;
            int cooldown_ticks = 0;
        };
        std::vector<spawn_trigger_tile> m_mummy_triggers;
        uint32_t m_next_dynamic_mummy_id = 5000;

        // Fart visual effect tracking
        int m_fart_effect_ticks = 0;
        float m_fart_x = 0.0f;
        float m_fart_y = 0.0f;

        // Explosion visual effect & hazard tracking
        struct active_explosion {
            int gx = 0;
            int gy = 0;
            int ticks_remaining = 128;
        };
        std::vector<active_explosion> m_active_explosions;

        // Pause state members
        bool m_is_paused = false;
        bool m_in_settings = false;
        int m_pause_selected_index = 0;
        zwodee::input_state m_last_input{};
        zwodee::input_state m_current_input{};
        std::unique_ptr<zwodee::font> m_font;
        std::vector<zwodee::button> m_pause_buttons;
        zwodee::toggle_switch m_sound_switch;
        zwodee::slider m_volume_slider;
        uint32_t m_ignored_buttons = 0;
        bool m_first_input = true;

        // Game Over state members
        bool m_game_over = false;
        int m_death_sequence_ticks = -1;
        int m_level_finish_sequence_ticks = -1;
        int m_level_entry_fade_ticks = 32;
        std::vector<zwodee::button> m_game_over_buttons;
        int m_game_over_selected_index = 0;

        // Game Won (Winner) state members
        bool m_game_won = false;
        std::vector<zwodee::button> m_game_won_buttons;
        int m_game_won_selected_index = 0;

    public:
        struct player_persistent_state {
            int score = 0;
            int diamonds = 0;
            int garlic = 0;
            int onion = 0;
            bool has_pickaxe = false;
        };

        void set_persistent_state(const player_persistent_state& state);
        void advance_to_next_level();
        void save_game(int target_level = -1);

    private:
        player_persistent_state m_persisted_state;
        bool m_has_persisted_state = false;
    };
}
