#pragma once
#include <gtkmm.h>
#include <functional>

/**
 * @brief A realistic toggle switch drawn with Cairo.
 *
 * Clicking or pressing up/down arrow toggles between up (off) and
 * down (on) positions.  A selection rectangle is drawn when focused.
 */
class ToggleSwitch : public Gtk::DrawingArea
{
public:
    ToggleSwitch();
    
    void set_on(bool on);
    bool is_on() const { return on_; }
    
    /** Visual highlight when keyboard-selected. */
    void set_selected(bool sel);
    bool is_selected() const { return selected_; }
    
    using ToggleCB = std::function<void(bool)>;
    void set_toggle_callback(ToggleCB cb) { cb_ = std::move(cb); }
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                 int width, int height);
    void on_click_pressed(int n_press, double x, double y);
    
    bool on_ = false;
    bool selected_ = false;
    ToggleCB cb_;
    Glib::RefPtr<Gtk::GestureClick> click_;
};
