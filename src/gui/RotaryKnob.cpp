#include "RotaryKnob.hpp"
#include <cmath>

RotaryKnob::RotaryKnob()
{
    set_content_width(72);
    set_content_height(72);
    set_draw_func(sigc::mem_fun(*this, &RotaryKnob::on_draw));
    
    click_ = Gtk::GestureClick::create();
    click_->signal_pressed().connect(
        sigc::mem_fun(*this, &RotaryKnob::on_click_pressed));
    add_controller(click_);
    
    scroll_ = Gtk::EventControllerScroll::create();
    scroll_->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    scroll_->signal_scroll().connect(
        sigc::mem_fun(*this, &RotaryKnob::on_scroll), false);
    add_controller(scroll_);
}

double RotaryKnob::get_frequency_hz() const
{
    return frequencies_[step_];
}

void RotaryKnob::set_step(int step)
{
    step = std::max(0, std::min(step, NUM_STEPS - 1));
    if (step_ != step)
    {
        step_ = step;
        queue_draw();
        if (cb_)
        {
            cb_(step_, frequencies_[step_]);
        }
    }
}

void RotaryKnob::on_click_pressed(int /*n_press*/, double x, double /*y*/)
{
    int w = get_width();
    if (x < w / 2.0)
    {
        set_step(step_ - 1);
    }
    else
    {
        set_step(step_ + 1);
    }
}

bool RotaryKnob::on_scroll(double /*dx*/, double dy)
{
    if (dy < 0)
    {
        set_step(step_ + 1);
    }
    else if (dy > 0)
    {
        set_step(step_ - 1);
    }
    return true;
}

void RotaryKnob::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                          int width, int height)
{
    double cx = width / 2.0;
    double cy = height / 2.0;
    double r = std::min(cx, cy) * 0.75;
    
    // Outer chrome ring
    auto ring = Cairo::RadialGradient::create(cx, cy, r * 0.85, cx, cy, r + 2);
    ring->add_color_stop_rgb(0.0, 0.6, 0.6, 0.6);
    ring->add_color_stop_rgb(1.0, 0.3, 0.3, 0.3);
    cr->arc(cx, cy, r + 2, 0, 2 * M_PI);
    cr->set_source(ring);
    cr->fill();
    
    // Knob body (dark, ridged)
    auto body = Cairo::RadialGradient::create(
        cx - r * 0.15, cy - r * 0.15, r * 0.05, cx, cy, r);
    body->add_color_stop_rgb(0.0, 0.22, 0.22, 0.22);
    body->add_color_stop_rgb(1.0, 0.10, 0.10, 0.10);
    cr->arc(cx, cy, r, 0, 2 * M_PI);
    cr->set_source(body);
    cr->fill();
    
    // Ridges (circular lines)
    cr->set_source_rgba(1, 1, 1, 0.07);
    for (int i = 0; i < 24; ++i)
    {
        double a = i * M_PI * 2.0 / 24.0;
        cr->move_to(cx + r * 0.55 * cos(a), cy + r * 0.55 * sin(a));
        cr->line_to(cx + r * 0.92 * cos(a), cy + r * 0.92 * sin(a));
    }
    cr->set_line_width(1.0);
    cr->stroke();
    
    // Position indicator line
    // Map step (0..6) to angle: -150° to +150° (in radians)
    double angle_min = -150.0 * M_PI / 180.0;
    double angle_max =  150.0 * M_PI / 180.0;
    double angle_range = angle_max - angle_min;
    double frac = static_cast<double>(step_) / (NUM_STEPS - 1);
    double angle = angle_min + frac * angle_range - M_PI / 2.0; // -90 to rotate CW from top
    
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->set_line_width(2.5);
    cr->move_to(cx + r * 0.3 * cos(angle), cy + r * 0.3 * sin(angle));
    cr->line_to(cx + r * 0.85 * cos(angle), cy + r * 0.85 * sin(angle));
    cr->stroke();
    
    // Tick marks and labels around the knob
    cr->set_source_rgb(0.85, 0.85, 0.85);
    cr->select_font_face("monospace", Cairo::ToyFontFace::Slant::NORMAL,
                          Cairo::ToyFontFace::Weight::NORMAL);
    cr->set_font_size(7.0);
    
    for (int i = 0; i < NUM_STEPS; ++i)
    {
        double f = static_cast<double>(i) / (NUM_STEPS - 1);
        double a = angle_min + f * angle_range - M_PI / 2.0;
        
        // Tick mark
        cr->set_line_width(1.2);
        cr->move_to(cx + (r + 4) * cos(a), cy + (r + 4) * sin(a));
        cr->line_to(cx + (r + 8) * cos(a), cy + (r + 8) * sin(a));
        cr->stroke();
    }
    
    // Current frequency label below knob
    cr->set_font_size(9.0);
    Cairo::TextExtents extents;
    cr->get_text_extents(labels_[step_], extents);
    cr->move_to(cx - extents.width / 2.0, height - 2);
    cr->show_text(labels_[step_]);
}
