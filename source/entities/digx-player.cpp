#include "entities/digx-player.hpp"

#include <algorithm>
#include <cmath>
#include "levels/digx-level.hpp"
#include "entities/digx-stone.hpp"

namespace
{
    void play_footstep(zwodee::audio_manager* audio)
    {
        if (audio)
        {
            int rand_idx = (std::rand() % 8) + 1;
            audio->play_sound("running_" + std::to_string(rand_idx));
        }
    }

    void play_dig_sound(zwodee::audio_manager* audio, bool is_pickaxe)
    {
        if (audio)
        {
            int rand_idx = (std::rand() % 4) + 1;
            std::string sound_name = (is_pickaxe ? "pickaxe_dig_" : "shovel_dig_") + std::to_string(rand_idx);
            audio->play_sound(sound_name);
        }
    }
}

namespace digx
{
    player::player(uint32_t network_id, 
                   const zwodee::texture* shovel_idle_tex, 
                   const zwodee::texture* shovel_running_tex, 
                   const zwodee::texture* shovel_running_up_tex,
                   const zwodee::texture* shovel_running_down_tex,
                   const zwodee::texture* pickaxe_idle_tex, 
                   const zwodee::texture* pickaxe_running_tex, 
                   const zwodee::texture* pickaxe_running_up_tex,
                   const zwodee::texture* pickaxe_running_down_tex,
                   zwodee::audio_manager* audio)
        : zwodee::entity_player(network_id, shovel_idle_tex, 100),
          m_shovel_idle_tex(shovel_idle_tex),
          m_shovel_running_tex(shovel_running_tex),
          m_shovel_running_up_tex(shovel_running_up_tex),
          m_shovel_running_down_tex(shovel_running_down_tex),
          m_pickaxe_idle_tex(pickaxe_idle_tex),
          m_pickaxe_running_tex(pickaxe_running_tex),
          m_pickaxe_running_up_tex(pickaxe_running_up_tex),
          m_pickaxe_running_down_tex(pickaxe_running_down_tex),
          m_audio(audio)
    {
        set_speed(m_tunnel_speed);
        configure_animator(1, 1, true);
    }

