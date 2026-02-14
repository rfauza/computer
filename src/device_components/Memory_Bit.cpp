#include "Memory_Bit.hpp"
#include <sstream>
#include <iomanip>

Memory_Bit::Memory_Bit(const std::string& name)
    : Component(name),
    data_inverter(1, name.empty() ? std::string("data_inverter_in_memory_bit") : name + "_data_inverter"),
      set_and(2, name.empty() ? std::string("set_and_in_memory_bit") : name + "_set_and"),
      reset_and(2, name.empty() ? std::string("reset_and_in_memory_bit") : name + "_reset_and"),
      output_and(2, name.empty() ? std::string("output_and_in_memory_bit") : name + "_output_and"),
      flip_flop(name.empty() ? std::string("flip_flop_in_memory_bit") : name + "_flip_flop")
{
    std::ostringstream oss;
    oss << "Memory_Bit 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    if (!name.empty())
    {
        oss << " - " << name;
    }
    component_name = oss.str();
    num_inputs = 3;  // [Data, Write_Enable, Read_Enable]
    num_outputs = 1; // [Q (output)]
    
    allocate_IO_arrays();
    
    // Wire internal components
    // Data → set_and[0], Write_Enable → set_and[1]
    // Data → data_inverter[0], inverted data → reset_and[0], Write_Enable → reset_and[1]
    // set_and output → flip_flop Set, reset_and output → flip_flop Reset
    // flip_flop Q → output_and[0], Read_Enable → output_and[1]
    data_inverter.connect_output(&reset_and, 0, 0);
    set_and.connect_output(&flip_flop, 0, 0);
    reset_and.connect_output(&flip_flop, 0, 1);
    flip_flop.connect_output(&output_and, 0, 0);
}

Memory_Bit::~Memory_Bit()
{
}

bool Memory_Bit::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Route inputs to internal components
    if (input_index == 0)
    {
        // input[0] (Data) → data_inverter and set_and
        data_inverter.connect_input(inputs[0], 0);
        return set_and.connect_input(inputs[0], 0);
    }
    else if (input_index == 1)
    {
        // input[1] (Write_Enable) → set_and[1] and reset_and[1]
        set_and.connect_input(inputs[1], 1);
        return reset_and.connect_input(inputs[1], 1);
    }
    else if (input_index == 2)
    {
        // input[2] (Read_Enable) → output_and[1]
        return output_and.connect_input(inputs[2], 1);
    }
    
    return true;
}

void Memory_Bit::evaluate()
{
    // Evaluate in order: data_inverter → AND gates → flip_flop → output_and
    data_inverter.evaluate();
    set_and.evaluate();
    reset_and.evaluate();
    flip_flop.evaluate();
    output_and.evaluate();
    
    // Output is from output_and
    outputs[0] = output_and.get_output(0);
}

void Memory_Bit::update()
{
    // Phase 2: update internal components so flip-flop and gating latch
    data_inverter.update();
    set_and.update();
    reset_and.update();
    flip_flop.update();
    output_and.update();

    // Refresh output from output_and after update
    outputs[0] = output_and.get_output(0);

    // Do not propagate updates here; parent Register or system will call downstream updates.
}
