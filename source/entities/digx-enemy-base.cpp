#include "entities/digx-enemy-base.hpp"
#include "entities/digx-player.hpp"
#include "entities/digx-stone.hpp"
#include "entities/digx-mummy.hpp"
#include "levels/digx-level.hpp"
#include <cmath>
#include <queue>
#include <vector>
#include <utility>

namespace digx
{
    enemy_base::enemy_base(uint32_t network_id, const zwodee::texture* tex, float speed, int max_hp)
        : zwodee::entity(network_id, tex, max_hp), m_speed(speed)
    {
        m_width = 32.0f;
        m_height = 32.0f;
    }

    void enemy_base::tick()
    {
        if (is_dead())
        {
            m_vx = 0.0f;
            m_vy = 0.0f;
            return;
        }

        if (m_stun_ticks > 0)
        {
            m_stun_ticks--;
            m_stun_anim_ticks++;
            m_vx = 0.0f;
            m_vy = 0.0f;
            m_is_moving = false;
            zwodee::entity::tick();
            return;
        }

        if (m_is_moving)
        {
            float dx = m_target_x - m_x;
            float dy = m_target_y - m_y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= m_speed)
            {
                m_x = m_target_x;
                m_y = m_target_y;
                m_is_moving = false;
                m_vx = 0.0f;
                m_vy = 0.0f;
            }
            else
            {
                m_vx = (dx / dist) * m_speed;
                m_vy = (dy / dist) * m_speed;
                m_x += m_vx;
                m_y += m_vy;
            }
        }
        else
        {
            m_vx = 0.0f;
            m_vy = 0.0f;
        }

        // Call base entity tick (but clear vx/vy so it doesn't double-move)
        float saved_vx = m_vx;
        float saved_vy = m_vy;
        m_vx = 0.0f;
        m_vy = 0.0f;
        zwodee::entity::tick();
        m_vx = saved_vx;
        m_vy = saved_vy;
    }

    void enemy_base::update_enemy_movement(player* player)
    {
        if (!player || is_dead())
        {
            return;
        }

        auto* lvl = dynamic_cast<digx::level*>(player->get_level());
        if (!lvl)
        {
            return;
        }

        if (!m_initialized_grid)
        {
            m_target_x = std::round(m_x / 32.0f) * 32.0f;
            m_target_y = std::round(m_y / 32.0f) * 32.0f;
            m_x = m_target_x;
            m_y = m_target_y;
            m_initialized_grid = true;
        }

        if (m_is_moving)
        {
            return;
        }

        float gx = std::round(m_x / 32.0f);
        float gy = std::round(m_y / 32.0f);

        int py = static_cast<int>(std::round(player->get_y() / 32.0f));
        int ey = static_cast<int>(gy);

        // If player's Y is further than 5 tiles away, align Y to get closer
        if (std::abs(py - ey) > 5)
        {
            float dir_y = (py > ey) ? 1.0f : -1.0f;
            if (is_direction_clear(lvl, 0.0f, dir_y))
            {
                m_dir_x = 0.0f;
                m_dir_y = dir_y;
                m_target_x = m_x;
                m_target_y = (gy + dir_y) * 32.0f;
                m_is_moving = true;
                return;
            }
        }

        // Gather all clear horizontal and vertical directions for random wandering patrol
        std::vector<std::pair<float, float>> clear_dirs;
        std::pair<float, float> dirs[] = { {1.0f, 0.0f}, {-1.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, -1.0f} };
        for (const auto& d : dirs)
        {
            if (is_direction_clear(lvl, d.first, d.second))
            {
                clear_dirs.push_back(d);
            }
        }

        if (!clear_dirs.empty())
        {
            // Pick a direction randomly among all clear choices on each tile step
            auto chosen_dir = clear_dirs[std::rand() % clear_dirs.size()];
            m_dir_x = chosen_dir.first;
            m_dir_y = chosen_dir.second;
            m_target_x = (gx + m_dir_x) * 32.0f;
            m_target_y = (gy + m_dir_y) * 32.0f;
            m_is_moving = true;
        }
        else
        {
            m_dir_x = 0.0f;
            m_dir_y = 0.0f;
        }
    }

    bool enemy_base::is_direction_clear(level* lvl, float dir_x, float dir_y) const
    {
        float gx = std::round(m_x / 32.0f);
        float gy = std::round(m_y / 32.0f);
        return check_tile_clear(lvl, static_cast<int>(gx + dir_x), static_cast<int>(gy + dir_y));
    }

    bool enemy_base::check_tile_clear(level* lvl, int tx, int ty) const
    {
        if (tx < 0 || tx >= static_cast<int>(lvl->get_width()) ||
            ty < 0 || ty >= static_cast<int>(lvl->get_height()))
        {
            return false;
        }

        bool is_mummy = (dynamic_cast<const mummy*>(this) != nullptr);

        if (!lvl->is_tile_digged(tx, ty) && !is_mummy)
        {
            return false;
        }

        size_t idx = static_cast<size_t>(ty) * lvl->get_width() + static_cast<size_t>(tx);
        if (idx < lvl->get_static_objects().size() && lvl->get_static_objects()[idx])
        {
            if (lvl->get_static_objects()[idx]->is_collidable())
            {
                return false;
            }
        }

        // Block if a moveable stone is at the target tile
        for (const auto& ent : lvl->get_entities())
        {
            if (auto* st = dynamic_cast<stone*>(ent.get()))
            {
                if (!st->is_dead())
                {
                    int st_gx = static_cast<int>(std::round(st->get_x() / 32.0f));
                    int st_gy = static_cast<int>(std::round(st->get_y() / 32.0f));
                    if (st_gx == tx && st_gy == ty)
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool enemy_base::update_fart_stun(player* player)
    {
        bool fart_active = false;
        float fart_x = 0.0f;
        float fart_y = 0.0f;
        if (auto* lvl = dynamic_cast<digx::level*>(player->get_level()))
        {
            fart_active = lvl->is_fart_active();
            fart_x = lvl->get_fart_x();
            fart_y = lvl->get_fart_y();
        }

        if (!fart_active)
        {
            m_fart_affected = false;
        }

        if (!m_fart_affected && fart_active)
        {
            float fdx = fart_x - m_x;
            float fdy = fart_y - m_y;
            if (std::abs(fdx) <= 32.1f && std::abs(fdy) <= 32.1f)
            {
                m_stun_ticks = 320;
                m_fart_affected = true;
                m_vx = 0.0f;
                m_vy = 0.0f;
                m_is_moving = false;
                return true;
            }
        }
        return false;
    }

    void enemy_base::render(zwodee::renderer& target_renderer, double alpha)
    {
        if (!m_texture) return;
        float render_x = m_x + (m_vx * static_cast<float>(alpha));
        float render_y = m_y + (m_vy * static_cast<float>(alpha));

        if (m_stun_ticks > 0)
        {
            render_x += std::sin(static_cast<float>(m_stun_anim_ticks) * 0.375f) * 2.0f;
        }

        int frame_width = m_texture->get_width();
        int frame_height = m_texture->get_height();
        target_renderer.draw_sprite(*m_texture, 0, 0, frame_width, frame_height, render_x, render_y, m_width, m_height, m_flip_horizontal);
    }

    zwodee::render_node enemy_base::get_render_node() const
    {
        zwodee::render_node node = zwodee::entity::get_render_node();
        if (m_stun_ticks > 0)
        {
            node.x += std::sin(static_cast<float>(m_stun_anim_ticks) * 0.375f) * 2.0f;
        }
        return node;
    }
}
