#include "ComputerWindow.hpp"
#include "../computers/Computer.hpp"
#include <iostream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <giomm.h>
// Avoid pulling in unavailable specific headers; use umbrella <gtkmm.h>

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

    build_ui();

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
    set_default_size(1340, 550);

    // ── Global CSS ─────────────────────────────────────────────────────
    auto css = Gtk::CssProvider::create();
    css->load_from_data(
        /* Window / base */
        "window { background-color: #42464d; }"
        "label  { color: #000000; }"

        /* Physical-unit panels */
        ".panel-unit {"
        "  background-color: #5a6069;"
        "  border-radius: 5px;"
        "  padding: 6px;"
        "}"

        /* Thin dark line between switch sections */
        ".section-sep {"
        "  background-color: #23262b;"
        "  min-width: 1px;"
        "  min-height: 28px;"
        "  margin-top: 2px;"
        "  margin-bottom: 2px;"
        "}"

        /* Section label (beneath each switch group) */
        ".section-label {"
        "  font-size: small;"
        "  color: #000000;"
        "}");
    Gtk::StyleContext::add_provider_for_display(
        Gdk::Display::get_default(), css,
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    // ── Root vertical box ──────────────────────────────────────────────
    auto* root = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
    root->set_margin_start(6);
    root->set_margin_end(6);
    root->set_margin_top(6);
    root->set_margin_bottom(6);

    // Top menu bar: File / Edit / Help using Gtk::MenuBar
    // nothing here; dialogs moved to public methods open_load_dialog/open_about_dialog

    // Top menu using PopoverMenuBar (Gio::Menu model)
    auto menu_model = Gio::Menu::create();

    // File submenu
    auto file_menu = Gio::Menu::create();
    file_menu->append("Load Program...", "app.load");
    menu_model->append_submenu("File", file_menu);

    // Edit submenu (placeholder)
    auto edit_menu = Gio::Menu::create();
    edit_menu->append("(no actions)", "");
    menu_model->append_submenu("Edit", edit_menu);

    // Help submenu
    auto help_menu = Gio::Menu::create();
    help_menu->append("About", "app.about");
    menu_model->append_submenu("Help", help_menu);

    auto* popbar = Gtk::manage(new Gtk::PopoverMenuBar(menu_model));

    // Application-level actions handled in main.cpp (app.add_action)

    root->append(*popbar);

    // ── Top row: seven-seg unit + LED-matrix unit ──────────────────────
    auto* top_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 6));

    auto* seg_unit = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    seg_unit->add_css_class("panel-unit");
    seg_unit->append(*build_seven_seg_bar());
    top_row->append(*seg_unit);

    auto* mat_unit = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    mat_unit->add_css_class("panel-unit");
    mat_unit->append(*build_led_matrix_panel());
    top_row->append(*mat_unit);

    root->append(*top_row);

    // ── Bottom row: main unit + RAM-LED unit + RAM-Seg unit ────────────
    auto* bot_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 6));

    auto* main_unit = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    main_unit->add_css_class("panel-unit");
    main_unit->append(*build_main_panel());
    bot_row->append(*main_unit);

    auto* ram_led_unit = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    ram_led_unit->add_css_class("panel-unit");
    ram_led_unit->append(*build_ram_led_panel());
    bot_row->append(*ram_led_unit);

    auto* ram_seg_unit = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    ram_seg_unit->add_css_class("panel-unit");
    ram_seg_unit->append(*build_ram_seg_panel());
    bot_row->append(*ram_seg_unit);

    root->append(*bot_row);

    set_child(*root);

    // Register application actions (make the GUI self-sufficient).
    auto register_actions = [this]() {
        if (auto app = get_application()) {
            app->add_action("load", [this](const Glib::VariantBase&) { open_load_dialog(); });
            app->add_action("about", [this](const Glib::VariantBase&) { open_about_dialog(); });
        }
    };
    if (get_application())
        register_actions();
    else
        signal_map().connect_once(register_actions);

    // Keyboard controller on the window
    auto key_ctrl = Gtk::EventControllerKey::create();
    key_ctrl->signal_key_pressed().connect(
        sigc::mem_fun(*this, &ComputerWindow::on_key_pressed), false);
    add_controller(key_ctrl);
}

