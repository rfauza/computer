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
    // Only mark pressed if the pointer is inside the color diffuser area
    auto alloc = get_allocation();
    double w = alloc.get_width();
    double h = alloc.get_height();
    double cx = w / 2.0;
    double cy = h / 2.0;
    double radius = std::min(cx, cy) * 0.8;
    double dx = x - cx;
    double dy = y - cy;
    double inner_r = radius * 0.64; // match diffuser disk radius
    bool inside = (dx * dx + dy * dy) <= (inner_r * inner_r);

    if (inside)
    {
        pressed_ = true;
        pressed_inside_ = true;
        queue_draw();
    }
}

void PushButton::on_release(int /*n_press*/, double x, double y)
{
    // on release, determine if release happened inside the active area
    auto alloc = get_allocation();
    double w = alloc.get_width();
    double h = alloc.get_height();
    double cx = w / 2.0;
    double cy = h / 2.0;
    double radius = std::min(cx, cy) * 0.8;
    double dx = x - cx;
    double dy = y - cy;
    double dist2 = dx * dx + dy * dy;
    double inner_r = radius * 0.64;

    bool released_inside = dist2 <= (inner_r * inner_r);

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
    double cx = w / 2.0;
    double cy = h / 2.0;
    double radius = std::min(cx, cy) * 0.8;
    double dx = x - cx;
    double dy = y - cy;
    double dist2 = dx * dx + dy * dy;
    double inner_r = radius * 0.64;
    bool inside = dist2 <= (inner_r * inner_r);

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
    double r = std::min(cx, cy) * 0.8;

    // Outer bezel ring (neutral grey)
    cr->arc(cx, cy, r + 2.0, 0, 2 * M_PI);
    cr->set_source_rgb(0.32, 0.32, 0.32);
    cr->fill();

    // Bright glowing diffuser with radial gradient
    // Center is very bright (nearly white-ish tinted with color),
    // fades to edge which approaches the bezel color
    double intensity = pressed_ ? 0.6 : 1.0;
    auto glow = Cairo::RadialGradient::create(cx, cy, r * 0.1,
                                              cx, cy, r);
    
    // Center: very bright, tinted with the button color
    glow->add_color_stop_rgba(0.0, 
        std::min(1.0, r_ * intensity + 0.3),
        std::min(1.0, g_ * intensity + 0.3),
        std::min(1.0, b_ * intensity + 0.3),
        1.0);
    
    // Mid: still bright, more saturated color
    glow->add_color_stop_rgba(0.5,
        std::min(1.0, r_ * intensity),
        std::min(1.0, g_ * intensity),
        std::min(1.0, b_ * intensity),
        1.0);
    
    // Edge: fades to dark
    glow->add_color_stop_rgba(1.0, 0.12, 0.12, 0.12, 1.0);

    cr->arc(cx, cy, r * 0.95, 0, 2 * M_PI);
    cr->set_source(glow);
    cr->fill();

    // Glossy highlight on upper part (like a dome's reflection)
    cr->set_source_rgba(1.0, 1.0, 1.0, pressed_ ? 0.08 : 0.22);
    cr->arc(cx - r * 0.25, cy - r * 0.30, r * 0.22, 0, 2 * M_PI);
    cr->fill();
}
