#include "Adder.hpp"
#include <sstream>
#include <iomanip>

Adder::Adder(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Adder 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    num_inputs = 2 * num_bits;  // A (num_bits) + B (num_bits)
    num_outputs = num_bits;      // Sum (num_bits)
    
    allocate_IO_arrays();
    
    // Allocate Full_Adder array
    adders = new Full_Adder[num_bits];
    
    // Create signal generator for carry-in (0)
    carry_in_signal = new Signal_Generator(1);
    carry_in_signal->go_low();
    
    // Connect carry_in to first adder
    adders[0].connect_input(&carry_in_signal->get_outputs()[0], 2);
    
    // Wire carry chain between adders
    for (uint16_t i = 1; i < num_bits; ++i)
    {
        adders[i].connect_input(&adders[i-1].get_outputs()[1], 2);  // Carry out -> Carry in
    }
}

Adder::~Adder()
{
    delete[] adders;
    delete carry_in_signal;
}

bool Adder::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route A inputs (0 to num_bits-1)
    if (input_index < num_bits)
    {
        return adders[input_index].connect_input(inputs[input_index], 0);
    }
    // Route B inputs (num_bits to 2*num_bits-1)
    else if (input_index < 2 * num_bits)
    {
        uint16_t bit_index = input_index - num_bits;
        return adders[bit_index].connect_input(inputs[input_index], 1);
    }
    
    return true;
}

void Adder::update()
{
    // Evaluate all adders in sequence
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        adders[i].evaluate();
    }
    
    // Copy sum outputs
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        outputs[i] = adders[i].get_output(0);
    }
    
    // Signal all downstream components to update
    for (Component* downstream : downstream_components)
    {
        if (downstream)
        {
            downstream->update();
        }
    }
}
