#include "Adder_Subtractor.hpp"
#include <cstdlib>
#include <sstream>
#include <iomanip>
#include <iostream>

Adder_Subtractor::Adder_Subtractor(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Adder_Subtractor 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    num_inputs = (2 * num_bits) + 2;  // data_input_A (num_bits) + data_input_B (num_bits) + subtract_enable (1) + output_enable (1)
    num_outputs = num_bits;            // data_output
    
    allocate_IO_arrays();
    
    // Initialize outputs to false
    for (uint16_t i = 0; i < num_outputs; ++i)
    {
        outputs[i] = false;
    }
    
    // Set up aliases to Component's inputs and outputs
    data_input = inputs;
    data_output = outputs;
    subtract_enable = nullptr;
    output_enable = nullptr;
    
    // Allocate internal_output array (raw sum + carry, pre-output_enable)
    internal_output = new bool[num_bits + 1];
    
    // Initialize AND gates pointer
    output_AND_gates = nullptr;
    
    // Array of single-bit Full_Adder_Subtractors
    adder_subtractors = new Full_Adder_Subtractor[num_bits];    
    // Wire carry chain between FAS components
    for (uint16_t i = 1; i < num_bits; ++i)
    {
        adder_subtractors[i].connect_input(&adder_subtractors[i-1].get_outputs()[1], 2);
    }

    // Allocate AND gates
    output_AND_gates = new AND_Gate[num_bits];

    // Connect FAS sum outputs to AND gate input 0
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        output_AND_gates[i].connect_input(&adder_subtractors[i].get_outputs()[0], 0);
    }
}

Adder_Subtractor::~Adder_Subtractor()
{
    delete[] output_AND_gates;
    delete[] adder_subtractors;
    delete[] internal_output;
}

bool Adder_Subtractor::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs to Full_Adder_Subtractor components
    if (input_index < num_bits)
    {
        // A inputs: route to FAS[i] input 0
        return adder_subtractors[input_index].connect_input(inputs[input_index], 0);
    }
    else if (input_index < 2 * num_bits)
    {
        // B inputs: route to FAS[i] input 1
        uint16_t bit_index = input_index - num_bits;
        return adder_subtractors[bit_index].connect_input(inputs[input_index], 1);
    }
    else if (input_index == 2 * num_bits)
    {
        // Subtract enable: route to all FAS input 3, and first FAS input 2 (carry-in)
        subtract_enable = inputs[input_index];
        bool result = true;
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            result &= adder_subtractors[i].connect_input(subtract_enable, 3);
        }
        result &= adder_subtractors[0].connect_input(subtract_enable, 2);
        return result;
    }
    else if (input_index == 2 * num_bits + 1)
    {
        // Output enable: route to all AND gate input 1
        output_enable = inputs[input_index];
        bool result = true;
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            result &= output_AND_gates[i].connect_input(output_enable, 1);
        }
        return result;
    }
    
    return true;
}

void Adder_Subtractor::evaluate()
{
    // Evaluate all Full_Adder_Subtractor components in sequence
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        adder_subtractors[i].evaluate();
    }

    // Evaluate AND gates
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        output_AND_gates[i].evaluate();
    }
    
    // Copy raw sum to internal_output
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        internal_output[i] = adder_subtractors[i].get_outputs()[0];  // Raw sum
    }
    // Copy final carry to internal_output
    internal_output[num_bits] = adder_subtractors[num_bits - 1].get_outputs()[1];  // Final carry
    
    // Copy outputs from AND gates to device outputs
    for (uint16_t i = 0; i < num_bits; ++i)
    {
        outputs[i] = output_AND_gates[i].get_output(0);  // Gated sum output
    }
}

void Adder_Subtractor::update()
{
    evaluate();
    
    // Signal all downstream components to update
    for (Component* downstream : downstream_components)
    {
        if (downstream)
        {
            downstream->update();
        }
    }
}

