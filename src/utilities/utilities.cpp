#include "utilities.hpp"
#include "../components/Signal_Generator.hpp"
#include "../devices/Adder_Subtractor.hpp"
#include <iostream>
#include <vector>

void test_component(Component* device, const std::string& binary_input)
{
    uint16_t num_inputs = device->get_num_inputs();
    uint16_t num_outputs = device->get_num_outputs();
    
    if (binary_input.length() != num_inputs)
    {
        std::cout << "Error: input length " << binary_input.length() 
                  << " doesn't match component inputs " << num_inputs << std::endl;
        return;
    }
    
    // Create signal generators for each input
    std::vector<Signal_Generator*> sig_gens;
    for (uint16_t i = 0; i < num_inputs; i++)
    {
        sig_gens.push_back(new Signal_Generator());
        sig_gens[i]->connect_output(device, 0, i);
    }
    
    // Set input values (LSB first, so index 0 is leftmost character)
    for (uint16_t i = 0; i < num_inputs; i++)
    {
        if (binary_input[i] == '1')
            sig_gens[i]->go_high();
        else
            sig_gens[i]->go_low();
        sig_gens[i]->update();
    }
    
    // Update device
    device->update();
    
    // Print results
    std::cout << "Input:  " << binary_input << std::endl;
    std::cout << "Output: ";
    for (uint16_t i = 0; i < num_outputs; i++)
    {
        std::cout << device->get_output(i);
    }
    std::cout << std::endl << std::endl;
    
    // Cleanup
    for (Signal_Generator* sg : sig_gens)
    {
        delete sg;
    }
}

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

void test_adder_subtractor(Adder_Subtractor& device, int start_index)
{
    int num_inputs = device.get_num_inputs();
    if (num_inputs < 4)
    {
        std::cerr << "Error: Adder_Subtractor has too few inputs:" << num_inputs << std::endl;
        return;
    }

    int num_bits = (num_inputs - 2) / 2;
    int total_combos = 1 << num_inputs;
    int mismatches = 0;

    // Create signal generators for each input
    std::vector<Signal_Generator*> signal_generators;
    for (int i = 0; i < num_inputs; ++i)
    {
        Signal_Generator* sig_gen = new Signal_Generator();
        signal_generators.push_back(sig_gen);
        sig_gen->connect_output(&device, 0, i);
    }

    for (int i = start_index; i < total_combos; ++i)
    {
        // Set signal generator states based on current combination
        for (int b = 0; b < num_inputs; ++b)
        {
            bool bit_value = (i >> b) & 1;
            if (bit_value)
                signal_generators[b]->go_high();
            else
                signal_generators[b]->go_low();
        }

        // Evaluate the device
        device.update();

        // Decode inputs (LSB first)
        int a = 0;
        int b = 0;
        for (int bit = 0; bit < num_bits; ++bit)
        {
            a |= (((i >> bit) & 1) << bit);
            b |= (((i >> (bit + num_bits)) & 1) << bit);
        }
        bool subtract = ((i >> (2 * num_bits)) & 1) != 0;
        bool output_enable = ((i >> (2 * num_bits + 1)) & 1) != 0;

        int modulus = 1 << num_bits;
        int expected = subtract ? (a - b) : (a + b);
        expected = (expected % modulus + modulus) % modulus;
        if (!output_enable)
        {
            expected = 0;
        }

        // Read outputs
        int actual = 0;
        for (int bit = 0; bit < num_bits; ++bit)
        {
            actual |= (device.get_output(bit) ? (1 << bit) : 0);
        }

        // Build binary strings (LSB first to match input order)
        std::string a_bits;
        std::string b_bits;
        std::string out_bits;
        a_bits.reserve(num_bits);
        b_bits.reserve(num_bits);
        out_bits.reserve(num_bits);
        for (int bit = 0; bit < num_bits; ++bit)
        {
            a_bits.push_back(((a >> bit) & 1) ? '1' : '0');
            b_bits.push_back(((b >> bit) & 1) ? '1' : '0');
            out_bits.push_back(((actual >> bit) & 1) ? '1' : '0');
        }

        std::cout << "input: " << a_bits << " " << b_bits << " "
                  << (subtract ? '1' : '0') << " " << (output_enable ? '1' : '0')
                  << " = " << a << (subtract ? "-" : "+") << b << std::endl;
        std::cout << "output: " << out_bits << " = " << actual;
        if (actual != expected)
        {
            std::cout << " (expected " << expected << ")";
        }
        std::cout << std::endl;

        if (actual != expected)
        {
            ++mismatches;
        }
    }

    if (mismatches == 0)
    {
        std::cout << "All Adder_Subtractor outputs match expected results." << std::endl;
    }
    else
    {
        std::cout << "Total mismatches: " << mismatches << std::endl;
    }

    for (Signal_Generator* sig_gen : signal_generators)
    {
        delete sig_gen;
    }
}



void test_all_components(uint16_t min_inputs, uint16_t max_inputs)
{
    test_component_type<AND_Gate>(min_inputs, max_inputs);
    test_component_type<OR_Gate>(min_inputs, max_inputs);
    test_component_type<NAND_Gate>(min_inputs, max_inputs);
    test_component_type<NOR_Gate>(min_inputs, max_inputs);
    test_component_type<XOR_Gate>(min_inputs, max_inputs);
    test_component_type<Buffer>(min_inputs, max_inputs);
    test_component_type<Inverter>(min_inputs, max_inputs);
}