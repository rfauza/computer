// Replace CLI runner with GUI launcher for the 3-bit computer
// This main creates a Computer_3bit_v1, optionally loads a .mc program
// passed as the first positional argument, and shows the ComputerWindow.

#include <gtkmm.h>
#include <iostream>
#include <memory>
#include <filesystem>
#include "gui/ComputerWindow.hpp"
#include "computers/Computer_3bit_v1.hpp"

// Utilities used by non-GUI runners/tests
#include "utilities/assembler.hpp"
#include "utilities/evaluator.hpp"
#include "utilities/program_memory_loader.hpp"
#include "utilities/main_memory_loader.hpp"
#include "testing/program_memory_tester.hpp"
#include "testing/main_memory_tester.hpp"


int run_with_no_gui();
int run_gui();

int main()
{
    // Assembler ass;
    // ass.assemble("../programs/3bit_v1/running_LEDs.ass", "../programs/3bit_v1/running_LEDs.mc");
    
    return run_gui();
}

int run_with_no_gui()
{
    // Assemble the provided .ass file and then run the Evaluator on the
    // generated machine-code file. This allows automated verification.
    // Paths are relative to the build directory when the executable runs there.
    const std::string asm_path = "../programs/3bit_v1/long_mult.ass";
    const std::string out_mc   = "../programs/3bit_v1/long_mult.mc";
    
    Assembler assembler;
    Evaluator eval;

    assembler.assemble(asm_path, out_mc);
    bool pass = eval.evaluate(out_mc, true);
    std::cout << "Evaluator result: " << (pass ? "PASS" : "FAIL") << "\n";

    return pass ? 0 : 2;
}

int run_gui()
{
    auto app = Gtk::Application::create("org.comp3bit.gui");

    auto computer = std::make_unique<Computer_3bit_v1>("gui_main");

    Computer_3bit_v1* comp_ptr = computer.get();

    app->signal_activate().connect([&app, comp_ptr]()
    {
        auto* window = new ComputerWindow(comp_ptr, 3);
        app->add_window(*window);
        window->signal_hide().connect([window]() { delete window; });
        window->set_visible(true);
    });

    return app->run();
}

bool loadPM()
{
    // Get the absolute path of the executable's directory
    std::string exe_path = std::filesystem::current_path().string();
    // Debug: Print the current working directory
    std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";

    // Adjust the path to point to the project root directory from the build folder
    std::string file_path = "../programs/3bit_v1/program1.txt";
    if (!std::filesystem::exists(file_path)) {
        std::cerr << "Error: File not found at " << file_path << "\n";
        return false;
    }
    file_path = std::filesystem::absolute(file_path).string();

    Program_Memory pm(8, 4);  // 8-bit address, 4-bit data
    bool success = load_and_verify_program_memory(pm, file_path);
    std::cout << "Sucess: " << (success ? "Yes" : "No") << "\n";
    return success;
}


void testPM()
{
    // Create a Program Memory with defaults (12-bit address, 4-bit data)
    Program_Memory pm;
    
    std::cout << "=== Program Memory Test ===\n\n";
    
    // Write random data to 2 random addresses
    program_memory_tester(pm, "5 7 3 2 1 1 0");
    program_memory_tester(pm, "10 14 11 9 6 1 0");
    
    // Read from those addresses and a third with other data on the bus
    std::cout << "--- Reading back ---\n";
    program_memory_tester(pm, "5 5 5 5 5 0 1");
    program_memory_tester(pm, "10 8 8 8 8 0 1");
    program_memory_tester(pm, "20 15 15 15 15 0 1");
}

void testMainMemory() {
    std::cout << "=== Main Memory Test ===\n\n";
    
    // Create a Main Memory with 8-bit addresses (256 locations) and 4-bit data
    Main_Memory mm(8, 4);
    
    std::cout << "Created Main Memory with:\n";
    std::cout << "  Address bits: " << mm.get_address_bits() << " (" << (1 << mm.get_address_bits()) << " addresses)\n";
    std::cout << "  Data bits: " << mm.get_data_bits() << "\n\n";
    
    // Test 1: Manual testing
    std::cout << "--- Manual Test ---\n";
    std::cout << "Writing value 10 to address 10...\n";
    main_memory_tester(mm, "10 10 1 0");
    
    std::cout << "Reading from address 10...\n";
    main_memory_tester(mm, "10 0 0 1");
    
    std::cout << "Writing value 15 to address 0...\n";
    main_memory_tester(mm, "0 15 1 0");
    
    std::cout << "Reading from address 0...\n";
    main_memory_tester(mm, "0 0 0 1");
    
    // Test 2: Load and verify from file
    std::cout << "\n--- File Load Test ---\n";
    std::string test_file = std::filesystem::absolute("../src/main_memory_test_data.txt").string();
    
    if (load_and_verify_main_memory(mm, test_file)) {
        std::cout << "\nFile load and verification successful!\n";
    } else {
        std::cout << "\nFile load failed!\n";
        return;
    }
    
    // Test 3: Verify some of the loaded values
    std::cout << "\n--- Spot Check Loaded Values ---\n";
    std::cout << "Reading address 0 (should be 0001 = 1):\n";
    main_memory_tester(mm, "0 0 0 1");
    
    std::cout << "Reading address 5 (should be 1111 = 15):\n";
    main_memory_tester(mm, "5 0 0 1");
    
    std::cout << "Reading address 10 (should be 1010 = 10):\n";
    main_memory_tester(mm, "10 0 0 1");
    
    std::cout << "Reading address 255 (should be 1001 = 9):\n";
    main_memory_tester(mm, "255 0 0 1");
    
    std::cout << "\n=== All Tests Complete ===\n";
}