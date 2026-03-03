#include "ToggleSwitch.hpp"
#include <cmath>

ToggleSwitch::ToggleSwitch()
{
    set_content_width(28);
    set_content_height(48);
    set_draw_func(sigc::mem_fun(*this, &ToggleSwitch::on_draw));
    
    click_ = Gtk::GestureClick::create();
    click_->signal_pressed().connect(
        sigc::mem_fun(*this, &ToggleSwitch::on_click_pressed));
    add_controller(click_);
    
    set_focusable(true);
}

void ToggleSwitch::set_on(bool on)
{
    if (on_ != on)
    {
        on_ = on;
        queue_draw();
        if (cb_)
        {
            cb_(on_);
        }
    }
}

void ToggleSwitch::set_selected(bool sel)
{
    if (selected_ != sel)
    {
        selected_ = sel;
        queue_draw();
    }
}

void ToggleSwitch::on_click_pressed(int /*n_press*/, double /*x*/, double /*y*/)
{
    set_on(!on_);
}

void ToggleSwitch::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                           int width, int height)
{
    double cx = width / 2.0;
    double cy = height / 2.0;
    
    // Selection rectangle
    if (selected_)
    {
        cr->set_source_rgba(0.5, 0.7, 1.0, 0.35);
        cr->rectangle(0, 0, width, height);
        cr->fill();
        cr->set_source_rgba(0.5, 0.7, 1.0, 0.8);
        cr->set_line_width(2.0);
        cr->rectangle(1, 1, width - 2, height - 2);
        cr->stroke();
    }
    
    // Base plate (dark grey circle)
    double base_radius = std::min(cx, cy) * 0.55;
    cr->arc(cx, cy, base_radius, 0, 2 * M_PI);
    cr->set_source_rgb(0.25, 0.25, 0.25);
    cr->fill();
    
    // Inner ring
    cr->arc(cx, cy, base_radius * 0.85, 0, 2 * M_PI);
    cr->set_source_rgb(0.3, 0.3, 0.3);
    cr->fill();
    
    // Toggle lever — a 3D-looking rod
    double lever_len = height * 0.38;
    double lever_w = base_radius * 0.35;
    double angle = on_ ? M_PI * 0.22 : -M_PI * 0.22; // tilt direction
    
    double tip_x = cx;
    double tip_y = cy + (on_ ? lever_len * 0.9 : -lever_len * 0.9);
    
    // Shadow under the lever
    cr->set_source_rgba(0.0, 0.0, 0.0, 0.3);
    cr->move_to(cx - lever_w, cy);
    cr->line_to(tip_x - lever_w * 0.5, tip_y);
    cr->line_to(tip_x + lever_w * 0.5, tip_y);
    cr->line_to(cx + lever_w, cy);
    cr->close_path();
    cr->fill();
    
    // Lever body (metallic silver)
    auto lever_grad = Cairo::LinearGradient::create(
        cx - lever_w, cy, cx + lever_w, cy);
    lever_grad->add_color_stop_rgb(0.0, 0.55, 0.55, 0.55);
    lever_grad->add_color_stop_rgb(0.4, 0.85, 0.85, 0.85);
    lever_grad->add_color_stop_rgb(0.6, 0.9, 0.9, 0.9);
    lever_grad->add_color_stop_rgb(1.0, 0.6, 0.6, 0.6);
    
    cr->move_to(cx - lever_w, cy);
    cr->line_to(tip_x - lever_w * 0.35, tip_y);
    cr->line_to(tip_x + lever_w * 0.35, tip_y);
    cr->line_to(cx + lever_w, cy);
    cr->close_path();
    cr->set_source(lever_grad);
    cr->fill_preserve();
    cr->set_source_rgb(0.35, 0.35, 0.35);
    cr->set_line_width(0.5);
    cr->stroke();
    
    // Lever tip (ball/knob)
    double knob_r = lever_w * 0.6;
    auto knob_grad = Cairo::RadialGradient::create(
        tip_x - knob_r * 0.2, tip_y - knob_r * 0.2, knob_r * 0.1,
        tip_x, tip_y, knob_r);
    knob_grad->add_color_stop_rgb(0.0, 0.95, 0.95, 0.95);
    knob_grad->add_color_stop_rgb(1.0, 0.5, 0.5, 0.5);
    
    cr->arc(tip_x, tip_y, knob_r, 0, 2 * M_PI);
    cr->set_source(knob_grad);
    cr->fill();
}