    void player::tick()
    {
        if (!m_initialized_grid)
        {
            m_target_x = m_x;
            m_target_y = m_y;
            m_initialized_grid = true;
        }

        if (m_fart_cooldown > 0)
        {
            m_fart_cooldown--;
        }
        if (m_breath_cooldown > 0)
        {
            m_breath_cooldown--;
        }

        if (m_current_input.is_down(zwodee::input_state::action_1))
        {
            if (m_garlic_count > 0 && m_fart_cooldown == 0)
            {
                m_garlic_count--;
                m_fart_cooldown = 128 * 3;
            }
        }

        if (m_current_input.is_down(zwodee::input_state::action_2))
        {
            if (m_onion_count > 0 && m_breath_cooldown == 0)
            {
                m_onion_count--;
                m_breath_cooldown = 128 * 5;
            }
        }

        // Track new keypresses and add them to horizontal/vertical history
        uint32_t pressed = m_current_input.buttons & ~m_prev_input.buttons;
        if (pressed & zwodee::input_state::move_left) m_horiz_history.push_back(zwodee::input_state::move_left);
        if (pressed & zwodee::input_state::move_right) m_horiz_history.push_back(zwodee::input_state::move_right);
        if (pressed & zwodee::input_state::move_up) m_vert_history.push_back(zwodee::input_state::move_up);
        if (pressed & zwodee::input_state::move_down) m_vert_history.push_back(zwodee::input_state::move_down);

        // Remove released keys from history
        m_horiz_history.erase(
            std::remove_if(m_horiz_history.begin(), m_horiz_history.end(),
                [this](zwodee::input_state::button_mask btn) {
                    return !m_current_input.is_down(btn);
                }),
            m_horiz_history.end()
        );
        m_vert_history.erase(
            std::remove_if(m_vert_history.begin(), m_vert_history.end(),
                [this](zwodee::input_state::button_mask btn) {
                    return !m_current_input.is_down(btn);
                }),
            m_vert_history.end()
        );

        // Fallback: populate history if empty but keys are down
        if (m_horiz_history.empty())
        {
            if (m_current_input.is_down(zwodee::input_state::move_left)) m_horiz_history.push_back(zwodee::input_state::move_left);
            else if (m_current_input.is_down(zwodee::input_state::move_right)) m_horiz_history.push_back(zwodee::input_state::move_right);
        }
        if (m_vert_history.empty())
        {
            if (m_current_input.is_down(zwodee::input_state::move_up)) m_vert_history.push_back(zwodee::input_state::move_up);
            else if (m_current_input.is_down(zwodee::input_state::move_down)) m_vert_history.push_back(zwodee::input_state::move_down);
        }

        // Contrary release buffer logic
        bool conflicting_horiz = m_current_input.is_down(zwodee::input_state::move_left) && 
                                 m_current_input.is_down(zwodee::input_state::move_right);
        bool conflicting_vert  = m_current_input.is_down(zwodee::input_state::move_up) && 
                                 m_current_input.is_down(zwodee::input_state::move_down);

        // Trigger horizontal delay buffer if we transitioned from conflicting to non-conflicting (releasing one key first)
        if (m_was_conflicting_horiz && !conflicting_horiz)
        {
            if (m_current_input.is_down(zwodee::input_state::move_left) || m_current_input.is_down(zwodee::input_state::move_right))
            {
                m_contrary_release_buffer_x = 5; // ~39ms delay buffer
            }
        }
        m_was_conflicting_horiz = conflicting_horiz;

        // Trigger vertical delay buffer if we transitioned from conflicting to non-conflicting
        if (m_was_conflicting_vert && !conflicting_vert)
        {
            if (m_current_input.is_down(zwodee::input_state::move_up) || m_current_input.is_down(zwodee::input_state::move_down))
            {
                m_contrary_release_buffer_y = 5;
            }
        }
        m_was_conflicting_vert = conflicting_vert;

        // Reset buffers if all keys on that axis are released
        if (!m_current_input.is_down(zwodee::input_state::move_left) && !m_current_input.is_down(zwodee::input_state::move_right))
        {
            m_contrary_release_buffer_x = 0;
        }
        if (!m_current_input.is_down(zwodee::input_state::move_up) && !m_current_input.is_down(zwodee::input_state::move_down))
        {
            m_contrary_release_buffer_y = 0;
        }

        // Decrement release buffers
        if (m_contrary_release_buffer_x > 0) m_contrary_release_buffer_x--;
        if (m_contrary_release_buffer_y > 0) m_contrary_release_buffer_y--;

        // Queue-movement: check if keys are pressed while already moving
        if (m_is_moving)
        {
            bool cancel_tap = false;
            if (pressed & zwodee::input_state::action_1) cancel_tap = true;
            if (pressed & zwodee::input_state::action_2) cancel_tap = true;

            // Increment queued steps if pressing in same direction
            if (pressed & zwodee::input_state::move_left)
            {
                if (m_dir_x == -1.0f) m_queued_steps++;
                else cancel_tap = true;
            }
            if (pressed & zwodee::input_state::move_right)
            {
                if (m_dir_x == 1.0f) m_queued_steps++;
                else cancel_tap = true;
            }
            if (pressed & zwodee::input_state::move_up)
            {
                if (m_dir_y == -1.0f) m_queued_steps++;
                else cancel_tap = true;
            }
            if (pressed & zwodee::input_state::move_down)
            {
                if (m_dir_y == 1.0f) m_queued_steps++;
                else cancel_tap = true;
            }

            if (cancel_tap)
            {
                m_queued_steps = 0;
                m_has_queued_move = false;

                // Queue the new direction (including reversing directions)
                if (pressed & zwodee::input_state::move_left) {
                    m_queued_dir_x = -1.0f; m_queued_dir_y = 0.0f; m_has_queued_move = true;
                }
                else if (pressed & zwodee::input_state::move_right) {
                    m_queued_dir_x = 1.0f; m_queued_dir_y = 0.0f; m_has_queued_move = true;
                }
                else if (pressed & zwodee::input_state::move_up) {
                    m_queued_dir_x = 0.0f; m_queued_dir_y = -1.0f; m_has_queued_move = true;
                }
                else if (pressed & zwodee::input_state::move_down) {
                    m_queued_dir_x = 0.0f; m_queued_dir_y = 1.0f; m_has_queued_move = true;
                }

                pressed = 0; // Clear pressed mask so the canceling input is not processed
            }
        }

        m_prev_input = m_current_input;

        // Grid-locked movement updates
        if (m_is_moving)
        {
            if (m_is_digging)
            {
                m_digging_ticks_remaining--;
                m_vx = 0.0f;
                m_vy = 0.0f;

                if (m_has_pickaxe)
                {
                    if (m_digging_ticks_remaining == 47)
                    {
                        play_dig_sound(m_audio, true);
                    }
                }
                else
                {
                    if (m_digging_ticks_remaining == 95 || m_digging_ticks_remaining == 47)
                    {
                        play_dig_sound(m_audio, false);
                    }
                }
                
                if (m_digging_ticks_remaining <= 0)
                {
                    m_is_digging = false;
                    
                    // Dig the tile!
                    if (m_level)
                    {
                        int tgx = static_cast<int>(std::round(m_target_x / 32.0f));
                        int tgy = static_cast<int>(std::round(m_target_y / 32.0f));
                        if (auto* digx_lvl = dynamic_cast<digx::level*>(m_level))
                        {
                            digx_lvl->dig_tile(tgx, tgy);
                        }
                    }
                }
            }
            else
            {
                m_vx = m_dir_x * m_tunnel_speed;
                m_vy = m_dir_y * m_tunnel_speed;
                m_x += m_vx;
                m_y += m_vy;

                // Check if we reached or overshot the destination coordinates on both axes
                bool reached_x = (m_dir_x == 0.0f) || 
                                 (m_dir_x > 0.0f && m_x >= m_target_x) || 
                                 (m_dir_x < 0.0f && m_x <= m_target_x);
                bool reached_y = (m_dir_y == 0.0f) || 
                                 (m_dir_y > 0.0f && m_y >= m_target_y) || 
                                 (m_dir_y < 0.0f && m_y <= m_target_y);

                if (reached_x && reached_y)
                {
                    m_x = m_target_x;
                    m_y = m_target_y;
                    m_vx = 0.0f;
                    m_vy = 0.0f;
                    m_is_moving = false;

                    // Process queued steps first
                    if (m_queued_steps > 0)
                    {
                        float next_target_x = m_x + m_dir_x * 32.0f;
                        float next_target_y = m_y + m_dir_y * 32.0f;

                        if (can_player_move_to(next_target_x, next_target_y, m_dir_x, m_dir_y))
                        {
                            m_target_x = next_target_x;
                            m_target_y = next_target_y;
                            m_is_moving = true;
                            m_queued_steps--;

                            bool target_undigged = false;
                            if (m_level)
                            {
                                int tgx = static_cast<int>(std::round(m_target_x / 32.0f));
                                int tgy = static_cast<int>(std::round(m_target_y / 32.0f));
                                if (auto* digx_lvl = dynamic_cast<digx::level*>(m_level))
                                {
                                    target_undigged = !digx_lvl->is_tile_digged(tgx, tgy);
                                }
                            }
                            if (target_undigged)
                            {
                                m_is_digging = true;
                                m_digging_ticks_remaining = m_has_pickaxe ? 48 : 96;
                            }
                            else
                            {
                                m_is_digging = false;
                                m_digging_ticks_remaining = 0;
                            }

                            play_footstep(m_audio);
                        }
                        else
                        {
                            m_queued_steps = 0; // Blocked, clear the queue
                        }
                    }

                    // Process queued turn/new direction movement if we stopped
                    if (!m_is_moving && m_has_queued_move)
                    {
                        float next_target_x = m_x + m_queued_dir_x * 32.0f;
                        float next_target_y = m_y + m_queued_dir_y * 32.0f;

                        if (can_player_move_to(next_target_x, next_target_y, m_queued_dir_x, m_queued_dir_y))
                        {
                            m_target_x = next_target_x;
                            m_target_y = next_target_y;
                            m_dir_x = m_queued_dir_x;
                            m_dir_y = m_queued_dir_y;
                            m_is_moving = true;

                            bool target_undigged = false;
                            if (m_level)
                            {
                                int tgx = static_cast<int>(std::round(m_target_x / 32.0f));
                                int tgy = static_cast<int>(std::round(m_target_y / 32.0f));
                                if (auto* digx_lvl = dynamic_cast<digx::level*>(m_level))
                                {
                                    target_undigged = !digx_lvl->is_tile_digged(tgx, tgy);
                                }
                            }
                            if (target_undigged)
                            {
                                m_is_digging = true;
                                m_digging_ticks_remaining = m_has_pickaxe ? 48 : 96;
                            }
                            else
                            {
                                m_is_digging = false;
                                m_digging_ticks_remaining = 0;
                            }

                            play_footstep(m_audio);
                        }
                        m_has_queued_move = false;
                    }
                }
            }
        }

        if (!m_is_moving)
        {
            m_queued_steps = 0; // Reset queued steps if stopped

            float dir_x = 0.0f;
            float dir_y = 0.0f;

            if (!m_horiz_history.empty() && !conflicting_horiz && m_contrary_release_buffer_x == 0)
            {
                auto active_btn = m_horiz_history.back();
                dir_x = (active_btn == zwodee::input_state::move_left) ? -1.0f : 1.0f;
            }
            if (!m_vert_history.empty() && !conflicting_vert && m_contrary_release_buffer_y == 0)
            {
                auto active_btn = m_vert_history.back();
                dir_y = (active_btn == zwodee::input_state::move_up) ? -1.0f : 1.0f;
            }

            // Prevent diagonal movement (45 degrees) by resolving to a single axis
            if (dir_x != 0.0f && dir_y != 0.0f)
            {
                float target_x_only = m_x + dir_x * 32.0f;
                bool horiz_clear = can_player_move_to(target_x_only, m_y, dir_x, 0.0f);

                float target_y_only = m_y + dir_y * 32.0f;
                bool vert_clear = can_player_move_to(m_x, target_y_only, 0.0f, dir_y);

                if (horiz_clear && !vert_clear)
                {
                    dir_y = 0.0f;
                }
                else if (vert_clear && !horiz_clear)
                {
                    dir_x = 0.0f;
                }
                else
                {
                    dir_y = 0.0f; // Prioritize horizontal axis when both are clear
                }
            }

            if (dir_x != 0.0f || dir_y != 0.0f)
            {
                float next_target_x = m_x + dir_x * 32.0f;
                float next_target_y = m_y + dir_y * 32.0f;

                if (can_player_move_to(next_target_x, next_target_y, dir_x, dir_y))
                {
                    m_target_x = next_target_x;
                    m_target_y = next_target_y;
                    m_dir_x = dir_x;
                    m_dir_y = dir_y;
                    m_is_moving = true;

                    bool target_undigged = false;
                    if (m_level)
                    {
                        int tgx = static_cast<int>(std::round(m_target_x / 32.0f));
                        int tgy = static_cast<int>(std::round(m_target_y / 32.0f));
                        if (auto* digx_lvl = dynamic_cast<digx::level*>(m_level))
                        {
                            target_undigged = !digx_lvl->is_tile_digged(tgx, tgy);
                        }
                    }
                    if (target_undigged)
                    {
                        m_is_digging = true;
                        m_digging_ticks_remaining = m_has_pickaxe ? 48 : 96;
                    }
                    else
                    {
                        m_is_digging = false;
                        m_digging_ticks_remaining = 0;
                    }

                    play_footstep(m_audio);
                }
            }
        }

        // Advance animation frame (skipping double-velocity-adding zwodee::entity::tick)
        m_animator.update(1);

        // Update direction and active texture
        if (m_dir_x < 0.0f)
        {
            m_facing_left = true;
        }
        else if (m_dir_x > 0.0f)
        {
            m_facing_left = false;
        }
        set_flip_horizontal(m_facing_left);

        const zwodee::texture* target_tex = nullptr;
        if (m_is_digging)
        {
            int frame_idx = (m_digging_ticks_remaining / 12) % 2;
            if (m_has_pickaxe)
            {
                if (m_dir_y < 0.0f) target_tex = m_pickaxe_dig_up_texs[frame_idx];
                else if (m_dir_y > 0.0f) target_tex = m_pickaxe_dig_down_texs[frame_idx];
                else target_tex = m_pickaxe_dig_texs[frame_idx];

                if (!target_tex)
                {
                    if (m_dir_y < 0.0f) target_tex = m_pickaxe_running_up_tex;
                    else if (m_dir_y > 0.0f) target_tex = m_pickaxe_running_down_tex;
                    else target_tex = m_pickaxe_running_tex;
                }
            }
            else
            {
                if (m_dir_y < 0.0f) target_tex = m_shovel_dig_up_texs[frame_idx];
                else if (m_dir_y > 0.0f) target_tex = m_shovel_dig_down_texs[frame_idx];
                else target_tex = m_shovel_dig_texs[frame_idx];

                if (!target_tex)
                {
                    if (m_dir_y < 0.0f) target_tex = m_shovel_running_up_tex;
                    else if (m_dir_y > 0.0f) target_tex = m_shovel_running_down_tex;
                    else target_tex = m_shovel_running_tex;
                }
            }
        }
        else if (m_has_pickaxe)
        {
            if (m_is_moving)
            {
                if (m_dir_y < 0.0f)
                {
                    target_tex = m_pickaxe_running_up_tex;
                }
                else if (m_dir_y > 0.0f)
                {
                    target_tex = m_pickaxe_running_down_tex;
                }
                else
                {
                    target_tex = m_pickaxe_running_tex;
                }
            }
            else
            {
                target_tex = m_pickaxe_idle_tex;
            }
        }
        else
        {
            if (m_is_moving)
            {
                if (m_dir_y < 0.0f)
                {
                    target_tex = m_shovel_running_up_tex;
                }
                else if (m_dir_y > 0.0f)
                {
                    target_tex = m_shovel_running_down_tex;
                }
                else
                {
                    target_tex = m_shovel_running_tex;
                }
            }
            else
            {
                target_tex = m_shovel_idle_tex;
            }
        }
        if (target_tex)
        {
            set_texture(target_tex);
        }
    }

