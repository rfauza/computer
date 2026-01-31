#include "Half_Adder.hpp"
#include <sstream>
#include <iomanip>

Half_Adder::Half_Adder()
{
    std::ostringstream oss;
    oss << "Half_Adder 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    num_inputs = 2;
    num_outputs = 2; // [0] is Sum, [1] is Carry
    
    allocate_IO_arrays();
    
    // Wire internal gates
    nand_gate1.connect_output(&nand_gate2, 0, 1);
    nand_gate1.connect_output(&nand_gate3, 0, 0);
    nand_gate1.connect_output(&inverter1, 0, 0);
    nand_gate2.connect_output(&nand_gate4, 0, 1);
    nand_gate3.connect_output(&nand_gate4, 0, 0);
}

Half_Adder::~Half_Adder()
{
}

bool Half_Adder::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs to NAND gates
    if (input_index == 0)
    {
        // input[0] -> nand1[0] and nand2[0]
        nand_gate1.connect_input(inputs[0], 0);
        nand_gate2.connect_input(inputs[0], 0);
    }
    else if (input_index == 1)
    {
        // input[1] -> nand1[1] and nand3[1]
        nand_gate1.connect_input(inputs[1], 1);
        nand_gate3.connect_input(inputs[1], 1);
    }
    
    return true;
}

void Half_Adder::evaluate()
{
    // Evaluate all internal components in topological order
    nand_gate1.evaluate();
    nand_gate2.evaluate();
    nand_gate3.evaluate();
    nand_gate4.evaluate();
    inverter1.evaluate();
    
    // Route outputs: gate4 -> output[0] (Sum), inverter -> output[1] (Carry)
    outputs[0] = nand_gate4.get_output(0);
    outputs[1] = inverter1.get_output(0);
}

void Half_Adder::update()
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

