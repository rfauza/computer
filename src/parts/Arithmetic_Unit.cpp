#include "Arithmetic_Unit.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

Arithmetic_Unit::Arithmetic_Unit(uint16_t num_bits) 
: Part(num_bits), 
    adder_subtractor(num_bits),
    multiplier(num_bits),
    adder_output_enable_or(nullptr),
    adder_subtract_enable_or(nullptr)
{
    // create component name string
    std::ostringstream oss;
    oss << "Arithmetic_Unit 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    // Inputs: data_a (num_bits) + data_b (num_bits) + add_enable (1) + sub_enable (1) + 
    // increment_enable (1) + decrement_enable (1) + mul_enable (1)
    num_inputs = (2 * num_bits) + 5;
    num_outputs = num_bits;
    
    allocate_IO_arrays();
    
    // Create the OR gates on the heap
    adder_output_enable_or = new OR_Gate(4);
    adder_subtract_enable_or = new OR_Gate(2);
    
    // Wire OR output to adder_subtractor's output_enable input
    adder_output_enable_or->connect_output(&adder_subtractor, 0, 2 * num_bits + 1);
    
    // Wire subtract OR output to adder_subtractor's subtract_enable input
    adder_subtract_enable_or->connect_output(&adder_subtractor, 0, 2 * num_bits);
    
    // Set up aliases
    data_a = inputs;
    data_b = &inputs[num_bits];
    add_enable = nullptr;
    sub_enable = nullptr;
    inc_enable = nullptr;
    dec_enable = nullptr;
    mul_enable = nullptr;
}

Arithmetic_Unit::~Arithmetic_Unit()
{
    delete adder_output_enable_or;
    delete adder_subtract_enable_or;
}

bool Arithmetic_Unit::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs: [data_a (num_bits), data_b (num_bits),  add_enable (1), sub_enable (1), 
    // inc_enable (1), dec_enable (1), mul_enable (1)]
    if (input_index < num_bits)
    {
        // Data A input: route to all arithmetic devices
        bool result = true;
        result &= adder_subtractor.connect_input(inputs[input_index], input_index);
        result &= multiplier.connect_input(inputs[input_index], input_index);
        return result;
    }
    else if (input_index < 2 * num_bits)
    {
        // Data B input: route to all arithmetic devices
        bool result = true;
        result &= adder_subtractor.connect_input(inputs[input_index], input_index);
        result &= multiplier.connect_input(inputs[input_index], input_index);
        return result;
    }
    else if (input_index == 2 * num_bits)
    {
        // add_enable - route to OR gate input 0
        add_enable = inputs[input_index];
        adder_output_enable_or->connect_input(inputs[input_index], 0);
    }
    else if (input_index == 2 * num_bits + 1)
    {
        // sub_enable - route to output OR gate input 1, and subtract OR gate input 0
        sub_enable = inputs[input_index];
        adder_output_enable_or->connect_input(inputs[input_index], 1);
        adder_subtract_enable_or->connect_input(inputs[input_index], 0);
    }
    else if (input_index == 2 * num_bits + 2)
    {
        // inc_enable - route to OR gate input 2
        inc_enable = inputs[input_index];
        adder_output_enable_or->connect_input(inputs[input_index], 2);
    }
    else if (input_index == 2 * num_bits + 3)
    {
        // dec_enable - route to output OR gate input 3, and subtract OR gate input 1
        dec_enable = inputs[input_index];
        adder_output_enable_or->connect_input(inputs[input_index], 3);
        adder_subtract_enable_or->connect_input(inputs[input_index], 1);
    }
    else if (input_index == 2 * num_bits + 4)
    {
        // mul_enable - store locally and connect to multiplier's output_enable
        mul_enable = inputs[input_index];
        multiplier.connect_input(inputs[input_index], 2 * num_bits);
    }
    
    return true;
}

void Arithmetic_Unit::evaluate()
{
    // Evaluate the operation enable OR gates
    adder_output_enable_or->evaluate();
    adder_subtract_enable_or->evaluate();
    
    // Determine which operation is enabled and evaluate only that device
    if (add_enable && *add_enable)
    {
        adder_subtractor.evaluate();
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = adder_subtractor.get_output(i);
        }
    }
    else if (sub_enable && *sub_enable)
    {
        adder_subtractor.evaluate();
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = adder_subtractor.get_output(i);
        }
    }
    else if (mul_enable && *mul_enable)
    {
        multiplier.evaluate();
        // Multiplier outputs 2*num_bits, but we gate to num_bits output ports
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = multiplier.get_output(i);
        }
    }
    else
    {
        // No operation enabled, set output to zero
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = false;
        }
    }
}

void Arithmetic_Unit::update()
{
    evaluate();
    
    // Propagate to downstream components
    for (Component* downstream : downstream_components)
    {
        if (downstream)
        {
            downstream->update();
        }
    }
}

void Arithmetic_Unit::print_adder_inputs() const
{
    std::cout << "Adder_Subtractor inputs: ";
    adder_subtractor.print_io();
}

void Arithmetic_Unit::print_multiplier_io() const
{
    std::cout << "Multiplier IO: ";
    multiplier.print_io();
}
