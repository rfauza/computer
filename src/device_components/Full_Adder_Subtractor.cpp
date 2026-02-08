#include "Full_Adder_Subtractor.hpp"
#include <sstream>
#include <iomanip>

Full_Adder_Subtractor::Full_Adder_Subtractor()
    : xor_gate_1(2)
{
    std::ostringstream oss;
    oss << "Full_Adder_Subtractor 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    num_inputs = 4; // [A, B, Carry-In, Subtract]
    num_outputs = 2; // [Sum, Carry]
    
    allocate_IO_arrays();
    
    // Wire internal components
    // connect A input to full_adder input 0, B input to xor_gate_1 input 0
    // Subtract input to xor_gate_1 input 1, xor_gate_1 output to full_adder input 1
    // Carry-In input to full_adder input 2
    // connect output of full_adder to outputs
    xor_gate_1.connect_output(&full_adder, 0, 1); // XOR output to B input of FA
}

Full_Adder_Subtractor::~Full_Adder_Subtractor()
{
}

bool Full_Adder_Subtractor::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs: [A, B, Carry-In, Subtract]
    if (input_index == 0)
    {
        // input[0] → full_adder[0] (A)
        return full_adder.connect_input(inputs[0], 0);
    }
    else if (input_index == 1)
    {
        // input[1] → xor_gate_1[0] (B)
        return xor_gate_1.connect_input(inputs[1], 0);
    }
    else if (input_index == 2)
    {
        // input[2] → full_adder[2] (Carry-In)
        return full_adder.connect_input(inputs[2], 2);
    }
    else if (input_index == 3)
    {
        // input[3] → xor_gate_1[1] (Subtract)
        return xor_gate_1.connect_input(inputs[3], 1);
    }
    
    return true;
}

void Full_Adder_Subtractor::evaluate()
{
    // Evaluate all internal components in topological order
    xor_gate_1.evaluate();
    full_adder.evaluate();
    
    // Route outputs from full_adder: Sum -> output[0], Carry -> output[1]
    outputs[0] = full_adder.get_output(0);
    outputs[1] = full_adder.get_output(1);
}

void Full_Adder_Subtractor::update()
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

