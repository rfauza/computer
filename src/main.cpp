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

int main()
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
    
    return 0;
}


