#include "ThreeWaySwitch.hpp"
#include <cmath>

ThreeWaySwitch::ThreeWaySwitch()
{
    set_content_width(28);
    set_content_height(56);
    set_draw_func(sigc::mem_fun(*this, &ThreeWaySwitch::on_draw));
    
    click_ = Gtk::GestureClick::create();
    click_->signal_pressed().connect(
        sigc::mem_fun(*this, &ThreeWaySwitch::on_click_pressed));
    add_controller(click_);
    
    set_focusable(true);
}

void ThreeWaySwitch::set_position(int pos)
{
    pos = std::clamp(pos, 0, 2);
    if (position_ != pos)
    {
        position_ = pos;
        queue_draw();
        if (cb_)
        {
            cb_(position_);
        }
    }
}

void ThreeWaySwitch::on_click_pressed(int /*n_press*/, double /*x*/, double y)
{
    double h = get_height();
    int new_pos;
    if (y < h / 3.0)
    {
        new_pos = 0;
    }
    else if (y < 2.0 * h / 3.0)
    {
        new_pos = 1;
    }
    else
    {
        new_pos = 2;
    }
    set_position(new_pos);
}

void ThreeWaySwitch::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                             int width, int height)
{
    double cx = width / 2.0;
    double cy = height / 2.0;
    
    // Base plate (dark grey circle)
    double base_radius = std::min(cx, cy) * 0.45;
    cr->arc(cx, cy, base_radius, 0, 2 * M_PI);
    cr->set_source_rgb(0.25, 0.25, 0.25);
    cr->fill();
    
    // Inner ring
    cr->arc(cx, cy, base_radius * 0.85, 0, 2 * M_PI);
    cr->set_source_rgb(0.3, 0.3, 0.3);
    cr->fill();
    
    // Toggle lever — 3 positions
    double lever_len = height * 0.35;
    double lever_w = base_radius * 0.35;
    
    double tip_y;
    if (position_ == 0)
    {
        tip_y = cy - lever_len * 0.9;
    }
    else if (position_ == 1)
    {
        tip_y = cy;
    }
    else
    {
        tip_y = cy + lever_len * 0.9;
    }
    
    double tip_x = cx;
    
    // Shadow
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
    
    // Lever tip knob
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
