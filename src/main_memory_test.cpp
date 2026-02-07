#include <iostream>
#include "parts/Main_Memory.hpp"
#include "utilities/main_memory_loader.hpp"
#include "testing/main_memory_tester.hpp"

void test_main_memory()
{
    std::cout << "=== Main Memory Test ===\n\n";
    
    // Create a Main Memory with 8-bit addresses (256 locations) and 4-bit data
    Main_Memory mm(8,4);  // Uses defaults: 8-bit address, 4-bit data
    
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
    std::string test_file = "src/main_memory_test_data.txt";
    
    if (load_and_verify_main_memory(mm, test_file))
    {
        std::cout << "\nFile load and verification successful!\n";
    }
    else
    {
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
