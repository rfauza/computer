#include "ComputerWindow.hpp"
#include "../computers/Computer.hpp"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>

// ═══════════════════════════════════════════════════════════════════════
// Construction / destruction
// ═══════════════════════════════════════════════════════════════════════

ComputerWindow::ComputerWindow(Computer* computer, uint16_t num_bits)
    : computer_(computer),
      num_bits_(num_bits),
      pc_bits_(computer->get_pc_bits()),
      num_ram_addr_bits_(computer->get_num_ram_addr_bits()),
      pages_per_ram_(static_cast<uint16_t>(1u << num_bits)),
      addrs_per_page_(static_cast<uint16_t>(1u << num_bits))
{
    set_title("Computer 3 Bit v1 ISA");
    set_default_size(1200, 520);
    
    build_ui();
    
    // Keyboard controller on the window
    auto key_ctrl = Gtk::EventControllerKey::create();
    key_ctrl->signal_key_pressed().connect(
        sigc::mem_fun(*this, &ComputerWindow::on_key_pressed), false);
    add_controller(key_ctrl);
    
    // Start in program / write mode; set switches and update displays
    mode_ = Mode::PROGRAM;
    prog_sub_ = ProgSub::WRITE;
    // Ensure the UI switches reflect these defaults:
    // mode_switch_: up(off)=Program -> set_off(false)
    // sub_switch_: down(on)=Write   -> set_on(true)
    if (mode_switch_) mode_switch_->set_on(false);
    if (sub_switch_)  sub_switch_->set_on(true);
    update_all_displays();
}

ComputerWindow::~ComputerWindow()
{
    stop_auto_timer();
}

// ═══════════════════════════════════════════════════════════════════════
// UI construction
// ═══════════════════════════════════════════════════════════════════════

void ComputerWindow::build_ui()
{
    // Root vertical box
    auto* root = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    root->set_margin_start(4);
    root->set_margin_end(4);
    root->set_margin_top(4);
    root->set_margin_bottom(4);
    
    // ── Top row: seven-seg bar + LED matrix ───────────────────────────
    auto* top_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 8));
    top_row->append(*build_seven_seg_bar());
    top_row->append(*build_led_matrix_panel());
    root->append(*top_row);
    
    // Title bar
    title_label_ = Gtk::manage(new Gtk::Label("Computer 3 Bit v1 ISA"));
    title_label_->set_markup("<span foreground='#cccccc' size='small'>Computer 3 Bit v1 ISA</span>");
    title_label_->set_halign(Gtk::Align::CENTER);
    root->append(*title_label_);
    
    // ── Bottom row: main panel + RAM panels ───────────────────────────
    auto* bot_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 8));
    bot_row->append(*build_main_panel());
    bot_row->append(*build_ram_led_panel());
    bot_row->append(*build_ram_seg_panel());
    root->append(*bot_row);
    
    set_child(*root);
    
    // Style the window background
    auto css = Gtk::CssProvider::create();
    css->load_from_data(
        "window { background-color: #5a5e63; }"
        "label  { color: #cccccc; }");
    Gtk::StyleContext::add_provider_for_display(
        Gdk::Display::get_default(), css,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

// ── Seven-Segment Display Bar ─────────────────────────────────────────

Gtk::Box* ComputerWindow::build_seven_seg_bar()
{
    auto* bar = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 6));
    bar->set_margin_start(4);
    bar->set_margin_end(4);
    bar->set_margin_top(4);
    bar->set_margin_bottom(4);
    
    // PM Addr — multiple digits
    // For pc_bits_: number of octal digits needed = ceil(pc_bits_ / log2(max_digit+1))
    // We display as decimal 7-seg digits. Number of digits = ceil(pc_bits_ / 4) for hex,
    // but the image shows 3 digits for 9-bit addr (max 511). We'll use ceil(pc_bits_/3)
    // to show in octal, or just ceil(log10(2^pc_bits_)) decimal digits.
    int num_pm_digits = static_cast<int>(std::ceil(pc_bits_ / 3.0)); // octal digits
    
    auto* pm_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    auto* pm_segs_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = 0; i < num_pm_digits; ++i)
    {
        auto* seg = Gtk::manage(new SevenSegDisplay());
        pm_addr_segs_.push_back(seg);
        pm_segs_row->append(*seg);
    }
    pm_box->append(*pm_segs_row);
    auto* pm_lbl = Gtk::manage(new Gtk::Label("PM Addr"));
    pm_lbl->set_margin_top(2);
    pm_box->append(*pm_lbl);
    bar->append(*pm_box);
    
    // Spacer
    auto* sp1 = Gtk::manage(new Gtk::Box());
    sp1->set_size_request(20, -1);
    bar->append(*sp1);
    
    // OP Code
    auto* op_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    opcode_seg_ = Gtk::manage(new SevenSegDisplay());
    op_box->append(*opcode_seg_);
    op_box->append(*Gtk::manage(new Gtk::Label("OP Code")));
    bar->append(*op_box);
    
    // A
    auto* a_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    a_seg_ = Gtk::manage(new SevenSegDisplay());
    a_box->append(*a_seg_);
    a_box->append(*Gtk::manage(new Gtk::Label("A")));
    bar->append(*a_box);
    
    // B
    auto* b_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    b_seg_ = Gtk::manage(new SevenSegDisplay());
    b_box->append(*b_seg_);
    b_box->append(*Gtk::manage(new Gtk::Label("B")));
    bar->append(*b_box);
    
    // C
    auto* c_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    c_seg_ = Gtk::manage(new SevenSegDisplay());
    c_box->append(*c_seg_);
    c_box->append(*Gtk::manage(new Gtk::Label("C")));
    bar->append(*c_box);
    
    return bar;
}

