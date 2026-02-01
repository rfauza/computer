#include "Comparator.hpp"
#include <sstream>
#include <iomanip>

Comparator::Comparator(uint16_t num_bits) : Device(num_bits), 
zero_flag_nor(num_bits)
{
    std::ostringstream oss;
    oss << "Comparator 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    // Inputs: raw_sum (num_bits) + carry_out (1) + A_MSB (1) + B_MSB (1) + subtract_flag (1)
    num_inputs = num_bits + 4;
    
    // Outputs: Z, N, C, V
    num_outputs = 4;
    
    allocate_IO_arrays();
    
    // Set up aliases
    raw_sum = inputs;
    final_carry = nullptr;
    a_msb = nullptr;
    b_msb = nullptr;
    subtract_flag = nullptr;
}

Comparator::~Comparator()
{
}

bool Comparator::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs to internal class aliases:
    // [raw_sum (num_bits) | carry_out (1) | A_MSB (1) | B_MSB (1) | subtract_flag (1)]
    if (input_index < num_bits)
    {
        // raw_sum inputs -> zero_flag_nor inputs
        if (!zero_flag_nor.connect_input(inputs[input_index], input_index))
            return false;
    }
    else if (input_index == num_bits)
    {
        // final_carry
        final_carry = inputs[input_index];
    }
    else if (input_index == num_bits + 1)
    {
        // A_MSB
        a_msb = inputs[input_index];
    }
    else if (input_index == num_bits + 2)
    {
        // B_MSB
        b_msb = inputs[input_index];
    }
    else if (input_index == num_bits + 3)
    {
        // subtract_flag
        subtract_flag = inputs[input_index];
    }
    
    return true;
}

void Comparator::evaluate()
{
    // Compute Z (Zero): NOR of raw_sum
    zero_flag_nor.evaluate();
    outputs[0] = zero_flag_nor.get_output(0);  // Z flag
    
    // Compute N (Negative): MSB of raw_sum (for signed numbers)
    outputs[1] = (*raw_sum)[num_bits - 1];  // N flag
    
    // Compute C (Carry): final carry (unsigned overflow)
    outputs[2] = *final_carry;  // C flag
    
    // Compute V (Overflow): Detect signed overflow
    // (A_MSB == B_MSB (both have same sign)) && (A_MSB != Sum_MSB (result has different sign than A and B))
    if (a_msb && b_msb)
    {
        bool a_sign = *a_msb;
        bool b_sign = *subtract_flag ? !(*b_msb) : *b_msb;  // Invert B if subtract
        bool sum_msb = (*raw_sum)[num_bits - 1];
        outputs[3] = (a_sign == b_sign) && (a_sign != sum_msb);  // V flag
    }
    else
    {
        outputs[3] = false;  // Default to no overflow if inputs not connected
    }
}

void Comparator::update()
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
