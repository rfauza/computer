#pragma once
#include <gtkmm.h>

/**
 * @brief 14-segment alphanumeric display drawn with Cairo.
 *
 * Can display A-Z, 0-9, and a few symbols.
 * Supports LED (red/green/blue) and Nixie tube visual styles.
 *
 * Segment layout:
 *        ──a──
 *       |\ | /|
 *       f  h i b
 *       |  \|/ |
 *        ─g1 g2─
 *       |  /|\ |
 *       e  k j c
 *       | / | \|
 *        ──d──
 */
class MultiSegDisplay : public Gtk::DrawingArea
{
public:
    enum class Style { LED, NIXIE };
    
    MultiSegDisplay();
    
    /** Set the character to display (A-Z, 0-9, space). */
    void set_char(char ch);
    char get_char() const { return char_; }
    
    /** Set display color (r, g, b). */
    void set_color(double r, double g, double b);
    
    /** Set display style (LED or Nixie). */
    void set_style(Style style);
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                 int width, int height);
    
    void draw_hseg(const Cairo::RefPtr<Cairo::Context>& cr,
                   double x, double y, double w, double thick, bool on);
    void draw_vseg(const Cairo::RefPtr<Cairo::Context>& cr,
                   double x, double y, double h, double thick, bool on);
    void draw_diag(const Cairo::RefPtr<Cairo::Context>& cr,
                   double x1, double y1, double x2, double y2,
                   double thick, bool on);
    
    void draw_nixie_bg(const Cairo::RefPtr<Cairo::Context>& cr,
                       int width, int height);
    void draw_led_bg(const Cairo::RefPtr<Cairo::Context>& cr,
                     int width, int height);
    
    char char_ = ' ';
    double r_ = 0.2, g_ = 0.6, b_ = 1.0;  // default blue
    Style style_ = Style::LED;
    
    /** 14-segment bitmask for each character.
     *  Bits: 13..0 = a,b,c,d,e,f, g1,g2, h,i,j,k  (MSB to LSB)
     *  bit13=a, bit12=b, bit11=c, bit10=d, bit9=e, bit8=f,
     *  bit7=g1, bit6=g2, bit5=h, bit4=i, bit3=j, bit2=k */
    static uint16_t char_to_segs(char ch);
};
