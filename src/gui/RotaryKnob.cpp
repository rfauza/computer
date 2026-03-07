#include "RotaryKnob.hpp"
#include <cmath>

RotaryKnob::RotaryKnob()
{
    set_content_width(140);
    set_content_height(140);
    set_draw_func(sigc::mem_fun(*this, &RotaryKnob::on_draw));
    set_cursor("ns-resize");

    drag_ = Gtk::GestureDrag::create();
    drag_->signal_drag_begin().connect(
        sigc::mem_fun(*this, &RotaryKnob::on_drag_begin));
    drag_->signal_drag_update().connect(
        sigc::mem_fun(*this, &RotaryKnob::on_drag_update));
    add_controller(drag_);

    scroll_ = Gtk::EventControllerScroll::create();
    scroll_->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
    scroll_->signal_scroll().connect(
        sigc::mem_fun(*this, &RotaryKnob::on_scroll), false);
    add_controller(scroll_);
}

double RotaryKnob::get_frequency_hz() const
{
    const double log_min = std::log10(FREQ_MIN);
    const double log_max = std::log10(FREQ_MAX);
    return std::pow(10.0, log_min + value_ * (log_max - log_min));
}

void RotaryKnob::set_value(double value)
{
    value = std::max(0.0, std::min(1.0, value));
    if (value_ != value)
    {
        value_ = value;
        queue_draw();
        if (cb_)
        {
            cb_(get_frequency_hz());
        }
    }
}

void RotaryKnob::on_drag_begin(double /*x*/, double /*y*/)
{
    drag_start_value_ = value_;
}

void RotaryKnob::on_drag_update(double /*dx*/, double dy)
{
    set_value(drag_start_value_ - dy / DRAG_SENS);
}

bool RotaryKnob::on_scroll(double /*dx*/, double dy)
{
    set_value(value_ - dy / 60.0);
    return true;
}

void RotaryKnob::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                          int width, int height)
{
    const double cx = width  / 2.0;
    const double cy = height / 2.0;
    const double base = std::min(cx, cy);
    const double r       = base * 0.38;   // knob radius
    const double r_track = r + 5.0;       // arc track radius
    const double r_label = base * 0.76;   // label centre radius

    // Angle conventions:
    //   angle_min (value=0) -> 7 o'clock (lower-left)
    //   angle_max (value=1) -> 5 o'clock (lower-right)
    //   The -pi/2 shift rotates the zero ref from 3-o'clock to 12-o'clock.
    constexpr double ANGLE_MIN   = -150.0 * M_PI / 180.0;
    constexpr double ANGLE_RANGE =  300.0 * M_PI / 180.0;

    auto val_to_angle = [&](double v) -> double {
        return ANGLE_MIN + v * ANGLE_RANGE - M_PI / 2.0;
    };

    // ---- Outer chrome ring ----
    auto ring = Cairo::RadialGradient::create(cx, cy, r * 0.85, cx, cy, r + 2);
    ring->add_color_stop_rgb(0.0, 0.6, 0.6, 0.6);
    ring->add_color_stop_rgb(1.0, 0.3, 0.3, 0.3);
    cr->arc(cx, cy, r + 2, 0, 2 * M_PI);
    cr->set_source(ring);
    cr->fill();

    // ---- Knob body ----
    auto body = Cairo::RadialGradient::create(
        cx - r * 0.15, cy - r * 0.15, r * 0.05, cx, cy, r);
    body->add_color_stop_rgb(0.0, 0.22, 0.22, 0.22);
    body->add_color_stop_rgb(1.0, 0.10, 0.10, 0.10);
    cr->arc(cx, cy, r, 0, 2 * M_PI);
    cr->set_source(body);
    cr->fill();

    // ---- Ridges ----
    cr->set_source_rgba(1, 1, 1, 0.07);
    for (int i = 0; i < 24; ++i)
    {
        double a = i * M_PI * 2.0 / 24.0;
        cr->move_to(cx + r * 0.55 * std::cos(a), cy + r * 0.55 * std::sin(a));
        cr->line_to(cx + r * 0.92 * std::cos(a), cy + r * 0.92 * std::sin(a));
    }
    cr->set_line_width(1.0);
    cr->stroke();

    // ---- Arc track (dim, full range) ----
    const double arc_start = val_to_angle(0.0);
    const double arc_end   = val_to_angle(1.0);
    cr->set_source_rgba(0.45, 0.45, 0.45, 0.6);
    cr->set_line_width(2.0);
    cr->arc(cx, cy, r_track, arc_start, arc_end);
    cr->stroke();

    // ---- Arc fill (amber, min to current) ----
    const double current_angle = val_to_angle(value_);
    if (value_ > 0.0)
    {
        cr->set_source_rgba(0.9, 0.45, 0.1, 0.85);
        cr->set_line_width(2.5);
        cr->arc(cx, cy, r_track, arc_start, current_angle);
        cr->stroke();
    }

    // ---- Position indicator ----
    cr->set_source_rgb(1.0, 1.0, 1.0);
    cr->set_line_width(2.5);
    cr->move_to(cx + r * 0.3  * std::cos(current_angle),
                cy + r * 0.3  * std::sin(current_angle));
    cr->line_to(cx + r * 0.85 * std::cos(current_angle),
                cy + r * 0.85 * std::sin(current_angle));
    cr->stroke();

    // ---- Tick marks and labels around arc ----
    cr->select_font_face("monospace", Cairo::ToyFontFace::Slant::NORMAL,
                          Cairo::ToyFontFace::Weight::NORMAL);
    cr->set_font_size(7.5);

    const double log_min = std::log10(FREQ_MIN);
    const double log_max = std::log10(FREQ_MAX);

    for (int i = 0; i < NUM_LABELS; ++i)
    {
        const double frac = (std::log10(label_freqs_[i]) - log_min) / (log_max - log_min);
        const double a    = val_to_angle(frac);
        const double ca   = std::cos(a);
        const double sa   = std::sin(a);

        // Tick
        const double tick_inner = r + 4.0;
        const double tick_outer = r + 9.0;
        cr->set_source_rgb(0.75, 0.75, 0.75);
        cr->set_line_width(1.2);
        cr->move_to(cx + tick_inner * ca, cy + tick_inner * sa);
        cr->line_to(cx + tick_outer * ca, cy + tick_outer * sa);
        cr->stroke();

        // Label centred at r_label along the tick direction
        Cairo::TextExtents te;
        cr->get_text_extents(label_texts_[i], te);
        const double tx = cx + r_label * ca;
        const double ty = cy + r_label * sa;
        cr->set_source_rgb(0.0, 0.0, 0.0);
        cr->move_to(tx - te.x_bearing - te.width  / 2.0,
                    ty - te.y_bearing - te.height / 2.0);
        cr->show_text(label_texts_[i]);
    }
}