// ── Main Panel ────────────────────────────────────────────────────────

Gtk::Box* ComputerWindow::build_main_panel()
{
    auto* panel = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 4));
    panel->set_margin_start(4);
    panel->set_margin_end(4);
    panel->set_margin_top(4);
    panel->set_margin_bottom(4);
    
    // Control column (knob, button, mode switches)
    panel->append(*build_control_column());
    
    // PM Addr switches
    panel->append(*build_switch_led_column("PM Addr Sel", pc_bits_,
                                            pm_addr_switches_, pm_addr_leds_, true));
    
    // OP Code switches
    panel->append(*build_switch_led_column("OP Code", num_bits_,
                                            opcode_switches_, opcode_leds_, true));
    
    // A switches
    panel->append(*build_switch_led_column("A", num_bits_,
                                            a_switches_, a_leds_, true));
    
    // B switches
    panel->append(*build_switch_led_column("B", num_bits_,
                                            b_switches_, b_leds_, true));
    
    // C switches
    panel->append(*build_switch_led_column("C", num_bits_,
                                            c_switches_, c_leds_, true));
    
    return panel;
}

// ── Control Column ────────────────────────────────────────────────────

Gtk::Box* ComputerWindow::build_control_column()
{
    auto* col = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 4));
    col->set_margin_end(8);
    col->set_margin_start(4);
    
    // Clock speed label
    auto* freq_lbl = Gtk::manage(new Gtk::Label());
    freq_lbl->set_markup("<span size='small'>Clock Speed</span>");
    col->append(*freq_lbl);
    
    // Rotary knob
    knob_ = Gtk::manage(new RotaryKnob());
    knob_->set_change_callback(
        sigc::mem_fun(*this, &ComputerWindow::on_knob_changed));
    col->append(*knob_);
    
    // Mode switch (Program / Run)
    auto* mode_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    auto* prog_lbl = Gtk::manage(new Gtk::Label());
    prog_lbl->set_markup("<span size='x-small'>Program</span>");
    mode_box->append(*prog_lbl);
    mode_switch_ = Gtk::manage(new ToggleSwitch());
    mode_switch_->set_toggle_callback([this](bool) { on_mode_switch_toggled(); });
    mode_box->append(*mode_switch_);
    auto* run_lbl_w = Gtk::manage(new Gtk::Label());
    run_lbl_w->set_markup("<span size='x-small'>Run</span>");
    mode_box->append(*run_lbl_w);
    col->append(*mode_box);
    
    mode_label_ = Gtk::manage(new Gtk::Label("Program"));
    mode_label_->set_markup("<span size='x-small' weight='bold'>Program</span>");
    col->append(*mode_label_);
    
    // Sub-mode switch
    auto* sub_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    auto* rw_lbl = Gtk::manage(new Gtk::Label());
    rw_lbl->set_markup("<span size='x-small'>Auto/\nRead</span>");
    sub_box->append(*rw_lbl);
    sub_switch_ = Gtk::manage(new ToggleSwitch());
    sub_switch_->set_toggle_callback([this](bool) { on_mode_switch_toggled(); });
    sub_box->append(*sub_switch_);
    auto* pw_lbl = Gtk::manage(new Gtk::Label());
    pw_lbl->set_markup("<span size='x-small'>Pulse/\nWrite</span>");
    sub_box->append(*pw_lbl);
    col->append(*sub_box);
    
    sub_label_ = Gtk::manage(new Gtk::Label("Write"));
    sub_label_->set_markup("<span size='x-small' weight='bold'>Write</span>");
    col->append(*sub_label_);
    
    // Pulse / Write button
    pulse_button_ = Gtk::manage(new PushButton());
    pulse_button_->set_press_callback(
        sigc::mem_fun(*this, &ComputerWindow::on_pulse_pressed));
    col->append(*pulse_button_);
    
    auto* btn_lbl = Gtk::manage(new Gtk::Label());
    btn_lbl->set_markup("<span size='x-small'>Pulse / Write</span>");
    col->append(*btn_lbl);
    
    return col;
}