    void player::set_digging(bool is_digging)
    {
        (void)is_digging;
        set_speed(m_tunnel_speed);
    }

    void player::collect_gold(int amount)
    {
        m_gold_collected += amount;
        m_score += 100 * amount;
    }

    void player::collect_diamond(int amount)
    {
        m_diamonds_collected += amount;
        m_score += 250 * amount;
    }

    void player::collect_garlic(int amount)
    {
        m_garlic_count += amount;
    }

    void player::use_garlic()
    {
        if (m_garlic_count > 0)
        {
            m_garlic_count--;
        }
    }

    void player::collect_onion(int amount)
    {
        m_onion_count += amount;
    }

    void player::obtain_pickaxe()
    {
        m_has_pickaxe = true;
    }

    int player::get_gold_count() const
    {
        return m_gold_collected;
    }

    int player::get_diamond_count() const
    {
        return m_diamonds_collected;
    }

    int player::get_garlic_count() const
    {
        return m_garlic_count;
    }

    int player::get_onion_count() const
    {
        return m_onion_count;
    }

    bool player::has_pickaxe() const
    {
        return m_has_pickaxe;
    }

    int player::get_score() const
    {
        return m_score;
    }

    float player::get_fart_active_time() const
    {
        return static_cast<float>(m_fart_cooldown) / 128.0f;
    }

