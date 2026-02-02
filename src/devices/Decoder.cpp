#include "Decoder.hpp"
#include <sstream>
#include <iomanip>

Decoder::Decoder(uint16_t num_bits)
    : Device(num_bits)
{
    std::ostringstream oss;
    oss << "Decoder 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    
    if (num_inputs == 0)
        num_inputs = 1;
    
    num_outputs = static_cast<uint16_t>(1u << num_inputs); // left-shift 1 by num_inputs to get 2^n outputs
    allocate_IO_arrays();
    
    input_inverters = new Inverter[num_inputs];
    output_ands = new AND_Gate[num_outputs];
    for (uint16_t i = 0; i < num_outputs; ++i)
    {
        output_ands[i] = AND_Gate(num_inputs);
        outputs[i] = &output_ands[i].get_outputs()[0];
    }
}

Decoder::~Decoder()
{
    delete[] input_inverters;
    delete[] output_ands;
}

bool Decoder::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    if (input_index >= num_inputs)
        return false;
    
    // Wire input to its inverter
    input_inverters[input_index].connect_input(inputs[input_index], 0);
    
    // Wire this input (or its inverted version) to every output AND gate
    for (uint16_t output = 0; output < num_outputs; ++output)
    {
        bool bit_is_one = ((output >> input_index) & 0x1) != 0;
        if (bit_is_one)
        {
            output_ands[output].connect_input(inputs[input_index], input_index);
        }
        else
        {
            output_ands[output].connect_input(&input_inverters[input_index].get_outputs()[0], input_index);
        }
    }
    
    return true;
}

void Decoder::evaluate()
{
    for (uint16_t i = 0; i < num_inputs; ++i)
    {
        input_inverters[i].evaluate();
    }
    for (uint16_t i = 0; i < num_outputs; ++i)
    {
        output_ands[i].evaluate();
    }
}

void Decoder::update()
{
    evaluate();
    
    for (auto& component : downstream_components)
    {
        component->update();
    }
}
