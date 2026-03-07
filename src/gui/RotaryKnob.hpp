#pragma once
#include <gtkmm.h>
#include <functional>

/**
 * @brief A continuous rotary knob for selecting clock frequency.
 *
 * Drag up or scroll up to increase frequency. Maps knob position to a
 * logarithmic frequency scale from 0.5 Hz to 500 kHz. Labels are drawn
 * around the arc at each decade.
 */
class RotaryKnob : public Gtk::DrawingArea
{
public:
    RotaryKnob();

    /** @brief Get the frequency in Hz for the current position. */
    double get_frequency_hz() const;

    /** @brief Set the knob position (0.0 = 0.5 Hz, 1.0 = 500 kHz). */
    void set_value(double value);

    using KnobCB = std::function<void(double)>;
    void set_change_callback(KnobCB cb) { cb_ = std::move(cb); }

private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr, int width, int height);
    void on_drag_begin(double x, double y);
    void on_drag_update(double dx, double dy);
    bool on_scroll(double dx, double dy);

    double value_            = 0.0;
    double drag_start_value_ = 0.0;

    KnobCB cb_;

    Glib::RefPtr<Gtk::GestureDrag>           drag_;
    Glib::RefPtr<Gtk::EventControllerScroll> scroll_;

    static constexpr double FREQ_MIN  =      0.5;
    static constexpr double FREQ_MAX  = 500000.0;
    static constexpr double DRAG_SENS =    280.0;  // px for full range

    static constexpr int NUM_LABELS = 7;
    static constexpr double label_freqs_[NUM_LABELS] = {
        0.5, 5.0, 50.0, 500.0, 5000.0, 50000.0, 500000.0
    };
    static constexpr const char* label_texts_[NUM_LABELS] = {
        "0.5Hz", "5Hz", "50Hz", "500Hz", "5kHz", "50kHz", "500kHz"
    };
};
