#pragma once

#include "zwodee.hpp"
#include <string>
#include <SDL3/SDL.h>

namespace digx
{
    class button
    {
    public:
        button(const std::string& text, float x, float y, float w, float h)
            : m_text(text), m_x(x), m_y(y), m_w(w), m_h(h) {}

        [[nodiscard]] bool is_hovered(float mx, float my) const
        {
            return mx >= m_x && mx <= m_x + m_w && my >= m_y && my <= m_y + m_h;
        }

        void add_to_snapshot(zwodee::render_snapshot& snapshot, const zwodee::font& f, bool selected_or_hovered) const
        {
            // 1. Background node (solid color rect)
            zwodee::render_node bg_node{};
            bg_node.x = m_x;
            bg_node.y = m_y;
            bg_node.w = m_w;
            bg_node.h = m_h;
            bg_node.tex = nullptr; // Solid color
            bg_node.is_ui = true;
            if (selected_or_hovered)
            {
                bg_node.r = 60; bg_node.g = 60; bg_node.b = 80; bg_node.a = 200;
            }
            else
            {
                bg_node.r = 30; bg_node.g = 30; bg_node.b = 40; bg_node.a = 150;
            }
            snapshot.push_back(bg_node);

            // 2. Border lines (4 thin rects)
            uint8_t br = selected_or_hovered ? 220 : 100;
            uint8_t bg = selected_or_hovered ? 220 : 100;
            uint8_t bb = selected_or_hovered ? 255 : 120;
            uint8_t ba = 255;

            auto add_border_rect = [&](float bx, float by, float bw, float bh) {
                zwodee::render_node border_node{};
                border_node.x = bx;
                border_node.y = by;
                border_node.w = bw;
                border_node.h = bh;
                border_node.tex = nullptr;
                border_node.is_ui = true;
                border_node.r = br;
                border_node.g = bg;
                border_node.b = bb;
                border_node.a = ba;
                snapshot.push_back(border_node);
            };

            add_border_rect(m_x, m_y, m_w, 2.0f);                 // Top
            add_border_rect(m_x, m_y + m_h - 2.0f, m_w, 2.0f);    // Bottom
            add_border_rect(m_x, m_y, 2.0f, m_h);                 // Left
            add_border_rect(m_x + m_w - 2.0f, m_y, 2.0f, m_h);    // Right

            // 3. Text nodes
            float font_size = f.get_font_size();
            float scale = (m_h - 12.0f) / font_size;
            if (scale > 1.0f) scale = 1.0f;

            float text_w = 0.0f;
            for (char c : m_text)
            {
                text_w += f.get_glyph(c).xadvance * scale;
            }
            float text_h = font_size * scale;

            float tx = m_x + (m_w - text_w) * 0.5f;
            float ty = m_y + m_h * 0.5f + (font_size * scale) * 0.3f;

            uint8_t tr = selected_or_hovered ? 255 : 200;
            uint8_t tg = selected_or_hovered ? 255 : 200;
            uint8_t tb = selected_or_hovered ? 100 : 200;
            uint8_t ta = 255;

            std::vector<zwodee::render_node> text_nodes = f.get_text_nodes(m_text, tx, ty, scale, tr, tg, tb, ta);
            for (auto& node : text_nodes)
            {
                node.is_ui = true;
            }
            snapshot.insert(snapshot.end(), text_nodes.begin(), text_nodes.end());
        }

        [[nodiscard]] float get_x() const { return m_x; }
        [[nodiscard]] float get_y() const { return m_y; }
        [[nodiscard]] float get_w() const { return m_w; }
        [[nodiscard]] float get_h() const { return m_h; }
        [[nodiscard]] const std::string& get_text() const { return m_text; }

    private:
        std::string m_text;
        float m_x, m_y, m_w, m_h;
    };
}
