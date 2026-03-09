#include "MultiSegDisplay.hpp"
#include <cmath>
#include <cctype>

MultiSegDisplay::MultiSegDisplay()
{
    set_content_width(40);
    set_content_height(60);
    set_draw_func(sigc::mem_fun(*this, &MultiSegDisplay::on_draw));
}

void MultiSegDisplay::set_char(char ch)
{
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    if (char_ != ch)
    {
        char_ = ch;
        queue_draw();
    }
}

void MultiSegDisplay::set_color(double r, double g, double b)
{
    r_ = r;
    g_ = g;
    b_ = b;
    queue_draw();
}

//

// 14-segment encoding
// Segment bits (MSB first):
//  bit13=a (top), bit12=b (upper-right), bit11=c (lower-right),
//  bit10=d (bottom), bit9=e (lower-left), bit8=f (upper-left),
//  bit7=g1 (middle-left), bit6=g2 (middle-right),
//  bit5=h (upper-left diag), bit4=i (upper vertical),
//  bit3=j (upper-right diag), bit2=k (lower-right diag),
//  bit1=l (lower vertical), bit0=m (lower-left diag)
uint16_t MultiSegDisplay::char_to_segs(char ch)
{
    //                          a  b  c  d  e  f  g1 g2 h  i  j  k  l  m
    // bit:                    13 12 11 10  9  8  7  6  5  4  3  2  1  0
    switch (ch)
    {
        // Digits
        case '0': return 0b11111100100100; // a b c d e f + j m (diags for slash)
        case '1': return 0b01100000000000; // b c
        case '2': return 0b11011011000000; // a b d e g1 g2
        case '3': return 0b11110011000000; // a b c d g2  - wait, let me reconsider
        case '4': return 0b01100111000000; // b c f g1 g2
        case '5': return 0b10110111000000; // a c d f g1 g2
        case '6': return 0b10111111000000; // a c d e f g1 g2
        case '7': return 0b11100000000000; // a b c
        case '8': return 0b11111111000000; // a b c d e f g1 g2
        case '9': return 0b11110111000000; // a b c d f g1 g2
        
        // Letters
        case 'A': return 0b11101111000000; // a b c e f g1 g2
        case 'B': return 0b11110001010010; // a b c d + g2 i l
        case 'C': return 0b10011100000000; // a d e f
        case 'D': return 0b11110000010010; // a b c d + i l
        case 'E': return 0b10011111000000; // a d e f g1 g2
        case 'F': return 0b10001111000000; // a e f g1 g2
        case 'G': return 0b10111101000000; // a c d e f g2
        case 'H': return 0b01101111000000; // b c e f g1 g2
        case 'I': return 0b10010000010010; // a d + i l
        case 'J': return 0b01111000000000; // b c d e
        case 'K': return 0b00001110001100; // e f g1 + j k
        case 'L': return 0b00011100000000; // d e f
        case 'M': return 0b01101100101000; // b c e f + h j
        case 'N': return 0b01101100100100; // b c e f + h k
        case 'O': return 0b11111100000000; // a b c d e f
        case 'P': return 0b11001111000000; // a b e f g1 g2
        case 'Q': return 0b11111100000100; // a b c d e f + k
        case 'R': return 0b11001111000100; // a b e f g1 g2 + k
        case 'S': return 0b10110111000000; // a c d f g1 g2
        case 'T': return 0b10000000010010; // a + i l
        case 'U': return 0b01111100000000; // b c d e f
        case 'V': return 0b00001100001001; // e f + j m
        case 'W': return 0b01101100000101; // b c e f + k m
        case 'X': return 0b00000000101101; // h j k m
        case 'Y': return 0b00000000101010; // h j l
        case 'Z': return 0b10010000001001; // a d + j m
        
        case ' ':
        default:  return 0b00000000000000;
    }
}

void MultiSegDisplay::draw_hseg(const Cairo::RefPtr<Cairo::Context>& cr,
                                double x, double y, double w, double thick,
                                bool on)
{
    double dim = 0.12;
    if (on)
    {
        // Glow
        cr->set_source_rgba(r_, g_, b_, 0.3);
        cr->rectangle(x - 1, y - 1, w + 2, thick + 2);
        cr->fill();
        cr->set_source_rgb(r_, g_, b_);
    }
    else
    {
        cr->set_source_rgb(r_ * dim, g_ * dim, b_ * dim);
    }
    
    // Hexagonal horizontal segment
    double mid = thick / 2.0;
    cr->move_to(x + mid, y);
    cr->line_to(x + w - mid, y);
    cr->line_to(x + w, y + mid);
    cr->line_to(x + w - mid, y + thick);
    cr->line_to(x + mid, y + thick);
    cr->line_to(x, y + mid);
    cr->close_path();
    cr->fill();
}

void MultiSegDisplay::draw_vseg(const Cairo::RefPtr<Cairo::Context>& cr,
                                double x, double y, double h, double thick,
                                bool on)
{
    double dim = 0.12;
    if (on)
    {
        cr->set_source_rgba(r_, g_, b_, 0.3);
        cr->rectangle(x - 1, y - 1, thick + 2, h + 2);
        cr->fill();
        cr->set_source_rgb(r_, g_, b_);
    }
    else
    {
        cr->set_source_rgb(r_ * dim, g_ * dim, b_ * dim);
    }
    
    // Hexagonal vertical segment
    double mid = thick / 2.0;
    cr->move_to(x + mid, y);
    cr->line_to(x + thick, y + mid);
    cr->line_to(x + thick, y + h - mid);
    cr->line_to(x + mid, y + h);
    cr->line_to(x, y + h - mid);
    cr->line_to(x, y + mid);
    cr->close_path();
    cr->fill();
}

