#include "program_memory_tester.hpp"
#include "../parts/Program_Memory.hpp"
#include "../components/Signal_Generator.hpp"
#include <iostream>
#include <sstream>
#include <vector>

void program_memory_tester(Program_Memory& pm, const std::string& input_str)
{
    uint16_t decoder_bits = pm.get_decoder_bits();
    uint16_t data_bits = pm.get_data_bits();
    uint16_t num_inputs = decoder_bits + 4 * data_bits + 2;
    
    // Parse input string: "addr opcode C A B WE RE"
    std::istringstream iss(input_str);
    int addr, opcode, c, a, b, we, re;
    
    if (!(iss >> addr >> opcode >> c >> a >> b >> we >> re))
    {
        std::cout << "Error: Invalid input format. Expected: addr opcode C A B WE RE\n";
        return;
    }
    
    // Static signal generators (persists across calls)
    static std::vector<Signal_Generator> sig_gens;
    
    // Create and connect signal generators on first call
    if (sig_gens.empty())
    {
        sig_gens.resize(num_inputs);
        for (uint16_t i = 0; i < num_inputs; ++i)
        {
            sig_gens[i].connect_output(&pm, 0, i);
        }
    }
    
    // Set address bits
    for (uint16_t i = 0; i < decoder_bits; ++i)
    {
        if ((addr >> i) & 1)
            sig_gens[i].go_high();
        else
            sig_gens[i].go_low();
    }
    
    // Set data bits (opcode, C, A, B)
    int data_values[4] = {opcode, c, a, b};
    for (uint16_t reg = 0; reg < 4; ++reg)
    {
        for (uint16_t bit = 0; bit < data_bits; ++bit)
        {
            uint16_t idx = decoder_bits + reg * data_bits + bit;
            if ((data_values[reg] >> bit) & 1)
                sig_gens[idx].go_high();
            else
                sig_gens[idx].go_low();
        }
    }
    
    // Set enables
    if (we)
        sig_gens[decoder_bits + 4 * data_bits].go_high();
    else
        sig_gens[decoder_bits + 4 * data_bits].go_low();
    
    if (re)
        sig_gens[decoder_bits + 4 * data_bits + 1].go_high();
    else
        sig_gens[decoder_bits + 4 * data_bits + 1].go_low();
    
    // Update all signal generators
    for (auto& sg : sig_gens)
    {
        sg.update();
    }
    
    // Evaluate and update PM
    pm.evaluate();
    pm.update();
    
    std::cout << "Input:  addr=" << addr << " opcode=" << opcode << " C=" << c 
              << " A=" << a << " B=" << b << " WE=" << we << " RE=" << re << "\n";
    
    // Print labeled outputs
    std::cout << "Output: OP:";
    for (uint16_t i = 0; i < data_bits; ++i)
        std::cout << pm.get_output(i);
    
    std::cout << " C:";
    for (uint16_t i = 0; i < data_bits; ++i)
        std::cout << pm.get_output(data_bits + i);
    
    std::cout << " A:";
    for (uint16_t i = 0; i < data_bits; ++i)
        std::cout << pm.get_output(2 * data_bits + i);
    
    std::cout << " B:";
    for (uint16_t i = 0; i < data_bits; ++i)
        std::cout << pm.get_output(3 * data_bits + i);
    
    std::cout << "\n\n";
}
