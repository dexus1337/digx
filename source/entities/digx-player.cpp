#include "entities/digx-player.hpp"

#include <algorithm>
#include <cmath>

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
}

namespace digx
{
    player::player(uint32_t network_id, 
                   const zwodee::texture* shovel_idle_tex, 
                   const zwodee::texture* shovel_running_tex, 
                   const zwodee::texture* pickaxe_idle_tex, 
                   const zwodee::texture* pickaxe_running_tex, 
                   zwodee::audio_manager* audio)
        : zwodee::entity_player(network_id, shovel_idle_tex, 100),
          m_shovel_idle_tex(shovel_idle_tex),
          m_shovel_running_tex(shovel_running_tex),
          m_pickaxe_idle_tex(pickaxe_idle_tex),
          m_pickaxe_running_tex(pickaxe_running_tex),
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

                // If turning, queue the new direction
                if ((pressed & zwodee::input_state::move_left) && m_dir_x == 0.0f) {
                    m_queued_dir_x = -1.0f; m_queued_dir_y = 0.0f; m_has_queued_move = true;
                }
                else if ((pressed & zwodee::input_state::move_right) && m_dir_x == 0.0f) {
                    m_queued_dir_x = 1.0f; m_queued_dir_y = 0.0f; m_has_queued_move = true;
                }
                else if ((pressed & zwodee::input_state::move_up) && m_dir_y == 0.0f) {
                    m_queued_dir_x = 0.0f; m_queued_dir_y = -1.0f; m_has_queued_move = true;
                }
                else if ((pressed & zwodee::input_state::move_down) && m_dir_y == 0.0f) {
                    m_queued_dir_x = 0.0f; m_queued_dir_y = 1.0f; m_has_queued_move = true;
                }

                pressed = 0; // Clear pressed mask so the canceling input is not processed
            }
        }

        m_prev_input = m_current_input;

        // Grid-locked movement updates
        if (m_is_moving)
        {
            m_vx = m_dir_x * get_speed();
            m_vy = m_dir_y * get_speed();
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

                    if (!is_tile_blocked(next_target_x, next_target_y))
                    {
                        m_target_x = next_target_x;
                        m_target_y = next_target_y;
                        m_is_moving = true;
                        m_queued_steps--;
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

                    if (!is_tile_blocked(next_target_x, next_target_y))
                    {
                        m_target_x = next_target_x;
                        m_target_y = next_target_y;
                        m_dir_x = m_queued_dir_x;
                        m_dir_y = m_queued_dir_y;
                        m_is_moving = true;
                        play_footstep(m_audio);
                    }
                    m_has_queued_move = false;
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

            if (dir_x != 0.0f || dir_y != 0.0f)
            {
                float next_target_x = m_x + dir_x * 32.0f;
                float next_target_y = m_y + dir_y * 32.0f;

                if (!is_tile_blocked(next_target_x, next_target_y))
                {
                    m_target_x = next_target_x;
                    m_target_y = next_target_y;
                    m_dir_x = dir_x;
                    m_dir_y = dir_y;
                    m_is_moving = true;
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
        if (m_has_pickaxe)
        {
            target_tex = m_is_moving ? m_pickaxe_running_tex : m_pickaxe_idle_tex;
        }
        else
        {
            target_tex = m_is_moving ? m_shovel_running_tex : m_shovel_idle_tex;
        }
        if (target_tex)
        {
            set_texture(target_tex);
        }
    }

    void player::set_digging(bool is_digging)
    {
        if (!is_digging)
        {
            set_speed(m_tunnel_speed);
            return;
        }

        float speed = m_has_pickaxe ? m_pickaxe_speed : m_shovel_speed;
        set_speed(speed);
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
}