void ComputerWindow::open_load_dialog()
{
    auto* dialog = new Gtk::FileChooserDialog(*this, "Load Program", Gtk::FileChooser::Action::OPEN);
    dialog->add_button("_Open", Gtk::ResponseType::ACCEPT);
    dialog->add_button("_Cancel", Gtk::ResponseType::CANCEL);
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Machine code files");
    filter->add_pattern("*.mc");
    filter->add_pattern("*.ass");
    dialog->add_filter(filter);
    dialog->set_transient_for(*this);

    dialog->signal_response().connect([this, dialog](int response_id) {
        if (response_id == static_cast<int>(Gtk::ResponseType::ACCEPT))
        {
            auto file = dialog->get_file();
            if (file)
            {
                std::string path = file->get_path();
                bool ok = computer_->load_program(path);
                if (!ok)
                {
                    auto* err = new Gtk::MessageDialog(*this, "Failed to load program file",
                                                       false, Gtk::MessageType::ERROR);
                    err->set_secondary_text(path);
                    err->set_transient_for(*this);
                    err->signal_response().connect([err](int) { err->hide(); delete err; });
                    err->show();
                }
                else
                {
                    computer_->prepare_run();
                    update_all_displays();
                }
            }
        }
        dialog->hide();
        delete dialog;
    });

    dialog->show();
}

void ComputerWindow::open_about_dialog()
{
    auto* dlg = new Gtk::MessageDialog(*this, "16Bit Computer", false, Gtk::MessageType::INFO);
    dlg->set_secondary_text("16BitComp GUI");
    dlg->signal_response().connect([dlg](int){ dlg->hide(); delete dlg; });
    dlg->show();
}

// ── Seven-Segment Display Bar ─────────────────────────────────────────

Gtk::Box* ComputerWindow::build_seven_seg_bar()
{
    auto* bar = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 10));
    bar->set_margin_start(6);
    bar->set_margin_end(6);
    bar->set_margin_top(4);
    bar->set_margin_bottom(4);

    // ── PM Addr ───────────────────────────────────────────────────────
    int num_pm_digits = static_cast<int>(std::ceil(pc_bits_ / 3.0));

    auto* pm_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    auto* pm_segs_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = 0; i < num_pm_digits; ++i)
    {
        auto* seg = Gtk::manage(new SevenSegDisplay());
        pm_addr_segs_.push_back(seg);
        pm_segs_row->append(*seg);
    }
    pm_box->append(*pm_segs_row);
    auto* pm_lbl = Gtk::manage(new Gtk::Label("PM Addr"));
    pm_lbl->add_css_class("section-label");
    pm_lbl->set_halign(Gtk::Align::CENTER);
    pm_box->append(*pm_lbl);
    bar->append(*pm_box);

    // ── OP Code ───────────────────────────────────────────────────────
    // Single 7-seg, same size as the others
    auto* op_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    auto* op_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));

    opcode_seg_ = Gtk::manage(new SevenSegDisplay());
    op_row->append(*opcode_seg_);

    op_box->append(*op_row);
    auto* op_lbl = Gtk::manage(new Gtk::Label("OP Code"));
    op_lbl->add_css_class("section-label");
    op_lbl->set_halign(Gtk::Align::CENTER);
    op_box->append(*op_lbl);
    bar->append(*op_box);

    // ── A ─────────────────────────────────────────────────────────────
    auto* a_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    a_seg_ = Gtk::manage(new SevenSegDisplay());
    a_box->append(*a_seg_);
    auto* a_lbl = Gtk::manage(new Gtk::Label("A"));
    a_lbl->add_css_class("section-label");
    a_lbl->set_halign(Gtk::Align::CENTER);
    a_box->append(*a_lbl);
    bar->append(*a_box);

    // ── B ─────────────────────────────────────────────────────────────
    auto* b_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    b_seg_ = Gtk::manage(new SevenSegDisplay());
    b_box->append(*b_seg_);
    auto* b_lbl = Gtk::manage(new Gtk::Label("B"));
    b_lbl->add_css_class("section-label");
    b_lbl->set_halign(Gtk::Align::CENTER);
    b_box->append(*b_lbl);
    bar->append(*b_box);

    // ── C ─────────────────────────────────────────────────────────────
    auto* c_box = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    c_seg_ = Gtk::manage(new SevenSegDisplay());
    c_box->append(*c_seg_);
    auto* c_lbl = Gtk::manage(new Gtk::Label("C"));
    c_lbl->add_css_class("section-label");
    c_lbl->set_halign(Gtk::Align::CENTER);
    c_box->append(*c_lbl);
    bar->append(*c_box);

    return bar;
}

