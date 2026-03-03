#include "PushButton.hpp"
#include <cmath>

PushButton::PushButton()
{
    set_content_width(52);
    set_content_height(52);
    set_draw_func(sigc::mem_fun(*this, &PushButton::on_draw));
    
    click_ = Gtk::GestureClick::create();
    click_->signal_pressed().connect(
        sigc::mem_fun(*this, &PushButton::on_click_pressed));
    add_controller(click_);
    
    set_focusable(true);
}

void PushButton::flash()
{
    pressed_ = true;
    queue_draw();
    Glib::signal_timeout().connect_once([this]()
    {
        pressed_ = false;
        queue_draw();
    }, 100);
}

void PushButton::on_click_pressed(int /*n_press*/, double /*x*/, double /*y*/)
{
    flash();
    if (cb_)
    {
        cb_();
    }
}

void PushButton::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                          int width, int height)
{
    double cx = width / 2.0;
    double cy = height / 2.0;
    double r = std::min(cx, cy) * 0.8;
    
    // Outer chrome ring
    cr->arc(cx, cy, r + 3, 0, 2 * M_PI);
    cr->set_source_rgb(0.4, 0.4, 0.4);
    cr->fill();
    
    cr->arc(cx, cy, r + 1, 0, 2 * M_PI);
    cr->set_source_rgb(0.25, 0.25, 0.25);
    cr->fill();
    
    // Button body
    double shade = pressed_ ? 0.55 : 0.75;
    auto body = Cairo::RadialGradient::create(
        cx - r * 0.2, cy - r * 0.25, r * 0.1,
        cx, cy, r);
    body->add_color_stop_rgb(0.0, shade, 0.05 * shade, 0.02 * shade);
    body->add_color_stop_rgb(0.7, shade * 0.8, 0.02 * shade, 0.01 * shade);
    body->add_color_stop_rgb(1.0, shade * 0.5, 0.0, 0.0);
    
    cr->arc(cx, cy, r, 0, 2 * M_PI);
    cr->set_source(body);
    cr->fill();
    
    // Specular highlight
    cr->arc(cx - r * 0.15, cy - r * 0.2, r * 0.35, 0, 2 * M_PI);
    cr->set_source_rgba(1.0, 1.0, 1.0, pressed_ ? 0.12 : 0.25);
    cr->fill();
}
