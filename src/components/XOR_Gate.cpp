#include "XOR_Gate.hpp"
#include <sstream>
#include <iomanip>

XOR_Gate::XOR_Gate(uint16_t num_bits_param)
    : buffer_A(num_bits_param), 
      buffer_B(num_bits_param),
      and_gate1(num_bits_param),
      and_gate2(num_bits_param),
      output_or_gate(num_bits_param),
      input_inverter_A(num_bits_param),
      input_inverter_B(num_bits_param)
{
    num_bits = num_bits_param;
    std::ostringstream oss;
    oss << "XOR_Gate 0x" << std::hex << reinterpret_cast<uintptr_t>(this);
    component_name = oss.str();
    num_inputs = 2 * num_bits;
    num_outputs = num_bits;
    
    // Allocate inputs
    inputs = new bool*[num_inputs];
    for (uint16_t i = 0; i < num_inputs; ++i)
        inputs[i] = nullptr;
    
    // Allocate outputs as bool array (not bool* array)
    outputs = new bool[num_outputs];
    
    // create inverted input signals
    buffer_A.connect_output(&input_inverter_A, 0, 0);
    buffer_B.connect_output(&input_inverter_B, 0, 0);
    
    // connect AND 1 (A & !B)
    buffer_A.connect_output(&and_gate1, 0, 0);
    input_inverter_B.connect_output(&and_gate1, 0, 1);
    
    // connect AND 2 (!A & B)
    input_inverter_A.connect_output(&and_gate2, 0, 0);
    buffer_B.connect_output(&and_gate2, 0, 1);
    
    // connect AND 3
    and_gate1.connect_output(&output_or_gate, 0, 0);
    and_gate2.connect_output(&output_or_gate, 0, 1);
};

XOR_Gate::~XOR_Gate()
{
}

bool XOR_Gate::connect_input(const bool* const upstream_output_p, uint16_t input_index)
{
    // Call parent to set our inputs array
    if (!Component::connect_input(upstream_output_p, input_index))
        return false;
    
    // Wire the appropriate buffer with the actual signal
    if (input_index < num_bits)
        return buffer_A.connect_input(inputs[input_index], input_index);
    else
        return buffer_B.connect_input(inputs[input_index], input_index - num_bits);
}

void XOR_Gate::evaluate()
{
    // Evaluate all internal components in topological order
    buffer_A.evaluate();
    buffer_B.evaluate();
    input_inverter_A.evaluate();
    input_inverter_B.evaluate();
    and_gate1.evaluate();
    and_gate2.evaluate();
    output_or_gate.evaluate();
    
    // Copy OR gate output to our output
    for (uint16_t i = 0; i < num_bits; ++i) {
        outputs[i] = output_or_gate.get_output(i);
    }
}

void XOR_Gate::update()
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