// ── Main Panel ────────────────────────────────────────────────────────

Gtk::Box* ComputerWindow::build_main_panel()
{
    auto* panel = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 6));
    panel->set_margin_start(4);
    panel->set_margin_end(4);
    // Move the entire main section down by 50 pixels (previously 24 + 30)
    panel->set_margin_top(54);
    panel->set_margin_bottom(4);

    // Create a vertical container so we can place a centered title above
    // the main horizontal panel. Title is centered across the entire main
    // section and moved up relative to the panel by 40px (we set a small
    // bottom margin on the title to achieve that spacing).
    auto* main_vbox = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));

    title_label_ = Gtk::manage(new Gtk::Label(""));
    title_label_->set_markup(
        "<span foreground='#000000' size='small'>Computer 3 Bit v1 ISA</span>");
    title_label_->set_halign(Gtk::Align::CENTER);
    // Move title up 40px relative to the panel (panel margin_top is 54,
    // so leave a 14px gap below the title: 54 - 40 = 14)
    title_label_->set_margin_bottom(14);
    main_vbox->append(*title_label_);

    // Left column: controls
    auto* left_col = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
    left_col->set_margin_end(8);
    left_col->append(*build_control_column());
    panel->append(*left_col);

    // ── Switch / LED bank with section separators ──────────────────────
    auto* sw_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 0));

    // Helper lambda to create a thin dark separator
    auto make_sep = [&]() -> Gtk::Box*
    {
        auto* s = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
        s->add_css_class("section-sep");
        s->set_size_request(1, 100);
        s->set_vexpand(false);
        s->set_valign(Gtk::Align::CENTER);
        s->set_margin_start(4);
        s->set_margin_end(4);
        return s;
    };

    sw_row->append(*build_switch_led_column("PM Addr Sel", pc_bits_,
                                             pm_addr_switches_, pm_addr_leds_, true));
    sw_row->append(*make_sep());
    sw_row->append(*build_switch_led_column("OP Code", num_bits_,
                                             opcode_switches_, opcode_leds_, true));
    sw_row->append(*make_sep());
    sw_row->append(*build_switch_led_column("A", num_bits_,
                                             a_switches_, a_leds_, true));
    sw_row->append(*make_sep());
    sw_row->append(*build_switch_led_column("B", num_bits_,
                                             b_switches_, b_leds_, true));
    sw_row->append(*make_sep());
    sw_row->append(*build_switch_led_column("C", num_bits_,
                                             c_switches_, c_leds_, true));

    // Add a separator between the left control column and the PM selector
    panel->append(*make_sep());
    panel->append(*sw_row);

    // Append the horizontal panel into the vertical vbox and return the vbox
    main_vbox->append(*panel);
    return main_vbox;
}

// ── Control Row (knob + mode switches + button, laid out horizontally) ──

