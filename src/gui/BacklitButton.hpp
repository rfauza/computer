#pragma once
#include <gtkmm.h>
#include <functional>
#include <string>
#include <algorithm>
#include <cmath>

/**
 * @brief Industrial-style backlit panel button drawn with Cairo.
 *
 * Appears as a dark rounded-rect with the label "cut out" of the face so
 * the backlight (the selected color) shines through.  Unlit label matches
 * the off-LED brightness; while held the button floods with color and the
 * label glows bright.  Callback fires on mouse release.
 */
class BacklitButton : public Gtk::DrawingArea
{
public:
    explicit BacklitButton(const std::string& label = "");

    using ClickCB = std::function<void()>;
    void set_click_callback(ClickCB cb) { cb_ = std::move(cb); }

    /** Change the backlight color (accepts same un-normalised values as LED). */
    void set_color(double r, double g, double b);

private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int w, int h);
    void on_press(int n, double x, double y);
    void on_release(int n, double x, double y);
    void on_motion(double x, double y);
    static void rounded_rect(const Cairo::RefPtr<Cairo::Context>& cr,
                              double x, double y, double w, double h, double r);

    std::string label_;
    bool pressed_  = false;
    bool pressed_inside_ = false;
    double r_ = 1.5, g_ = 0.0, b_ = 0.0;   // default red (un-normalised)
    ClickCB cb_;
    Glib::RefPtr<Gtk::GestureClick> click_;
    Glib::RefPtr<Gtk::EventControllerMotion> motion_;
};