void MultiSegDisplay::draw_diag(const Cairo::RefPtr<Cairo::Context>& cr,
                                double x1, double y1, double x2, double y2,
                                double thick, bool on)
{
    double dim = 0.12;
    if (on)
    {
        cr->set_source_rgba(r_, g_, b_, 0.3);
        double dx = x2 - x1, dy = y2 - y1;
        double len = std::sqrt(dx * dx + dy * dy);
        double nx = -dy / len * (thick * 0.7);
        double ny =  dx / len * (thick * 0.7);
        cr->move_to(x1 + nx, y1 + ny);
        cr->line_to(x2 + nx, y2 + ny);
        cr->line_to(x2 - nx, y2 - ny);
        cr->line_to(x1 - nx, y1 - ny);
        cr->close_path();
        cr->fill();
        cr->set_source_rgb(r_, g_, b_);
    }
    else
    {
        cr->set_source_rgb(r_ * dim, g_ * dim, b_ * dim);
    }
    
    double dx = x2 - x1, dy = y2 - y1;
    double len = std::sqrt(dx * dx + dy * dy);
    double nx = -dy / len * (thick * 0.5);
    double ny =  dx / len * (thick * 0.5);
    cr->move_to(x1 + nx, y1 + ny);
    cr->line_to(x2 + nx, y2 + ny);
    cr->line_to(x2 - nx, y2 - ny);
    cr->line_to(x1 - nx, y1 - ny);
    cr->close_path();
    cr->fill();
}

//

void MultiSegDisplay::draw_led_bg(const Cairo::RefPtr<Cairo::Context>& cr,
                                  int width, int height)
{
    // Dark background panel (same as SevenSegDisplay)
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
}

void MultiSegDisplay::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                              int width, int height)
{
    // Background (LED style only)
    draw_led_bg(cr, width, height);
    
    uint16_t segs = char_to_segs(char_);
    
    // Layout
    double mx = width * 0.18;
    double my = height * 0.1;
    double seg_w = width - 2 * mx;
    double seg_h = (height - 2 * my) / 2.0;
    double thick = std::max(2.5, seg_w * 0.12);
    
    double left   = mx;
    double right  = mx + seg_w - thick;
    double top    = my;
    double mid_y  = my + seg_h - thick / 2.0;
    double bottom = my + 2 * seg_h - thick;
    double ctr_x  = mx + seg_w / 2.0 - thick / 2.0;
    
    // Extract segment bits
    bool a  = (segs >> 13) & 1;
    bool b  = (segs >> 12) & 1;
    bool c  = (segs >> 11) & 1;
    bool d  = (segs >> 10) & 1;
    bool e  = (segs >>  9) & 1;
    bool f  = (segs >>  8) & 1;
    bool g1 = (segs >>  7) & 1;
    bool g2 = (segs >>  6) & 1;
    bool h  = (segs >>  5) & 1;
    bool ii = (segs >>  4) & 1;
    bool j  = (segs >>  3) & 1;
    bool k  = (segs >>  2) & 1;
    bool l  = (segs >>  1) & 1;
    bool m  = (segs >>  0) & 1;
    
    // Horizontal segments: a (top), g1 (middle-left), g2 (middle-right), d (bottom)
    draw_hseg(cr, left, top, seg_w, thick, a);
    draw_hseg(cr, left, mid_y, seg_w / 2.0, thick, g1);
    draw_hseg(cr, left + seg_w / 2.0, mid_y, seg_w / 2.0, thick, g2);
    draw_hseg(cr, left, bottom, seg_w, thick, d);
    
    // Vertical segments: f (upper-left), b (upper-right),
    //                    e (lower-left), c (lower-right)
    draw_vseg(cr, left, top, seg_h, thick, f);
    draw_vseg(cr, right, top, seg_h, thick, b);
    draw_vseg(cr, left, mid_y, seg_h, thick, e);
    draw_vseg(cr, right, mid_y, seg_h, thick, c);
    
    // Center verticals: i (upper), l (lower)
    draw_vseg(cr, ctr_x, top + thick, seg_h - thick * 1.5, thick, ii);
    draw_vseg(cr, ctr_x, mid_y + thick, seg_h - thick * 1.5, thick, l);
    
    // Diagonals
    double diag_inset = thick * 1.5;
    // h = upper-left diagonal (top-left to center)
    draw_diag(cr, left + diag_inset, top + diag_inset,
              ctr_x, mid_y - thick * 0.3, thick, h);
    // j = upper-right diagonal (top-right to center)
    draw_diag(cr, right + thick - diag_inset, top + diag_inset,
              ctr_x + thick, mid_y - thick * 0.3, thick, j);
    // m = lower-left diagonal (center to bottom-left)
    draw_diag(cr, left + diag_inset, bottom + thick - diag_inset,
              ctr_x, mid_y + thick * 1.3, thick, m);
    // k = lower-right diagonal (center to bottom-right)
    draw_diag(cr, right + thick - diag_inset, bottom + thick - diag_inset,
              ctr_x + thick, mid_y + thick * 1.3, thick, k);
}
