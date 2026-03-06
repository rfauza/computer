#include <gtkmm.h>
#include <iostream>
#include <memory>
#include <cstdlib>
#include "ComputerWindow.hpp"
#include "../computers/Computer_3bit_v1.hpp"

/**
 * @brief GUI front-panel application for the 3-bit computer simulator.
 *
 * Usage:
 *   ./computer_gui [program.mc]
 *
 * If a program file is given it is loaded into Program Memory before
 * the GUI opens.  Otherwise PM starts empty and can be programmed
 * through the front-panel switches.
 */
// Provide a GUI entry function but do not define `main` here to avoid
// duplicate symbol when building the combined project. The real `main`
// lives in `src/main.cpp` which will call `gui_main` when appropriate.
int gui_main(int argc, char* argv[])
{
    // Force X11 backend to avoid "Compositor doesn't support moving popups" GDK
    // warning under WSL / Wayland compositors that don't support popup relocation.
    // Only sets if not already overridden by the environment.
    setenv("GDK_BACKEND", "x11", 0);

    auto app = Gtk::Application::create("org.comp3bit.gui");

    // Parse optional program-file argument (first positional arg)
    std::string program_file;
    if (argc > 1)
    {
        program_file = argv[1];
    }

    // The computer lives for the entire process
    auto computer = std::make_unique<Computer_3bit_v1>("gui");

    if (!program_file.empty())
    {
        std::cout << "Loading program: " << program_file << std::endl;
        if (!computer->load_program(program_file))
        {
            std::cerr << "Warning: Failed to load program file: "
                      << program_file << std::endl;
        }
    }

    Computer_3bit_v1* comp_ptr = computer.get();

    app->signal_activate().connect([&app, comp_ptr]()
    {
        auto* window = new ComputerWindow(comp_ptr, 3);
        app->add_window(*window);
        window->signal_hide().connect([window]() { delete window; });
        window->set_visible(true);
    });

    return app->run(argc, argv);
}