// ── Switch/LED Column ─────────────────────────────────────────────────

Gtk::Box* ComputerWindow::build_switch_led_column(
    const std::string& label, int num_switches,
    std::vector<ToggleSwitch*>& switches,
    std::vector<LED*>& leds,
    bool msb_labels)
{
    auto* col = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    
    // Bit-weight labels row
    auto* weight_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 1));
    for (int i = num_switches - 1; i >= 0; --i)
    {
        auto* lbl = Gtk::manage(new Gtk::Label());
        std::string txt = std::to_string(1 << i);
        lbl->set_markup("<span size='x-small'>" + txt + "</span>");
        lbl->set_size_request(28, -1);
        lbl->set_halign(Gtk::Align::CENTER);
        weight_row->append(*lbl);
    }
    col->append(*weight_row);
    
    // LEDs row
    auto* led_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 1));
    for (int i = num_switches - 1; i >= 0; --i)
    {
        auto* led = Gtk::manage(new LED(1.0, 0.0, 0.0));
        leds.push_back(led);
        led->set_size_request(28, 18);
        led_row->append(*led);
    }
    // leds are stored MSB-first in the vector; reverse so index 0 = bit 0
    std::reverse(leds.begin(), leds.end());
    col->append(*led_row);
    
    // Switches row
    auto* sw_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 1));
    for (int i = num_switches - 1; i >= 0; --i)
    {
        auto* sw = Gtk::manage(new ToggleSwitch());
        switches.push_back(sw);
        sw->set_size_request(28, 48);
        sw->set_toggle_callback([this](bool) { on_switch_toggled(); });
        sw_row->append(*sw);
    }
    // Same: reverse so index 0 = bit 0
    std::reverse(switches.begin(), switches.end());
    col->append(*sw_row);
    
    // Label
    auto* name_lbl = Gtk::manage(new Gtk::Label(label));
    name_lbl->set_halign(Gtk::Align::CENTER);
    col->append(*name_lbl);
    
    return col;
}

// ── RAM LED Panel ─────────────────────────────────────────────────────

Gtk::Box* ComputerWindow::build_ram_led_panel()
{
    auto* panel = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    panel->set_margin_start(8);
    
    // Title
    auto* title = Gtk::manage(new Gtk::Label());
    title->set_markup("<span size='small' weight='bold'>RAM Page Select</span>");
    panel->append(*title);
    
    // Page select switches
    auto* page_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = num_bits_ - 1; i >= 0; --i)
    {
        auto* sw = Gtk::manage(new ToggleSwitch());
        ram_page_switches_.push_back(sw);
        sw->set_size_request(24, 36);
        sw->set_toggle_callback([this](bool) { on_ram_page_changed(); });
        page_row->append(*sw);
    }
    std::reverse(ram_page_switches_.begin(), ram_page_switches_.end());
    panel->append(*page_row);
    
    // Bit-weight labels for page switches
    auto* pw_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = num_bits_ - 1; i >= 0; --i)
    {
        auto* lbl = Gtk::manage(new Gtk::Label(std::to_string(1 << i)));
        lbl->set_size_request(24, -1);
        lbl->set_halign(Gtk::Align::CENTER);
        pw_row->append(*lbl);
    }
    panel->append(*pw_row);
    
    // "Addr" label
    auto* addr_lbl = Gtk::manage(new Gtk::Label("Addr"));
    addr_lbl->set_halign(Gtk::Align::START);
    panel->append(*addr_lbl);
    
    // RAM LED grid: addrs_per_page_ rows × num_bits_ columns
    ram_leds_.resize(addrs_per_page_);
    for (int addr = 0; addr < addrs_per_page_; ++addr)
    {
        auto* row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 1));
        
        // Address label
        auto* a_lbl = Gtk::manage(new Gtk::Label(std::to_string(addr)));
        a_lbl->set_size_request(16, -1);
        row->append(*a_lbl);
        
        for (int bit = num_bits_ - 1; bit >= 0; --bit)
        {
            auto* led = Gtk::manage(new LED(1.0, 0.0, 0.0));
            led->set_size_request(18, 18);
            ram_leds_[addr].push_back(led);
            row->append(*led);
        }
        // Reverse so index 0 = bit 0
        std::reverse(ram_leds_[addr].begin(), ram_leds_[addr].end());
        
        panel->append(*row);
    }
    
    return panel;
}

