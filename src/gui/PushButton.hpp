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
    
    /** Set button color used for the body (r,g,b). */
    void set_color(double r, double g, double b);
    
    /** Flash the pressed visual for a moment. */
    void flash();

    /** Enable or disable the small gloss/specular highlight. */
    void set_gloss_enabled(bool en) { gloss_enabled_ = en; queue_draw(); }
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                 int width, int height);
    void on_click_pressed(int n_press, double x, double y);
    void on_press(int n_press, double x, double y);
    void on_release(int n_press, double x, double y);
    void on_motion(double x, double y);
    
    bool pressed_ = false;
    bool pressed_inside_ = false;
    PressCB cb_;
    Glib::RefPtr<Gtk::GestureClick> click_;
    Glib::RefPtr<Gtk::EventControllerMotion> motion_;
    double r_ = 0.9, g_ = 0.08, b_ = 0.02; // default red-ish
    bool gloss_enabled_ = true;
};
