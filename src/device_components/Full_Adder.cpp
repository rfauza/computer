#include "Full_Adder.hpp"
#include <sstream>
#include <iomanip>

Full_Adder::Full_Adder(const std::string& name) : Component(name)
{
    std::ostringstream oss;
    oss << "Full_Adder 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    num_inputs = 3; // [A, B, Carry-In]
    num_outputs = 2; // [Sum, Carry]
    
    allocate_IO_arrays();
    
    // Wire internal components
    half_adder_1.connect_output(&half_adder_2, 0, 0); // Sum output of HA1 to A input of HA2
    half_adder_1.connect_output(&or_gate_1, 1, 0); // Carry output of HA1 to OR gate input 0
    half_adder_2.connect_output(&or_gate_1, 1, 1); // Carry output of HA2 to OR gate input 1
}

Full_Adder::~Full_Adder()
{
}

bool Full_Adder::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs to half adders: [A, B, Cin] where A is LSB (index 0)
    if (input_index == 0)
    {
        // input[0] → half_adder_1[0] (A, LSB)
        return half_adder_1.connect_input(inputs[0], 0);
    }
    else if (input_index == 1)
    {
        // input[1] → half_adder_1[1] (B)
        return half_adder_1.connect_input(inputs[1], 1);
    }
    else if (input_index == 2)
    {
        // input[2] → half_adder_2[1] (Cin, MSB)
        return half_adder_2.connect_input(inputs[2], 1);
    }
    
    return true;
}

void Full_Adder::evaluate()
{
    // Evaluate all internal components in topological order
    half_adder_1.evaluate();
    half_adder_2.evaluate();
    or_gate_1.evaluate();
    
    // Route outputs: half_adder_2 Sum -> output[0], or_gate_1 -> output[1] (Carry)
    outputs[0] = half_adder_2.get_output(0);
    outputs[1] = or_gate_1.get_output(0);
}

void Full_Adder::update()
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