Gtk::Box* ComputerWindow::build_control_column()
{
    // Horizontal control row: [knob+button] [mode switch] [sub-mode switch]
    auto* row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 12));
    row->set_margin_start(4);
    row->set_margin_end(4);

    // Left group: knob and pulse button stacked closely
    auto* left_grp = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 6));
    auto* freq_lbl = Gtk::manage(new Gtk::Label());
    freq_lbl->set_markup("<span size='small'>Clock Speed</span>");
    freq_lbl->set_halign(Gtk::Align::CENTER);
    left_grp->append(*freq_lbl);

    knob_ = Gtk::manage(new RotaryKnob());
    knob_->set_size_request(60, 60);
    knob_->set_change_callback(sigc::mem_fun(*this, &ComputerWindow::on_knob_changed));
    left_grp->append(*knob_);

    pulse_button_ = Gtk::manage(new PushButton());
    pulse_button_->set_size_request(52, 52);
    pulse_button_->set_press_callback(sigc::mem_fun(*this, &ComputerWindow::on_pulse_pressed));
    left_grp->append(*pulse_button_);

    auto* btn_lbl = Gtk::manage(new Gtk::Label());
    btn_lbl->set_markup("<span size='x-small'>Pulse / Write</span>");
    btn_lbl->set_halign(Gtk::Align::CENTER);
    left_grp->append(*btn_lbl);

    row->append(*left_grp);

    // Middle: Program / Run switch (placed to the right of knob/button)
    auto* mode_grp = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    auto* prog_lbl = Gtk::manage(new Gtk::Label());
    prog_lbl->set_markup("<span size='x-small'>Program</span>");
    prog_lbl->set_halign(Gtk::Align::CENTER);
    mode_grp->append(*prog_lbl);

    mode_switch_ = Gtk::manage(new ToggleSwitch());
    mode_switch_->set_size_request(32, 56);
    mode_switch_->set_toggle_callback([this](bool) { on_mode_switch_toggled(); });
    mode_grp->append(*mode_switch_);

    auto* run_lbl_w = Gtk::manage(new Gtk::Label());
    run_lbl_w->set_markup("<span size='x-small'>Run</span>");
    run_lbl_w->set_halign(Gtk::Align::CENTER);
    mode_grp->append(*run_lbl_w);

    mode_grp->set_valign(Gtk::Align::CENTER);
    row->append(*mode_grp);

    // Right: Sub-mode (Auto/Read <-> Pulse/Write) to the right of mode switch
    auto* sub_grp = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    auto* rw_lbl = Gtk::manage(new Gtk::Label());
    rw_lbl->set_markup("<span size='x-small'>Auto/\nRead</span>");
    rw_lbl->set_halign(Gtk::Align::CENTER);
    sub_grp->append(*rw_lbl);

    sub_switch_ = Gtk::manage(new ToggleSwitch());
    sub_switch_->set_size_request(32, 56);
    sub_switch_->set_toggle_callback([this](bool) { on_mode_switch_toggled(); });
    sub_grp->append(*sub_switch_);

    auto* pw_lbl = Gtk::manage(new Gtk::Label());
    pw_lbl->set_markup("<span size='x-small'>Pulse/\nWrite</span>");
    pw_lbl->set_halign(Gtk::Align::CENTER);
    sub_grp->append(*pw_lbl);

    sub_grp->set_valign(Gtk::Align::CENTER);
    row->append(*sub_grp);

    return row;
}

// ── Switch/LED Column ─────────────────────────────────────────────────

