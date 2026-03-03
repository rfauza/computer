#pragma once
#include <gtkmm.h>
#include <functional>

/**
 * @brief A rotary knob for selecting clock frequency.
 *
 * Displays a circular knob with a position indicator.
 * Clicking left/right halves, or scrolling, rotates the position.
 * Discrete steps on a logarithmic scale (1Hz, 10Hz, 100Hz, 1kHz, 10kHz, 100kHz, 1MHz).
 */
class RotaryKnob : public Gtk::DrawingArea
{
public:
    RotaryKnob();
    
    /** @brief Get current step index (0=1Hz, 6=1MHz). */
    int get_step() const { return step_; }
    
    /** @brief Get the frequency in Hz for the current step. */
    double get_frequency_hz() const;
    
    /** @brief Set the step index (clamped to 0..num_steps-1). */
    void set_step(int step);
    
    using KnobCB = std::function<void(int, double)>;
    void set_change_callback(KnobCB cb) { cb_ = std::move(cb); }
    
    static constexpr int NUM_STEPS = 7;
    
private:
    void on_draw(const Cairo::RefPtr<Cairo::Context>& cr,
                 int width, int height);
    void on_click_pressed(int n_press, double x, double y);
    bool on_scroll(double dx, double dy);
    
    int step_ = 0;
    KnobCB cb_;
    Glib::RefPtr<Gtk::GestureClick> click_;
    Glib::RefPtr<Gtk::EventControllerScroll> scroll_;
    
    static constexpr double frequencies_[NUM_STEPS] = {
        1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0, 1000000.0
    };
    static constexpr const char* labels_[NUM_STEPS] = {
        "1Hz", "10Hz", "100Hz", "1kHz", "10kHz", "100kHz", "1MHz"
    };
};
