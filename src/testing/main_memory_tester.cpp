#include "main_memory_tester.hpp"
#include "../parts/Main_Memory.hpp"
#include "../components/Signal_Generator.hpp"
#include <iostream>
#include <sstream>
#include <vector>

void main_memory_tester(Main_Memory& mm, const std::string& input_str)
{
    uint16_t address_bits = mm.get_address_bits();
    uint16_t data_bits = mm.get_data_bits();
    uint16_t num_inputs = address_bits + data_bits + 2;
    
    // Parse input string: "addr data WE RE"
    std::istringstream iss(input_str);
    int addr, data, we, re;
    
    if (!(iss >> addr >> data >> we >> re))
    {
        std::cout << "Error: Invalid input format. Expected: addr data WE RE\n";
        return;
    }
    
    // Static signal generators (persists across calls within the same Main_Memory)
    static std::vector<Signal_Generator> sig_gens;
    
    // Create and connect signal generators on first call
    if (sig_gens.empty())
    {
        sig_gens.resize(num_inputs);
        for (uint16_t i = 0; i < num_inputs; ++i)
        {
            sig_gens[i].connect_output(&mm, 0, i);
        }
    }
    
    // Set address bits
    for (uint16_t i = 0; i < address_bits; ++i)
    {
        if ((addr >> i) & 1)
            sig_gens[i].go_high();
        else
            sig_gens[i].go_low();
    }
    
    // Set data bits
    for (uint16_t bit = 0; bit < data_bits; ++bit)
    {
        uint16_t idx = address_bits + bit;
        if ((data >> bit) & 1)
            sig_gens[idx].go_high();
        else
            sig_gens[idx].go_low();
    }
    
    // Set enables
    if (we)
        sig_gens[address_bits + data_bits].go_high();
    else
        sig_gens[address_bits + data_bits].go_low();
    
    if (re)
        sig_gens[address_bits + data_bits + 1].go_high();
    else
        sig_gens[address_bits + data_bits + 1].go_low();
    
    // Update all signal generators
    for (auto& sg : sig_gens)
    {
        sg.update();
    }
    
    // Evaluate and update MM
    mm.evaluate();
    mm.update();
    
    std::cout << "Input:  addr=" << addr << " data=" << data 
              << " WE=" << we << " RE=" << re << "\n";
    
    // Print output
    std::cout << "Output: ";
    for (uint16_t i = 0; i < data_bits; ++i)
        std::cout << mm.get_output(i);
    
    std::cout << "\n\n";
}
