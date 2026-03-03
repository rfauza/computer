#pragma once
#include <gtkmm.h>

/**
 * @brief Seven-segment display drawn with Cairo.
 *
 * Displays a single hex digit (0-F).  Segments glow red-on-black.
 */
class SevenSegDisplay : public Gtk::DrawingArea
{
public:
    SevenSegDisplay();
    
    /** @brief Set the digit to display (0-15, clamped). */
    void set_value(uint8_t val);
    uint8_t get_value() const { return value_; }
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                 int width, int height);
    
    void draw_segment(const Cairo::RefPtr<Cairo::Context>& cr,
                      double x, double y, double w, double h,
                      bool horizontal, bool on);
    
    uint8_t value_ = 0;
    
    // Segment truth table: segments a-g for digits 0-F
    static constexpr uint8_t seg_table[16] = {
        0x7E, // 0: a b c d e f
        0x30, // 1: b c
        0x6D, // 2: a b d e g
        0x79, // 3: a b c d g
        0x33, // 4: b c f g
        0x5B, // 5: a c d f g
        0x5F, // 6: a c d e f g
        0x70, // 7: a b c
        0x7F, // 8: all
        0x7B, // 9: a b c d f g
        0x77, // A: a b c e f g
        0x1F, // b: c d e f g
        0x4E, // C: a d e f
        0x3D, // d: b c d e g
        0x4F, // E: a d e f g
        0x47, // F: a e f g
    };
};
