#include "utilities.hpp"
#include "../components/Signal_Generator.hpp"
#include <iostream>
#include <vector>

void test_truth_table(Component* component, int start_index)
{
    int num_inputs = component->get_num_inputs();
    
    // Create signal generators for each input
    std::vector<Signal_Generator*> signal_generators;
    for (int i = 0; i < num_inputs; ++i)
    {
        Signal_Generator* sig_gen = new Signal_Generator();
        signal_generators.push_back(sig_gen);
        sig_gen->connect_output(component, 0, i);
    }
    
    // Iterate through all 2^num_inputs combinations, starting from start_index
    for (int i = start_index; i < (1 << num_inputs); ++i)
    {
        std::cout << "=== Inputs ";
        
        // Print inputs in order (LSB first)
        for (int b = 0; b < num_inputs; ++b)
        {
            bool bit_value = (i >> b) & 1;
            std::cout << bit_value;
            if (b < num_inputs - 1) std::cout << ",";
        }
        std::cout << " ===" << std::endl;
        
        // Set signal generator states based on current combination
        for (int b = 0; b < num_inputs; ++b)
        {
            bool bit_value = (i >> b) & 1;
            if (bit_value)
                signal_generators[b]->go_high();
            else
                signal_generators[b]->go_low();
        }
        
        // Evaluate the component
        component->update();
        component->print_io();
        std::cout << std::endl;
    }
    
    // Clean up signal generators
    for (Signal_Generator* sig_gen : signal_generators)
    {
        delete sig_gen;
    }
}

void flip_flop_tester(Flip_Flop& device)
{
    Signal_Generator siggen1;
    Signal_Generator siggen2;
    
    siggen1.go_low();
    siggen2.go_low();
    
    siggen1.connect_output(&device, 0, 0); // Connect to Set
    siggen2.connect_output(&device, 0, 1); // Connect to Reset
    
    // Initial state
    std::cout << "Initial state (Set=0, Reset=0):" << std::endl;
    device.update();
    device.print_io();
    std::cout << std::endl; 
    
    // Set = 1, Reset = 0
    std::cout << "Set=1, Reset=0:" << std::endl;
    siggen1.go_high();
    siggen2.go_low();
    device.update();
    device.print_io();
    std::cout << std::endl; 
    
    // Set = 0, Reset = 0
    std::cout << "Set=0, Reset=0:" << std::endl;
    siggen1.go_low();
    siggen2.go_low();
    device.update();
    device.print_io();
    std::cout << std::endl;
    
    // Set = 0, Reset = 1
    std::cout << "Set=0, Reset=1:" << std::endl;
    siggen1.go_low();
    siggen2.go_high();
    device.update();
    device.print_io();
    std::cout << std::endl;
    
    // Set = 0, Reset = 0
    std::cout << "Set=0, Reset=0:" << std::endl;
    siggen1.go_low();
    siggen2.go_low();
    device.update();
    device.print_io();
    std::cout << std::endl;
    
    // Set = 1, Reset = 0
    std::cout << "Set=1, Reset=0:" << std::endl;
    siggen1.go_high();
    siggen2.go_low();
    device.update();
    device.print_io();
    std::cout << std::endl; 
    
    // Set = 0, Reset = 0
    std::cout << "Set=0, Reset=0:" << std::endl;
    siggen1.go_low();
    siggen2.go_low();
    device.update();
    device.print_io();
    std::cout << std::endl;
    
    // Set = 1, Reset = 1 (invalid state)
    std::cout << "Set=1, Reset=1 (invalid state):" << std::endl;
    siggen1.go_high();
    siggen2.go_high();
    device.update();
    device.print_io();
    std::cout << std::endl;
}

void memory_bit_tester(Memory_Bit& device)
{
    Signal_Generator data_sig;
    Signal_Generator write_sig;
    Signal_Generator read_sig;
    
    // Initialize all low
    data_sig.go_low();
    write_sig.go_low();
    read_sig.go_low();
    
    // Connect to Memory_Bit inputs: [Data, Write_Enable, Read_Enable]
    data_sig.connect_output(&device, 0, 0);
    write_sig.connect_output(&device, 0, 1);
    read_sig.connect_output(&device, 0, 2);
    
    // Test sequence: [Data, Write_Enable]
    int test_sequence[][2] = {
        {0, 0}, // Initial state
        {1, 0}, // Data=1, no write
        {0, 0}, // Data=0, no write
        {0, 1}, // Write 0
        {0, 0}, // Hold
        {1, 1}, // Write 1
        {0, 0}, // Hold
        {1, 0}, // Data=1, no write
        {0, 0}, // Data=0, no write
        {0, 1}, // Write 0
        {0, 0}, // Hold
        {1, 0}  // Read stored value
    };
    
    int num_tests = sizeof(test_sequence) / sizeof(test_sequence[0]);
    
    for (int i = 0; i < num_tests; ++i)
    {
        // Run each test twice: first with RE=0, then with RE=1
        for (int read_enable = 0; read_enable <= 1; ++read_enable)
        {
            std::cout << "=== Test " << (i + 1) << (read_enable ? "b" : "a") 
                      << ": Data=" << test_sequence[i][0] 
                      << ", WE=" << test_sequence[i][1] 
                      << ", RE=" << read_enable << " ===" << std::endl;
            
            // Set signal generators
            if (test_sequence[i][0]) data_sig.go_high(); else data_sig.go_low();
            if (test_sequence[i][1]) write_sig.go_high(); else write_sig.go_low();
            if (read_enable) read_sig.go_high(); else read_sig.go_low();
            
            device.update();
            device.print_io();
            std::cout << std::endl;
        }
    }
}