Gtk::Box* ComputerWindow::build_switch_led_column(
    const std::string& label, int num_switches,
    std::vector<ToggleSwitch*>& switches,
    std::vector<LED*>& leds,
    bool msb_labels)
{
    auto* col = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    col->set_margin_start(2);
    col->set_margin_end(2);

    // Bit-weight labels row
    auto* weight_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 1));
    // Compute switch base diameter and use it to shift the whole column down.
    // Use a slightly larger fixed shift to ensure visible movement on-screen.
    // If this is still too small, tell me and I'll increase further.
    weight_row->set_margin_top(40);  // pixels (increased by 4)
    for (int i = num_switches - 1; i >= 0; --i)
    {
        auto* lbl = Gtk::manage(new Gtk::Label());
        std::string txt = std::to_string(1 << i);
        lbl->set_markup("<span size='x-small'>" + txt + "</span>");
        lbl->set_size_request(32, -1);
        lbl->set_halign(Gtk::Align::CENTER);
        weight_row->append(*lbl);
    }
    col->append(*weight_row);

    // LEDs + switches wrapped together
    auto* led_sw_grp = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 1));

    auto* led_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 1));
    for (int i = num_switches - 1; i >= 0; --i)
    {
        auto* led = Gtk::manage(new LED(1.0, 0.0, 0.0));
        leds.push_back(led);
        led->set_size_request(32, 20);
        led_row->append(*led);
    }
    std::reverse(leds.begin(), leds.end());
    led_sw_grp->append(*led_row);

    auto* sw_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 1));
    for (int i = num_switches - 1; i >= 0; --i)
    {
        auto* sw = Gtk::manage(new ToggleSwitch());
        switches.push_back(sw);
        sw->set_size_request(32, 56);
        sw->set_toggle_callback([this](bool) { on_switch_toggled(); });
        sw_row->append(*sw);
    }
    std::reverse(switches.begin(), switches.end());
    led_sw_grp->append(*sw_row);
    col->append(*led_sw_grp);

    // Section label
    auto* name_lbl = Gtk::manage(new Gtk::Label(label));
    name_lbl->add_css_class("section-label");
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

    // Bit-weight labels above LEDs (like main section)
    auto* weight_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = num_bits_ - 1; i >= 0; --i)
    {
        auto* lbl = Gtk::manage(new Gtk::Label());
        std::string txt = std::to_string(1 << i);
        lbl->set_markup("<span size='x-small'>" + txt + "</span>");
        lbl->set_size_request(32, -1);
        lbl->set_halign(Gtk::Align::CENTER);
        weight_row->append(*lbl);
    }
    panel->append(*weight_row);

    // LEDs above switches
    auto* led_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = num_bits_ - 1; i >= 0; --i)
    {
        auto* led = Gtk::manage(new LED(1.0, 0.0, 0.0));
        ram_page_leds_.push_back(led);
        led->set_size_request(32, 20);
        led_row->append(*led);
    }
    std::reverse(ram_page_leds_.begin(), ram_page_leds_.end());
    panel->append(*led_row);

    // Page select switches
    auto* page_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = num_bits_ - 1; i >= 0; --i)
    {
        auto* sw = Gtk::manage(new ToggleSwitch());
        ram_page_switches_.push_back(sw);
        sw->set_size_request(32, 48);
        sw->set_toggle_callback([this](bool) { on_ram_page_changed(); });
        page_row->append(*sw);
    }
    std::reverse(ram_page_switches_.begin(), ram_page_switches_.end());
    panel->append(*page_row);

    // "Addr" label
    auto* addr_lbl = Gtk::manage(new Gtk::Label());
    addr_lbl->set_markup("<span size='small'> Addr</span>");
    addr_lbl->set_halign(Gtk::Align::START);
    panel->append(*addr_lbl);

    // RAM LED grid: addrs_per_page_ rows × num_bits_ columns
    ram_leds_.resize(addrs_per_page_);
    for (int addr = 0; addr < addrs_per_page_; ++addr)
    {
        auto* row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 1));

        // Address label
        auto* a_lbl = Gtk::manage(new Gtk::Label("   "+std::to_string(addr)+"  "));
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
    
    // Top row: page selector switches (left) and slave/indep switch (right)
    auto* top_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 12));
    
    // Page select switches (for independent mode)
    auto* page2_row = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    for (int i = num_bits_ - 1; i >= 0; --i)
    {
        auto* sw = Gtk::manage(new ToggleSwitch());
        ram_page2_switches_.push_back(sw);
        sw->set_size_request(32, 48);
        sw->set_toggle_callback([this](bool) { on_ram_page2_changed(); });
        page2_row->append(*sw);
    }
    std::reverse(ram_page2_switches_.begin(), ram_page2_switches_.end());
    
    // Page display (seven-segment showing selected page number)
    auto* page_seg_col = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 2));
    ram_page_seg_ = Gtk::manage(new SevenSegDisplay());
    page_seg_col->append(*ram_page_seg_);
    auto* page_seg_lbl = Gtk::manage(new Gtk::Label());
    page_seg_lbl->set_markup("<span size='x-small'>Page</span>");
    page_seg_lbl->set_halign(Gtk::Align::CENTER);
    page_seg_col->append(*page_seg_lbl);

    top_row->append(*page_seg_col);

    top_row->append(*page2_row);

    // Slave/Independent switch (vertical layout with labels above/below)
    auto* slave_col = Gtk::manage(new Gtk::Box(Gtk::Orientation::VERTICAL, 0));
    auto* indep_lbl = Gtk::manage(new Gtk::Label());
    indep_lbl->set_markup("<span size='x-small'>Indep</span>");
    indep_lbl->set_halign(Gtk::Align::CENTER);
    slave_col->append(*indep_lbl);
    slave_switch_ = Gtk::manage(new ToggleSwitch());
    slave_switch_->set_size_request(32, 48);
    slave_switch_->set_toggle_callback([this](bool) { on_slave_toggled(); });
    slave_col->append(*slave_switch_);
    auto* slave_lbl = Gtk::manage(new Gtk::Label());
    slave_lbl->set_markup("<span size='x-small'>Slave</span>");
    slave_lbl->set_halign(Gtk::Align::CENTER);
    slave_col->append(*slave_lbl);
    top_row->append(*slave_col);
    
    
    
    panel->append(*top_row);
    
    // "Data" label
    auto* data_lbl = Gtk::manage(new Gtk::Label("Data"));
    data_lbl->set_halign(Gtk::Align::CENTER);
    panel->append(*data_lbl);
    
    // Seven-segment displays — arrange in 2 rows of 4
    int total_segs = addrs_per_page_;
    int half = total_segs / 2;
    
    auto* row1 = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    row1->set_halign(Gtk::Align::CENTER);
    for (int i = 0; i < half; ++i)
    {
        auto* seg = Gtk::manage(new SevenSegDisplay());
        ram_segs_.push_back(seg);
        row1->append(*seg);
    }
    panel->append(*row1);
    
    // Address labels for first row
    auto* lbl_row1 = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    lbl_row1->set_halign(Gtk::Align::CENTER);
    for (int i = 0; i < half; ++i)
    {
        auto* lbl = Gtk::manage(new Gtk::Label(std::to_string(i)));
        lbl->set_size_request(36, -1);
        lbl->set_halign(Gtk::Align::CENTER);
        lbl_row1->append(*lbl);
    }
    panel->append(*lbl_row1);
    
    auto* row2 = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    row2->set_halign(Gtk::Align::CENTER);
    for (int i = half; i < total_segs; ++i)
    {
        auto* seg = Gtk::manage(new SevenSegDisplay());
        ram_segs_.push_back(seg);
        row2->append(*seg);
    }
    panel->append(*row2);
    
    // Address labels for second row
    auto* lbl_row2 = Gtk::manage(new Gtk::Box(Gtk::Orientation::HORIZONTAL, 2));
    lbl_row2->set_halign(Gtk::Align::CENTER);
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
    // Capture the state at the time of the click
    Mode prev_mode = mode_;
    bool mode_on = mode_switch_->is_on();
    bool sub_on = sub_switch_->is_on();
    
    // Defer ALL heavy work (mode transitions, prepare_run, timers, display updates)
    // to the idle handler so the switch click returns immediately (<1ms)
    Glib::signal_idle().connect_once([this, prev_mode, mode_on, sub_on]() {
        // mode_switch_: up(off)=Program, down(on)=Run
        if (mode_on)
        {
            mode_ = Mode::RUN;
            
            // Only prepare the computer when transitioning from PROGRAM to RUN
            if (prev_mode == Mode::PROGRAM)
            {
                computer_->prepare_run();
            }
            
            // sub_switch_: up(off)=Auto, down(on)=Pulse
            if (sub_on)
            {
                run_sub_ = RunSub::PULSE;
                stop_auto_timer();
            }
            else
            {
                run_sub_ = RunSub::AUTO;
                start_auto_timer();
            }
        }
        else
        {
            mode_ = Mode::PROGRAM;
            stop_auto_timer();
            // sub_switch_: down(on)=Write, up(off)=Read
            if (sub_on)
            {
                prog_sub_ = ProgSub::WRITE;
            }
            else
            {
                prog_sub_ = ProgSub::READ;
            }
        }
        
        update_all_displays();
    });
}

// ═══════════════════════════════════════════════════════════════════════
// Switch and button handlers
// ═══════════════════════════════════════════════════════════════════════

void ComputerWindow::on_switch_toggled()
{
    // Defer display updates to idle handler to avoid blocking on switch interaction
    Glib::signal_idle().connect_once(sigc::mem_fun(*this, &ComputerWindow::update_all_displays));
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
    
    // If slave mode, update the RAM seg display (without moving the switches)
    if (slave_switch_ && slave_switch_->is_on())
    {
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
    // Just update the display; switches remain independent
    update_ram_seg_display();
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
    
    // Opcode, A, B, C — single nibble each (opcode shown as 2 digits)
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
    
    // Update the page selector LEDs to reflect switch positions
    set_leds_from_value(ram_page_leds_, page);
    
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
    
    // Update the page display seven-segment
    if (ram_page_seg_)
    {
        ram_page_seg_->set_value(static_cast<uint8_t>(page & 0x0F));
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
