#pragma once
#include <gtkmm.h>

/**
 * @brief A round LED indicator drawn with Cairo.
 *
 * When on, the LED glows brightly; when off it is a dim dark colour.
 * The colour can be configured (defaults to red).
 */
class LED : public Gtk::DrawingArea
{
public:
    LED(double r = 1.0, double g = 0.0, double b = 0.0);
    
    void set_on(bool on);
    bool is_on() const { return on_; }
    
    void set_color(double r, double g, double b);
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                 int width, int height);
    
    bool on_ = false;
    double r_ = 1.0, g_ = 0.0, b_ = 0.0;
};
