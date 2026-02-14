#include "computers/Computer_3bit.hpp"
#include <iostream>

// Note: this file intentionally does NOT define `main` to avoid
// multiple-definition link errors. Call `run_3bit_test` from
// your existing `main` or build a separate binary that calls it.

int run_3bit_test(int argc, char* argv[])
{
    std::cout << "=== 3-Bit Computer Test ===" << std::endl;

    // Allocate Computer_3bit on heap to avoid stack overflow
    // (has large vector members: 9 + 12 + 3 Signal_Generators = ~24 objects)
    Computer_3bit* computer = new Computer_3bit("test_computer");

    // Determine program file
    std::string program_file = "test_program.txt";
    if (argc > 1)
    {
        program_file = argv[1];
    }

    // Load program
    if (!computer->load_program(program_file))
    {
        std::cerr << "Failed to load program" << std::endl;
        delete computer;
        return 1;
    }

    // Run interactively
    computer->run_interactive();

    delete computer;
    return 0;
}
