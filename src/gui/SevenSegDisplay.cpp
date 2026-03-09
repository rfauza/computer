#include "SevenSegDisplay.hpp"
#include <cmath>

SevenSegDisplay::SevenSegDisplay()
{
    set_content_width(36);
    set_content_height(56);
    set_draw_func(sigc::mem_fun(*this, &SevenSegDisplay::on_draw));
}

void SevenSegDisplay::set_value(uint8_t val)
{
    val &= 0x0F;
    if (value_ != val)
    {
        value_ = val;
        queue_draw();
    }
}

void SevenSegDisplay::set_color(double r, double g, double b)
{
    r_ = r;
    g_ = g;
    b_ = b;
    queue_draw();
}

//

void SevenSegDisplay::draw_segment(const Cairo::RefPtr<Cairo::Context>& cr,
                                   double x, double y, double w, double h,
                                   bool horizontal, bool on)
{
    double dim = 0.12;
    
    if (on)
    {
        // Glow
        cr->set_source_rgba(r_, g_, b_, 0.3);
        cr->rectangle(x - 1, y - 1, w + 2, h + 2);
        cr->fill();
        cr->set_source_rgb(r_, g_, b_);
    }
    else
    {
            cr->set_source_rgb(r_ * dim, g_ * dim, b_ * dim);
    }
    
    // Draw the segment as a hexagonal shape
    if (horizontal)
    {
        double mid = h / 2.0;
        cr->move_to(x + mid, y);
        cr->line_to(x + w - mid, y);
        cr->line_to(x + w, y + mid);
        cr->line_to(x + w - mid, y + h);
        cr->line_to(x + mid, y + h);
        cr->line_to(x, y + mid);
        cr->close_path();
    }
    else
    {
        double mid = w / 2.0;
        cr->move_to(x + mid, y);
        cr->line_to(x + w, y + mid);
        cr->line_to(x + w, y + h - mid);
        cr->line_to(x + mid, y + h);
        cr->line_to(x, y + h - mid);
        cr->line_to(x, y + mid);
        cr->close_path();
    }
    cr->fill();
}

void SevenSegDisplay::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                              int width, int height)
{
    // Standard dark background panel
    cr->set_source_rgb(0.05, 0.05, 0.05);
    double corner = 4.0;
    cr->move_to(corner, 0);
    cr->line_to(width - corner, 0);
    cr->arc(width - corner, corner, corner, -M_PI / 2, 0);
    cr->line_to(width, height - corner);
    cr->arc(width - corner, height - corner, corner, 0, M_PI / 2);
    cr->line_to(corner, height);
    cr->arc(corner, height - corner, corner, M_PI / 2, M_PI);
    cr->line_to(0, corner);
    cr->arc(corner, corner, corner, M_PI, 3 * M_PI / 2);
    cr->close_path();
    cr->fill();
    
    uint8_t segs = seg_table[value_];
    
    // Layout dimensions
    double margin_x = width * 0.18;
    double margin_y = height * 0.1;
    double seg_w = width - 2 * margin_x;   // horizontal segment length
    double seg_h = (height - 2 * margin_y) / 2.0; // vertical half-height
    double thick = std::max(3.0, seg_w * 0.16);
    
    double left   = margin_x;
    double right  = margin_x + seg_w - thick;
    double top    = margin_y;
    double mid_y  = margin_y + seg_h - thick / 2.0;
    double bottom = margin_y + 2 * seg_h - thick;
    
    // Segments: a=top, b=upper-right, c=lower-right, d=bottom,
    //           e=lower-left, f=upper-left, g=middle
    // Bit layout in seg_table: bit6=a, bit5=b, bit4=c, bit3=d,
    //                          bit2=e, bit1=f, bit0=g
    
    bool a = (segs >> 6) & 1;
    bool b_seg = (segs >> 5) & 1;
    bool c_seg = (segs >> 4) & 1;
    bool d = (segs >> 3) & 1;
    bool e = (segs >> 2) & 1;
    bool f = (segs >> 1) & 1;
    bool g = (segs >> 0) & 1;
    
    // Draw horizontal segments (a, d, g)
    draw_segment(cr, left, top, seg_w, thick, true, a);
    draw_segment(cr, left, mid_y, seg_w, thick, true, g);
    draw_segment(cr, left, bottom, seg_w, thick, true, d);
    
    // Draw vertical segments (b, c, e, f)
    draw_segment(cr, right, top, thick, seg_h, false, b_seg);
    draw_segment(cr, right, mid_y, thick, seg_h, false, c_seg);
    draw_segment(cr, left, mid_y, thick, seg_h, false, e);
    draw_segment(cr, left, top, thick, seg_h, false, f);
}