// ── RAM Seven-Seg Panel ───────────────────────────────────────────────

Gtk::Box* ComputerWindow::build_ram_seg_panel()
{
    auto* panel = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    panel->set_margin_start(8);
    
    // Title
    auto* title = Gtk::manage(new Gtk::Label());
    title->set_markup("<span size='small' weight='bold'>RAM Page Select</span>");
    panel->append(*title);
    
    // Slave/Independent switch
    auto* slave_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 4));
    auto* slave_lbl = Gtk::manage(new Gtk::Label());
    slave_lbl->set_markup("<span size='x-small'>Slave</span>");
    slave_row->append(*slave_lbl);
    slave_switch_ = Gtk::manage(new ToggleSwitch());
    slave_switch_->set_size_request(24, 36);
    slave_switch_->set_toggle_callback([this](bool) { on_slave_toggled(); });
    slave_row->append(*slave_switch_);
    auto* indep_lbl = Gtk::manage(new Gtk::Label());
    indep_lbl->set_markup("<span size='x-small'>Indep</span>");
    slave_row->append(*indep_lbl);
    panel->append(*slave_row);
    
    // Page select switches (for independent mode)
    auto* page2_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = num_bits_ - 1; i >= 0; --i)
    {
        auto* sw = Gtk::manage(new ToggleSwitch());
        ram_page2_switches_.push_back(sw);
        sw->set_size_request(24, 36);
        sw->set_toggle_callback([this](bool) { on_ram_page2_changed(); });
        page2_row->append(*sw);
    }
    std::reverse(ram_page2_switches_.begin(), ram_page2_switches_.end());
    panel->append(*page2_row);
    
    // "Data" label
    auto* data_lbl = Gtk::manage(new Gtk::Label("Data"));
    data_lbl->set_halign(Gtk::Align::CENTER);
    panel->append(*data_lbl);
    
    // Seven-segment displays — arrange in 2 rows of 4
    int total_segs = addrs_per_page_;
    int half = total_segs / 2;
    
    auto* row1 = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = 0; i < half; ++i)
    {
        auto* seg = Gtk::manage(new SevenSegDisplay());
        ram_segs_.push_back(seg);
        row1->append(*seg);
    }
    panel->append(*row1);
    
    // Address labels for first row
    auto* lbl_row1 = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = 0; i < half; ++i)
    {
        auto* lbl = Gtk::manage(new Gtk::Label(std::to_string(i)));
        lbl->set_size_request(36, -1);
        lbl->set_halign(Gtk::Align::CENTER);
        lbl_row1->append(*lbl);
    }
    panel->append(*lbl_row1);
    
    auto* row2 = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = half; i < total_segs; ++i)
    {
        auto* seg = Gtk::manage(new SevenSegDisplay());
        ram_segs_.push_back(seg);
        row2->append(*seg);
    }
    panel->append(*row2);
    
    // Address labels for second row
    auto* lbl_row2 = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = half; i < total_segs; ++i)
    {
        auto* lbl = Gtk::manage(new Gtk::Label(std::to_string(i)));
        lbl->set_size_request(36, -1);
        lbl->set_halign(Gtk::Align::CENTER);
        lbl_row2->append(*lbl);
    }
    panel->append(*lbl_row2);
    
    return panel;
}

