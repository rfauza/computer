#include "PushButton.hpp"
#include <cmath>

PushButton::PushButton()
{
    set_content_width(52);
    set_content_height(52);
    set_draw_func(sigc::mem_fun(*this, &PushButton::on_draw));
    
    click_ = Gtk::GestureClick::create();
    click_->signal_pressed().connect(
        sigc::mem_fun(*this, &PushButton::on_press));
    click_->signal_released().connect(
        sigc::mem_fun(*this, &PushButton::on_release));
    add_controller(click_);
    
    motion_ = Gtk::EventControllerMotion::create();
    motion_->signal_motion().connect(sigc::mem_fun(*this, &PushButton::on_motion));
    add_controller(motion_);
    
    set_focusable(true);
}

void PushButton::flash()
{
    // temporary flash regardless of pointer state
    pressed_ = true;
    pressed_inside_ = false;
    queue_draw();
    Glib::signal_timeout().connect_once([this]()
    {
        pressed_ = false;
        pressed_inside_ = false;
        queue_draw();
    }, 100);
}

void PushButton::set_color(double r, double g, double b)
{
    r_ = r; g_ = g; b_ = b;
    queue_draw();
}

void PushButton::on_press(int /*n_press*/, double x, double y)
{
    // Mark pressed when clicked anywhere on the button
    auto alloc = get_allocation();
    double w = alloc.get_width();
    double h = alloc.get_height();
    
    if (x >= 0 && x <= w && y >= 0 && y <= h)
    {
        pressed_ = true;
        pressed_inside_ = true;
        queue_draw();
    }
}

void PushButton::on_release(int /*n_press*/, double x, double y)
{
    // on release, determine if release happened inside the button
    auto alloc = get_allocation();
    double w = alloc.get_width();
    double h = alloc.get_height();
    
    bool released_inside = (x >= 0 && x <= w && y >= 0 && y <= h);

    // clear visual state
    pressed_ = false;
    bool triggered = pressed_inside_ && released_inside;
    pressed_inside_ = false;
    queue_draw();

    if (triggered && cb_)
        cb_();
}

void PushButton::on_motion(double x, double y)
{
    // if not pressed, ignore
    if (!pressed_ && !pressed_inside_)
        return;

    auto alloc = get_allocation();
    double w = alloc.get_width();
    double h = alloc.get_height();
    bool inside = (x >= 0 && x <= w && y >= 0 && y <= h);

    if (inside && !pressed_inside_)
    {
        pressed_inside_ = true;
        pressed_ = true;
        queue_draw();
    }
    else if (!inside && pressed_inside_)
    {
        // dragged off -> clear visual
        pressed_inside_ = false;
        pressed_ = false;
        queue_draw();
    }
}

void PushButton::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                          int width, int height)
{
    double cx = width / 2.0;
    double cy = height / 2.0;
    double outer_r = std::min(cx, cy) * 0.85;

    // Normalize color like backlit buttons do
    double max_c = std::max({r_, g_, b_});
    if (max_c < 1e-6) max_c = 1.0;
    double nr = r_ / max_c, ng = g_ / max_c, nb = b_ / max_c;

    // ── 1. Outer bezel ring (neutral grey) ─────────────────────────
    cr->arc(cx, cy, outer_r + 1.5, 0, 2 * M_PI);
    cr->set_source_rgb(0.28, 0.28, 0.30);
    cr->fill();

    // ── 2. Black plastic ring (thinner band) ──────────────────────
    double ring_width = outer_r * 0.28;
    double inner_r = outer_r - ring_width;
    
    cr->arc(cx, cy, outer_r * 0.95, 0, 2 * M_PI);
    cr->set_source_rgb(0.08, 0.08, 0.09);
    cr->fill();

    // ── 3. Lit center disk (flatter, less gradient) ────────────────
    auto center = Cairo::RadialGradient::create(cx, cy, inner_r * 0.2,
                                                 cx, cy, inner_r);
    if (pressed_)
    {
        // Pressed: bright, less extreme gradient for flatter look
        center->add_color_stop_rgba(0.0, r_, g_, b_, 1.0);
        center->add_color_stop_rgba(1.0, r_ * 0.8, g_ * 0.8, b_ * 0.8, 1.0);
    }
    else
    {
        // Unpressed: dim, flatter appearance
        center->add_color_stop_rgba(0.0, nr * 0.32, ng * 0.32, nb * 0.32, 1.0);
        center->add_color_stop_rgba(1.0, nr * 0.25, ng * 0.25, nb * 0.25, 1.0);
    }
    cr->arc(cx, cy, inner_r * 0.95, 0, 2 * M_PI);
    cr->set_source(center);
    cr->fill();

    // ── 5. Minimal gloss highlight (very subtle for flatter look) ──
    if (gloss_enabled_)
    {
        cr->arc(cx - inner_r * 0.15, cy - inner_r * 0.20, inner_r * 0.20, 0, 2 * M_PI);
        cr->set_source_rgba(1.0, 1.0, 1.0, pressed_ ? 0.03 : 0.08);
        cr->fill();
    }
}
