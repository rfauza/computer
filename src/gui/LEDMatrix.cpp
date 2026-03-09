#include "LEDMatrix.hpp"
#include <cmath>

LEDMatrix::LEDMatrix()
{
    set_content_width(160);
    set_content_height(160);
    set_draw_func(sigc::mem_fun(*this, &LEDMatrix::on_draw));
}

void LEDMatrix::set_led(int row, int col, bool on)
{
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
    {
        grid_[row][col] = on;
    }
}

bool LEDMatrix::get_led(int row, int col) const
{
    if (row >= 0 && row < ROWS && col >= 0 && col < COLS)
    {
        return grid_[row][col];
    }
    return false;
}

void LEDMatrix::set_row(int row, uint8_t bits)
{
    if (row >= 0 && row < ROWS)
    {
        for (int col = 0; col < COLS; ++col)
        {
            grid_[row][col] = ((bits >> col) & 1) != 0;
        }
    }
}

void LEDMatrix::clear()
{
    for (int r = 0; r < ROWS; ++r)
    {
        for (int c = 0; c < COLS; ++c)
        {
            grid_[r][c] = false;
        }
    }
}

void LEDMatrix::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                        int width, int height)
{
    // Dark board background
    cr->set_source_rgb(0.08, 0.08, 0.08);
    cr->rectangle(0, 0, width, height);
    cr->fill();
    
    // Border
    cr->set_source_rgb(0.25, 0.25, 0.25);
    cr->set_line_width(2.0);
    cr->rectangle(1, 1, width - 2, height - 2);
    cr->stroke();
    
    double left_margin = 20.0;
    double top_margin = 25.0;
    double side_margin = 10.0;
    double avail_w = width - left_margin - side_margin;
    double avail_h = height - top_margin - side_margin;
    double cell = std::min(avail_w / COLS, avail_h / ROWS);
    double grid_w = cell * COLS;
    double grid_h = cell * ROWS;
    double ox = left_margin + (avail_w - grid_w) / 2.0;
    double oy = top_margin + (avail_h - grid_h) / 2.0;
    double led_r = cell * 0.32;
    
    // Title (in upper left)
    cr->set_source_rgb(0.7, 0.7, 0.7);
    cr->select_font_face("monospace", Cairo::ToyFontFace::Slant::NORMAL,
                          Cairo::ToyFontFace::Weight::BOLD);
    cr->set_font_size(9.0);
    Cairo::TextExtents ext;
    cr->get_text_extents("8x8 LED Matrix Display", ext);
    cr->move_to(left_margin, 12);
    cr->show_text("8x8 LED Matrix Display");
    
    // Row/column labels
    cr->set_source_rgb(0.55, 0.55, 0.55);
    cr->set_font_size(7.0);
    // Column labels on top
    for (int c = 0; c < COLS; ++c)
    {
        std::string lbl = std::to_string(c);
        cr->get_text_extents(lbl, ext);
        cr->move_to(ox + c * cell + (cell - ext.width) / 2.0, oy - 3);
        cr->show_text(lbl);
    }
    // Row labels on left
    for (int r = 0; r < ROWS; ++r)
    {
        std::string lbl = std::to_string(r);
        cr->get_text_extents(lbl, ext);
        cr->move_to(ox - left_margin + 3, oy + r * cell + (cell + ext.height) / 2.0);
        cr->show_text(lbl);
    }
    
    // Draw LEDs
    for (int r = 0; r < ROWS; ++r)
    {
        for (int c = 0; c < COLS; ++c)
        {
            double cx = ox + c * cell + cell / 2.0;
            double cy = oy + r * cell + cell / 2.0;
            bool on = grid_[r][c]; // Direct mapping, 0,0 at top-left
            
            if (on)
            {
                // Glow
                auto glow = Cairo::RadialGradient::create(cx, cy, led_r * 0.3,
                                                           cx, cy, led_r * 2.0);
                glow->add_color_stop_rgba(0.0, r_, g_, b_, 0.5);
                glow->add_color_stop_rgba(1.0, r_, g_, b_, 0.0);
                cr->arc(cx, cy, led_r * 2.0, 0, 2 * M_PI);
                cr->set_source(glow);
                cr->fill();
                
                // Bright body
                auto body = Cairo::RadialGradient::create(
                    cx - led_r * 0.2, cy - led_r * 0.2, led_r * 0.1,
                    cx, cy, led_r);
                body->add_color_stop_rgb(0.0, std::min(1.0, r_ * 1.2), std::min(1.0, g_ * 1.2), std::min(1.0, b_ * 1.2));
                body->add_color_stop_rgb(0.5, r_, g_, b_);
                body->add_color_stop_rgb(1.0, r_ * 0.6, g_ * 0.6, b_ * 0.6);
                cr->arc(cx, cy, led_r, 0, 2 * M_PI);
                cr->set_source(body);
                cr->fill();
            }
            else
            {
                // Dim body — perceptually equalize but at half strength for red/blue.
                // Green gets full brightness (previous state).
                auto body = Cairo::RadialGradient::create(
                    cx - led_r * 0.2, cy - led_r * 0.2, led_r * 0.1,
                    cx, cy, led_r);
                double max_c = std::max({r_, g_, b_});
                if (max_c < 1e-6) max_c = 1.0;
                double nr = r_ / max_c, ng = g_ / max_c, nb = b_ / max_c;
                const double BASE = 0.22;
                double norm_lum = 0.2126 * nr + 0.7152 * ng + 0.0722 * nb;
                double factor = (0.736 * BASE) / std::max(norm_lum, 0.05);
                factor = std::min(factor, 0.65);
                // Green only: use full brightness; red/blue: halved
                bool is_green = (g_ > r_ && g_ > b_);
                if (!is_green) factor *= 0.60;
                body->add_color_stop_rgb(0.0, nr * factor, ng * factor, nb * factor);
                body->add_color_stop_rgb(1.0, nr * factor * 0.45, ng * factor * 0.45, nb * factor * 0.45);
                cr->arc(cx, cy, led_r, 0, 2 * M_PI);
                cr->set_source(body);
                cr->fill();
            }
        }
    }
}

void LEDMatrix::set_color(double r, double g, double b)
{
    r_ = r; g_ = g; b_ = b;
    queue_draw();
}