    float player::get_breath_active_time() const
    {
        return static_cast<float>(m_breath_cooldown) / 128.0f;
    }

    void player::respawn(float x, float y)
    {
        set_health(100);
        set_position(x, y);
        m_target_x = x;
        m_target_y = y;
        m_vx = 0.0f;
        m_vy = 0.0f;
        m_is_moving = false;
        m_initialized_grid = true;
        m_has_pickaxe = false;
    }

    void player::set_grid_bounds(int cols, int rows)
    {
        m_level_cols = cols;
        m_level_rows = rows;
    }

    void player::set_level(zwodee::tile_level* lvl)
    {
        m_level = lvl;
    }

    zwodee::tile_level* player::get_level() const
    {
        return m_level;
    }
    
    zwodee::audio_manager* player::get_audio_manager() const
    {
        return m_audio;
    }

    bool player::is_tile_blocked(float tx, float ty) const
    {
        if (tx < 0.0f || tx > (m_level_cols - 1) * 32.0f ||
            ty < 0.0f || ty > (m_level_rows - 1) * 32.0f)
        {
            return true;
        }

        if (m_level)
        {
            int gx = static_cast<int>(tx / 32.0f);
            int gy = static_cast<int>(ty / 32.0f);
            uint32_t width = m_level->get_width();
            
            const auto& tiles = m_level->get_static_objects();
            size_t idx = static_cast<size_t>(gy) * width + static_cast<size_t>(gx);
            if (idx < tiles.size() && tiles[idx])
            {
                if (tiles[idx]->is_collidable())
                {
                    return true;
                }
            }
        }
        return false;
    }

