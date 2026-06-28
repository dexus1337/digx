#include "entities/digx-enemy-base.hpp"
#include "entities/digx-player.hpp"
#include "entities/digx-stone.hpp"
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

        if (!m_has_lock)
        {
            m_lock_x = std::round(player->get_x() / 32.0f) * 32.0f;
            m_lock_y = std::round(player->get_y() / 32.0f) * 32.0f;
            m_has_lock = true;
        }

        float lgx = std::round(m_lock_x / 32.0f);
        float lgy = std::round(m_lock_y / 32.0f);

        if (gx == lgx && gy == lgy)
        {
            m_lock_x = std::round(player->get_x() / 32.0f) * 32.0f;
            m_lock_y = std::round(player->get_y() / 32.0f) * 32.0f;
            lgx = std::round(m_lock_x / 32.0f);
            lgy = std::round(m_lock_y / 32.0f);
        }

        if (gx == lgx && gy == lgy)
        {
            return;
        }

        int start_x = static_cast<int>(gx);
        int start_y = static_cast<int>(gy);
        int end_x = static_cast<int>(lgx);
        int end_y = static_cast<int>(lgy);

        // BFS to find the shortest path from start grid to lock grid
        std::queue<std::pair<int, int>> q;
        std::vector<std::vector<std::pair<int, int>>> parent(lvl->get_height(), std::vector<std::pair<int, int>>(lvl->get_width(), {-1, -1}));
        std::vector<std::vector<bool>> visited(lvl->get_height(), std::vector<bool>(lvl->get_width(), false));

        q.push({start_x, start_y});
        visited[start_y][start_x] = true;

        bool found = false;
        std::pair<int, int> dirs[] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

        while (!q.empty())
        {
            auto curr = q.front();
            q.pop();

            if (curr.first == end_x && curr.second == end_y)
            {
                found = true;
                break;
            }

            for (const auto& d : dirs)
            {
                int nx = curr.first + d.first;
                int ny = curr.second + d.second;

                if (nx >= 0 && nx < static_cast<int>(lvl->get_width()) &&
                    ny >= 0 && ny < static_cast<int>(lvl->get_height()))
                {
                    if (!visited[ny][nx] && check_tile_clear(lvl, nx, ny))
                    {
                        visited[ny][nx] = true;
                        parent[ny][nx] = curr;
                        q.push({nx, ny});
                    }
                }
            }
        }

        float step_x = 0.0f;
        float step_y = 0.0f;

        if (found)
        {
            // Backtrack to find the first step towards the target
            std::pair<int, int> curr = {end_x, end_y};
            while (parent[curr.second][curr.first] != std::make_pair(start_x, start_y))
            {
                curr = parent[curr.second][curr.first];
            }
            step_x = static_cast<float>(curr.first - start_x);
            step_y = static_cast<float>(curr.second - start_y);
        }
        else
        {
            // No direct path found: Wander to a random adjacent clear tile
            std::vector<std::pair<float, float>> clear_dirs;
            for (const auto& d : dirs)
            {
                if (check_tile_clear(lvl, start_x + d.first, start_y + d.second))
                {
                    clear_dirs.push_back({static_cast<float>(d.first), static_cast<float>(d.second)});
                }
            }

            if (!clear_dirs.empty())
            {
                int rand_idx = std::rand() % clear_dirs.size();
                step_x = clear_dirs[rand_idx].first;
                step_y = clear_dirs[rand_idx].second;
                // Re-lock next step
                m_has_lock = false;
            }
        }

        if (step_x == 0.0f && step_y == 0.0f)
        {
            // Fully trapped: stay idle
            return;
        }

        m_dir_x = step_x;
        m_dir_y = step_y;
        m_target_x = (gx + step_x) * 32.0f;
        m_target_y = (gy + step_y) * 32.0f;
        m_is_moving = true;
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

        if (!lvl->is_tile_digged(tx, ty))
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
}