// ── LED Matrix Panel ──────────────────────────────────────────────────

Gtk::Box* ComputerWindow::build_led_matrix_panel()
{
    auto* panel = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    
    led_matrix_ = Gtk::manage(new LEDMatrix());
    panel->append(*led_matrix_);
    
    return panel;
}

// ═══════════════════════════════════════════════════════════════════════
// Keyboard handling
// ═══════════════════════════════════════════════════════════════════════

int ComputerWindow::total_navigable_switches() const
{
    // PM addr + opcode + A + B + C
    return static_cast<int>(pm_addr_switches_.size() +
                            opcode_switches_.size() +
                            a_switches_.size() +
                            b_switches_.size() +
                            c_switches_.size());
}

ToggleSwitch* ComputerWindow::get_navigable_switch(int index) const
{
    int offset = 0;
    
    // PM addr switches (MSB first visually, but stored bit-0 first)
    // Navigation goes left-to-right visually = MSB to LSB = reverse index
    int n = static_cast<int>(pm_addr_switches_.size());
    if (index < offset + n)
    {
        return pm_addr_switches_[n - 1 - (index - offset)];
    }
    offset += n;
    
    n = static_cast<int>(opcode_switches_.size());
    if (index < offset + n)
    {
        return opcode_switches_[n - 1 - (index - offset)];
    }
    offset += n;
    
    n = static_cast<int>(a_switches_.size());
    if (index < offset + n)
    {
        return a_switches_[n - 1 - (index - offset)];
    }
    offset += n;
    
    n = static_cast<int>(b_switches_.size());
    if (index < offset + n)
    {
        return b_switches_[n - 1 - (index - offset)];
    }
    offset += n;
    
    n = static_cast<int>(c_switches_.size());
    if (index < offset + n)
    {
        return c_switches_[n - 1 - (index - offset)];
    }
    
    return nullptr;
}

void ComputerWindow::select_switch(int index)
{
    deselect_all();
    int total = total_navigable_switches();
    if (total == 0) return;
    
    // Wrap around
    if (index < 0) index = total - 1;
    if (index >= total) index = 0;
    
    selected_index_ = index;
    auto* sw = get_navigable_switch(index);
    if (sw)
    {
        sw->set_selected(true);
    }
}

void ComputerWindow::deselect_all()
{
    for (auto* sw : pm_addr_switches_)  sw->set_selected(false);
    for (auto* sw : opcode_switches_)   sw->set_selected(false);
    for (auto* sw : a_switches_)        sw->set_selected(false);
    for (auto* sw : b_switches_)        sw->set_selected(false);
    for (auto* sw : c_switches_)        sw->set_selected(false);
    selected_index_ = -1;
}

