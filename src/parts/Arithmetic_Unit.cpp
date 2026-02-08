#include "Arithmetic_Unit.hpp"
#include <sstream>
#include <iomanip>

Arithmetic_Unit::Arithmetic_Unit(uint16_t num_bits) 
    : Part(num_bits), 
      adder_subtractor(num_bits),
      multiplier(num_bits),
      divider(num_bits)
{
    // create component name string
    std::ostringstream oss;
    oss << "Arithmetic_Unit 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    // Inputs: data_a (num_bits) + data_b (num_bits) + add_enable (1) + sub_enable (1) + 
    // increment_enable (1) + decrement_enable (1) + mul_enable (1) + div_enable (1)
    num_inputs = (2 * num_bits) + 6;
    num_outputs = num_bits;
    
    allocate_IO_arrays();
    
    // Set up aliases
    data_a = inputs;
    data_b = &inputs[num_bits];
    add_enable = nullptr;
    sub_enable = nullptr;
    inc_enable = nullptr;
    dec_enable = nullptr;
    mul_enable = nullptr;
    div_enable = nullptr;
}

Arithmetic_Unit::~Arithmetic_Unit() = default;

bool Arithmetic_Unit::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs: [data_a (num_bits), data_b (num_bits),  add_enable (1), sub_enable (1), 
    // inc_enable (1), dec_enable (1), mul_enable (1), div_enable (1)]
    if (input_index < num_bits)
    {
        // Data A input: route to all arithmetic devices
        bool result = true;
        result &= adder_subtractor.connect_input(inputs[input_index], input_index);
        result &= multiplier.connect_input(inputs[input_index], input_index);
        result &= divider.connect_input(inputs[input_index], input_index);
        return result;
    }
    else if (input_index < 2 * num_bits)
    {
        // Data B input: route to all arithmetic devices
        bool result = true;
        result &= adder_subtractor.connect_input(inputs[input_index], input_index);
        result &= multiplier.connect_input(inputs[input_index], input_index);
        result &= divider.connect_input(inputs[input_index], input_index);
        return result;
    }
    else if (input_index == 2 * num_bits)
    {
        // add_enable - store locally and connect to adder_subtractor's output_enable
        add_enable = inputs[input_index];
        adder_subtractor.connect_input(inputs[input_index], 2 * num_bits + 1);
    }
    else if (input_index == 2 * num_bits + 1)
    {
        // sub_enable - connect to adder_subtractor's subtract_enable and output_enable
        sub_enable = inputs[input_index];
        adder_subtractor.connect_input(inputs[input_index], 2 * num_bits);
        adder_subtractor.connect_input(inputs[input_index], 2 * num_bits + 1);
    }
    else if (input_index == 2 * num_bits + 2)
    {
        // inc_enable - connect to adder_subtractor's output_enable
        inc_enable = inputs[input_index];
        adder_subtractor.connect_input(inputs[input_index], 2 * num_bits + 1);
    }
    else if (input_index == 2 * num_bits + 3)
    {
        // dec_enable - connect to adder_subtractor's subtract_enable and output_enable
        dec_enable = inputs[input_index];
        adder_subtractor.connect_input(inputs[input_index], 2 * num_bits);
        adder_subtractor.connect_input(inputs[input_index], 2 * num_bits + 1);
    }
    else if (input_index == 2 * num_bits + 4)
    {
        // mul_enable - store locally and connect to multiplier's output_enable
        mul_enable = inputs[input_index];
        multiplier.connect_input(inputs[input_index], 2 * num_bits + 1);
    }
    else if (input_index == 2 * num_bits + 5)
    {
        // div_enable - store locally and connect to divider's output_enable
        div_enable = inputs[input_index];
        divider.connect_input(inputs[input_index], 2 * num_bits + 1);
    }
    
    return true;
}

void Arithmetic_Unit::evaluate()
{
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
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = multiplier.get_output(i);
        }
    }
    else if (div_enable && *div_enable)
    {
        divider.evaluate();
        // Get quotient (first num_bits outputs)
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = divider.get_output(i);
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
