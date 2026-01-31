#include "Register.hpp"
#include <sstream>
#include <iomanip>

Register::Register(uint16_t num_bits) : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Register 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    // Set up I/O: num_bits data inputs + 2 control inputs, num_bits outputs
    num_inputs = num_bits + 2;  // [data[0..num_bits-1], write_enable, read_enable]
    num_outputs = num_bits;
    
    allocate_IO_arrays();
    
    // Create Memory_Bit for each bit position
    for (uint16_t i = 0; i < num_bits; i++)
    {
        memory_bits.push_back(new Memory_Bit());
    }
}

Register::~Register()
{
    for (Memory_Bit* bit : memory_bits)
    {
        delete bit;
    }
    memory_bits.clear();
}

bool Register::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    if (input_index < num_bits)
    {
        // input[0..num_bits-1]: data inputs to respective memory bits
        return memory_bits[input_index]->connect_input(inputs[input_index], 0);
    }
    else if (input_index == num_bits)
    {
        // input[num_bits]: write_enable to all memory bits
        for (Memory_Bit* bit : memory_bits)
        {
            bit->connect_input(inputs[num_bits], 1);
        }
        return true;
    }
    else if (input_index == num_bits + 1)
    {
        // input[num_bits+1]: read_enable to all memory bits
        for (Memory_Bit* bit : memory_bits)
        {
            bit->connect_input(inputs[num_bits + 1], 2);
        }
        return true;
    }
    
    return false;
}

void Register::evaluate()
{
    // Evaluate all memory bits and gather outputs
    for (uint16_t i = 0; i < memory_bits.size(); i++)
    {
        memory_bits[i]->evaluate();
        outputs[i] = memory_bits[i]->get_output(0);
    }
}

void Register::update()
{
    // Evaluate this component
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