bool ComputerWindow::on_key_pressed(guint keyval, guint /*keycode*/,
                                     Gdk::ModifierType /*state*/)
{
    switch (keyval)
    {
        case GDK_KEY_Left:
            select_switch(selected_index_ - 1);
            return true;
            
        case GDK_KEY_Right:
            select_switch(selected_index_ + 1);
            return true;
            
        case GDK_KEY_Up:
        case GDK_KEY_Down:
        {
            if (selected_index_ >= 0)
            {
                auto* sw = get_navigable_switch(selected_index_);
                if (sw)
                {
                    bool new_state = (keyval == GDK_KEY_Down); // down = on
                    sw->set_on(new_state);
                }
            }
            return true;
        }
        
        case GDK_KEY_space:
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
            on_pulse_pressed();
            pulse_button_->flash();
            return true;
            
        case GDK_KEY_Escape:
            deselect_all();
            return true;
            
        default:
            break;
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════
// Mode switching
// ═══════════════════════════════════════════════════════════════════════

void ComputerWindow::on_mode_switch_toggled()
{
    Mode prev_mode = mode_;
    
    // mode_switch_: up(off)=Program, down(on)=Run
    if (mode_switch_->is_on())
    {
        mode_ = Mode::RUN;
        
        // Only prepare the computer when transitioning from PROGRAM to RUN
        if (prev_mode == Mode::PROGRAM)
        {
            computer_->prepare_run();
        }
        
        // sub_switch_: up(off)=Auto, down(on)=Pulse
        if (sub_switch_->is_on())
        {
            run_sub_ = RunSub::PULSE;
            stop_auto_timer();
        }
        else
        {
            run_sub_ = RunSub::AUTO;
            start_auto_timer();
        }
        mode_label_->set_markup("<span size='x-small' weight='bold'>Run</span>");
        sub_label_->set_markup(run_sub_ == RunSub::AUTO
            ? "<span size='x-small' weight='bold'>Auto</span>"
            : "<span size='x-small' weight='bold'>Pulse</span>");
    }
    else
    {
        mode_ = Mode::PROGRAM;
        stop_auto_timer();
        // sub_switch_: down(on)=Write, up(off)=Read
        if (sub_switch_->is_on())
        {
            prog_sub_ = ProgSub::WRITE;
        }
        else
        {
            prog_sub_ = ProgSub::READ;
        }
        mode_label_->set_markup("<span size='x-small' weight='bold'>Program</span>");
        sub_label_->set_markup(prog_sub_ == ProgSub::WRITE
            ? "<span size='x-small' weight='bold'>Write</span>"
            : "<span size='x-small' weight='bold'>Read</span>");
    }
    
    update_all_displays();
}

// ═══════════════════════════════════════════════════════════════════════
// Switch and button handlers
// ═══════════════════════════════════════════════════════════════════════

void ComputerWindow::on_switch_toggled()
{
    update_all_displays();
}

void ComputerWindow::on_pulse_pressed()
{
    if (mode_ == Mode::PROGRAM)
    {
        if (prog_sub_ == ProgSub::WRITE)
        {
            // Write switch values to PM at the selected address
            uint16_t addr   = read_switch_value(pm_addr_switches_);
            uint16_t opcode = read_switch_value(opcode_switches_);
            uint16_t a_val  = read_switch_value(a_switches_);
            uint16_t b_val  = read_switch_value(b_switches_);
            uint16_t c_val  = read_switch_value(c_switches_);
            
            computer_->write_pm_instruction(addr, opcode, a_val, b_val, c_val);
        }
        // In read mode, button does nothing
    }
    else // Mode::RUN
    {
        if (run_sub_ == RunSub::PULSE)
        {
            // Single clock tick
            computer_->clock_tick();
            computer_->sync_pc();
            update_all_displays();
        }
        // In auto mode the timer handles ticks
    }
}

void ComputerWindow::on_knob_changed(int /*step*/, double freq)
{
    if (mode_ == Mode::RUN && run_sub_ == RunSub::AUTO)
    {
        // Restart timer with new frequency
        stop_auto_timer();
        start_auto_timer();
    }
}

void ComputerWindow::on_ram_page_changed()
{
    update_ram_led_display();
    
    // If slave mode, also update the RAM seg panel
    if (slave_switch_ && slave_switch_->is_on())
    {
        // Copy page switches to page2 switches
        for (size_t i = 0; i < ram_page_switches_.size() && i < ram_page2_switches_.size(); ++i)
        {
            ram_page2_switches_[i]->set_on(ram_page_switches_[i]->is_on());
        }
        update_ram_seg_display();
    }
}

void ComputerWindow::on_ram_page2_changed()
{
    if (!slave_switch_ || !slave_switch_->is_on())
    {
        update_ram_seg_display();
    }
}

void ComputerWindow::on_slave_toggled()
{
    if (slave_switch_->is_on())
    {
        // Slave mode: sync page2 from page1
        for (size_t i = 0; i < ram_page_switches_.size() && i < ram_page2_switches_.size(); ++i)
        {
            ram_page2_switches_[i]->set_on(ram_page_switches_[i]->is_on());
        }
        update_ram_seg_display();
    }
}

// ═══════════════════════════════════════════════════════════════════════
// Auto-run timer
// ═══════════════════════════════════════════════════════════════════════

void ComputerWindow::start_auto_timer()
{
    stop_auto_timer();
    double freq = knob_->get_frequency_hz();
    // For frequencies up to 1kHz, use a 1-tick-per-callback timer.
    // For higher frequencies, batch multiple ticks per callback to keep up.
    // GTK timers can't fire faster than ~1ms, so we batch at 1ms intervals.
    int interval_ms = std::max(1, static_cast<int>(1000.0 / freq));
    auto_timer_ = Glib::signal_timeout().connect(
        sigc::mem_fun(*this, &ComputerWindow::on_auto_tick), interval_ms);
}

void ComputerWindow::stop_auto_timer()
{
    if (auto_timer_.connected())
    {
        auto_timer_.disconnect();
    }
}

bool ComputerWindow::on_auto_tick()
{
    if (mode_ != Mode::RUN || run_sub_ != RunSub::AUTO)
    {
        return false; // stop the timer
    }
    
    if (!computer_->get_is_running())
    {
        stop_auto_timer();
        update_all_displays();
        return false;
    }
    
    // For frequencies above 1kHz, execute multiple ticks per callback
    // since the GTK timer can't fire faster than ~1ms.
    double freq = knob_->get_frequency_hz();
    int ticks_per_call = std::max(1, static_cast<int>(freq / 1000.0));
    
    for (int i = 0; i < ticks_per_call && computer_->get_is_running(); ++i)
    {
        computer_->clock_tick();
    }
    computer_->sync_pc();
    update_all_displays();
    
    return true; // keep ticking
}

// ═══════════════════════════════════════════════════════════════════════
// Display updates
// ═══════════════════════════════════════════════════════════════════════

uint16_t ComputerWindow::read_switch_value(const std::vector<ToggleSwitch*>& sw) const
{
    uint16_t val = 0;
    for (size_t i = 0; i < sw.size(); ++i)
    {
        if (sw[i]->is_on())
        {
            val |= (1u << i);
        }
    }
    return val;
}

void ComputerWindow::set_leds_from_value(std::vector<LED*>& leds, uint16_t value)
{
    for (size_t i = 0; i < leds.size(); ++i)
    {
        leds[i]->set_on(((value >> i) & 1) != 0);
    }
}

void ComputerWindow::update_all_displays()
{
    update_main_leds();
    update_seven_segs();
    update_ram_led_display();
    update_ram_seg_display();
    update_led_matrix_display();
}

void ComputerWindow::update_main_leds()
{
    if (mode_ == Mode::PROGRAM)
    {
        if (prog_sub_ == ProgSub::WRITE)
        {
            // LEDs reflect switch positions directly
            set_leds_from_value(pm_addr_leds_, read_switch_value(pm_addr_switches_));
            set_leds_from_value(opcode_leds_,  read_switch_value(opcode_switches_));
            set_leds_from_value(a_leds_,       read_switch_value(a_switches_));
            set_leds_from_value(b_leds_,       read_switch_value(b_switches_));
            set_leds_from_value(c_leds_,       read_switch_value(c_switches_));
        }
        else // READ
        {
            // PM addr LEDs reflect switch positions
            uint16_t addr = read_switch_value(pm_addr_switches_);
            set_leds_from_value(pm_addr_leds_, addr);
            
            // Data LEDs reflect PM contents at the selected address
            uint16_t opcode, a_val, b_val, c_val;
            computer_->read_pm_instruction(addr, opcode, a_val, b_val, c_val);
            set_leds_from_value(opcode_leds_, opcode);
            set_leds_from_value(a_leds_,      a_val);
            set_leds_from_value(b_leds_,      b_val);
            set_leds_from_value(c_leds_,      c_val);
        }
    }
    else // RUN
    {
        // LEDs reflect current PC and instruction
        uint16_t pc = computer_->get_pc();
        set_leds_from_value(pm_addr_leds_, pc);
        
        uint16_t opcode, a_val, b_val, c_val;
        computer_->get_current_instruction(opcode, a_val, b_val, c_val);
        set_leds_from_value(opcode_leds_, opcode);
        set_leds_from_value(a_leds_,      a_val);
        set_leds_from_value(b_leds_,      b_val);
        set_leds_from_value(c_leds_,      c_val);
    }
}

void ComputerWindow::update_seven_segs()
{
    // The 7-seg displays mirror the LEDs above them
    
    // PM Addr — displayed in octal digits, MSB first
    uint16_t addr;
    if (mode_ == Mode::PROGRAM)
    {
        addr = read_switch_value(pm_addr_switches_);
        if (prog_sub_ == ProgSub::READ)
        {
            addr = read_switch_value(pm_addr_switches_);
        }
    }
    else
    {
        addr = computer_->get_pc();
    }
    
    // Fill 7-seg digits (stored MSD-first in pm_addr_segs_)
    for (size_t i = 0; i < pm_addr_segs_.size(); ++i)
    {
        int shift = static_cast<int>((pm_addr_segs_.size() - 1 - i) * 3);
        uint8_t digit = (addr >> shift) & 0x07;
        pm_addr_segs_[i]->set_value(digit);
    }
    
    // Opcode, A, B, C — single digit each
    uint16_t opcode, a_val, b_val, c_val;
    if (mode_ == Mode::PROGRAM && prog_sub_ == ProgSub::WRITE)
    {
        opcode = read_switch_value(opcode_switches_);
        a_val  = read_switch_value(a_switches_);
        b_val  = read_switch_value(b_switches_);
        c_val  = read_switch_value(c_switches_);
    }
    else if (mode_ == Mode::PROGRAM && prog_sub_ == ProgSub::READ)
    {
        uint16_t pm_addr = read_switch_value(pm_addr_switches_);
        computer_->read_pm_instruction(pm_addr, opcode, a_val, b_val, c_val);
    }
    else
    {
        computer_->get_current_instruction(opcode, a_val, b_val, c_val);
    }
    
    opcode_seg_->set_value(static_cast<uint8_t>(opcode & 0x0F));
    a_seg_->set_value(static_cast<uint8_t>(a_val & 0x0F));
    b_seg_->set_value(static_cast<uint8_t>(b_val & 0x0F));
    c_seg_->set_value(static_cast<uint8_t>(c_val & 0x0F));
}

void ComputerWindow::update_ram_led_display()
{
    uint16_t page = read_switch_value(ram_page_switches_);
    
    for (int addr = 0; addr < addrs_per_page_; ++addr)
    {
        uint16_t full_addr = static_cast<uint16_t>((page << num_bits_) | addr);
        uint16_t val = 0;
        if (full_addr < computer_->get_num_ram_addresses())
        {
            val = computer_->read_ram(full_addr);
        }
        
        for (int bit = 0; bit < num_bits_; ++bit)
        {
            if (bit < static_cast<int>(ram_leds_[addr].size()))
            {
                ram_leds_[addr][bit]->set_on(((val >> bit) & 1) != 0);
            }
        }
    }
}

void ComputerWindow::update_ram_seg_display()
{
    uint16_t page;
    if (slave_switch_ && slave_switch_->is_on())
    {
        page = read_switch_value(ram_page_switches_);
    }
    else
    {
        page = read_switch_value(ram_page2_switches_);
    }
    
    for (int addr = 0; addr < addrs_per_page_ && addr < static_cast<int>(ram_segs_.size()); ++addr)
    {
        uint16_t full_addr = static_cast<uint16_t>((page << num_bits_) | addr);
        uint16_t val = 0;
        if (full_addr < computer_->get_num_ram_addresses())
        {
            val = computer_->read_ram(full_addr);
        }
        ram_segs_[addr]->set_value(static_cast<uint8_t>(val & 0x0F));
    }
}

void ComputerWindow::update_led_matrix_display()
{
    if (!led_matrix_) return;
    
    // The 8×8 LED matrix is wired to the last 3 pages of RAM (pages 5, 6, 7).
    // Mapping (0,0 is top-left):
    //   Columns 0-2:  page 5, bits 1,2,3
    //   Columns 3-5:  page 6, bits 1,2,7
    //   Columns 6-7:  page 7, bits 1,2
    // Each row maps to an address within each page.
    
    led_matrix_->clear();
    
    // Define the bit mapping for each visual column
    const int col_page[8]  = {5, 5, 5, 6, 6, 6, 7, 7};
    // Use zero-based bit indices within the RAM data width (0..NUM_BITS-1).
    // Previous values included out-of-range indices (3 and 7) which always read as 0.
    const int col_bit[8]   = {0, 1, 2, 0, 1, 2, 0, 1};
    
    for (int row = 0; row < 8 && row < static_cast<int>(addrs_per_page_); ++row)
    {
        for (int col = 0; col < 8; ++col)
        {
            int page = col_page[col];
            int bit = col_bit[col];
            
            uint16_t full_addr = static_cast<uint16_t>((page << num_bits_) | row);
            uint16_t val = 0;
            if (full_addr < computer_->get_num_ram_addresses())
            {
                val = computer_->read_ram(full_addr);
            }
            
            bool led_on = ((val >> bit) & 1) != 0;
            led_matrix_->set_led(row, col, led_on);
        }
    }
    
    led_matrix_->refresh();
}
