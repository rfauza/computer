#include "BacklitButton.hpp"
#include <cmath>

BacklitButton::BacklitButton(const std::string& label)
    : label_(label)
{
    set_content_width(80);
    set_content_height(36);
    set_draw_func(sigc::mem_fun(*this, &BacklitButton::on_draw));

    click_ = Gtk::GestureClick::create();
    click_->signal_pressed().connect(
        sigc::mem_fun(*this, &BacklitButton::on_press));
    click_->signal_released().connect(
        sigc::mem_fun(*this, &BacklitButton::on_release));
    add_controller(click_);

    motion_ = Gtk::EventControllerMotion::create();
    motion_->signal_motion().connect(sigc::mem_fun(*this, &BacklitButton::on_motion));
    add_controller(motion_);

    set_focusable(true);
}

void BacklitButton::set_color(double r, double g, double b)
{
    r_ = r; g_ = g; b_ = b;
    queue_draw();
}

void BacklitButton::on_press(int /*n*/, double x, double y)
{
    pressed_ = true;
    
    // Check if press is inside button bounds
    auto alloc = get_allocation();
    double w = alloc.get_width();
    double h = alloc.get_height();
    if (x >= 0 && x <= w && y >= 0 && y <= h)
    {
        pressed_inside_ = true;
    }
    
    queue_draw();
}

void BacklitButton::on_release(int /*n*/, double x, double y)
{
    // Determine button bounds for release-inside check
    auto alloc = get_allocation();
    double w = alloc.get_width();
    double h = alloc.get_height();
    bool released_inside = (x >= 0 && x <= w && y >= 0 && y <= h);

    pressed_ = false;
    bool triggered = pressed_inside_ && released_inside;
    pressed_inside_ = false;
    queue_draw();

    if (triggered && cb_) cb_();
}

void BacklitButton::on_motion(double x, double y)
{
    // If not pressed, ignore motion
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
        pressed_inside_ = false;
        pressed_ = false;
        queue_draw();
    }
}

void BacklitButton::rounded_rect(const Cairo::RefPtr<Cairo::Context>& cr,
                                  double x, double y, double w, double h, double r)
{
    cr->move_to(x + r, y);
    cr->line_to(x + w - r, y);
    cr->arc(x + w - r, y + r, r, -M_PI / 2, 0);
    cr->line_to(x + w, y + h - r);
    cr->arc(x + w - r, y + h - r, r, 0, M_PI / 2);
    cr->line_to(x + r, y + h);
    cr->arc(x + r, y + h - r, r, M_PI / 2, M_PI);
    cr->line_to(x, y + r);
    cr->arc(x + r, y + r, r, M_PI, 3 * M_PI / 2);
    cr->close_path();
}

