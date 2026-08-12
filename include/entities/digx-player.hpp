#pragma once

/**
 * @file    digx-player.hpp
 * @author  dexus1337
 * @brief   Defines the player class representing the goblin miner character.
 * @version 1.0
 * @date    22.06.2026
 */

#include "zwodee.hpp"

namespace digx
{
    class player : public zwodee::entity_player
    {
    public:
        static constexpr int shovel_digging_ticks  = 90;
        static constexpr int pickaxe_digging_ticks = 40;

        player(uint32_t network_id, zwodee::audio_manager* audio);

        void tick() override;

        void set_digging(bool is_digging);

        void collect_gold(int amount = 1);
        void collect_diamond(int amount = 1);
        void collect_garlic(int amount = 1);
        void use_garlic();
        void collect_onion(int amount = 1);
        void obtain_pickaxe();
        void respawn(float x, float y);

        void apply_persistent_state(int score, int diamonds, int garlic, int onion, bool pickaxe);

        int get_gold_count() const;
        int get_diamond_count() const;
        int get_garlic_count() const;
        int get_onion_count() const;
        bool has_pickaxe() const;
        void set_has_pickaxe(bool pickaxe) { m_has_pickaxe = pickaxe; }
        
        void set_shovel_dig_ticks(int ticks) { m_shovel_dig_ticks = ticks; }
        int get_shovel_dig_ticks() const { return m_shovel_dig_ticks; }
        
        void set_pickaxe_dig_ticks(int ticks) { m_pickaxe_dig_ticks = ticks; }
        int get_pickaxe_dig_ticks() const { return m_pickaxe_dig_ticks; }

        void set_tunnel_speed(float speed) { m_tunnel_speed = speed; set_speed(speed); }
        float get_tunnel_speed() const { return m_tunnel_speed; }

        int get_score() const;

        float get_fart_active_time() const;
        float get_breath_active_time() const;

        void set_grid_bounds(int cols, int rows);
        void set_level(zwodee::tile_level* lvl);
        zwodee::tile_level* get_level() const;
        zwodee::audio_manager* get_audio_manager() const;
        bool is_digging() const { return m_is_digging; }
        int get_digging_ticks_remaining() const { return m_digging_ticks_remaining; }
        float get_target_x() const { return m_target_x; }
        float get_target_y() const { return m_target_y; }
        bool is_facing_left() const { return m_facing_left; }

    private:
        zwodee::tile_level* m_level = nullptr;
        int m_gold_collected = 0;
        int m_diamonds_collected = 0;
        int m_garlic_count = 0;
        int m_onion_count = 0;
        bool m_has_pickaxe = false;
        int m_score = 0;

        float m_tunnel_speed = 1.0f;
        int m_shovel_dig_ticks = shovel_digging_ticks;
        int m_pickaxe_dig_ticks = pickaxe_digging_ticks;
        bool m_is_digging = false;
        int m_digging_ticks_remaining = 0;

        int m_fart_cooldown = 0;
        int m_breath_cooldown = 0;

        // Grid movement control
        float m_target_x = 0.0f;
        float m_target_y = 0.0f;
        float m_dir_x = 0.0f;
        float m_dir_y = 0.0f;
        bool m_is_moving = false;

        int m_level_cols = 20;
        int m_level_rows = 15;
        bool m_initialized_grid = false;

        std::vector<zwodee::input_state::button_mask> m_horiz_history;
        std::vector<zwodee::input_state::button_mask> m_vert_history;
        zwodee::input_state m_prev_input;

        bool is_tile_blocked(float tx, float ty) const;
        class stone* get_stone_at(float tx, float ty) const;
        bool is_tile_clear_for_stone(float tx, float ty) const;
        bool can_player_move_to(float next_target_x, float next_target_y, float dir_x, float dir_y);

        // Contrary release buffers
        int m_contrary_release_buffer_x = 0;
        int m_contrary_release_buffer_y = 0;
        bool m_was_conflicting_horiz = false;
        bool m_was_conflicting_vert = false;

        // Queued movement for tap-move and buffer-turn features
        float m_queued_dir_x = 0.0f;
        float m_queued_dir_y = 0.0f;
        bool m_has_queued_move = false;
        int m_queued_steps = 0;

        int m_run_anim_ticks = 0;
        
        zwodee::audio_manager* m_audio = nullptr;
        bool m_facing_left = false;
    };
}
