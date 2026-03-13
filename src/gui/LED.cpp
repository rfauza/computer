#include "LED.hpp"

LED::LED(double r, double g, double b)
    : r_(r), g_(g), b_(b)
{
    set_content_width(16);
    set_content_height(16);
    set_draw_func(sigc::mem_fun(*this, &LED::on_draw));
}

void LED::set_on(bool on)
{
    if (on_ != on)
    {
        on_ = on;
        queue_draw();
    }
}

void LED::set_color(double r, double g, double b)
{
    r_ = r;
    g_ = g;
    b_ = b;
    queue_draw();
}

void LED::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                  int width, int height)
{
    double cx = width / 2.0;
    double cy = height / 2.0;
    double radius = std::min(cx, cy) * 0.7;
    
    // Dark bezel ring
    cr->arc(cx, cy, radius + 1.5, 0, 2 * M_PI);
    cr->set_source_rgb(0.15, 0.15, 0.15);
    cr->fill();
    
    if (on_)
    {
        // Glow halo
        auto glow = Cairo::RadialGradient::create(cx, cy, radius * 0.3,
                                                   cx, cy, radius * 2.0);
        glow->add_color_stop_rgba(0.0, r_, g_, b_, 0.6);
        glow->add_color_stop_rgba(1.0, r_, g_, b_, 0.0);
        cr->arc(cx, cy, radius * 2.0, 0, 2 * M_PI);
        cr->set_source(glow);
        cr->fill();
        
        // Bright LED body
        auto body = Cairo::RadialGradient::create(cx - radius * 0.2,
                                                   cy - radius * 0.2,
                                                   radius * 0.1,
                                                   cx, cy, radius);
        body->add_color_stop_rgb(0.0, std::min(1.0, r_ + 0.5),
                                       std::min(1.0, g_ + 0.5),
                                       std::min(1.0, b_ + 0.5));
        body->add_color_stop_rgb(0.6, r_, g_, b_);
        body->add_color_stop_rgb(1.0, r_ * 0.7, g_ * 0.7, b_ * 0.7);
        cr->arc(cx, cy, radius, 0, 2 * M_PI);
        cr->set_source(body);
        cr->fill();
    }
    else
    {
        // Dim LED body — normalize by max component first so changing color
        // doesn't change off-state brightness. Dominant channel is always 0.32.
        auto body = Cairo::RadialGradient::create(cx - radius * 0.2,
                                                   cy - radius * 0.2,
                                                   radius * 0.1,
                                                   cx, cy, radius);
        double max_c = std::max({r_, g_, b_});
        if (max_c < 1e-6) max_c = 1.0;
        double nr = r_ / max_c, ng = g_ / max_c, nb = b_ / max_c;
        // Make green-dominant LEDs darker in the off state so the "off" green
        // doesn't appear too bright compared to other colors.
        double hi_mul = 0.32;
        double lo_mul = 0.13;
        if (g_ > r_ && g_ > b_)
        {
            hi_mul = 0.24;
            lo_mul = 0.09;
        }
        body->add_color_stop_rgb(0.0, nr * hi_mul, ng * hi_mul, nb * hi_mul);
        body->add_color_stop_rgb(1.0, nr * lo_mul, ng * lo_mul, nb * lo_mul);
        cr->arc(cx, cy, radius, 0, 2 * M_PI);
        cr->set_source(body);
        cr->fill();
    }
    
    // Specular highlight
    cr->arc(cx - radius * 0.15, cy - radius * 0.25, radius * 0.35, 0, 2 * M_PI);
    cr->set_source_rgba(1.0, 1.0, 1.0, on_ ? 0.45 : 0.12);
    cr->fill();
}
