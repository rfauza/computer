#pragma once
#include <gtkmm.h>
#include <functional>

/**
 * @brief A realistic round push-button drawn with Cairo.
 *
 * Clicking, pressing Space, or pressing Enter triggers the button.
 */
class PushButton : public Gtk::DrawingArea
{
public:
    PushButton();
    
    using PressCB = std::function<void()>;
    void set_press_callback(PressCB cb) { cb_ = std::move(cb); }
    
    /** Flash the pressed visual for a moment. */
    void flash();
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                 int width, int height);
    void on_click_pressed(int n_press, double x, double y);
    
    bool pressed_ = false;
    PressCB cb_;
    Glib::RefPtr<Gtk::GestureClick> click_;
};
