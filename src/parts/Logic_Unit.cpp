#include "Logic_Unit.hpp"
#include <sstream>
#include <iomanip>

Logic_Unit::Logic_Unit(uint16_t num_bits) : Part(num_bits)
{
    std::ostringstream oss;
    oss << "Logic_Unit 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    // Inputs: data_a (num_bits) + data_b (num_bits) + and_enable (1) + or_enable (1) + xor_enable (1) + not_enable (1) + r_shift_enable (1) + l_shift_enable (1)
    num_inputs = (2 * num_bits) + 6;
    num_outputs = num_bits;
    
    allocate_IO_arrays();
    
    // Set up aliases
    data_a = inputs;
    data_b = &inputs[num_bits];
    and_enable = nullptr;
    or_enable = nullptr;
    xor_enable = nullptr;
    not_enable = nullptr;
    r_shift_enable = nullptr;
    l_shift_enable = nullptr;
    
    // Allocate logic gate arrays
    and_gates = new AND_Gate[num_bits];
    or_gates = new OR_Gate[num_bits];
    xor_gates = new XOR_Gate[num_bits];
    not_gates = new Inverter[num_bits];
}

Logic_Unit::~Logic_Unit()
{
    delete[] and_gates;
    delete[] or_gates;
    delete[] xor_gates;
    delete[] not_gates;
}

bool Logic_Unit::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs: [data_a (num_bits) | data_b (num_bits) | and_enable (1) | or_enable (1) | xor_enable (1) | not_enable (1) | r_shift_enable (1) | l_shift_enable (1)]
    if (input_index < num_bits)
    {
        // Data A input: route to AND/OR/XOR input 0 and NOT input 0
        bool result = true;
        result &= and_gates[input_index].connect_input(inputs[input_index], 0);
        result &= or_gates[input_index].connect_input(inputs[input_index], 0);
        result &= xor_gates[input_index].connect_input(inputs[input_index], 0);
        result &= not_gates[input_index].connect_input(inputs[input_index], 0);
        return result;
    }
    else if (input_index < 2 * num_bits)
    {
        // Data B input: route to AND/OR/XOR input 1
        uint16_t bit_index = input_index - num_bits;
        bool result = true;
        result &= and_gates[bit_index].connect_input(inputs[input_index], 1);
        result &= or_gates[bit_index].connect_input(inputs[input_index], 1);
        result &= xor_gates[bit_index].connect_input(inputs[input_index], 1);
        return result;
    }
    else if (input_index == 2 * num_bits)
    {
        // and_enable
        and_enable = inputs[input_index];
    }
    else if (input_index == 2 * num_bits + 1)
    {
        // or_enable
        or_enable = inputs[input_index];
    }
    else if (input_index == 2 * num_bits + 2)
    {
        // xor_enable
        xor_enable = inputs[input_index];
    }
    else if (input_index == 2 * num_bits + 3)
    {
        // not_enable
        not_enable = inputs[input_index];
    }
    else if (input_index == 2 * num_bits + 4)
    {
        // r_shift_enable
        r_shift_enable = inputs[input_index];
    }
    else if (input_index == 2 * num_bits + 5)
    {
        // l_shift_enable
        l_shift_enable = inputs[input_index];
    }
    
    return true;
}

void Logic_Unit::evaluate()
{
    // Determine which operation is enabled and evaluate only those gates
    if (and_enable && *and_enable)
    {
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            and_gates[i].evaluate();
            outputs[i] = and_gates[i].get_output(0);
        }
    }
    else if (or_enable && *or_enable)
    {
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            or_gates[i].evaluate();
            outputs[i] = or_gates[i].get_output(0);
        }
    }
    else if (xor_enable && *xor_enable)
    {
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            xor_gates[i].evaluate();
            outputs[i] = xor_gates[i].get_output(0);
        }
    }
    else if (not_enable && *not_enable)
    {
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            not_gates[i].evaluate();
            outputs[i] = not_gates[i].get_output(0);
        }
    }
    else if (r_shift_enable && *r_shift_enable)
    {
        // Right shift: output[0] = 0, output[i] = input[i-1]
        outputs[0] = false;
        for (uint16_t i = 1; i < num_bits; ++i)
        {
            outputs[i] = *data_a[i - 1];
        }
    }
    else if (l_shift_enable && *l_shift_enable)
    {
        // Left shift: output[i] = input[i+1], output[num_bits-1] = 0
        for (uint16_t i = 0; i < num_bits - 1; ++i)
        {
            outputs[i] = *data_a[i + 1];
        }
        outputs[num_bits - 1] = false;
    }
    else
    {
        // Default to 0 if no operation enabled
        for (uint16_t i = 0; i < num_bits; ++i)
        {
            outputs[i] = false;
        }
    }
}

void Logic_Unit::update()
{
    // Evaluate internal components
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