void BacklitButton::on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                             int width, int height)
{
    // Normalise stored color so the dominant channel is 1.0.
    // This keeps perceived brightness constant across color selections.
    double max_c = std::max({r_, g_, b_});
    if (max_c < 1e-6) max_c = 1.0;
    double nr = r_ / max_c, ng = g_ / max_c, nb = b_ / max_c;

    const double margin  = 2.5;
    const double corner  = 5.0;
    double bx = margin, by = margin;
    double bw = width  - 2 * margin;
    double bh = height - 2 * margin;

    // ── 1. Outer bezel ────────────────────────────────────────────────
    rounded_rect(cr, bx, by, bw, bh, corner);
    auto bezel = Cairo::LinearGradient::create(bx, by, bx, by + bh);
    bezel->add_color_stop_rgb(0.0, 0.28, 0.28, 0.30);
    bezel->add_color_stop_rgb(1.0, 0.14, 0.14, 0.16);
    cr->set_source(bezel);
    cr->fill();

    // ── 2. Inner plastic face ─────────────────────────────────────────
    const double pad = 2.0;
    double fx = bx + pad, fy = by + pad;
    double fw = bw - 2 * pad, fh = bh - 2 * pad;
    double fc = corner - 1.5;

    rounded_rect(cr, fx, fy, fw, fh, fc);
    cr->set_source_rgb(0.08, 0.08, 0.09);
    cr->fill();

    // ── 3. Very subtle ambient glow (text carries the color) ────────
    rounded_rect(cr, fx, fy, fw, fh, fc);
    auto ambient = Cairo::RadialGradient::create(
        fx + fw * 0.5, fy + fh * 0.5, 0,
        fx + fw * 0.5, fy + fh * 0.5, std::max(fw, fh) * 0.7);
    ambient->add_color_stop_rgba(0.0, nr * 0.08, ng * 0.08, nb * 0.08, 0.15);
    ambient->add_color_stop_rgba(1.0, nr * 0.02, ng * 0.02, nb * 0.02, 0.0);
    cr->set_source(ambient);
    cr->fill();

    // ── 4. Label text via cutout path ────────────────────────────────
    // Size the font to fit inside the button face.
    double font_size = 12.0;
    cr->select_font_face("sans-serif", Cairo::ToyFontFace::Slant::NORMAL,
                          Cairo::ToyFontFace::Weight::BOLD);
    cr->set_font_size(font_size);
    Cairo::TextExtents te;
    cr->get_text_extents(label_, te);

    // Shrink font if it's too wide for the face
    if (te.width > fw - 8.0)
    {
        font_size *= (fw - 8.0) / te.width;
        cr->set_font_size(font_size);
        cr->get_text_extents(label_, te);
    }

    double tx = fx + fw / 2.0 - te.x_bearing - te.width  / 2.0;
    double ty = fy + fh / 2.0 - te.y_bearing - te.height / 2.0;

    // Clip to the glyph shapes and paint the backlight through the cutout
    cr->save();
    cr->move_to(tx, ty);
    cr->text_path(label_);
    cr->clip();
    cr->rectangle(0, 0, width, height);
    if (pressed_)
    {
        // Text shines using the LED's mid-gradient color (which is what
        // the eye actually perceives as the dominant lit LED color).
        // Cairo renders values > 1.0 as high brightness, so this matches
        // what you see in the LED's lit appearance.
        cr->set_source_rgba(r_, g_, b_, 1.0);
    }
    else
    {
        // Dim cutout — match LED off-state color (normalized scaling)
        cr->set_source_rgba(nr * 0.32, ng * 0.32, nb * 0.32, 1.0);
    }
    cr->fill();
    cr->restore();

    // ── 5. Plastic gloss sheen (top half highlight) ───────────────────
    cr->save();
    rounded_rect(cr, fx, fy, fw, fh / 2.4, fc);
    cr->clip();
    auto gloss = Cairo::LinearGradient::create(fx, fy, fx, fy + fh / 2.4);
    gloss->add_color_stop_rgba(0.0, 1.0, 1.0, 1.0, pressed_ ? 0.05 : 0.12);
    gloss->add_color_stop_rgba(1.0, 1.0, 1.0, 1.0, 0.0);
    rounded_rect(cr, fx, fy, fw, fh / 2.4, fc);
    cr->set_source(gloss);
    cr->fill();
    cr->restore();

    // ── 6. Lit edge border (inset a few pixels) ──────────────────────
    cr->save();
    const double edge_inset = 3.0;
    const double edge_radius = corner - 0.8;
    rounded_rect(cr, bx + edge_inset, by + edge_inset,
                 bw - 2 * edge_inset, bh - 2 * edge_inset, edge_radius);
    cr->set_line_width(1.5);
    if (pressed_)
    {
        cr->set_source_rgba(r_, g_, b_, 0.85);
    }
    else
    {
        cr->set_source_rgba(nr * 0.32, ng * 0.32, nb * 0.32, 0.6);
    }
    cr->stroke();
    cr->restore();
}