    stone* player::get_stone_at(float tx, float ty) const
    {
        if (m_level)
        {
            int tgx = static_cast<int>(std::round(tx / 32.0f));
            int tgy = static_cast<int>(std::round(ty / 32.0f));
            for (const auto& ent : m_level->get_entities())
            {
                if (auto* st = dynamic_cast<stone*>(ent.get()))
                {
                    int sgx = static_cast<int>(std::round(st->get_x() / 32.0f));
                    int sgy = static_cast<int>(std::round(st->get_y() / 32.0f));
                    if (sgx == tgx && sgy == tgy && !st->is_dead())
                    {
                        return st;
                    }
                }
            }
        }
        return nullptr;
    }

    bool player::is_tile_clear_for_stone(float tx, float ty) const
    {
        if (tx < 0.0f || tx > (m_level_cols - 1) * 32.0f ||
            ty < 0.0f || ty > (m_level_rows - 1) * 32.0f)
        {
            return false;
        }

        if (is_tile_blocked(tx, ty))
        {
            return false;
        }

        if (m_level)
        {
            int gx = static_cast<int>(tx / 32.0f);
            int gy = static_cast<int>(ty / 32.0f);
            if (auto* digx_lvl = dynamic_cast<digx::level*>(m_level))
            {
                if (!digx_lvl->is_tile_digged(gx, gy))
                {
                    return false;
                }
            }
        }

        if (get_stone_at(tx, ty))
        {
            return false;
        }

        return true;
    }

