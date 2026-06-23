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
        player(uint32_t network_id, 
               const zwodee::texture* shovel_idle_tex, 
               const zwodee::texture* shovel_running_tex, 
               const zwodee::texture* pickaxe_idle_tex, 
               const zwodee::texture* pickaxe_running_tex, 
               zwodee::audio_manager* audio);

        void tick() override;

        void set_digging(bool is_digging);

        void collect_gold(int amount = 1);
        void collect_diamond(int amount = 1);
        void collect_garlic(int amount = 1);
        void collect_onion(int amount = 1);
        void obtain_pickaxe();
        void respawn(float x, float y);

        [[nodiscard]] int get_gold_count() const;
        [[nodiscard]] int get_diamond_count() const;
        [[nodiscard]] int get_garlic_count() const;
        [[nodiscard]] int get_onion_count() const;
        [[nodiscard]] bool has_pickaxe() const;
        [[nodiscard]] int get_score() const;

        [[nodiscard]] float get_fart_active_time() const;
        [[nodiscard]] float get_breath_active_time() const;

        void set_grid_bounds(int cols, int rows);
        void set_level(zwodee::tile_level* lvl);
        [[nodiscard]] zwodee::audio_manager* get_audio_manager() const;

    private:
        zwodee::tile_level* m_level = nullptr;
        int m_gold_collected = 0;
        int m_diamonds_collected = 0;
        int m_garlic_count = 0;
        int m_onion_count = 0;
        bool m_has_pickaxe = false;
        int m_score = 0;

        float m_tunnel_speed = 1.0f;
        float m_shovel_speed = 0.5f;
        float m_pickaxe_speed = 0.75f;

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

        const zwodee::texture* m_shovel_idle_tex = nullptr;
        const zwodee::texture* m_shovel_running_tex = nullptr;
        const zwodee::texture* m_pickaxe_idle_tex = nullptr;
        const zwodee::texture* m_pickaxe_running_tex = nullptr;
        zwodee::audio_manager* m_audio = nullptr;
        bool m_facing_left = false;
    };
}
