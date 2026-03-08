#pragma once
#include <gtkmm.h>
#include <functional>

/**
 * @brief A 3-position toggle switch drawn with Cairo.
 *
 * Clicking cycles through up (0), middle (1), and down (2) positions.
 */
class ThreeWaySwitch : public Gtk::DrawingArea
{
public:
    ThreeWaySwitch();
    
    void set_position(int pos);
    int  get_position() const { return position_; }
    
    using ChangeCB = std::function<void(int)>;
    void set_change_callback(ChangeCB cb) { cb_ = std::move(cb); }
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                 int width, int height);
    void on_click_pressed(int n_press, double x, double y);
    
    int position_ = 0;  // 0=up, 1=middle, 2=down
    ChangeCB cb_;
    Glib::RefPtr<Gtk::GestureClick> click_;
};