    bool player::can_player_move_to(float next_target_x, float next_target_y, float dir_x, float dir_y)
    {
        if (is_tile_blocked(next_target_x, next_target_y))
        {
            return false;
        }

        if (auto* st = get_stone_at(next_target_x, next_target_y))
        {
            if (dir_y != 0.0f)
            {
                return false;
            }

            float stone_target_x = next_target_x + dir_x * 32.0f;
            float stone_target_y = next_target_y + dir_y * 32.0f;
            if (is_tile_clear_for_stone(stone_target_x, stone_target_y))
            {
                st->start_move(dir_x, 0.0f);
                if (m_audio)
                {
                    m_audio->play_sound("stone_move");
                }
                return true;
            }
            return false;
        }

        return true;
    }

    void player::set_digging_textures(
        const zwodee::texture* shovel_dig_1, const zwodee::texture* shovel_dig_2,
        const zwodee::texture* shovel_dig_up_1, const zwodee::texture* shovel_dig_up_2,
        const zwodee::texture* shovel_dig_down_1, const zwodee::texture* shovel_dig_down_2,
        const zwodee::texture* pickaxe_dig_1, const zwodee::texture* pickaxe_dig_2,
        const zwodee::texture* pickaxe_dig_up_1, const zwodee::texture* pickaxe_dig_up_2,
        const zwodee::texture* pickaxe_dig_down_1, const zwodee::texture* pickaxe_dig_down_2
    )
    {
        m_shovel_dig_texs[0] = shovel_dig_1;
        m_shovel_dig_texs[1] = shovel_dig_2;
        m_shovel_dig_up_texs[0] = shovel_dig_up_1;
        m_shovel_dig_up_texs[1] = shovel_dig_up_2;
        m_shovel_dig_down_texs[0] = shovel_dig_down_1;
        m_shovel_dig_down_texs[1] = shovel_dig_down_2;
        m_pickaxe_dig_texs[0] = pickaxe_dig_1;
        m_pickaxe_dig_texs[1] = pickaxe_dig_2;
        m_pickaxe_dig_up_texs[0] = pickaxe_dig_up_1;
        m_pickaxe_dig_up_texs[1] = pickaxe_dig_up_2;
        m_pickaxe_dig_down_texs[0] = pickaxe_dig_down_1;
        m_pickaxe_dig_down_texs[1] = pickaxe_dig_down_2;
    }
}
