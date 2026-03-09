#pragma once
#include <gtkmm.h>
#include <vector>
#include <cstdint>

/**
 * @brief An 8×8 LED matrix display drawn with Cairo.
 *
 * Each LED can be individually addressed.  Internally stores an
 * 8×8 boolean grid.
 */
class LEDMatrix : public Gtk::DrawingArea
{
public:
    LEDMatrix();
    
    /** Set an individual LED. */
    void set_led(int row, int col, bool on);
    
    /** Get an individual LED state. */
    bool get_led(int row, int col) const;
    
    /** Set an entire row from a bitmask (bit 0 = column 0). */
    void set_row(int row, uint8_t bits);
    
    /** Clear all LEDs. */
    void clear();
    
    /** Set the global color used for on/off rendering (r,g,b in 0..1). */
    void set_color(double r, double g, double b);
    
    /** Get the last-set color. */
    void get_color(double &r, double &g, double &b) const { r = r_; g = g_; b = b_; }

    /** Call after a batch of set_led/set_row calls to repaint. */
    void refresh() { queue_draw(); }
    
    static constexpr int ROWS = 8;
    static constexpr int COLS = 8;
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                 int width, int height);
    
    bool grid_[ROWS][COLS] = {};
    double r_ = 0.9, g_ = 0.08, b_ = 0.02;
};
