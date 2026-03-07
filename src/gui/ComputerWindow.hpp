#pragma once
#include <gtkmm.h>
#include <vector>
#include <cstdint>
#include <sigc++/connection.h>

#include "LED.hpp"
#include "ToggleSwitch.hpp"
#include "SevenSegDisplay.hpp"
#include "RotaryKnob.hpp"
#include "PushButton.hpp"
#include "LEDMatrix.hpp"

// Forward-declare the simulator base class so we only need the header
// in the .cpp file.
class Computer;

/**
 * @brief Main GUI window for the 3-bit (or N-bit) computer front panel.
 *
 * Mirrors the physical front-panel layout shown in the reference image.
 * Takes a Computer* and a bit-width so the same window class works for
 * 3-bit v1, 3-bit v2, and 4-bit variants.
 */
class ComputerWindow : public Gtk::Window
{
public:
    /**
     * @param computer  Pointer to the simulator instance (must outlive this window).
     * @param num_bits  Data-path width (e.g. 3 for Computer_3bit_v1).
     */
    ComputerWindow(Computer* computer, uint16_t num_bits);
    ~ComputerWindow() override;
    // Menu actions callable from application menu
    void open_load_dialog();
    void open_about_dialog();
    
private:
    // ── Mode enumerations ──────────────────────────────────────────────
    enum class Mode       { PROGRAM, RUN };
    enum class ProgSub    { WRITE, READ };
    enum class RunSub     { AUTO, PULSE };
    
    // ── Construction helpers ───────────────────────────────────────────
    void build_ui();
    Gtk::Box* build_seven_seg_bar();
    Gtk::Box* build_main_panel();
    Gtk::Box* build_control_column();
    Gtk::Box* build_switch_led_column(const std::string& label,
                                      int num_switches,
                                      std::vector<ToggleSwitch*>& switches,
                                      std::vector<LED*>& leds,
                                      bool msb_labels);
    Gtk::Box* build_ram_led_panel();
    Gtk::Box* build_ram_seg_panel();
    Gtk::Box* build_led_matrix_panel();
    
    // ── Event handlers ─────────────────────────────────────────────────
    bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);
    void on_switch_toggled();
    void on_mode_switch_toggled();
    void on_pulse_pressed();
    void on_goto_pc_pressed();
    void on_knob_changed(double freq);
    void on_ram_page_changed();
    void on_ram_page2_changed();
    void on_slave_toggled();
    void on_reset_pc();
    void on_reset_ram();
    void on_reset_all();
    
    // ── Periodic auto-run timer ────────────────────────────────────────
    bool on_auto_tick();
    void start_auto_timer();
    void stop_auto_timer();
    
    // ── Display update helpers ─────────────────────────────────────────
    void update_all_displays();
    void update_main_leds();
    void update_seven_segs();
    void update_ram_led_display();
    void update_ram_seg_display();
    void update_led_matrix_display();
    
    /** Read the value from a group of toggle switches (index 0 = bit 0). */
    uint16_t read_switch_value(const std::vector<ToggleSwitch*>& sw) const;
    
    /** Set LEDs from a value (index 0 = bit 0). */
    void set_leds_from_value(std::vector<LED*>& leds, uint16_t value);
    
    // ── Keyboard navigation ───────────────────────────────────────────
    void select_switch(int index);
    void deselect_all();
    int total_navigable_switches() const;
    ToggleSwitch* get_navigable_switch(int index) const;
    
    // ── Computer interface ─────────────────────────────────────────────
    Computer* computer_;
    uint16_t  num_bits_;
    uint16_t  pc_bits_;
    uint16_t  num_ram_addr_bits_;
    uint16_t  pages_per_ram_;
    uint16_t  addrs_per_page_;
    
    // ── Current mode ───────────────────────────────────────────────────
    Mode    mode_    = Mode::PROGRAM;
    ProgSub prog_sub_ = ProgSub::WRITE;
    RunSub  run_sub_  = RunSub::PULSE;
    
    // ── Widgets: controls ──────────────────────────────────────────────
    RotaryKnob*   knob_         = nullptr;
    PushButton*   pulse_button_ = nullptr;
    ToggleSwitch* mode_switch_  = nullptr;   // up=Program, down=Run
    ToggleSwitch* sub_switch_   = nullptr;   // Program: up=Write, down=Read
                                              // Run:     up=Auto,  down=Pulse
    Gtk::Button* goto_pc_button_ = nullptr;
    Gtk::Button* reset_pc_btn_  = nullptr;
    Gtk::Button* reset_ram_btn_ = nullptr;
    Gtk::Button* reset_all_btn_ = nullptr;
    
    // ── Widgets: main switch/LED columns ───────────────────────────────
    std::vector<ToggleSwitch*> pm_addr_switches_;
    std::vector<LED*>          pm_addr_leds_;
    std::vector<ToggleSwitch*> opcode_switches_;
    std::vector<LED*>          opcode_leds_;
    std::vector<ToggleSwitch*> a_switches_;
    std::vector<LED*>          a_leds_;
    std::vector<ToggleSwitch*> b_switches_;
    std::vector<LED*>          b_leds_;
    std::vector<ToggleSwitch*> c_switches_;
    std::vector<LED*>          c_leds_;
    
    // ── Widgets: seven-segment displays ────────────────────────────────
    std::vector<SevenSegDisplay*> pm_addr_segs_;
    SevenSegDisplay* opcode_seg_ = nullptr;
    SevenSegDisplay* a_seg_         = nullptr;
    SevenSegDisplay* b_seg_         = nullptr;
    SevenSegDisplay* c_seg_         = nullptr;
    
    // ── Widgets: RAM LED panel ─────────────────────────────────────────
    std::vector<ToggleSwitch*>         ram_page_switches_;
    std::vector<LED*>                  ram_page_leds_;      // LEDs above page switches
    std::vector<std::vector<LED*>>     ram_leds_;   // [addr][bit]
    
    // ── Widgets: RAM seven-seg panel ───────────────────────────────────
    std::vector<ToggleSwitch*>         ram_page2_switches_;
    ToggleSwitch*                      slave_switch_ = nullptr;
    SevenSegDisplay*                   ram_page_seg_ = nullptr; // Shows selected page
    std::vector<SevenSegDisplay*>      ram_segs_;    // 8 displays
    
    // ── Widgets: LED matrix ────────────────────────────────────────────
    LEDMatrix* led_matrix_ = nullptr;
    
    // ── Keyboard selection ─────────────────────────────────────────────
    int selected_index_ = -1;
    
    // ── Timer ──────────────────────────────────────────────────────────
    sigc::connection auto_timer_;
    
    // ── Labels that update ─────────────────────────────────────────────
    Gtk::Label* mode_label_  = nullptr;
    Gtk::Label* sub_label_   = nullptr;
    Gtk::Label* title_label_ = nullptr;
};
