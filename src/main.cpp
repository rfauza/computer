#include <iostream>
#include "components/AND_Gate.hpp"
#include "components/OR_Gate.hpp"
#include "components/Inverter.hpp"
#include "components/Buffer.hpp"
#include "components/Signal_Generator.hpp"
#include "components/NAND_Gate.hpp"
#include "components/NOR_Gate.hpp"
#include "components/XOR_Gate.hpp"
#include "device_components/Half_Adder.hpp"
#include "device_components/Full_Adder.hpp"
#include "device_components/Full_Adder_Subtractor.hpp"
#include "device_components/Memory_Bit.hpp"
#include "device_components/Flip_Flop.hpp"
#include "devices/Bus.hpp"
#include "devices/Register.hpp"
#include "devices/Adder.hpp"
#include "devices/Adder_Subtractor.hpp"
#include "devices/Decoder.hpp"
#include "parts/Program_Memory.hpp"
#include "testing/component_tests.hpp"
#include "testing/program_memory_tester.hpp"
#include "testing/arithmetic_unit_tests.hpp"
#include "testing/alu_tests.hpp"
#include "utilities/program_memory_loader.hpp"
#include "parts/Program_Memory.hpp"
#include "parts/Main_Memory.hpp"
#include "utilities/main_memory_loader.hpp"
#include "testing/main_memory_tester.hpp"
#include <filesystem>

// Forward declaration of testMainMemory
void testMainMemory();

int main()
{
    // Test Arithmetic Unit (4-bit)
    test_arithmetic_unit_truth_table();
    
    // Test ALU (4-bit, print only failures)
    test_alu_truth_table(4, true);
    
    // Uncomment to test ALU with all tests printed:
    // test_alu_truth_table(4, true);
    
    return 0;
}

bool loadPM()
{
    // Get the absolute path of the executable's directory
    std::string exe_path = std::filesystem::current_path().string();
    // Debug: Print the current working directory
    std::cout << "Current working directory: " << std::filesystem::current_path() << "\n";

    // Adjust the path to point to the project root directory from the build folder
    std::string file_path = "../program1.txt";
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